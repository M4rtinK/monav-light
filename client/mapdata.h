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

#ifndef MAPDATA_H
#define MAPDATA_H

#include "interfaces/iaddresslookup.h"
#include "interfaces/igpslookup.h"
#include "interfaces/irenderer.h"
#include "interfaces/irouter.h"
#include "utils/coordinates.h"
#include <QObject>
#include <QString>
#include <QVector>


// Manages map data and plugins
// all settings are stored in MoNav/MapData
class MapData : public QObject
{

	Q_OBJECT

public:

	enum ModuleType {
		AddressLookup = 0, Rendering = 2, Routing = 3
	};

	struct Module {
		// name of the module
		QString name;
		// path of the module
		// mostly used for internal porpuses
		QString path;
		// name of plugins required
		QStringList plugins;
		// file formats
		QVector< int > fileFormats;
	};

	struct MapPackage {
		// name of the map packge
		QString name;
		// path of the map package
		QString path;
		// bounding box of the map package
		UnsignedCoordinate min;
		UnsignedCoordinate max;
	};

	~MapData();

	// returns the instance of MapData
	static MapData* instance();

	// the path of the current map package
	QString path() const;
	void setPath( QString path );

	// does the directory contain MapData?
	static bool containsMapData( QString directory, MapPackage* data = NULL );
	bool containsMapData( MapPackage* data = NULL ) const;

	// searches recursivly in subdirectories for map data
	static bool searchForMapPackages( QString directory, QVector< MapPackage >* data, int depth = 2 );
	static bool unpackModule( QString filename );

	// names of the last loaded modules
	void lastModules( QString* routing, QString* rendering, QString* addressLookup );

	// is a map loaded?
	bool loaded() const;
	// tries to load the current directory
	// automatically calls loadInformation()
	bool load( const Module& routingModule, const Module& renderingModule, const Module& addressLookupModule );
	// tries loading modules by searching for the last used module names
	bool loadLast();
	// tries to unload the map data
	bool unload();

	// is the map information loaded?
	bool informationLoaded() const;
	// loads information about the map data
	// -> module list, map package name, bounding box
	bool loadInformation();

	// Information about the current directory
	// only valid after loadInformation() or load() was called successfully

	// the information associated with the current map package
	const MapPackage& information() const;
	// returns all available modules
	QVector< Module > modules( ModuleType plugin ) const;

	// returns an instance of a required plugin
	// only available after a successfull call to load() / loadLast()
	IAddressLookup* addressLookup();
	IGPSLookup* gpsLookup();
	IRenderer* renderer();
	IRouter* router();

signals:

	void dataLoaded();
	void dataUnloaded();
	void informationChanged();

public slots:

	// call before the QApplication object is destroyed
	// neccessary as Qt does not delete static instances itself // CHECK WHENEVER QT VERSION CHANGES!!!
	void cleanup();

private:

	MapData();
	struct PrivateImplementation;
	PrivateImplementation* const d;

};

#endif // MAPDATA_H
