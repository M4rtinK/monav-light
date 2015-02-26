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

#include "utils/directorypacker.h"
#include "utils/directoryunpacker.h"
#include "stdio.h"

#include <QtCore/QCoreApplication>
#include <QString>
#include <QStringList>

void printHelp()
{
	printf( "Usage:\n" );
	printf( "\tmonav-packager e dir [dictionary-size block-size]\n" );
	printf( "\tmonav-packager d compressed-file [buffer-size]\n" );
}

int main(int argc, char *argv[])
{
	QCoreApplication a(argc, argv);

	QStringList args = a.arguments();
	if ( args.size() < 3 ) {
		printHelp();
		a.quit();
		return -1;
	}
	if ( args[1] == "e" && ( args.size() == 3 || args.size() == 5 ) ) {
		DirectoryPacker packer( args[2] );
		if ( args.size() == 5 ) {
			bool ok;
			int dictionarySize = args[3].toInt( &ok );
			if ( !ok ) {
				printHelp();
				a.quit();
				return -1;
			}
			int blockSize = args[4].toInt( &ok );
			if ( !ok ) {
				printHelp();
				a.quit();
				return -1;
			}
			bool result = packer.compress( dictionarySize, blockSize );
			a.quit();
			return result ? 0 : -1;
		} else {
			bool result = packer.compress();
			a.quit();
			return result ? 0 : -1;
		}
	}
	else if ( args[1] == "d" && ( args.size() == 3 || args.size() == 4 ) ) {
		if ( args.size() == 4 ) {
			bool ok;
			int bufferSize = args[3].toInt( &ok );
			if ( !ok ) {
				printHelp();
				a.quit();
				return -1;
			}
			DirectoryUnpacker unpacker( args[2], bufferSize );
			bool result = unpacker.decompress( false );
			a.quit();
			return result ? 0 : -1;
		} else {
			DirectoryUnpacker unpacker( args[2] );
			bool result = unpacker.decompress( false );
			a.quit();
			return result ? 0 : -1;
		}
	} else {
		printHelp();
		a.quit();
		return -1;
	}
	a.quit();
	return 0;
}
