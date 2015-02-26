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

#include "utils/commandlineparser.h"
#include "utils/formattedoutput.h"
#include "pluginmanager.h"
#include "interfaces/iconsolesettings.h"
#include "utils/log.h"
#include "omp.h"

#include <QtPlugin>
#include <QSettings>
#include <QtCore/QCoreApplication>

Q_IMPORT_PLUGIN( MapnikRenderer );
Q_IMPORT_PLUGIN( ContractionHierarchies );
Q_IMPORT_PLUGIN( GPSGrid );
Q_IMPORT_PLUGIN( UnicodeTournamentTrie );
Q_IMPORT_PLUGIN( OSMRenderer );
Q_IMPORT_PLUGIN( QtileRenderer );
Q_IMPORT_PLUGIN( OSMImporter );
Q_IMPORT_PLUGIN( TestImporter );

class Commands : public IConsoleSettings {
	Q_INTERFACES( IConsoleSettings );

public:

	Commands()
	{
		listPlugins = false;
		importing = false;
		config = false;
		del = false;
		verbose = false;
		help = false;
	}

	virtual QString GetModuleName()
	{
		return "Global";
	}

	enum SettingsID {
		Input = 0,
		Output,
		Name,
		Plugins,
		Importer,
		Router,
		GPSLookup,
		Renderer,
		AddressLookup,
		DoImporting,
		DoRouting,
		DoRendering,
		DoAddressLookup,
		DoConfig,
		DoDelTmp,
		DoPackage,
		ModuleBlockSize,
		ModuleDictionarySize,
		Log,
		Settings,
		Verbose,
		Threads,
		Help
	};

	virtual bool GetSettingsList( QVector< Setting >* settings )
	{
		settings->push_back( Setting( "i", "input", "input file", "filename" ) );
		settings->push_back( Setting( "o", "output", "output directory", "directory" ) );

		settings->push_back( Setting( "n", "name", "map package name", "string" ) );

		settings->push_back( Setting( "p", "plugins", "lists plugins", "" ) );

		settings->push_back( Setting( "pi", "importer", "importer plugin", "plugin name" ) );
		settings->push_back( Setting( "pro", "router", "router plugin", "plugin name" ) );
		settings->push_back( Setting( "pg", "gps-lookup", "gps lookup plugin", "plugin name" ) );
		settings->push_back( Setting( "pre", "renderer", "renderer plugin", "plugin name" ) );
		settings->push_back( Setting( "pa", "address-lookup", "address lookup plugin", "plugin name" ) );

		settings->push_back( Setting( "di", "do-importing", "runs importer", "" ) );
		settings->push_back( Setting( "dro", "do-routing", "creates routing module", "module name" ) );
		settings->push_back( Setting( "dre", "do-rendering", "creates rendering module", "module name" ) );
		settings->push_back( Setting( "da", "do-address-lookup", "creates address lookup module", "module name" ) );
		settings->push_back( Setting( "dc", "do-config", "writes main map config file", "" ) );
		settings->push_back( Setting( "dd", "do-del-tmp", "deletes temporary files", "" ) );
		settings->push_back( Setting( "m", "do-package", "packages modules", "" ) );
		settings->push_back( Setting( "mb", "module-block-size", "block size used for packaged map modules", "integer" ) );
		settings->push_back( Setting( "md", "module-dictionary-size", "block size used for packaged map modules", "integer" ) );

		settings->push_back( Setting( "l", "log", "writes log to file", "filename" ) );

		settings->push_back( Setting( "s", "settings", "use settings file", "settings filename" ) );

		settings->push_back( Setting( "v", "verbose", "verbose logging", "" ) );

		settings->push_back( Setting( "t", "threads", "number of threads", "integer" ) );

		settings->push_back( Setting( "h", "help", "displays this help page", "" ) );

		return true;
	}

	virtual bool SetSetting( int id, QVariant data )
	{
		PluginManager* pluginManager = PluginManager::instance();
		bool ok = true;
		SettingsID type = SettingsID( id );
		switch ( type ) {
		case Input:
			pluginManager->setInputFile( data.toString() );
			break;
		case Output:
			pluginManager->setOutputDirectory( data.toString() );
			break;
		case Name:
			pluginManager->setName( data.toString() );
			break;
		case Plugins:
			listPlugins = true;
			break;
		case Importer:
			importer = data.toString();
			break;
		case Router:
			router = data.toString();
			break;
		case GPSLookup:
			gpsLookup = data.toString();
			break;
		case Renderer:
			renderer = data.toString();
			break;
		case AddressLookup:
			addressLookup = data.toString();
			break;
		case DoImporting:
			importing = true;
			break;
		case DoRouting:
			routingModule = data.toString();
			break;
		case DoRendering:
			renderingModule = data.toString();
			break;
		case DoAddressLookup:
			addressLookupModule = data.toString();
			break;
		case DoConfig:
			config = true;
			break;
		case DoDelTmp:
			del = true;
			break;
		case DoPackage:
			pluginManager->setPackaging( true );
			break;
		case ModuleBlockSize:
			pluginManager->setBlockSize( data.toInt( &ok ) );
			break;
		case ModuleDictionarySize:
			pluginManager->setDictionarySize( data.toInt( &ok ) );
			break;
		case Log:
			Log::instance()->setLogFile( data.toString() );
			break;
		case Settings:
			settings = data.toString();
			break;
		case Verbose:
			verbose = true;
			break;
		case Threads:
			omp_set_num_threads( data.toInt( &ok ) );
			break;
		case Help:
			help = true;
			break;
		default:
			return false;
		}

		return ok;
	}

	virtual ~Commands()
	{
	}

	bool listPlugins;
	QString importer;
	QString router;
	QString gpsLookup;
	QString renderer;
	QString addressLookup;
	bool importing;
	QString routingModule;
	QString renderingModule;
	QString addressLookupModule;
	bool config;
	bool del;
	QString settings;
	bool verbose;
	bool help;

};

int main(int argc, char *argv[])
{
	QCoreApplication a(argc, argv);

	CommandLineParser parser;
	Commands commands;

	parser.registerDataSink( &commands );
	if ( !parser.parse( true ) ) {
		a.quit();
		return -1;
	}

	Log::instance()->setMessageTypeEnabled( QtDebugMsg, commands.verbose );
	PluginManager* pluginManager = PluginManager::instance();
	pluginManager->loadPlugins();
	if ( commands.listPlugins ) {
		printf( "%s\n\n", printStringTable( pluginManager->importerPlugins(), 1, "Importer Plugins" ).toUtf8().constData() );
		printf( "%s\n\n", printStringTable( pluginManager->routerPlugins(), 1, "Router Plugins" ).toUtf8().constData() );
		printf( "%s\n\n", printStringTable( pluginManager->gpsLookupPlugins(), 1, "GPS Lookup Plugins" ).toUtf8().constData() );
		printf( "%s\n\n", printStringTable( pluginManager->rendererPlugins(), 1, "Renderer Plugins" ).toUtf8().constData() );
		printf( "%s\n\n", printStringTable( pluginManager->addressLookupPlugins(), 1, "Address Lookup Plugins" ).toUtf8().constData() );
		a.quit();
		return 0;
	}

	if ( !commands.settings.isEmpty() ) {
		QSettings settings( commands.settings, QSettings::IniFormat );
		pluginManager->loadSettings( &settings );

		commands.importer = settings.value( "importer" ).toString();
		commands.router = settings.value( "router" ).toString();
		commands.gpsLookup = settings.value( "gpsLookup" ).toString();
		commands.renderer = settings.value( "renderer" ).toString();
		commands.addressLookup = settings.value( "addressLookup" ).toString();
	}

	// parse a second time including all plugins
	// otherwise console settings would be overwritten by the settings file
	QVector< QObject* > plugins = pluginManager->plugins();
	foreach( QObject* plugin, plugins ) {
		IConsoleSettings* settings = qobject_cast< IConsoleSettings* >( plugin );
		if ( settings!= NULL ) {
			parser.registerDataSink( settings );
		}
	}

	if ( !parser.parse() ) {
		a.quit();
		return -1;
	}

	if ( commands.help ) {
		parser.displayHelp();
		a.quit();
		return 0;
	}

	if ( commands.importing ) {
		if ( !pluginManager->processImporter( commands.importer ) ) {
			qCritical() << "Failed to import";
			a.quit();
			return -1;
		}
	}

	if ( !commands.routingModule.isEmpty() ) {
		if ( !pluginManager->processRoutingModule( commands.routingModule, commands.importer, commands.router, commands.gpsLookup ) ) {
			qCritical() << "Failed to preprocess routing module";
			a.quit();
			return -1;
		}
	}

	if ( !commands.renderingModule.isEmpty() ) {
		if ( !pluginManager->processRenderingModule( commands.renderingModule, commands.importer, commands.renderer ) ) {
			qCritical() << "Failed to preprocess rendering module";
			a.quit();
			return -1;
		}
	}

	if ( !commands.addressLookupModule.isEmpty() ) {
		if ( !pluginManager->processAddressLookupModule( commands.addressLookupModule, commands.importer, commands.addressLookup ) ) {
			qCritical() << "Failed to preprocess address lookup module";
			a.quit();
			return -1;
		}
	}

	if ( commands.config ) {
		if ( !pluginManager->writeConfig( commands.importer ) ) {
			qCritical() << "Failed to write main map config";
			a.quit();
			return -1;
		}
	}

	if ( commands.del ) {
		if ( !pluginManager->deleteTemporaryFiles() ) {
			qCritical() << "Failed to delete temporary files";
			a.quit();
			return -1;
		}
	}

	a.quit();
	return 0;
}

