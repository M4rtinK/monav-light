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

Alternatively, this file may be used under the terms of the GNU
Library General Public License as published by the Free Software
Foundation; either version 2 of the License, or (at your option)
any later version.
*/

#ifndef SIGNALS_H
#define SIGNALS_H

#include <QString>
#include <QVector>
#include <QDataStream>
#include <QStringList>

#include "signals.pb.h"

namespace MoNav {

	// Message should be one of the protocol buffer message types
	// defined in signals.proto.
	template <class Message, class Socket>
	class MessageWrapper {

	public:
		static void write( QIODevice* out, Message& result )
		{
			std::string buffer;
			result.SerializeToString( &buffer );

			// Write an unsigned integer containing the size of the message.
			qint32 size = buffer.size();
			out->write( ( const char* ) &size, sizeof( qint32 ) );

			qDebug("Posting message of size: %d",  size);

			// Write message.
			// For std::string, c_str() is \0-terminated, data() is not.
			out->write( buffer.c_str(), size );
		}

		static bool read( Socket* in, Message& command )
		{
			// Read an unsigned integer containing the size of the message.
			qint32 size;
			while ( in->bytesAvailable() < ( int ) sizeof( qint32 ) ) {
				if ( in->state() != Socket::ConnectedState ) {
					return false;
				}
				in->waitForReadyRead( 100 );
			}

			in->read( ( char* ) &size, sizeof( quint32 ) );

			qDebug("Reading message of size: %d",  size);

			// Read message.
			while ( in->bytesAvailable() < size ) {
				if ( in->state() != Socket::ConnectedState ) {
					return false;
				}
				in->waitForReadyRead( 100 );
			}

			QByteArray buffer = in->read( size );

			/* Parse message.
			 *
			 * Note:
			 * From http://doc.qt.nokia.com/latest/qbytearray.html#data:
			 * "A QByteArray can store any byte values
			 * including '\0's, but most functions that take char *
			 * arguments assume that the data ends at the first
			 * '\0' they encounter."
			 *
			 * So this is wrong:
			 * http://stackoverflow.com/questions/4252912/qt-protobuf-types
			 * std::istream stringstream( buffer.data() );
			 * command.ParseFromIstream( &stringstream );
			 */
			command.ParseFromArray( buffer, size );

			return true;
		}
	};
}
#endif // SIGNALS_H
