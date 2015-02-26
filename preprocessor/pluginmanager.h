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

#ifndef PLUGINMANAGER_H
#define PLUGINMANAGER_H

#include <QVector>
#include <QObject>
#include <QStringList>

class QSettings;

class PluginManager : public QObject
{
	Q_OBJECT

public:

	// returns the single instance of this class
	static PluginManager* instance();

	// returns a list of plugins
	// plugins are uniquely identified by their name
	QStringList importerPlugins();
	QStringList routerPlugins();
	QStringList gpsLookupPlugins();
	QStringList rendererPlugins();
	QStringList addressLookupPlugins();

	// returns pointers to the plugins
	QVector< QObject* > plugins();

	// input filename for processing
	QString inputFile();
	// name of the data set to be created
	QString name();
	// map data directory that will contain everything
	QString outputDirectory();
	// package the map modules?
	bool packaging();
	// dictionary size for lzma packaging
	int dictionarySize();
	// block size used for packaging, each block is secured with a CRC-16 checksum
	int blockSize();

	// waits until all current processing step is finished
	void waitForFinish();
	// is a plugin currently processing?
	bool processing();

	// saves settings
	bool saveSettings( QSettings* settings );
	// restores settings
	bool loadSettings( QSettings* settings );

public slots:

	// loads dynamic plugins and initializes dynamic and static plugins
	bool loadPlugins();

	// unloads all plugins, including static ones
	// static plugins will not be available after this
	bool unloadPlugins();

	void setInputFile( QString filename );
	void setName( QString name );
	void setOutputDirectory( QString directory );
	void setPackaging( bool enabled );
	void setDictionarySize( int size );
	void setBlockSize( int size );

	// process plugins asynchronously, fails if processing is underway
	// accessing the plugins during processing is forbidden
	// even having their settings page enabled is forbidden
	bool processImporter( QString plugin, bool async = false );
	bool processRoutingModule( QString moduleName, QString importer, QString router, QString gpsLookup, bool async = false );
	bool processRenderingModule( QString moduleName, QString importer, QString renderer, bool async = false );
	bool processAddressLookupModule( QString moduleName, QString importer, QString addressLookup, bool async = false );

	// writes main config file
	// contains description of the map package
	bool writeConfig( QString importer );
	// deletes temporary files
	bool deleteTemporaryFiles();

signals:

	// finished preprocessing
	void finished( bool success );

private:

	struct PrivateImplementation;

	PrivateImplementation* d;

	PluginManager();
	~PluginManager();

private slots:

	// for internal use only
	void finish();
};

#endif // PLUGINMANAGER_H
