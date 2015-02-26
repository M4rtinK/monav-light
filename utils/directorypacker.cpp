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

#include "directorypacker.h"
#include "utils/lzma/Types.h"
#include "utils/lzma/LzmaEnc.h"
#include "utils/qthelpers.h"
#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QDataStream>
#include <QBuffer>
#include <QtEndian>

static const int MAX_STRING_LENGTH = 256 * 256;
static const int MAX_FILENAMES = 256;
static const int INPUT_BUFFER_SIZE = 16 * 1024 * 1024;

struct DirectoryPacker::PrivateImplementation {

	static size_t writeToFile( void *p, const void *buf, size_t size )
	{
		FileWrite* write = ( FileWrite* ) p;
		DirectoryPacker::PrivateImplementation* d = write->data;

		d->outputBuffer.write( ( const char* ) buf, size );
		//qDebug() << "write" << size;
		if ( d->outputBuffer.size() >= d->blockSize ) {
			d->storeOutputBlock();
		}
		return size;
	}

	static SRes readFromFile(void *p, void *buf, size_t *size)
	{
		FileRead* read = ( FileRead* ) p;
		DirectoryPacker::PrivateImplementation* d = read->data;
		//qDebug() << "read" << *size;

		qint64 bytes = d->inputFile.read( ( char* ) buf, *size );
		*size = bytes;
		return SZ_OK;
	}

	struct FileWrite {
		ISeqOutStream f;
		PrivateImplementation* data;
	};

	struct FileRead
	{
		ISeqInStream f;
		PrivateImplementation* data;
	};

	static void* SzAlloc( void* /*p*/, size_t size )
	{
		return ( void* ) new char[size];
	}

	static void SzFree( void* /*p*/, void *address )
	{
		delete ( char* ) address;
	}

	bool encode()
	{
		CLzmaEncHandle enc;
		SRes res;
		CLzmaEncProps props;

		ISzAlloc g_Alloc = { SzAlloc, SzFree };
		FileRead read;
		FileWrite write;
		read.f.Read = readFromFile;
		read.data = this;
		write.f.Write = writeToFile;
		write.data = this;

		enc = LzmaEnc_Create( &g_Alloc );
		if ( enc == 0 ) {
			qCritical() << "Error allocating memory";
			return false;
		}

		LzmaEncProps_Init( &props );
		props.dictSize = dictionarySize;
		res = LzmaEnc_SetProps( enc, &props );
		if ( res != SZ_OK ) {
			qCritical() << "Error setting LZMA probs";
			return false;
		}

		Byte header[LZMA_PROPS_SIZE];
		size_t headerSize = LZMA_PROPS_SIZE;

		res = LzmaEnc_WriteProperties( enc, header, &headerSize );
		if ( write.f.Write( &write.f, header, headerSize ) != headerSize ) {
			qCritical() << "Error writing LZMA header";
			return false;
		}

		res = LzmaEnc_Encode( enc, &write.f, &read.f, NULL, &g_Alloc, &g_Alloc );
		if ( res != SZ_OK ) {
			qCritical() << "Error encoding LZMA";
			return false;
		}

		LzmaEnc_Destroy(enc, &g_Alloc, &g_Alloc);

		return true;
	}

	template< class T>
	void storeData( const T& data )
	{
		outputBuffer.write( ( const char* ) &data, sizeof( T ) );
	}

	bool storeString( QString string )
	{
		QByteArray utf8 = string.toUtf8();
		if ( utf8.size() > MAX_STRING_LENGTH ) {
			qCritical() << "string too long:" << string;
			return false;
		}
		storeData( qToLittleEndian( ( int ) utf8.size() ) );
		outputBuffer.write( utf8 );

		return true;
	}

	bool writeHeader()
	{
		if ( !outputFile.write( "MoNav Map Module" ) == strlen( "MoNav Map Module" ) )
			return false;

		return true;
	}

	bool writeFileInfo( QFileInfoList files )
	{
		initOutput();

		storeData( qToLittleEndian( ( int ) files.size() ) );
		for ( int i = 0; i < files.size(); i++ ) {
			QString name = files[i].fileName();
			if ( !storeString( name ) )
				return false;
			storeData( qToLittleEndian( files[i].size() ) );
		}

		if ( !storeOutputBlock( false ) )
			return false;

		return true;
	}

	bool storeOutputBlock( bool split = true )
	{
		int numBlocks = 1;
		if ( split )
			numBlocks = outputBuffer.size() / blockSize;
		if ( !split && numBlocks != 1 ) {
			qCritical() << "Block size too small to store non-splittable block:" << outputBuffer.size();
			return false;
		}

		for ( int i = 0; i < numBlocks; i++ ) {
			int size = std::min( blockSize, ( int ) outputBuffer.size() );
			quint16 checksum = qChecksum( outputBuffer.buffer().constData() + i * blockSize, size );
			//qDebug() << "store block:" << size << ", checksum:" << checksum;

			int outSize = qToLittleEndian( size );
			quint16 outChecksum = qToLittleEndian( checksum );

			if ( outputFile.write( ( const char* ) &outSize, sizeof( int ) ) != sizeof( int ) )
				return false;

			if ( outputFile.write( ( const char* ) &outChecksum, sizeof( quint16 ) ) != sizeof( quint16 ) )
				return false;

			if ( outputFile.write( outputBuffer.buffer().constData() + i * blockSize, size ) != size )
				return false;

			//qDebug() << "block written:" << size;
			//qDebug() << outputBuffer.buffer().toBase64();
		}

		if ( !split )
			outputBuffer.buffer().clear();
		else
			outputBuffer.buffer() = outputBuffer.buffer().right( outputBuffer.size() - outputBuffer.size() / blockSize * blockSize );
		outputBuffer.seek( outputBuffer.size() );

		//qDebug() << "leftover:" << outputBuffer.size();

		return true;
	}

	void initOutput()
	{
		if ( outputBuffer.isOpen() )
			outputBuffer.close();
		outputBuffer.buffer().clear();
		outputBuffer.open( QIODevice::WriteOnly );
	}

	bool compress( QFileInfoList files )
	{
		for ( int i = 0; i < files.size(); i++ ) {
			const QFileInfo& info = files[i];
			qDebug() << "Compress file:" << info.filePath() << ", size:" << info.size();
			qint64 lastSize = outputFile.size();
			if ( inputFile.isOpen() )
				inputFile.close();
			inputFile.setFileName( info.absoluteFilePath() );
			if ( !openQFile( &inputFile, QIODevice::ReadOnly ) )
				return false;

			initOutput();

			if ( !encode() )
				return false;
			// finish block -> only one file per block
			if ( outputBuffer.buffer().size() != 0 ) {
				if ( !storeOutputBlock( false ) )
					return false;
			}

			qDebug() << "Compressed file:" << ( int ) ( 100.0 * ( outputFile.size() - lastSize ) / info.size() ) << "%";
		}

		return true;
	}

	QString dir;
	int dictionarySize;
	int blockSize;
	QBuffer outputBuffer;
	QFile inputFile;
	QFile outputFile;

};

DirectoryPacker::DirectoryPacker( QString directory )
{
	d = new PrivateImplementation;
	d->dir = directory;
}

DirectoryPacker::~DirectoryPacker()
{
	delete d;
}

bool DirectoryPacker::compress( int dictionarySize, int blockSize )
{
	Timer time;
	d->dictionarySize = dictionarySize;
	d->blockSize = blockSize - sizeof( int ) - sizeof( quint16 );
	if ( dictionarySize < 1024 ) {
		qCritical() << "Dictionary size too small:" << dictionarySize;
		return false;
	}
	if ( blockSize < 1024 ) {
		qCritical() << "Block size too small:" << blockSize;
		return false;
	}

	qDebug() << "Compress directory:" << d->dir << ", block size:" << blockSize << ", dictionary size:" << dictionarySize;

	QDir dir( d->dir );
	d->outputFile.setFileName( QDir::cleanPath( dir.absolutePath() ) + ".mmm" );
	if ( !openQFile( &d->outputFile, QIODevice::WriteOnly ) )
		return false;

	qDebug() << "MoNav Map Module file:" << d->outputFile.fileName();

	if ( !d->writeHeader() )
		return false;

	QFileInfoList files = dir.entryInfoList( QDir::Files, QDir::Name );

	if ( !d->writeFileInfo( files ) )
		return false;

	if ( !d->compress( files ) )
		return false;

	qint64 combinedSize = 0;
	foreach( const QFileInfo& info, files )
		combinedSize += info.size();
	qDebug() << "Finished packaging:" << time.elapsed() / 1000 << "s" << combinedSize / 1024.0 / 1024.0 / ( time.elapsed() / 1000.0 ) << "MB/s";

	return true;
}
