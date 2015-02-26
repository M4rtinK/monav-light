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

#include "osmrenderer.h"
#ifndef NOGUI
#include "orsettingsdialog.h"
#endif
#include "utils/qthelpers.h"
#include <QFile>
#include <QSettings>

OSMRenderer::OSMRenderer()
{
}

OSMRenderer::~OSMRenderer()
{
}

QString OSMRenderer::GetName()
{
	return "OSM Renderer";
}

int OSMRenderer::GetFileFormatVersion()
{
	return 1;
}

OSMRenderer::Type OSMRenderer::GetType()
{
	return Renderer;
}

bool OSMRenderer::LoadSettings( QSettings* settings )
{
	settings->beginGroup( "OSM Renderer" );

	m_settings.zoomLevels.clear();
	for ( int zoom = 0; zoom < 19; zoom++ ) {
		QString name = QString( "zoom%1" ).arg( zoom );
		if ( settings->value( name, true ).toBool() )
			m_settings.zoomLevels.push_back( zoom );
	}

	settings->endGroup();
	return true;
}

bool OSMRenderer::SaveSettings( QSettings* settings )
{
	settings->beginGroup( "OSM Renderer" );

	int index = 0;
	for ( int zoom = 0; zoom < 19; zoom++ ) {
		QString name = QString( "zoom%1" ).arg( zoom );
		bool included = false;
		if ( index < ( int ) m_settings.zoomLevels.size() && m_settings.zoomLevels[index] == zoom ) {
			included = true;
			index++;
		}
		settings->setValue( name, included );
	}

	settings->endGroup();
	return true;
}

bool OSMRenderer::Preprocess( IImporter*, QString dir )
{
	QFile settingsFile( fileInDirectory( dir, "OSM Renderer" ) + "_settings" );
	if ( !openQFile( &settingsFile, QIODevice::WriteOnly ) )
		return false;

	for ( int zoom = 0; zoom < ( int ) m_settings.zoomLevels.size(); zoom ++ ) {
		int zoomLevel = m_settings.zoomLevels[zoom];
		settingsFile.write( ( const char* ) &zoomLevel, sizeof( zoomLevel ) );
	}
	return true;
}

#ifndef NOGUI
bool OSMRenderer::GetSettingsWindow( QWidget** window )
{
	if ( window == NULL )
		return false;
	*window = new ORSettingsDialog();
	return true;
}

bool OSMRenderer::FillSettingsWindow( QWidget* window )
{
	ORSettingsDialog* settings = qobject_cast< ORSettingsDialog* >( window );
	if ( settings == NULL )
		return false;
	return settings->readSettings( m_settings );
}

bool OSMRenderer::ReadSettingsWindow( QWidget* window )
{
	ORSettingsDialog* settings = qobject_cast< ORSettingsDialog* >( window );
	if ( settings == NULL )
		return false;
	return settings->fillSettings( &m_settings );
}
#endif
