/*
Copyright 2010  Christian Vetter veaac.fdirct@gmail.com

This file is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This file is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this file. If not, see <http://www.gnu.org/licenses/>.
*/

#include "signals.pb.h"
#include "signals.h"
#include <QtCore>
#include <QLocalSocket>

using namespace MoNav;

bool processArguments( CommandType& type, UnpackCommand& unpack, RoutingCommand& routing, int argc, char** argv ) {
	if ( argc == 2 ) {
		type.set_value( CommandType::UNPACK_COMMAND );
		unpack.set_map_module_file( argv[1] );
		return true;
	}
	if ( argc < 6 || ( argc & 1 ) == 1 ) {
		return false;
	}

	type.set_value( CommandType::ROUTING_COMMAND );
	routing.set_data_directory( argv[1] );

	for ( int i = 2; i < argc; i+=2 ) {
		bool ok;
		Node* waypoint = routing.add_waypoints();
		waypoint->set_latitude( QString( argv[i] ).toDouble( &ok ) );
		if ( !ok ) {
			return false;
		}
		waypoint->set_longitude( QString( argv[i + 1] ).toDouble( &ok ) );
		if ( !ok ) {
			return false;
		}
	}
	return true;
}

int main( int argc, char *argv[] ) {
	CommandType commandType;

	UnpackCommand unpackCommand;

	RoutingCommand routingCommand;
	routingCommand.set_lookup_edge_names( true );

	if ( !processArguments( commandType, unpackCommand, routingCommand, argc, argv ) ) {
		qDebug() << "usage:" << argv[0] << "data-directory latitude1 longitude1 latitude2 longitude2 [...latitudeN longitudeN]";
		qDebug() << "\tcomputes a route using between the specified waypoints";
		qDebug() << "usage:" << argv[0] << "monav-map-module-file";
		qDebug() << "\tunpacks a map module";
		return 1;
	}

	QLocalSocket connection;
	connection.connectToServer( "MoNavD" );

	if ( !connection.waitForConnected() ) {
		qDebug() << "failed to connect to daemon:" << connection.error();
		return 2;
	}

	MessageWrapper<CommandType, QLocalSocket>::write( &connection, commandType );
	connection.flush();

	if ( commandType.value() == CommandType::UNPACK_COMMAND ) {
		MessageWrapper<UnpackCommand, QLocalSocket>::write( &connection, unpackCommand );
		connection.flush();

		UnpackResult reply;
		MessageWrapper<UnpackResult, QLocalSocket>::read( &connection, reply );

		qDebug() << connection.state();

		if ( reply.type() == UnpackResult::FAIL_UNPACKING ) {
			qDebug() << "failed to unpack map file";
			return 3;
		}
		qDebug() << "finished unpacking map file";
		return 0;
	}

	MessageWrapper<RoutingCommand, QLocalSocket>::write( &connection, routingCommand );
	connection.flush();

	RoutingResult reply;
	MessageWrapper<RoutingResult, QLocalSocket>::read( &connection, reply );

	if ( reply.type() == RoutingResult::LOAD_FAILED ) {
		qDebug() << "failed to load data directory";
		return 3;
	} else if ( reply.type() == RoutingResult::LOOKUP_FAILED ) {
		qDebug() << "failed to lookup nearest edge";
		return 3;
	} else if ( reply.type() == RoutingResult::ROUTE_FAILED ) {
		qDebug() << "failed to compute route";
		return 3;
	} else if ( reply.type() == RoutingResult::NAME_LOOKUP_FAILED ) {
		qDebug() << "failed to compute route";
		return 3;
	} else if ( reply.type() == RoutingResult::TYPE_LOOKUP_FAILED ) {
		qDebug() << "failed to compute route";
		return 3;
	}else if ( reply.type() == RoutingResult::SUCCESS ) {
		int seconds = reply.seconds();
		qDebug() << "distance:" << seconds / 60 / 60 << "h" << ( seconds / 60 ) % 60 << "m" << seconds % 60 << "s";
		qDebug() << "nodes:" << reply.nodes_size();
		qDebug() << "edges:" << reply.edges_size();

		unsigned n_nodes = 0;
		for ( int i = 0; i < reply.edges_size(); i++ ) {
			Edge edge = reply.edges(i);
			QString name = reply.edge_names( edge.name_id() ).c_str();
			QString type = reply.edge_types( edge.type_id() ).c_str();

			qDebug() << "name:" << name.toUtf8() << "type:" << type << "nodes:" << edge.n_segments() + 1 << "seconds:" << edge.seconds() << "branching possible:" << edge.branching_possible();

			for ( unsigned j = 0; j <= edge.n_segments(); j++ ) {
				QString latitude, longitude;
				Node node = reply.nodes( j + n_nodes );
				latitude.setNum( node.latitude(), 'g', 10 );
				longitude.setNum( node.longitude(), 'g', 10 );
				qDebug() << latitude.toLatin1().data() << longitude.toLatin1().data();
			}
			n_nodes += reply.edges(i).n_segments();
		}
	} else {
		qDebug() << "return value not recognized";
		return 5;
	}
}
