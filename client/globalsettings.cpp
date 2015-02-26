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

#include "globalsettings.h"

#include <QSettings>
#include <QApplication>
#include <QStyle>
#include <QDir>

struct GlobalSettings::PrivateImplementation {
	int iconSize;
	int defaultIconSize;
	bool useDefaultIconSize;
	int magnification;
	MenuMode menuMode;
	int zoomMainMap;
	int zoomStreetChooser;
	int zoomPlaceChooser;
	bool autoRotation;
};

GlobalSettings::GlobalSettings()
{
	d = new PrivateImplementation;
	d->defaultIconSize = QApplication::style()->pixelMetric( QStyle::PM_ToolBarIconSize );
}

GlobalSettings::~GlobalSettings()
{
	delete d;
}

GlobalSettings* GlobalSettings::privateInstance()
{
	static GlobalSettings globalSettings;
	return &globalSettings;
}

void GlobalSettings::loadSettings( QSettings *settings )
{
	GlobalSettings* instance = privateInstance();
	settings->beginGroup( "Global Settings" );
	instance->d->iconSize = settings->value( "iconSize", instance->d->defaultIconSize ).toInt();
	instance->d->useDefaultIconSize = settings->value( "useDefaultIconSize", true ).toBool();
	instance->d->magnification = settings->value( "magnification", 1 ).toInt();
	instance->d->menuMode = MenuMode( settings->value( "menuMode", ( int ) MenuOverlay ).toInt() );
	instance->d->zoomMainMap = settings->value( "zoomMainMap", 9 ).toInt();
	instance->d->zoomStreetChooser = settings->value( "zoomStreetChooser", 14 ).toInt();
	instance->d->zoomPlaceChooser = settings->value( "zoomPlaceChooser", 11 ).toInt();
	instance->d->autoRotation = settings->value( "autoRotation", true ).toBool();
	settings->endGroup();
}

void GlobalSettings::saveSettings( QSettings *settings )
{
	GlobalSettings* instance = privateInstance();
	settings->beginGroup( "Global Settings" );
	settings->setValue( "iconSize", instance->d->iconSize );
	settings->setValue( "useDefaultIconSize", instance->d->useDefaultIconSize );
	settings->setValue( "magnification", instance->d->magnification );
	settings->setValue( "menuMode", ( int ) instance->d->menuMode );
	settings->setValue( "zoomMainMap", instance->d->zoomMainMap );
	settings->setValue( "zoomStreetChooser", instance->d->zoomStreetChooser );
	settings->setValue( "zoomPlaceChooser", instance->d->zoomPlaceChooser );
	settings->setValue( "autoRotation", instance->d->autoRotation );
	settings->endGroup();
}

int GlobalSettings::iconSize()
{
	GlobalSettings* instance = privateInstance();
	if ( instance->d->useDefaultIconSize )
		return instance->d->defaultIconSize;
	else
		return instance->d->iconSize;
}

void GlobalSettings::setIconSize( int size )
{
	GlobalSettings* instance = privateInstance();
	instance->d->iconSize = size;
	instance->d->useDefaultIconSize = size == instance->d->defaultIconSize;
}

bool GlobalSettings::autoRotation()
{
	GlobalSettings* instance = privateInstance();
	return instance->d->autoRotation;
}

void GlobalSettings::setAutoRotation( bool autoRotation )
{
	GlobalSettings* instance = privateInstance();
	instance->d->autoRotation = autoRotation;
}

void GlobalSettings::setDefaultIconsSize()
{
	GlobalSettings* instance = privateInstance();
	instance->d->iconSize = instance->d->defaultIconSize;
	instance->d->useDefaultIconSize = true;
}

int GlobalSettings::magnification()
{
	GlobalSettings* instance = privateInstance();
	return instance->d->magnification;
}

void GlobalSettings::setMagnification( int factor )
{
	GlobalSettings* instance = privateInstance();
	instance->d->magnification = factor;
}

int GlobalSettings::zoomMainMap()
{
	GlobalSettings* instance = privateInstance();
	return instance->d->zoomMainMap;
}

void GlobalSettings::setZoomMainMap( int zoom )
{
	GlobalSettings* instance = privateInstance();
	instance->d->zoomMainMap = zoom;
}

int GlobalSettings::zoomPlaceChooser()
{
	GlobalSettings* instance = privateInstance();
	return instance->d->zoomPlaceChooser;
}

void GlobalSettings::setZoomPlaceChooser( int zoom )
{
	GlobalSettings* instance = privateInstance();
	instance->d->zoomPlaceChooser = zoom;
}

int GlobalSettings::zoomStreetChooser()
{
	GlobalSettings* instance = privateInstance();
	return instance->d->zoomStreetChooser;
}

void GlobalSettings::setZoomStreetChooser( int zoom )
{
	GlobalSettings* instance = privateInstance();
	instance->d->zoomStreetChooser = zoom;
}

GlobalSettings::MenuMode GlobalSettings::menuMode()
{
	GlobalSettings* instance = privateInstance();
	return instance->d->menuMode;
}

void GlobalSettings::setMenuMode( MenuMode mode )
{
	GlobalSettings* instance = privateInstance();
	instance->d->menuMode = mode;
}
