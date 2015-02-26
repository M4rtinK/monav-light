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

#include "mapnikrendererclient.h"
#include "utils/qthelpers.h"
#include <QtDebug>

MapnikRendererClient::MapnikRendererClient()
{
	m_fileZoom = -1;
	m_indexFile = NULL;
	m_tileFile = NULL;
}

MapnikRendererClient::~MapnikRendererClient()
{
}

void MapnikRendererClient::unload()
{
	m_boxes.clear();
	if ( m_indexFile != NULL )
		delete m_indexFile;
	if ( m_tileFile != NULL )
		delete m_tileFile;
	m_indexFile = NULL;
	m_tileFile = NULL;
}

QString MapnikRendererClient::GetName()
{
	return "Mapnik Renderer";
}

bool MapnikRendererClient::IsCompatible( int fileFormatVersion )
{
	if ( fileFormatVersion == 1 )
		return true;
	return false;
}

bool MapnikRendererClient::load()
{
	QString filename = fileInDirectory( m_directory, "Mapnik Renderer" );
	FileStream configData( filename );
	if ( !configData.open( QIODevice::ReadOnly ) )
		return false;

	quint32 zoomLevels, size;
	configData >> size >> zoomLevels;
	m_tileSize = size;
	for ( int i = 0; i < ( int ) zoomLevels; i++ )
	{
		quint32 zoom, minX, maxX, minY, maxY;
		configData >> zoom >> minX >> maxX >> minY >> maxY;
		if ( configData.status() == QDataStream::ReadPastEnd )
			return false;
		Box box;
		box.minX = minX;
		box.maxX = maxX;
		box.minY = minY;
		box.maxY = maxY;
		m_boxes.resize( zoom + 1 );
		m_boxes[zoom] = box;
		m_zoomLevels.push_back( zoom );
	}

	m_fileZoom = -1;
	m_indexFile = new QFile( this );
	m_tileFile = new QFile( this );

	return true;
}

bool MapnikRendererClient::loadTile( int x, int y, int zoom, int /*magnification*/, QPixmap** tile )
{
	const Box& box = m_boxes[zoom];
	if ( x < box.minX || x >= box.maxX )
		return false;
	if ( y < box.minY || y >= box.maxY )
		return false;

	if ( m_fileZoom != zoom ) {
		m_indexFile->close();
		m_tileFile->close();

		QString filename = fileInDirectory( m_directory, "Mapnik Renderer" );
		m_indexFile->setFileName( filename + QString( "_%1_index" ).arg( zoom ) );
		if ( !openQFile( m_indexFile, QIODevice::ReadOnly ) )
			return false;
		m_tileFile->setFileName( filename + QString( "_%1_tiles" ).arg( zoom ) );
		if ( !openQFile( m_tileFile, QIODevice::ReadOnly ) )
			return false;
		m_fileZoom = zoom;
	}

	qint64 indexPosition = qint64( y - box.minY ) * ( box.maxX - box.minX ) + ( x - box.minX );
	m_indexFile->seek( indexPosition * ( sizeof( qint64 ) + sizeof( int ) ) );
	qint64 start;
	int size;
	m_indexFile->read( ( char* ) &start, sizeof( start ) );
	m_indexFile->read( ( char* ) &size, sizeof( size ) );
	if ( size == 0 )
		return false;

	m_tileFile->seek( start );
	QByteArray data = m_tileFile->read( size );
	*tile = new QPixmap( m_tileSize, m_tileSize );
	if ( !( *tile )->loadFromData( data, "PNG" ) ) {
		qDebug() << "Failed to load picture:" << x << y << zoom << " data: (" << start << "-" << start + size << ")";
		delete *tile;
		return false;
	}
	return true;
}
