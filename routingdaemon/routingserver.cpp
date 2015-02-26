/*
Copyright 2011  Thomas Miedema thomasmiedema@gmail.com

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

#include <QCoreApplication>
#include "routingserver.h"

Q_IMPORT_PLUGIN( contractionhierarchiesclient );
Q_IMPORT_PLUGIN( gpsgridclient );

int main( int argc, char** argv )
{
	if ( argc == 2 && argv[1] == QString( "--help" ) ) {
		qDebug() << "usage:" << argv[0] << "<port>";
		return 1;
	}

	// Set default port.
	quint16 port = 8040;

	if ( argc == 2 ) {
		port = atoi(argv[1]);
	}

	QCoreApplication* app = new QCoreApplication(argc, argv);
	RoutingServer server(port, app);
	
	qDebug() << "Starting MoNav TcpServer on port" << port;
	return app->exec();
}
