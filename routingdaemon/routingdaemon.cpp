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

#include <QtDebug>
#
#include "routingdaemon.h"

Q_IMPORT_PLUGIN( ContractionHierarchiesClient );
Q_IMPORT_PLUGIN( GPSGridClient );

QtMessageHandler oldHandler = NULL;
RoutingDaemon* servicePointer = NULL;

void MessageBoxHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
	switch (type) {
	case QtDebugMsg:
		servicePointer->logMessage( msg.toLocal8Bit().constData(), QtServiceBase::Information );
		break;
	case QtWarningMsg:
		servicePointer->logMessage( msg.toLocal8Bit().constData(), QtServiceBase::Warning );
		break;
	case QtCriticalMsg:
		servicePointer->logMessage( msg.toLocal8Bit().constData(), QtServiceBase::Error );
		break;
	case QtFatalMsg:
		servicePointer->logMessage( msg.toLocal8Bit().constData(), QtServiceBase::Error );
		exit( -1 );
		break;
	}
	if ( oldHandler != NULL )
		oldHandler( type, context, msg.toLocal8Bit().constData() );
}

int main( int argc, char** argv )
{
	if ( argc == 2 && argv[1] == QString( "--help" ) ) {
		qDebug() << "usage:" << argv[0];
		qDebug() << "\tstarts the service";
		qDebug() << "usage:" << argv[0] << "-i | -install";
		qDebug() << "\tinstalls the service";
		qDebug() << "usage:" << argv[0] << "-u | -uninstall";
		qDebug() << "\tuninstalls the service";
		qDebug() << "usage:" << argv[0] << "-t | -terminate";
		qDebug() << "\tterminates the service";
		qDebug() << "usage:" << argv[0] << "-v | -version";
		qDebug() << "\tdisplays version and status";
		return 1;
	}

	if ( argc == 2 && ( argv[1] == QString( "-v" ) || argv[1] == QString( "-version" ) ) )
	{
		qDebug() << "Monav Routing Daemon";
		qDebug() << "Version: " << "0.4";
	}

	RoutingDaemon service( argc, argv );
	servicePointer = &service;

	oldHandler = qInstallMessageHandler( MessageBoxHandler );
	return service.exec();
}

