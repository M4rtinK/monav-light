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

#ifndef GLOBALSETTINGS_H
#define GLOBALSETTINGS_H

class QSettings;

class GlobalSettings
{

public:

	static void saveSettings( QSettings* settings );
	static void loadSettings( QSettings* settings );

	static int iconSize();
	static void setIconSize( int size );
	static void setDefaultIconsSize();
	static bool autoRotation();
	static void setAutoRotation( bool autoRotation );

	enum MenuMode {
		MenuPopup, MenuOverlay
	};
	static MenuMode menuMode();
	static void setMenuMode( MenuMode mode );

	static int magnification();
	static void setMagnification( int factor );
	static int zoomMainMap();
	static void setZoomMainMap( int zoom );
	static int zoomPlaceChooser();
	static void setZoomPlaceChooser( int zoom );
	static int zoomStreetChooser();
	static void setZoomStreetChooser( int zoom );

private:

	struct PrivateImplementation;
	PrivateImplementation* d;

	GlobalSettings();
	~GlobalSettings();
	static GlobalSettings* privateInstance();
};

#endif // GLOBALSETTINGS_H
