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

#include "utils/qthelpers.h"
#include <QNetworkReply>
#include <QtDebug>
#include <QDesktopServices>
#include <QFile>

#include "osmrendererclient.h"
#include "osmrsettingsdialog.h"

OSMRendererClient::OSMRendererClient()
{
	network = NULL;
	diskCache = NULL;
}

OSMRendererClient::~OSMRendererClient()
{
	unload();
}

void OSMRendererClient::unload()
{
	if ( network != NULL )
		delete network;
	network = NULL;
}

QString OSMRendererClient::GetName()
{
	return "OSM Renderer";
}

bool OSMRendererClient::IsCompatible( int fileFormatVersion )
{
	if ( fileFormatVersion == 1 )
		return true;
	return false;
}

void OSMRendererClient::advancedSettingsChanged()
{
	assert( m_advancedSettings != NULL );
	OSMRSettingsDialog* dialog = qobject_cast< OSMRSettingsDialog* >( m_advancedSettings );
	if ( dialog == NULL )
		return;
	OSMRSettingsDialog::Settings settings;
	dialog->getSettings( &settings );
	if ( m_server != settings.tileURL ) {
		m_cache.clear();
		emit changed();
	}
	m_server = settings.tileURL;
}

bool OSMRendererClient::load()
{
	if ( m_advancedSettings == NULL )
		m_advancedSettings = new OSMRSettingsDialog();
	advancedSettingsChanged();
	network = new QNetworkAccessManager( this );
	diskCache = new QNetworkDiskCache( this );
	QString cacheDir = QStandardPaths::standardLocations( QStandardPaths::CacheLocation )[0];
	if ( cacheDir == "" ) {
		cacheDir = QStandardPaths::standardLocations( QStandardPaths::TempLocation )[0];
		QDir dir( cacheDir );
		dir.mkdir( "osmrenderer" );
		dir.cd( "osmrenderer" );
		cacheDir = dir.path();
	}
	qDebug() << "set disk cache to: " << cacheDir;
	diskCache->setCacheDirectory( cacheDir );
	network->setCache( diskCache );
	connect( network, SIGNAL(finished(QNetworkReply*)), this, SLOT(finished(QNetworkReply*)) );
	tileSize = 256;

	QFile settingsFile( fileInDirectory( m_directory, "OSM Renderer" ) + "_settings" );
	if ( !openQFile( &settingsFile, QIODevice::ReadOnly ) )
		return false;

	while ( true ) {
		int zoomLevel;
		if ( settingsFile.read( ( char* ) &zoomLevel, sizeof( zoomLevel ) ) != sizeof( zoomLevel ) )
			break;
		m_zoomLevels.push_back( zoomLevel );
	}

	return true;
}

void OSMRendererClient::finished( QNetworkReply* reply ) {
	long long id = reply->request().attribute( QNetworkRequest::User ).toLongLong();
	if ( reply->error() ) {
		m_cache.remove( id );
		qDebug() << "failed to get: " << reply->url();
		return;
	}

	QImage image;
	if ( !image.load( reply, "PNG" ) ) {
		m_cache.remove( id );
		qDebug() << "failed to load image: " << id;
		return;
	}
	QPixmap* tile = new QPixmap( QPixmap::fromImage( image ) );
	m_cache.insert( id, tile , tileSize * tileSize * tile->depth() / 8 );
	reply->deleteLater();
	emit changed();
}

bool OSMRendererClient::loadTile( int x, int y, int zoom, int /*magnification*/, QPixmap** tile )
{
	long long id = tileID( x, y, zoom );

	//QString path = "http://tile.openstreetmap.org/%1/%2/%3.png";
	QUrl url = QUrl( m_server.arg( zoom ).arg( x ).arg( y ) );

	QIODevice* cacheItem = diskCache->data( url );
	if ( cacheItem != NULL ) {
		QImage image;
		if ( !image.load( cacheItem, 0 ) ) {
			m_cache.remove( id );
			qDebug() << "failed to load image from cache: " << id;
			return false;
		}

		*tile = new QPixmap( QPixmap::fromImage( image ) );
		delete cacheItem;
		return true;
	}

	QNetworkRequest request;
	request.setUrl( url );
	request.setRawHeader( "User-Agent", "MoNav OSM Renderer 1.0" );
	request.setAttribute( QNetworkRequest::User, QVariant( id ) );
	request.setAttribute( QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::PreferCache );
	request.setAttribute( QNetworkRequest::HttpPipeliningAllowedAttribute, true );
	network->get( request );

	return false;
}
