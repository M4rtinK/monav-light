/*
Copyright 2010  Christian Vetter veaac.fdirct@gmail.com

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

#ifndef TABLE_H
#define TABLE_H

#include <QtGlobal>
#include <QCache>
#include <QFile>
#include <string.h>
#include <vector>
#include <QtDebug>

namespace gg {

	template< class T, int size >
	struct IndexTable {
		T index[size * size];
		IndexTable()
		{
			for ( int i = 0; i < size * size; i++ )
				index[i] = -1;
		}
		IndexTable( const char* buffer )
		{
			Read( buffer );
		}

		T GetIndex( int x, int y ) const
		{
			if ( x < 0 || x >= 32 )
				return -1;
			if ( y < 0 || y >= 32 )
				return -1;
			return index[x + y * size];
		}

		void SetIndex( int x, int y, T data )
		{
			assert( x >= 0 );
			assert( x < 32 );
			assert( y >= 0 );
			assert( y < 32 );
			assert( index[x + y *size] == -1 );
			index[x + y * size] = data;
		}

		static size_t Size()
		{
			return size * size * sizeof( T );
		}

		void Write( char* buffer ) const
		{
			memcpy( buffer, index, size * size * sizeof( T ) );
		}

		void Read( const char* buffer )
		{
			memcpy( index, buffer, size * size * sizeof( T ) );
		}

		void Debug()
		{
			for ( int i = 0; i < size; i++ ) {
				QString row;
				for ( int j = 0; j < size; j++)
					row += GetIndex( i, j ) == -1 ? "." : "#";
				qDebug() << row;
			}
		}
	};

	struct GridIndex {
		qint64 position;
		int x;
		int y;
	};

	class Index {

	public:
		Index( QString filename ) :
				file2( filename + "_2" ), file3( filename + "_3" )
		{
			QFile file1( filename + "_1" );
			file1.open( QIODevice::ReadOnly );
			top.Read( file1.read( top.Size() ).constData() );
			file2.open( QIODevice::ReadOnly );
			file3.open( QIODevice::ReadOnly );
		}

		qint64 GetIndex( int x, int y ) {
			int topx = x / 32 / 32;
			int topy = y / 32 / 32;
			int middle = top.GetIndex( topx, topy );
			if ( middle == -1 )
				return -1;

			int middlex = ( x / 32 ) % 32;
			int middley = ( y / 32 ) % 32;
			if ( !cache2.contains( middle ) ) {
				IndexTable< int, 32 >* newEntry;
				file2.seek( middle * newEntry->Size() );
				newEntry = new IndexTable< int, 32 >( file2.read( newEntry->Size() ) );
				cache2.insert( middle, newEntry, newEntry->Size() );
			}
			if ( !cache2.contains( middle ) )
				return -1;
			IndexTable< int, 32 >* middleTable = cache2.object( middle );
			int bottom = middleTable->GetIndex( middlex, middley );
			if ( bottom == -1 )
				return -1;

			int bottomx = x % 32;
			int bottomy = y % 32;
			if ( !cache3.contains( bottom ) ) {
				IndexTable< qint64, 32 >* newEntry;
				file3.seek( bottom * newEntry->Size() );
				newEntry = new IndexTable< qint64, 32 >( file3.read( newEntry->Size() ) );
				cache3.insert( bottom, newEntry, newEntry->Size() );
			}
			if ( !cache3.contains( bottom ) )
				return -1;
			IndexTable< qint64, 32 >* bottomTable = cache3.object( bottom );
			qint64 position = bottomTable->GetIndex( bottomx, bottomy );
			return position;
		}

		void SetCacheSize( long long size )
		{
			assert( size > 0 );
			cache2.setMaxCost( size / 4 );
			cache3.setMaxCost( 3 * size / 4 );
		}

		static void Create( QString filename, const std::vector< GridIndex >& data )
		{
			gg::IndexTable< int, 32 > top;
			std::vector< gg::IndexTable< int, 32 > > middle;
			std::vector< gg::IndexTable< qint64, 32 > > bottom;

			for ( std::vector< GridIndex >::const_iterator i = data.begin(), iend = data.end(); i != iend; i++ ) {
				int topx = i->x / 32 / 32;
				int topy = i->y / 32 / 32;
				int middleIndex = top.GetIndex( topx, topy );
				if ( middleIndex == -1 ) {
					middleIndex =  middle.size();
					top.SetIndex( topx, topy, middleIndex );
					middle.push_back( gg::IndexTable< int, 32 >() );
				}

				int middlex = ( i->x / 32 ) % 32;
				int middley = ( i->y / 32 ) % 32;
				int bottomIndex = middle[middleIndex].GetIndex( middlex, middley );
				if ( bottomIndex == -1 ) {
					bottomIndex = bottom.size();
					middle[middleIndex].SetIndex( middlex, middley, bottomIndex );
					bottom.push_back( IndexTable< qint64, 32 >() );
				}

				int bottomx = i->x % 32;
				int bottomy = i->y % 32;
				bottom[bottomIndex].SetIndex( bottomx, bottomy, i->position );
			}

			qDebug() << "GPS Grid: top index filled: " << ( double ) middle.size() * 100 / 32 / 32 << "%";
			qDebug() << "GPS Grid: middle tables: " << middle.size();
			qDebug() << "GPS Grid: middle index filled: " << ( double ) bottom.size() * 100 / middle.size() / 32 / 32 << "%";
			qDebug() << "GPS Grid: bottom tables: " << bottom.size();
			qDebug() << "GPS Grid: bottom index filled: " << ( double ) data.size() * 100 / bottom.size() / 32 / 32 << "%";
			qDebug() << "GPS Grid: grid cells: " << data.size();

			QFile file1( filename + "_1" );
			QFile file2( filename + "_2" );
			QFile file3( filename + "_3" );
			file1.open( QIODevice::WriteOnly );
			file2.open( QIODevice::WriteOnly );
			file3.open( QIODevice::WriteOnly );

			char* buffer = new char[IndexTable< qint64, 32 >::Size()];

			top.Write( buffer );
			file1.write( buffer, IndexTable< int, 32 >::Size() );

			for ( int i = 0; i < ( int ) middle.size(); i++ ) {
				middle[i].Write( buffer );
				file2.write( buffer, IndexTable< int, 32 >::Size() );
			}

			for ( int i = 0; i < ( int ) bottom.size(); i++ ) {
				bottom[i].Write( buffer );
				file3.write( buffer, IndexTable< qint64, 32 >::Size() );
			}

			delete[] buffer;
		}

	private:
		QFile file2;
		QFile file3;
		IndexTable< int, 32 > top;
		QCache< int, IndexTable< int, 32 > > cache2;
		QCache< int, IndexTable< qint64, 32 > > cache3;
	};
}

#endif // TABLE_H
