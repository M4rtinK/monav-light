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

#ifndef ROUTINGCOMMON_H
#define ROUTINGCOMMON_H

#include <QtCore>
#include <QSettings>
#include <QFile>
#include <QtDebug>

#include "interfaces/irouter.h"
#include "interfaces/igpslookup.h"
#include "utils/directoryunpacker.h"

#include "signals.h"
#include "signals.pb.h"

template <class Socket>
class RoutingCommon {

public:
	RoutingCommon()
	{
		m_loaded = false;
		m_gpsLookup = NULL,
		m_router = NULL;
	}

	~RoutingCommon()
	{
		unloadPlugins();
	}

protected:

	// Handle the connection before the command type is known.
	void handleConnection( Socket* connection )
	{
		MoNav::CommandType type;

		// Read the command type.
		if ( !MoNav::MessageWrapper<MoNav::CommandType, Socket>::read( connection, type ) ) {
			qDebug() << "Could not read command type.";
			return;
		}

		// Call handleConnection for specific command type.
		if ( type.value() == MoNav::CommandType::VERSION_COMMAND ) {
			handleConnection<MoNav::VersionCommand, MoNav::VersionResult>( connection );
		} else if ( type.value() == MoNav::CommandType::UNPACK_COMMAND ) {
			handleConnection<MoNav::UnpackCommand, MoNav::UnpackResult>( connection );
		} else if ( type.value() == MoNav::CommandType::ROUTING_COMMAND ) {
			handleConnection<MoNav::RoutingCommand, MoNav::RoutingResult>( connection );
		}
	}

	// Handle the connection for the given command and result type.
	template <class Command, class Result>
	void handleConnection( Socket* connection ) {
		Command command;
		Result result;

		// Read the command from the socket.
		if ( !MoNav::MessageWrapper<Command, Socket>::read( connection, command ) ) {
			qDebug() << "Could not read command.";
			return;
		}

		// Execute the command.
		result = execute( command );

		if ( connection->state() != Socket::ConnectedState ) {
			qDebug() << "Client has disconnected unexpectedly.";
			return;
		}

		// Write the result to the socket.
		MoNav::MessageWrapper<Result, Socket>::write( connection, result );

		connection->flush();
	}

	// Execute version command.
	MoNav::VersionResult execute( const MoNav::VersionCommand command ) {
		MoNav::VersionResult result = MoNav::VersionResult();
		result.set_version("0.4");

		return result;
	}

	// Execute unpack command.
	MoNav::UnpackResult execute( const MoNav::UnpackCommand command )
	{
		MoNav::UnpackResult result;

		result.set_type( MoNav::UnpackResult::SUCCESS );

		DirectoryUnpacker unpacker( command.map_module_file().c_str() );
		if ( !unpacker.decompress( command.delete_file() ) ) {
			result.set_type( MoNav::UnpackResult::FAIL_UNPACKING );
		}

		return result;
	}

	// Execute routing command.
	MoNav::RoutingResult execute( const MoNav::RoutingCommand command )
	{
		MoNav::RoutingResult result;

		result.set_type( MoNav::RoutingResult::SUCCESS );

		QString dataDirectory = command.data_directory().c_str();
		if ( !m_loaded || dataDirectory != m_dataDirectory ) {
			unloadPlugins();
			m_loaded = loadPlugins( dataDirectory );
			m_dataDirectory = dataDirectory;
		}

		if ( m_loaded ) {
			QVector< IRouter::Node > pathNodes;
			QVector< IRouter::Edge > pathEdges;
			double distance = 0;
			bool success = true;
			for ( int i = 1; i < command.waypoints_size(); i++ ) {
				if ( i != 1 ) {
					// Remove last node.
					result.mutable_nodes( result.nodes_size() - 1 )->Clear();
				}
				double segmentDistance;
				pathNodes.clear();
				pathEdges.clear();
				result.set_type( computeRoute( &segmentDistance, &pathNodes, &pathEdges, command.waypoints( i - 1 ), command.waypoints( i ), command.lookup_radius() ) );
				if ( result.type() != MoNav::RoutingResult::SUCCESS ) {
					success = false;
					break;
				}
				distance += segmentDistance;

				for ( int j = 0; j < pathNodes.size(); j++ ) {
					GPSCoordinate gps = pathNodes[j].coordinate.ToGPSCoordinate();
					MoNav::Node* node = result.add_nodes();
					node->set_latitude( gps.latitude );
					node->set_longitude( gps.longitude );
				}

				for ( int j = 0; j < pathEdges.size(); j++ ) {
					MoNav::Edge* edge = result.add_edges();
					edge->set_n_segments( pathEdges[j].length );
					edge->set_name_id( pathEdges[j].name );
					edge->set_type_id( pathEdges[j].type );
					edge->set_seconds( pathEdges[j].seconds );
					edge->set_branching_possible( pathEdges[j].branchingPossible );
				}
			}
			result.set_seconds( distance );

			if ( success ) {
				if ( command.lookup_edge_names() ) {
					unsigned lastNameID = std::numeric_limits< unsigned >::max();
					QString lastName;
					unsigned lastTypeID = std::numeric_limits< unsigned >::max();
					QString lastType;
					for ( int j = 0; j < result.edges_size(); j++ ) {
						MoNav::Edge* edge = result.mutable_edges( j );

						if ( lastNameID != edge->name_id() ) {
							lastNameID = edge->name_id();
							if ( !m_router->GetName( &lastName, lastNameID ) )
								result.set_type( MoNav::RoutingResult::NAME_LOOKUP_FAILED );
							result.add_edge_names( lastName.toStdString() );
						}

						if ( lastTypeID != edge->type_id() ) {
							lastTypeID = edge->type_id();
							if ( !m_router->GetType( &lastType, lastTypeID ) )
								result.set_type( MoNav::RoutingResult::TYPE_LOOKUP_FAILED );
							result.add_edge_types( lastType.toStdString() );
						}

						edge->set_name_id( result.edge_names_size() - 1 );
						edge->set_type_id( result.edge_types_size() - 1 );
					}
				}
			}
		} else {
			result.set_type( MoNav::RoutingResult::LOAD_FAILED );
		}

		return result;
	}

	MoNav::RoutingResult::Type computeRoute( double* resultDistance, QVector< IRouter::Node >* resultNodes, QVector< IRouter::Edge >* resultEdge, MoNav::Node source, MoNav::Node target, double lookupRadius )
	{
		if ( m_gpsLookup == NULL || m_router == NULL ) {
			qCritical() << "tried to query route before setting valid data directory";
			return MoNav::RoutingResult::LOAD_FAILED;
		}
		UnsignedCoordinate sourceCoordinate( GPSCoordinate( source.latitude(), source.longitude() ) );
		UnsignedCoordinate targetCoordinate( GPSCoordinate( target.latitude(), target.longitude() ) );
		IGPSLookup::Result sourcePosition;
		QTime time;
		time.start();
		bool found = m_gpsLookup->GetNearestEdge( &sourcePosition, sourceCoordinate, lookupRadius, source.heading_penalty(), source.heading() );
		qDebug() << "GPS Lookup:" << time.restart() << "ms";
		if ( !found ) {
			qDebug() << "no edge near source found";
			return MoNav::RoutingResult::LOOKUP_FAILED;
		}
		IGPSLookup::Result targetPosition;
		found = m_gpsLookup->GetNearestEdge( &targetPosition, targetCoordinate, lookupRadius, target.heading_penalty(), target.heading() );
		qDebug() << "GPS Lookup:" << time.restart() << "ms";
		if ( !found ) {
			qDebug() << "no edge near target found";
			return MoNav::RoutingResult::LOOKUP_FAILED;
		}
		found = m_router->GetRoute( resultDistance, resultNodes, resultEdge, sourcePosition, targetPosition );
		qDebug() << "Routing:" << time.restart() << "ms";

		if ( !found ) {
			return MoNav::RoutingResult::ROUTE_FAILED;
		}

		return MoNav::RoutingResult::SUCCESS;
	}

	bool loadPlugins( QString dataDirectory )
	{
		QDir dir( dataDirectory );
		QString configFilename = dir.filePath( "Module.ini" );
		if ( !QFile::exists( configFilename ) ) {
			qCritical() << "Not a valid routing module directory: Missing Module.ini";
			return false;
		}
		QSettings pluginSettings( configFilename, QSettings::IniFormat );
		int iniVersion = pluginSettings.value( "configVersion" ).toInt();
		if ( iniVersion != 2 ) {
			qCritical() << "Config File not compatible";
			return false;
		}
		QString routerName = pluginSettings.value( "router" ).toString();
		QString gpsLookupName = pluginSettings.value( "gpsLookup" ).toString();

		foreach ( QObject *plugin, QPluginLoader::staticInstances() )
			testPlugin( plugin, routerName, gpsLookupName );

		try
		{
			if ( m_gpsLookup == NULL ) {
				qCritical() << "GPSLookup plugin not found:" << gpsLookupName;
				return false;
			}
			int gpsLookupFileFormatVersion = pluginSettings.value( "gpsLookupFileFormatVersion" ).toInt();
			if ( !m_gpsLookup->IsCompatible( gpsLookupFileFormatVersion ) ) {
				qCritical() << "GPS Lookup file format not compatible";
				return false;
			}
			m_gpsLookup->SetInputDirectory( dataDirectory );
			if ( !m_gpsLookup->LoadData() ) {
				qCritical() << "could not load GPSLookup data";
				return false;
			}

			if ( m_router == NULL ) {
				qCritical() << "router plugin not found:" << routerName;
				return false;
			}
			int routerFileFormatVersion = pluginSettings.value( "routerFileFormatVersion" ).toInt();
			if ( !m_gpsLookup->IsCompatible( routerFileFormatVersion ) ) {
				qCritical() << "Router file format not compatible";
				return false;
			}
			m_router->SetInputDirectory( dataDirectory );
			if ( !m_router->LoadData() ) {
				qCritical() << "could not load router data";
				return false;
			}
		}
		catch ( ... )
		{
			qCritical() << "caught exception while loading plugins";
			return false;
		}

		qDebug() << "loaded:" << pluginSettings.value( "name" ).toString() << pluginSettings.value( "description" ).toString();

		return true;
	}

	void testPlugin( QObject* plugin, QString routerName, QString gpsLookupName )
	{
		if ( IGPSLookup *interface = qobject_cast< IGPSLookup* >( plugin ) ) {
			qDebug() << "found plugin:" << interface->GetName();
			if ( interface->GetName() == gpsLookupName )
				m_gpsLookup = interface;
		}
		if ( IRouter *interface = qobject_cast< IRouter* >( plugin ) ) {
			qDebug() << "found plugin:" << interface->GetName();
			if ( interface->GetName() == routerName )
				m_router = interface;
		}
	}

	void unloadPlugins()
	{
		m_router = NULL;
		m_gpsLookup = NULL;
	}

	bool m_loaded;
	QString m_dataDirectory;
	IGPSLookup* m_gpsLookup;
	IRouter* m_router;
};

#endif // ROUTINGCOMMON_H
