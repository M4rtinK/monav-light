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
#include <QList>
#include <QJsonDocument>

#include "interfaces/irouter.h"
#include "interfaces/igpslookup.h"
#include "utils/directoryunpacker.h"

#include "signals.h"
//#include "signals.pb.h"

template <class QObject>
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


	//	// Execute unpack command.
	//	MoNav::UnpackResult execute( const MoNav::UnpackCommand command )
	//	{
	//		MoNav::UnpackResult result;

	//		result.set_type( MoNav::UnpackResult::SUCCESS );

	//		DirectoryUnpacker unpacker( command.map_module_file().c_str() );
	//		if ( !unpacker.decompress( command.delete_file() ) ) {
	//			result.set_type( MoNav::UnpackResult::FAIL_UNPACKING );
	//		}

	//		return result;
	//	}

	// Execute routing command.
	QJsonObject route( const QJsonObject command) {
		qDebug("execute running");
		//QJsonObject command = commandDoc.object();

		//MoNav::RoutingResult result;
		QJsonObject result;

		QJsonArray nodes;
		QJsonArray edges;
		QJsonArray edgeNames;
		QJsonArray edgeTypes;

		//result.set_type( MoNav::RoutingResult::SUCCESS );
		result["status"] = QString("SUCCESS");
		qDebug("parsing data directory");
		QString dataDirectory = command["dataDirectory"].toString();
		qDebug() << dataDirectory;
		if ( !m_loaded || dataDirectory != m_dataDirectory ) {
			qDebug() << "unloading plugins";
			unloadPlugins();
			qDebug() << "loading plugins";
			m_loaded = loadPlugins( dataDirectory );
			qDebug() << "loaded plugins";
			m_dataDirectory = dataDirectory;
		}

		qDebug("loading routing data");
		if ( m_loaded ) {
			qDebug("routing data loaded");
			QVector< IRouter::Node > pathNodes;
			QVector< IRouter::Edge > pathEdges;
			double distance = 0;
			bool success = true;

			QJsonArray waypoints = command["waypoints"].toArray();

			for ( int i = 1; i < waypoints.count(); i++ ) {
				if ( i != 1 ) {
					// Remove last node.
					//result.mutable_nodes( result.nodes_size() - 1 )->Clear();
					result["nodes"].toArray().removeLast();
				}
				double segmentDistance;
				pathNodes.clear();
				pathEdges.clear();
				// compute the route segment
				QJsonObject routingStatus = computeRoute( &segmentDistance, &pathNodes, &pathEdges, waypoints.at(i - 1).toArray(), waypoints.at(i).toArray(), command["routingRadius"].toDouble() );
				// set the rotuing status to the result
				result["status"] = routingStatus["status"];
				result["statusMessage"] = routingStatus["statusMessage"];
				if ( result["status"].toString() != "SUCCESS" ) {
					qDebug() << "routing failed";
					success = false;
					break;
				}
				distance += segmentDistance;

				for ( int j = 0; j < pathNodes.size(); j++ ) {
					GPSCoordinate gps = pathNodes[j].coordinate.ToGPSCoordinate();
					//MoNav::Node* node = result.add_nodes();
					//node->set_latitude( gps.latitude );
					//node->set_longitude( gps.longitude );
					QJsonArray nodeArray;
					nodeArray.append(gps.latitude);
					nodeArray.append(gps.longitude);
					//nodes.append(QJsonArray::fromVariantList(QVariantList(gps.latitude, gps.longitude)));
					nodes.append(nodeArray);
				}

				for ( int j = 0; j < pathEdges.size(); j++ ) {
					//MoNav::Edge* edge = result.add_edges();
					//edge->set_n_segments( pathEdges[j].length );
					//edge->set_name_id( pathEdges[j].name );
					//edge->set_type_id( pathEdges[j].type );
					//edge->set_seconds( pathEdges[j].seconds );
					//edge->set_branching_possible( pathEdges[j].branchingPossible );
					QJsonArray edgeArray;
					edgeArray.append(QJsonValue(pathEdges[j].length));
					edgeArray.append(QJsonValue(int(pathEdges[j].name)));
					edgeArray.append(QJsonValue(pathEdges[j].type));
					edgeArray.append(QJsonValue(int(pathEdges[j].seconds)));
					edgeArray.append(QJsonValue(pathEdges[j].branchingPossible));
					//edgeArray  << pathEdges[j].length << pathEdges[j].name << pathEdges[j].type << pathEdges[j].seconds << pathEdges[j].branchingPossible;
					//edges.append(QJsonArray(pathEdges[j].length, pathEdges[j].name, pathEdges[j].type, pathEdges[j].seconds, pathEdges[j].branchingPossible));
					edges.append(edgeArray);
				}
			}
			//TODO: find why distance is equal to seconds in Monav routing results :D
			result["seconds"] = distance;

			if ( success ) {
				if ( command["lookupEdgeNames"].toBool() ) {
					qDebug() <<"looking up edge names";
					unsigned lastNameID = std::numeric_limits< unsigned >::max();
					QString lastName;
					unsigned lastTypeID = std::numeric_limits< unsigned >::max();
					QString lastType;
					//for ( int j = 0; j < result.edges_size(); j++ ) {
					for ( int j = 0; j < edges.count(); j++ ) {
						//MoNav::Edge* edge = result.mutable_edges( j );

						// Edge fields:
						// n_segments, name_id, type_id, seconds, branching_possible

						unsigned currentNameId = edges.at(j).toArray().at(1).toInt();
						unsigned currentTypeId = edges.at(j).toArray().at(2).toInt();
						//if ( lastNameID != edge->name_id() ) {
						if ( lastNameID != currentNameId ) {
							//lastNameID = edge->name_id();
							lastNameID = currentNameId;
							if ( !m_router->GetName( &lastName, lastNameID ) )
								//result.set_type( MoNav::RoutingResult::NAME_LOOKUP_FAILED );
								result["status"] = QString("NAME_LOOKUP_FAILED");
							result["statusMessage"] = QString("edge name lookup failed");
							//result.add_edge_names( lastName.toStdString() );
							edgeNames.append(lastName);
						}

						//if ( lastTypeID != edge->type_id() ) {
						if ( lastTypeID != currentTypeId ) {
							//lastTypeID = edge->type_id();
							lastTypeID = currentTypeId;
							if ( !m_router->GetType( &lastType, lastTypeID ) )
								//result.set_type( MoNav::RoutingResult::TYPE_LOOKUP_FAILED );
								result["status"] = QString("TYPE_LOOKUP_FAILED");
							result["statusMessage"] = QString("edge type lookup failed");
							//result.add_edge_types( lastType.toStdString() );
							edgeTypes.append(lastType);
						}

						//edge->set_name_id( result.edge_names_size() - 1 );
						//edge->set_type_id( result.edge_types_size() - 1 );

						// to modify the edge arrays, we need to retrieve them, modify them
						// and thern replace the old ones with the modified ones in the main
						// edge array
						QJsonArray edgeA = edges.at(j).toArray();
						edgeA.replace(1, QJsonValue(edgeNames.count() - 1));
						edgeA.replace(2, QJsonValue(edgeTypes.count() - 1));
						edges.replace(j, edgeA);

					}
				}

				// add the individual JSON arrays to the main JSON object
				result["nodes"] = nodes;
				result["edges"] = edges;
				result["edgeNames"] = edgeNames;
				result["edgeTypes"] = edgeTypes;

			}
		} else {
			result["status"] = QString("LOAD_FAILED");
			result["statusMessage"] = QString("loading of offline routing data failed");
		}

		return result;
	}

	QJsonObject computeRoute( double* resultDistance, QVector< IRouter::Node >* resultNodes, QVector< IRouter::Edge >* resultEdge, QJsonArray source, QJsonArray target, double lookupRadius ) {
		qDebug() << "computeRoute running";
		QJsonObject result;
		if ( m_gpsLookup == NULL || m_router == NULL ) {
			QString loadFailed = QString("tried to query route before setting valid data directory");
			qCritical() << loadFailed;
			result["status"] = QString("LOAD_FAILED");
			result["statusMessage"] = loadFailed;
			return result;
		}
		qDebug() << "parsing start and destination";
		UnsignedCoordinate sourceCoordinate( GPSCoordinate( source.at(0).toDouble(), source.at(1).toDouble() ) );
		UnsignedCoordinate targetCoordinate( GPSCoordinate( target.at(0).toDouble(), target.at(1).toDouble() ) );
		IGPSLookup::Result sourcePosition;
		QTime time;
		time.start();
		bool found = m_gpsLookup->GetNearestEdge( &sourcePosition, sourceCoordinate, lookupRadius, source.at(2).toInt(), source.at(3).toInt() );
		qDebug() << "GPS Lookup:" << time.restart() << "ms";
		if ( !found ) {
			QString noEdgeNearSource = QString("no edge near source found");
			qDebug() << noEdgeNearSource;
			result["status"] = QString("SOURCE_LOOKUP_FAILED");
			result["statusMessage"] = noEdgeNearSource;
			return result;
		}
		IGPSLookup::Result targetPosition;
		found = m_gpsLookup->GetNearestEdge( &targetPosition, targetCoordinate, lookupRadius, target.at(2).toInt(), target.at(3).toInt() );
		qDebug() << "GPS Lookup:" << time.restart() << "ms";
		if ( !found ) {
			QString noEdgeNearTarget = QString("no edge near target found");
			qDebug() << noEdgeNearTarget;
			result["status"] = QString("TARGET_LOOKUP_FAILED");
			result["statusMessage"] = noEdgeNearTarget;
			return result;
		}
		found = m_router->GetRoute( resultDistance, resultNodes, resultEdge, sourcePosition, targetPosition );

		qDebug() << "Routing:" << time.restart() << "ms";
		qDebug() << resultNodes->count();

		if ( !found ) {
			QString routeFailed = QString("routing failed");
			qDebug() << routeFailed;
			result["status"] = QString("ROUTE_FAILED");
			result["statusMessage"] = routeFailed;
			return result;
		}

		result["status"] = QString("SUCCESS");
		result["statusMessage"] = QString("routing successfull");
		return result;
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
				qDebug() << "mrouter!";
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
