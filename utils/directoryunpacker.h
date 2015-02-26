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

#ifndef DIRECTORYUNPACKER_H
#define DIRECTORYUNPACKER_H

#include <QString>
#include <QNetworkReply>

// decompresses a .mmm file ( MoNav Map Module )
// compressed by the DirectoryPacker class
class DirectoryUnpacker : public QObject
{
	Q_OBJECT

public:

	DirectoryUnpacker( QString filename, int bufferSize = 16 * 1024 );
	DirectoryUnpacker( QString filename, QNetworkReply *reply, int bufferSize = 16 * 1024 );
	~DirectoryUnpacker();

	bool decompress( bool deleteFile = true );

public slots:
	void processNetworkData();

signals:
	void error();

protected slots:
	void processBlock();
	void cleanUp();

private:

	enum STAGE { READY, FILEINFO, SETFILE, DECODE_HEADER, DECODE_CONTENT, FINISHED};

	struct PrivateImplementation;
	PrivateImplementation* d;

};

#endif // DIRECTORYUNPACKER_H
