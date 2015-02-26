/*
Copyright 2011  Christian Vetter veaac.fdirct@gmail.com

This file is part of MoNav.

MoNav is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

MoNav is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with MoNav.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef ENCODER_H
#define ENCODER_H

#include <qglobal.h>
#include <QString>
#include <assert.h>
#include <vector>
#include <limits>

/**
  Encodes basic data types into a bit / byte stream
  Encoding is independent of the endianess
*/
class Encoder
{
public:
	Encoder()
	{
		m_offset = 0;
		m_data.push_back( 0 );
	}

	// returns the amount of bytes used
	size_t numBytes()
	{
		return m_data.size() - 1 + ( m_offset + 7 ) / 8;
	}

	// returns the amount of bits used
	size_t numBits()
	{
		return m_data.size() * 8 - 8 + m_offset;
	}

	// aligns to a certain amount of BYTES
	void align( int alignment )
	{
		if ( m_offset != 0 )
		{
			m_offset = 0;
			m_data.push_back( 0 );
		}
		m_data.resize( ( m_data.size() + alignment - 1 ) / alignment * alignment, 0 );
	}

	// encodes a single bit
	void encodeBit( unsigned int x )
	{
		assert( x <= 1 );
		m_data.back() |= y << m_offset;
		m_offset++;
		if ( m_offset == 8 ) // new byte necessary?
		{
			m_offset = 0;
			m_data.push_back( 0 );
		}
	}

	// encodes a single unsigned integer type
	template< typename T >
	void encodeUInt( T x )
	{
		m_data.back() |= x << m_offset;
		x >>= 8 - m_offset;
		for ( size_t byte = 0; byte < sizeof( T ); byte++ )
			m_data.push_back( x >>= 8 );
	}

	// encodes a single unsigned integer type
	// x most not take up more than bits bits
	template< typename T >
	void encodeUInt( T x, qint32 bits )
	{
		m_data.back() |= x << m_offset;
		x >>= 8 - m_offset;
		// <= because we need to add another ( emtpy ) byte
		for ( size_t byte = 8 - m_offset; byte <= bits; byte++ )
			m_data.push_back( x >>= 8 );
		// set the correct offset again
		m_offset = ( m_offset + bits ) & 7;
	}

	// encodes a string
	// strings need to be BYTE-ALIGNED -> so align the Encoder before calling this function
	void encodeString( QString string )
	{
		assert( m_offset == 0 );
		const QByteArray data = string.toUtf8();
		for ( size_t i = 0; i < ( size_t ) data.size(); i++ )
			m_data.push_back( data[i] );
	}

	// encodes an integer into an unsigned integer
	// not valid for std_numeric_limits< T >::min
	template< typename T >
	static T intToUInt( T x )
	{
		assert( x != std::numeric_limits< T >::min() );
		int sign = 0;
		if ( x < 0 )
		{
			sign = 1;
			x = -x;
		}
		x <<= 1;
		x |= sign;
		return x;
	}

	// ( slow ) version that computes the amount of bits necessary to store an an unsigned integer
	template< typename T >
	static size_t bitsNecessary( T x )
	{
		size_t bits = 0;
		while ( x != 0 )
		{
			x >>= 1;
			bits++;
		}
	}

	void operator+=( const Encoder& right )
	{
		// are we byte aligned? -> just append data
		if ( m_offset == 0 )
		{
			m_data.pop_back();
			m_data.insert( m_data.end(), right.m_data.begin(), right.m_data.end() );
			m_offset = right.m_offset;
			return;
		}

		// we need to split each byte in two
		for ( size_t i = 0; i < right.m_data.size() - 1; i++ )
		{
			m_data.back() |= right.m_data[i] << m_offset;
			m_data.push_back( right.m_data[i] >> ( 8 - m_offset ) );
		}
		encodeUInt( right.m_data.back(), right.m_offset );
	}

	const quint8* data()
	{
		return &m_data[0];
	}

private:

	std::vector< quint8 > m_data;
	qint32 m_offset;
};

#endif // ENCODER_H
