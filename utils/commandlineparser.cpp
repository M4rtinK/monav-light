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

#include "commandlineparser.h"
#include "formattedoutput.h"

#include <cassert>
#include <QVector>
#include <QMultiHash>
#include <QtDebug>
#include <QCoreApplication>
#include <QStringList>

struct Location {
	IConsoleSettings* dataSink;
	int id;

	Location( IConsoleSettings* dataSink, int id )
		: dataSink( dataSink ), id ( id )
	{
	}
};

struct CommandLineParser::PrivateImplementation {

	QVector< IConsoleSettings* > dataSinks;
	QMultiHash< QString, Location > shortIDs;
	QMultiHash< QString, Location > longIDs;

	QVector< QStringList > helpList;
	QStringList helpHeaders;
};

CommandLineParser::CommandLineParser()
{
	d = new PrivateImplementation;
}

CommandLineParser::~CommandLineParser()
{
	delete d;
}

void CommandLineParser::registerDataSink( IConsoleSettings *dataSink )
{
	assert( dataSink != NULL );

	QVector< IConsoleSettings::Setting > settings;
	if ( !dataSink->GetSettingsList( &settings ) ) {
		qWarning() << "Failed to get settings list from data sink";
		return;
	}
	QString name = dataSink->GetModuleName();

	d->helpHeaders << name;
	QStringList help;

	// add settings
	for ( int id = 0; id < settings.size(); id++ ) {
		if ( !settings[id].shortID.isEmpty() )
			d->shortIDs.insert( settings[id].shortID, Location( dataSink, id ) );
		d->longIDs.insert( settings[id].longID, Location( dataSink, id ) );

		if ( settings[id].shortID.isEmpty() )
			help << "";
		else
			help << "-" + settings[id].shortID;
		if ( settings[id].longID.isEmpty() )
			help << "";
		else
			help << "--" + settings[id].longID;

		help << settings[id].description << settings[id].type;
	}

	d->helpList.push_back( help );
}

void CommandLineParser::clean()
{
	d->dataSinks.clear();
	d->shortIDs.clear();
	d->longIDs.clear();
}

bool CommandLineParser::displayHelp()
{
	for ( int i = 0; i < d->helpList.size(); i++ ) {
		printf( "%s\n\n", printStringTable( d->helpList[i], 4, d->helpHeaders[i] ).toUtf8().constData() );
	}
	return true;
}

bool CommandLineParser::parse( bool ignoreMissing )
{
	QStringList args = QCoreApplication::arguments();
	for ( int i = 1; i < args.size(); i++ ) {

		QString argument = args[i];
		QString data = "yes";
		int separatorIndex = argument.indexOf( '=' );
		if ( separatorIndex != -1 ) {
			data = argument.mid( separatorIndex + 1 );
			argument = argument.left( separatorIndex );
		}

		// long or short setting?
		QList< Location > settings;
		if ( argument.startsWith( "--" ) ) {
			argument = argument.mid( 2 );
			settings = d->longIDs.values( argument );
		} else if ( argument.startsWith( '-' ) ) {
			argument = argument.mid( 1 );
			settings = d->shortIDs.values( argument );
		} else {
			qWarning() << "Invalid command line option:" << args[i];
			return false;
		}

		// setting available and uniquely identifiable?
		if ( settings.empty() ) {
			if ( ignoreMissing )
				continue;

			qWarning() << "Invalid command line option:" << args[i];
			return false;
		}
		if ( settings.size() > 1 ) {
			qWarning() << "Ambiguous command line option not available:" << args[i];
			return false;
		}

		// set setting
		settings.front().dataSink->SetSetting( settings.front().id, data );
	}
	return true;
}

