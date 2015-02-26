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

#include "directoryunpacker.h"

#include "utils/qthelpers.h"
#include "utils/lzma/LzmaDec.h"

#include <QFile>
#include <QByteArray>
#include <QtEndian>
#include <QBuffer>
#include <QVector>
#include <QDir>
#include <algorithm>

#include <string.h>

struct DirectoryUnpacker::PrivateImplementation {

	static void* SzAlloc( void* /*p*/, size_t size )
	{
		return ( void* ) new char[size];
	}

	static void SzFree( void* /*p*/, void *address )
	{
		delete ( char* ) address;
	}

	STAGE stage;
	QNetworkReply *data;
	QFile *inputFile;

	QString filename;
	QFile outputFile;

	QByteArray block;
	int blockPosition;
	qint64 blockSize;
	quint16 blockChecksum;

	unsigned char* lzmaInputBuffer;
	unsigned char* lzmaOutputBuffer;
	int lzmaBufferSize;

	CLzmaDec state;
	ISzAlloc g_Alloc;

	size_t inPos;
	size_t inSize;
	size_t outPos;
	qint64 bytesDecoded;

	int currentFileIndex;
	struct File {
		QString name;
		qint64 size;
	};
	QVector< File > fileList;


	bool readHeader( QIODevice* data )
	{
		qDebug() << "Output directory:" << filename.replace( ".mmm", "" );

		if ( data->read( strlen( "MoNav Map Module" ) ) != "MoNav Map Module" )
		{
			qCritical() << "Wrong file type";
			return false;
		}

		qDebug() << "Finished reading header";
		return true;
	}

	bool readFileInfo()
	{
		QBuffer buffer( &block );
		buffer.open( QIODevice::ReadOnly );

		int numFiles;
		if ( buffer.read( ( char * ) &numFiles, sizeof( int ) ) != sizeof( int ) )
			return false;
		numFiles = qFromLittleEndian( numFiles );

		for ( int i = 0; i < numFiles; i++ ) {
			File file;
			int stringLength;
			if ( buffer.read( ( char* ) &stringLength, sizeof( int ) ) != sizeof( int ) )
				return false;
			stringLength = qFromLittleEndian( stringLength );
			QByteArray data = buffer.read( stringLength );
			if ( data.size() != stringLength )
				return false;
			file.name = QString::fromUtf8( data.constData(), stringLength );
			if ( buffer.read( ( char* ) &file.size, sizeof( qint64 ) ) != sizeof( qint64 ) )
				return false;
			file.size = qFromLittleEndian( file.size );
			fileList.push_back( file );
		}

		return true;
	}

	bool readBlockHeader( QIODevice* data )
	{
		if( data->read( ( char* ) &blockSize, sizeof( int ) ) != sizeof( int ) )
			return false;
		blockSize = qFromLittleEndian( blockSize );

		if ( data->read( ( char* ) &blockChecksum, sizeof( quint16 ) ) != sizeof( quint16 ) )
			return false;
		blockChecksum = qFromLittleEndian( blockChecksum );

		return true;
	}

	bool checkBlockContent()
	{

		if ( block.size() != blockSize )
		{
			qCritical() << "Block size does not match, block corrupted";
			return false;
		}

		if ( blockChecksum != qChecksum( block.constData(), block.size() ) )
		{
			qCritical() << "Checksum not correct, file corrupted";
			return false;
		}

		blockPosition = 0;
		blockSize = 0;
		blockChecksum = 0;

		return true;
	}

	bool readBlock( QIODevice* data )
	{
		if( !readBlockHeader( data ) )
			return false;

		block = inputFile->read( blockSize );

		if( !checkBlockContent() )
			return false;

		return true;
	}

	bool readBlock( QNetworkReply* reply )
	{
		if( blockSize == 0 )
		{
			block.clear();

			if( reply->bytesAvailable() < (qint64) ( sizeof(int) + sizeof( quint16 ) ) )
				return false;

			if( !readBlockHeader( reply ) )
			{
				qCritical( "Block header corrupted");
				return false;
			}
		}

		if( blockSize > 0 )
		{
			qint64 bytesToRead = std::min( reply->bytesAvailable(), blockSize - block.size() );
			block.append( reply->read( bytesToRead ) );

			if( block.size() < blockSize )
				return false;

			if( !checkBlockContent() )
			{
				qCritical( "Block content corrupted");
				return false;
			}
		}

		return true;
	}

	bool readFromBlock()
	{
		if ( blockPosition == block.size() )
			return false;

		inSize = std::min( block.size() - blockPosition, lzmaBufferSize );
		memcpy( lzmaInputBuffer, block.constData() + blockPosition, inSize );
		blockPosition += inSize;

		inPos = 0;

		return true;
	}

	bool readLZMAHeader()
	{
		// header: 5 bytes of LZMA properties
		unsigned char header[LZMA_PROPS_SIZE];

		if ( block.size() < LZMA_PROPS_SIZE ) {
			qCritical() << "Error reading LZMA header";
			return false;
		}
		memcpy( header, block.constData(), LZMA_PROPS_SIZE );
		blockPosition = LZMA_PROPS_SIZE;

		//qDebug() << ( int ) header[0] << ( int ) header[1] << ( int ) header[2] << ( int ) header[3] << ( int ) header[4];

		LzmaDec_Construct( &state );

		g_Alloc.Alloc = SzAlloc;
		g_Alloc.Free = SzFree;

		LzmaDec_Free( &state, &g_Alloc );
		if ( LzmaDec_Allocate( &state, header, LZMA_PROPS_SIZE, &g_Alloc ) != SZ_OK ) {
			qCritical() << "Error allocating LZMA data structures";
			return false;
		}
		LzmaDec_Init( &state );

		inPos = 0;
		inSize = 0;
		outPos = 0;
		bytesDecoded = 0;

		return true;
	}

	bool decodeLZMAData()
	{
		//qDebug() << "round" << inPos << inSize << outPos;

		qint64 fileSize = fileList[currentFileIndex].size;
		size_t inLeft = inSize - inPos;
		size_t outLeft = lzmaBufferSize - outPos;
		ELzmaFinishMode mode = LZMA_FINISH_ANY;
		if ( outLeft + bytesDecoded >= ( size_t ) fileSize ) {
			mode = LZMA_FINISH_END;
			outLeft = fileSize - bytesDecoded;
		}
		ELzmaStatus status;
		//qDebug() << "left" << inLeft << outLeft;
		SRes res = LzmaDec_DecodeToBuf( &state, lzmaOutputBuffer + outPos, &outLeft, lzmaInputBuffer + inPos, &inLeft, mode, &status );
		if ( res != SZ_OK ) {
			//qDebug() << "res" << res;
			qCritical() << "Error decoding LZMA stream";
			return false;
		}
		inPos += inLeft;
		outPos += outLeft;
		bytesDecoded += outLeft;

		if ( ( size_t ) outputFile.write( ( const char* ) lzmaOutputBuffer, outPos ) != outPos ) {
			qCritical() << "Error writing to file:" << outputFile.fileName();
			return false;
		}
		outPos = 0;

		if ( bytesDecoded == fileSize )
		{
			qDebug() << "Finished decoding file:" << currentFileIndex << fileList[currentFileIndex].name;
			return true;
		}

		if ( inLeft == 0 && outLeft == 0) {
			qCritical() << "Internal LZMA error";
			return false;
		}

		return false;
	}

	bool setNextFile()
	{
		currentFileIndex = currentFileIndex + 1;

		qDebug() << "Started decoding file:" << currentFileIndex << fileList[currentFileIndex].name;

		QDir dir( filename );
		if ( !dir.exists()  && !dir.mkpath( filename ) )
		{
			qCritical( "Couldn't create target directory");
			return false;
		}
		dir.cd( filename );

		if ( outputFile.isOpen() )
			outputFile.close();

		outputFile.setFileName( dir.filePath( fileList[currentFileIndex].name ) );

		if ( !openQFile( &outputFile, QIODevice::WriteOnly ) )
		{
			qCritical( "Couldn't open target file");
			return false;
		}

		return true;
	}
};

DirectoryUnpacker::DirectoryUnpacker( QString filename, int bufferSize )
{
	d = new PrivateImplementation;
	d->filename = filename;
	d->lzmaInputBuffer = new unsigned char[bufferSize];
	d->lzmaOutputBuffer = new unsigned char[bufferSize];
	d->lzmaBufferSize = bufferSize;

	d->inputFile = new QFile( d->filename );
	d->data = NULL;

	d->stage = READY;
	d->blockSize = 0;
	d->blockChecksum = 0;
	d->currentFileIndex = -1;

	connect( this, SIGNAL( error() ), this, SLOT( cleanUp() ) );
}

DirectoryUnpacker::DirectoryUnpacker( QString filename, QNetworkReply *reply, int bufferSize )
{
	d = new PrivateImplementation;
	d->filename = filename;
	d->lzmaInputBuffer = new unsigned char[bufferSize];
	d->lzmaOutputBuffer = new unsigned char[bufferSize];
	d->lzmaBufferSize = bufferSize;

	d->inputFile = NULL;
	d->data = reply;

	d->stage = READY;
	d->blockSize = 0;
	d->blockChecksum = 0;
	d->currentFileIndex = -1;

	connect( this, SIGNAL( error() ), this, SLOT( cleanUp() ) );
	connect( reply, SIGNAL( readyRead() ), this, SLOT( processNetworkData() ) );
	connect( reply, SIGNAL( destroyed() ), this, SLOT( deleteLater() ) );
}

DirectoryUnpacker::~DirectoryUnpacker()
{
	if( d->inputFile != NULL )
		delete d->inputFile;

	delete d->lzmaInputBuffer;
	delete d->lzmaOutputBuffer;
	delete d;
}

bool DirectoryUnpacker::decompress( bool deleteFile )
{
	Timer time;

	if ( !openQFile( d->inputFile, QIODevice::ReadOnly ) ){
		qCritical() << "Failed to open file:" << d->inputFile->fileName();
		return false;
	}
	if ( !d->readHeader( d->inputFile ) ) {
		qCritical() << "Failed to read header";
		return false;
	}

	d->stage = FILEINFO;
	while( d->readBlock( d->inputFile ) )
		processBlock();

	if( !d->inputFile->atEnd() ){
		qCritical( "Unpacker finished before end of file" );
		return false;
	}

	qint64 combinedSize = 0;
	foreach( const PrivateImplementation::File& info, d->fileList )
		combinedSize += info.size;
	qDebug() << "Finished unpacking:" << time.elapsed() / 1000 << "s" << combinedSize / 1024.0 / 1024.0 / ( time.elapsed() / 1000.0 ) << "MB/s";

	d->inputFile->close();
	if ( deleteFile ) {
		d->inputFile->remove();
	}

	return true;
}

void DirectoryUnpacker::processNetworkData()
{
	if( d->data->error() != QNetworkReply::NoError )
	{
		qCritical( d->data->errorString().toUtf8() );
		emit error();
		return;
	}

	while( d->data->bytesAvailable() > 0 )
	{
		switch ( d->stage )
		{
			case READY:
			{
				qDebug() << "Unpacking MoNav Map Module:" << d->data->url().path();

				if( d->data->bytesAvailable() < strlen( "MoNav Map Module" ) )
				{
					//qDebug() << "Wating for header data";
					return;
				}

				if( !d->readHeader( d->data ) )
				{
					emit error();
					return;
				}

				d->stage = FILEINFO;
				break;
			}

			case FINISHED:
			{
				if( d->data->bytesAvailable() > 0 || !d->data->atEnd() )
				{
					qCritical( "Unpacker finished before end of data" );
					emit error();
					return;
				}

				return;
			}

			default:
			{
				if ( !d->readBlock( d->data ) )
				{
					//qDebug() << "Waiting for block data";
					break;
				}

				processBlock();
			}

		}
	}

	return;
}

void DirectoryUnpacker::processBlock()
{
	switch( d->stage )
	{
		case FILEINFO:
		{
			if ( !d->readFileInfo( ) ) {
				qCritical()  << "Failed to read file info";
				emit error();
				return;
			}

			qDebug() << "Finished reading file list";

			d->stage = SETFILE;
			return;
		}

		case SETFILE:
		{
			if( !d->setNextFile() )
			{
				emit error();
				return;
			}

			d->stage = DECODE_HEADER;
		}

		case DECODE_HEADER:
		{
			if ( !d->readLZMAHeader() )
			{
				emit error();
				return;
			}

			qDebug() << "Finished processing header";

			d->stage = DECODE_CONTENT;

		}

		case DECODE_CONTENT:
		{
			while( d->readFromBlock() )
			{
				while( d->inPos != d->inSize )
				{
					if( d->decodeLZMAData() )
					{
						if( d->currentFileIndex + 1 < d->fileList.size() )
							d->stage = SETFILE;

						else
						{
							d->stage = FINISHED;
							break;
						}
					}
				}
			}

			return;
		}

		default:
		{
			qCritical( "Unknown unpacker state" );
			emit error();
			return;
		}
	}

	qDebug() << "Finished unpacking:" << d->data->url().path() << "to" << d->filename;

	return;
}

void DirectoryUnpacker::cleanUp()
{
	if ( d->outputFile.isOpen() )
		d->outputFile.close();

	QString dirName = d->filename.replace( ".mmm", "" );
	QDir dir( dirName );
	if ( !dir.exists() )
		return;
	dir.cd( d->filename );

	for( int i=0; i < d->fileList.size(); i++)
		dir.remove( d->fileList[i].name );

	dir.cd( ".." );
	dir.rmdir( dirName );
}
