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

#include "pluginmanager.h"

#include "interfaces/iimporter.h"
#include "interfaces/ipreprocessor.h"
#include "utils/qthelpers.h"
#include "utils/directorypacker.h"
#include <QPluginLoader>
#include <QDir>
#include <QCoreApplication>
#include <QtDebug>
#include <QFuture>
#include <QFutureWatcher>
#include <QtConcurrent/QtConcurrentRun>
#include <QtDebug>
#include <QSettings>

struct PluginManager::PrivateImplementation {

	QVector< IImporter* > importerPlugins;
	QVector< IPreprocessor* > rendererPlugins;
	QVector< IPreprocessor* > routerPlugins;
	QVector< IPreprocessor* > gpsLookupPlugins;
	QVector< IPreprocessor* > addressLookupPlugins;

	QVector< QObject* > plugins;
	QVector< QPluginLoader* > pluginLoaders;

	bool testPlugin( QObject* plugin );

	QString inputFilename;
	QString outputDirectory;
	QString name;
	bool packaging;
	int dictionarySize;
	int blockSize;

	QFuture< bool > processingFuture;
	QFutureWatcher< bool > processingWatcher;

	template< class IPlugin >
	static int findPlugin( QVector< IPlugin > plugins, QString name );

};

void PluginManager::finish()
{
	emit finished( d->processingWatcher.result() );
}

template< class IPlugin >
int PluginManager::PrivateImplementation::findPlugin( QVector< IPlugin > plugins, QString name )
{
	// check if name uniquely specifies an importer plugin
	int index = -1;
	for ( int i = 0; i < plugins.size(); i++ ) {
		if ( plugins[i]->GetName() == name ) {
			if ( index != -1 ) {
				qCritical() << "Plugin name not unique:" << name;
				return -1;
			}
			index = i;
		}
	}
	if ( index == -1 )
		qCritical() << "Plugin name not found:" << name;

	return index;
}

PluginManager::PluginManager()
{
	d = new PrivateImplementation;
	d->packaging = false;
	d->dictionarySize = 16 * 1024;
	d->blockSize = 16 * 1024;
	connect( &d->processingWatcher, SIGNAL(finished()), this, SLOT(finish()) );
}
PluginManager::~PluginManager()
{
	delete d;
}

// returns the single instance of this class
PluginManager* PluginManager::instance()
{
	static PluginManager pluginManager;
	return &pluginManager;
}

// loads dynamic plugins and initializes dynamic and static plugins
bool PluginManager::loadPlugins()
{
	// dynamically load all available plugins
	QDir pluginDir( QCoreApplication::applicationDirPath() );
	if ( pluginDir.cd( "plugins_preprocessor" ) ) {
		foreach ( QString fileName, pluginDir.entryList( QDir::Files ) ) {
			QPluginLoader* loader = new QPluginLoader( pluginDir.absoluteFilePath( fileName ) );
			loader->setLoadHints( QLibrary::ExportExternalSymbolsHint );
			if ( !loader->load() )
				qDebug() << loader->errorString();

			if ( d->testPlugin( loader->instance() ) ) {
				d->pluginLoaders.push_back( loader );
			} else {
				loader->unload();
				delete loader;
			}
		}
	}

	// test static plugins
	foreach ( QObject *plugin, QPluginLoader::staticInstances() )
		d->testPlugin( plugin );

	if ( d->importerPlugins.size() == 0  || d->rendererPlugins.size() == 0  || d->routerPlugins.size() == 0 || d->gpsLookupPlugins.size() == 0  || d->addressLookupPlugins.size() == 0 ) {
		qWarning() << "plugin types are missing";
		return false;
	}

	return true;
}

// determines the type of the plugin
// adds it to the correct data structures
// returns whether this is a MoNav plugin
bool PluginManager::PrivateImplementation::testPlugin( QObject* plugin )
{
	bool needed = false;
	// importer plugin?
	if ( IImporter *interface = qobject_cast< IImporter* >( plugin ) )
	{
		importerPlugins.append( interface );
		needed = true;
	}
	// other preprocessing plugin?
	if ( IPreprocessor *interface = qobject_cast< IPreprocessor* >( plugin ) )
	{
		if ( interface->GetType() == IPreprocessor::Renderer )
			rendererPlugins.append( interface );
		if ( interface->GetType() == IPreprocessor::Router )
			routerPlugins.append( interface );
		if ( interface->GetType() == IPreprocessor::GPSLookup )
			gpsLookupPlugins.append( interface );
		if ( interface->GetType() == IPreprocessor::AddressLookup )
			addressLookupPlugins.append( interface );
		needed = true;
	}
	if ( needed )
		plugins.push_back( plugin );
	return needed;
}

bool PluginManager::unloadPlugins()
{
	foreach( QPluginLoader* pluginLoader, d->pluginLoaders )
	{
		pluginLoader->unload();
		delete pluginLoader;
	}
	// delete static instances
	// Qt does not do this, although the documentation states otherwise
	// TODO: CHECK FOR EACH NOW QT VERSION
	foreach ( QObject *plugin, QPluginLoader::staticInstances() )
		delete plugin;

	return true;
}

// returns a list of plugins
// plugins are uniquely identified by their name
QStringList PluginManager::importerPlugins()
{
	QStringList names;
	foreach( IImporter* importer, d->importerPlugins ) {
		names.push_back( importer->GetName() );
	}
	return names;
}
QStringList PluginManager::routerPlugins()
{
	QStringList names;
	foreach( IPreprocessor* plugin, d->routerPlugins ) {
		names.push_back( plugin->GetName() );
	}
	return names;
}
QStringList PluginManager::gpsLookupPlugins()
{
	QStringList names;
	foreach( IPreprocessor* plugin, d->gpsLookupPlugins ) {
		names.push_back( plugin->GetName() );
	}
	return names;
}
QStringList PluginManager::rendererPlugins()
{
	QStringList names;
	foreach( IPreprocessor* plugin, d->rendererPlugins ) {
		names.push_back( plugin->GetName() );
	}
	return names;
}
QStringList PluginManager::addressLookupPlugins()
{
	QStringList names;
	foreach( IPreprocessor* plugin, d->addressLookupPlugins ) {
		names.push_back( plugin->GetName() );
	}
	return names;
}

// returns pointers to the plugins
QVector< QObject* > PluginManager::plugins()
{
	return d->plugins;
}

// input filename for processing
QString PluginManager::inputFile()
{
	return d->inputFilename;
}
// name of the data set to be created
QString PluginManager::name()
{
	return d->name;
}

// map data directory that will contain everything
QString PluginManager::outputDirectory()
{
	return d->outputDirectory;
}

bool PluginManager::packaging()
{
	return d->packaging;
}

int PluginManager::dictionarySize()
{
	return d->dictionarySize;
}

int PluginManager::blockSize()
{
	return d->blockSize;
}

// waits until all current processing step is finished
void PluginManager::waitForFinish()
{
	d->processingFuture.waitForFinished();
	return;
}
// is a plugin currently processing?
bool PluginManager::processing()
{
	return !d->processingFuture.isFinished();
}


void PluginManager::setInputFile( QString filename )
{
	d->inputFilename = filename;
}

void PluginManager::setName( QString name )
{
	d->name = name;
}

void PluginManager::setOutputDirectory( QString directory )
{
	d->outputDirectory = directory;
}

void PluginManager::setPackaging( bool enabled )
{
	d->packaging = enabled;
}

void PluginManager::setDictionarySize( int size )
{
	d->dictionarySize = size;
}

void PluginManager::setBlockSize( int size )
{
	d->blockSize = size;
}

// saves settings
bool PluginManager::saveSettings( QSettings* settings )
{
	settings->setValue( "input", d->inputFilename );
	settings->setValue( "output", d->outputDirectory );
	settings->setValue( "name", d->name );
	settings->setValue( "packaging", d->packaging );
	settings->setValue( "dictionarySize", d->dictionarySize );
	settings->setValue( "blockSize", d->blockSize );

	foreach ( IImporter* plugin, d->importerPlugins ) {
		if ( !plugin->SaveSettings( settings ) )
			return false;
	}
	foreach ( IPreprocessor* plugin, d->routerPlugins ) {
		if ( !plugin->SaveSettings( settings ) )
			return false;
	}
	foreach ( IPreprocessor* plugin, d->rendererPlugins ) {
		if ( !plugin->SaveSettings( settings ) )
			return false;
	}
	foreach ( IPreprocessor* plugin, d->gpsLookupPlugins ) {
		if ( !plugin->SaveSettings( settings ) )
			return false;
	}
	foreach ( IPreprocessor* plugin, d->addressLookupPlugins ) {
		if ( !plugin->SaveSettings( settings ) )
			return false;
	}

	return true;
}

// restores settings
bool PluginManager::loadSettings( QSettings* settings )
{
	d->inputFilename = settings->value( "input" ).toString();
	d->outputDirectory = settings->value( "output" ).toString();
	d->name = settings->value( "name" ).toString();
	d->packaging = settings->value( "packaging", false ).toBool();
	d->dictionarySize = settings->value( "dictionarySize", 16 * 1024 ).toInt();
	d->blockSize = settings->value( "blockSize", 16 * 1024 ).toInt();

	foreach ( IImporter* plugin, d->importerPlugins ) {
		if ( !plugin->LoadSettings( settings ) )
			return false;
	}
	foreach ( IPreprocessor* plugin, d->routerPlugins ) {
		if ( !plugin->LoadSettings( settings ) )
			return false;
	}
	foreach ( IPreprocessor* plugin, d->rendererPlugins ) {
		if ( !plugin->LoadSettings( settings ) )
			return false;
	}
	foreach ( IPreprocessor* plugin, d->gpsLookupPlugins ) {
		if ( !plugin->LoadSettings( settings ) )
			return false;
	}
	foreach ( IPreprocessor* plugin, d->addressLookupPlugins ) {
		if ( !plugin->LoadSettings( settings ) )
			return false;
	}

	return true;
}

bool runImporter( QString inputFile, IImporter* importer )
{
	if ( !importer->Preprocess( inputFile ) ) {
		qCritical() << "Importer failed";
		return false;
	}
	return true;
}

struct PackerInfo {
	bool enabled;
	int dict;
	int block;
	PackerInfo( bool enabled, int dict, int block ) : enabled( enabled ), dict( dict ), block( block )
	{

	}
};

bool deleteDictionary( QString directory )
{
	qDebug() << "Deleting:" << directory;

	QDir dir( directory );
	if ( !dir.exists() )
		return true;
	dir.setFilter( QDir::Files );
	foreach ( QString filename, dir.entryList() )
	{
		QString file = dir.filePath( filename );
		qDebug() << "Deleting:" << file;
		if ( !QFile::remove( file ) )
			qWarning() << "Failed to delete:" << file;
	}

	dir.cdUp();
	if ( !dir.rmdir( directory ) )
		return false;

	return true;
}

bool runRouting( QString directory, IImporter* importer, IPreprocessor* router, IPreprocessor* gpsLookup, PackerInfo info )
{
	if ( !router->Preprocess( importer, directory ) ) {
		qCritical() << "Router failed";
		return false;
	}
	if ( !gpsLookup->Preprocess( importer, directory ) ) {
		qCritical() << "GPS Lookup failed";
		return false;
	}
	if ( info.enabled ) {
		DirectoryPacker packer( directory );
		if ( !packer.compress( info.dict, info.block ) ) {
			qCritical() << "Map module packaging failed";
			return false;
		}
		if ( !deleteDictionary( directory ) )
			return false;
	}
	return true;
}

bool runRendering( QString directory, IImporter* importer, IPreprocessor* renderer, PackerInfo info )
{
	if ( !renderer->Preprocess( importer, directory ) ) {
		qCritical() << "Renderer failed";
		return false;
	}
	if ( info.enabled ) {
		DirectoryPacker packer( directory );
		if ( !packer.compress( info.dict, info.block ) ) {
			qCritical() << "Map module packaging failed";
			return false;
		}
		if ( !deleteDictionary( directory ) )
			return false;
	}
	return true;
}

bool runAddressLookup( QString directory, IImporter* importer, IPreprocessor* addressLookup, PackerInfo info )
{
	if ( !addressLookup->Preprocess( importer, directory ) ) {
		qCritical() << "Address Lookup failed";
		return false;
	}
	if ( info.enabled ) {
		DirectoryPacker packer( directory );
		if ( !packer.compress( info.dict, info.block ) ) {
			qCritical() << "Map module packaging failed";
			return false;
		}
		if ( !deleteDictionary( directory ) )
			return false;
	}
	return true;
}

// process plugins asynchronously, fails if processing is underway
// accessing the plugins during processing is forbidden

bool PluginManager::processImporter( QString plugin, bool async )
{
	int index = d->findPlugin( d->importerPlugins, plugin );
	if ( index == -1 )
		return false;

	// find / create importer directory
	QDir dir( d->outputDirectory );
	if ( !dir.exists( "tmp" ) ) {
		if ( !dir.mkdir( "tmp" ) ) {
			qCritical() << "Could not create importer directory: tmp";
			return false;
		}
	}
	dir.cd( "tmp" );
	d->importerPlugins[index]->SetOutputDirectory( dir.path() );

	if ( !async ) {
		return runImporter( d->inputFilename, d->importerPlugins[index] );
	}

	// run async and return
	d->processingFuture = QtConcurrent::run( runImporter, d->inputFilename, d->importerPlugins[index] );
	d->processingWatcher.setFuture( d->processingFuture );
	return true;
}

bool PluginManager::processRoutingModule( QString moduleName, QString importer, QString router, QString gpsLookup, bool async )
{
	int importerIndex = d->findPlugin( d->importerPlugins, importer );
	int routerIndex = d->findPlugin( d->routerPlugins, router );
	int gpsLookupIndex = d->findPlugin( d->gpsLookupPlugins, gpsLookup );
	if ( importerIndex == -1 || routerIndex == -1 || gpsLookupIndex == -1 )
		return false;

	d->importerPlugins[importerIndex]->SetOutputDirectory( fileInDirectory( d->outputDirectory, "tmp") );

	// find / create module directory
	QString dirName = "routing_" + moduleName.toLower().replace( ' ', '_' );
	QDir dir( d->outputDirectory );
	if ( !dir.exists( dirName ) ) {
		if ( !dir.mkdir( dirName ) ) {
			qCritical() << "Could not create module directory:" << moduleName;
			return false;
		}
	}
	dir.cd( dirName );

	{
		QSettings settings( fileInDirectory( dir.path(), "Module.ini" ), QSettings::IniFormat );

		settings.setValue( "configVersion", 2 );
		settings.setValue( "name", moduleName );
		settings.setValue( "router", router );
		settings.setValue( "gpsLookup", gpsLookup );
		settings.setValue( "routerFileFormatVersion", d->routerPlugins[routerIndex]->GetFileFormatVersion() );
		settings.setValue( "gpsLookupFileFormatVersion", d->gpsLookupPlugins[gpsLookupIndex]->GetFileFormatVersion() );

		settings.sync();
		if ( settings.status() != QSettings::NoError )
		{
			qCritical() << "Error writing config:" << settings.fileName();
			return false;
		}
	}

	if ( !async ) {
		return runRouting( dir.path(), d->importerPlugins[importerIndex], d->routerPlugins[routerIndex], d->gpsLookupPlugins[gpsLookupIndex], PackerInfo( d->packaging, d->dictionarySize, d->blockSize ) );
	}

	// run async and return
	d->processingFuture =
			QtConcurrent::run( runRouting, dir.path(), d->importerPlugins[importerIndex], d->routerPlugins[routerIndex], d->gpsLookupPlugins[gpsLookupIndex], PackerInfo( d->packaging, d->dictionarySize, d->blockSize ) );
	d->processingWatcher.setFuture( d->processingFuture );
	return true;
}

bool PluginManager::processRenderingModule( QString moduleName, QString importer, QString renderer, bool async )
{
	int importerIndex = d->findPlugin( d->importerPlugins, importer );
	int rendererIndex = d->findPlugin( d->rendererPlugins, renderer );
	if ( importerIndex == -1 || rendererIndex == -1 )
		return false;

	d->importerPlugins[importerIndex]->SetOutputDirectory( fileInDirectory( d->outputDirectory, "tmp") );

	// find / create module directory
	QString dirName = "rendering_" + moduleName.toLower().replace( ' ', '_' );
	QDir dir( d->outputDirectory );
	if ( !dir.exists( dirName ) ) {
		if ( !dir.mkdir( dirName ) ) {
			qCritical() << "Could not create module directory:" << moduleName;
			return false;
		}
	}
	dir.cd( dirName );

	{
		QSettings settings( fileInDirectory( dir.path(), "Module.ini" ), QSettings::IniFormat );

		settings.setValue( "configVersion", 2 );
		settings.setValue( "name", moduleName );
		settings.setValue( "renderer", renderer );
		settings.setValue( "rendererFileFormatVersion", d->rendererPlugins[rendererIndex]->GetFileFormatVersion() );

		settings.sync();
		if ( settings.status() != QSettings::NoError )
		{
			qCritical() << "Error writing config:" << settings.fileName();
			return false;
		}
	}

	if ( !async ) {
		return runRendering( dir.path(), d->importerPlugins[importerIndex], d->rendererPlugins[rendererIndex], PackerInfo( d->packaging, d->dictionarySize, d->blockSize ) );
	}

	// run async and return
	d->processingFuture = QtConcurrent::run( runRendering, dir.path(), d->importerPlugins[importerIndex], d->rendererPlugins[rendererIndex], PackerInfo( d->packaging, d->dictionarySize, d->blockSize ) );
	d->processingWatcher.setFuture( d->processingFuture );
	return true;
}

bool PluginManager::processAddressLookupModule( QString moduleName, QString importer, QString addressLookup, bool async )
{
	int importerIndex = d->findPlugin( d->importerPlugins, importer );
	int addressLookupIndex = d->findPlugin( d->addressLookupPlugins, addressLookup );
	if ( importerIndex == -1 || addressLookupIndex == -1 )
		return false;

	d->importerPlugins[importerIndex]->SetOutputDirectory( fileInDirectory( d->outputDirectory, "tmp") );

	// find / create module directory
	QString dirName = "address_" + moduleName.toLower().replace( ' ', '_' );
	QDir dir( d->outputDirectory );
	if ( !dir.exists( dirName ) ) {
		if ( !dir.mkdir( dirName ) ) {
			qCritical() << "Could not create module directory:" << moduleName;
			return false;
		}
	}
	dir.cd( dirName );

	{
		QSettings settings( fileInDirectory( dir.path(), "Module.ini" ), QSettings::IniFormat );

		settings.setValue( "configVersion", 2 );
		settings.setValue( "name", moduleName );
		settings.setValue( "addressLookup", addressLookup );
		settings.setValue( "addressLookupFileFormatVersion", d->addressLookupPlugins[addressLookupIndex]->GetFileFormatVersion() );

		settings.sync();
		if ( settings.status() != QSettings::NoError )
		{
			qCritical() << "Error writing config:" << settings.fileName();
			return false;
		}
	}

	if ( !async ) {
		return runAddressLookup( dir.path(), d->importerPlugins[importerIndex], d->addressLookupPlugins[addressLookupIndex], PackerInfo( d->packaging, d->dictionarySize, d->blockSize ) );
	}

	// run async and return
	d->processingFuture = QtConcurrent::run( runAddressLookup, dir.path(), d->importerPlugins[importerIndex], d->addressLookupPlugins[addressLookupIndex], PackerInfo( d->packaging, d->dictionarySize, d->blockSize ) );
	d->processingWatcher.setFuture( d->processingFuture );
	return true;
}

// writes main config file
// contains description of the map package
bool PluginManager::writeConfig( QString importer )
{
	int importerIndex = d->findPlugin( d->importerPlugins, importer );
	if ( importerIndex == -1 )
		return false;

	IImporter::BoundingBox box;
	if ( !d->importerPlugins[importerIndex]->GetBoundingBox( &box ) )
		return false;

	QSettings configFile( fileInDirectory( d->outputDirectory, "MoNav.ini" ), QSettings::IniFormat );
	configFile.clear();

	configFile.setValue( "configVersion", 2 );
	configFile.setValue( "name", d->name );
	configFile.setValue( "minX", box.min.x );
	configFile.setValue( "maxX", box.max.x );
	configFile.setValue( "minY", box.min.y );
	configFile.setValue( "maxY", box.max.y );

	configFile.sync();
	if ( configFile.status() != QSettings::NoError )
	{
		qCritical() << "Error writing config:" << configFile.fileName();
		return false;
	}

	return true;
}
// deletes temporary files
bool PluginManager::deleteTemporaryFiles()
{
	QDir dir( d->outputDirectory );
	dir.cd( "tmp" );
	if ( !dir.exists() )
		return true;
	foreach ( QString filename, dir.entryList() ) {
		QFile::remove( dir.absoluteFilePath( filename ) );
	}
	dir.cdUp();
	dir.rmdir( "tmp" );
	return true;
}

