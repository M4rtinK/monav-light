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

#include "testimporter.h"
#include "utils/qthelpers.h"
#include <algorithm>
#include <QtDebug>
#include <QSettings>
#include <limits>
#include <vector>
#include <QTextStream>

TestImporter::TestImporter()
{
	m_settings.forbidUTurn = false;
	m_settings.uTurnPenalty = 0;
}

TestImporter::~TestImporter()
{
}

QString TestImporter::GetName()
{
	return "Test Importer";
}

void TestImporter::SetOutputDirectory( const QString& dir )
{
	m_outputDirectory = dir;
}

bool TestImporter::LoadSettings( QSettings* settings )
{
	if ( settings == NULL )
		return false;
	settings->beginGroup( "Test Importer" );
	m_settings.forbidUTurn = settings->value( "forbidUTurn", false ).toBool();
	m_settings.uTurnPenalty = settings->value( "uTurnPenalty", 0 ).toInt();
	settings->endGroup();
	return true;
}

bool TestImporter::SaveSettings( QSettings* settings )
{
	if ( settings == NULL )
		return false;
	settings->beginGroup( "Test Importer" );
	settings->setValue( "forbidUTurn", m_settings.forbidUTurn );
	settings->setValue( "uTurnPenalty", m_settings.uTurnPenalty );
	settings->endGroup();
	return true;
}

bool TestImporter::Preprocess( QString inputFilename )
{
	if ( inputFilename.endsWith( ".ddsg" ) )
		return parseDDSG( inputFilename );
	return false;
}

bool TestImporter::parseDDSG( QString filename )
{
	QString outputPrefix = fileInDirectory( m_outputDirectory, "Test Importer" );

	FileStream edgesData( outputPrefix + "_mapped_edges" );
	FileStream routingCoordinatesData( outputPrefix + "_routing_coordinates" );
	FileStream edgePathsData( outputPrefix + "_paths" );
	FileStream wayRefsData( outputPrefix + "_way_refs" );
	FileStream wayTypesData( outputPrefix + "_way_types" );
	FileStream penaltyData( outputPrefix + "_penalties" );
	FileStream boundingBoxData( outputPrefix + "_bounding_box" );

	if ( !edgesData.open( QIODevice::WriteOnly ) )
		return false;
	if ( !routingCoordinatesData.open( QIODevice::WriteOnly ) )
		return false;
	if ( !edgePathsData.open( QIODevice::WriteOnly ) )
		return false;
	if ( !wayRefsData.open( QIODevice::WriteOnly ) )
		return false;
	if ( !wayTypesData.open( QIODevice::WriteOnly ) )
		return false;
	if ( !penaltyData.open( QIODevice::WriteOnly ) )
		return false;
	if ( !boundingBoxData.open( QIODevice::WriteOnly ) )
		return false;

	QFile ddsgFile( filename );
	if ( !openQFile( &ddsgFile, QIODevice::ReadOnly ) )
		return false;
	QTextStream inputStream( &ddsgFile );

	QString id;
	inputStream >> id;
	if ( id != "d" ) {
		qCritical() << "Not a ddsg file";
		return false;
	}

	wayRefsData << "";
	wayTypesData << "";

	unsigned nodes;
	unsigned edges;
	inputStream >> nodes >> edges;
	std::vector< SimpleEdge > simpleEdges;
	qDebug() << "Test Importer:" << "reading" << nodes << " nodes and" << edges << "edges";
	for ( unsigned i = 0; i < edges; i++ ) {
		unsigned from;
		unsigned to;
		unsigned weight;
		unsigned direction;
		inputStream >> from >> to >> weight >> direction;
		if ( direction == 3 )
			continue;
		if ( from == to )
			continue;
		SimpleEdge newEdge;
		newEdge.source = from;
		newEdge.target = to;
		newEdge.bidirectional = direction == 1;
		newEdge.distance = weight / 10.0;
		newEdge.forward = direction == 1 || direction == 0;
		newEdge.backward = direction == 2 || direction == 0;
		if ( from > to ) {
			std::swap( newEdge.source, newEdge.target );
			std::swap( newEdge.forward, newEdge.backward );
		}
		simpleEdges.push_back( newEdge );
	}

	std::sort( simpleEdges.begin(), simpleEdges.end(), SimpleEdge::sortToMerge );
	unsigned edgesLeft = 1;
	for ( unsigned i = 1; i < simpleEdges.size(); i++ ) {
		SimpleEdge& edge = simpleEdges[i];
		SimpleEdge& otherEdge = simpleEdges[edgesLeft - 1];
		simpleEdges[edgesLeft++] = edge;
		if ( edge.source != otherEdge.source )
			continue;
		if ( edge.target != otherEdge.target )
			continue;
		if ( edge.distance != otherEdge.distance )
			continue;
		edgesLeft--;
		otherEdge.forward |= edge.forward;
		otherEdge.backward |= edge.backward;
	}
	simpleEdges.resize( edgesLeft );

	unsigned biEdges = 0;
	for ( unsigned i = 0; i < simpleEdges.size(); i++ ) {
		if ( !simpleEdges[i].forward ) {
			std::swap( simpleEdges[i].source, simpleEdges[i].target );
			std::swap( simpleEdges[i].forward, simpleEdges[i].backward );
		}
		simpleEdges[i].bidirectional = simpleEdges[i].forward && simpleEdges[i].backward;
		biEdges += simpleEdges[i].bidirectional ? 1 : 0;
		assert( simpleEdges[i].forward );
	}

	qDebug() << "Test Importer:" << biEdges << "bidirectional edges";
	qDebug() << "Test Importer:" << edges - biEdges * 2 << "unidirectional edges";
	qDebug() << "Test Importer:" << edges - edgesLeft << "edges removed";

	std::sort( simpleEdges.begin(), simpleEdges.end() );

	std::vector< char > in( nodes, 0 );
	std::vector< char > out( nodes, 0 );
	std::vector< char > bi( nodes, 0 );
	qDebug() << "Test Importer:" << "writing" << simpleEdges.size() << "edges";
	for ( unsigned i = 0; i < simpleEdges.size(); i++ ) {
		edgesData << simpleEdges[i].source << simpleEdges[i].target << simpleEdges[i].bidirectional << simpleEdges[i].distance;
		edgesData << unsigned( 0 ) << unsigned( 0 ) << unsigned ( 0 ); // name
		edgesData << unsigned( 0 ) << unsigned( 0 ); // path
		edgesData << unsigned( 0 ) << unsigned( 0 ); // address
		edgesData << qint8( out[simpleEdges[i].source]++ ) << qint8( in[simpleEdges[i].target]++ );
		if ( simpleEdges[i].bidirectional ) {
			in[simpleEdges[i].source]++;
			out[simpleEdges[i].target]++;
			bi[simpleEdges[i].source]++;
			bi[simpleEdges[i].target]++;
			assert( in[simpleEdges[i].source] == out[simpleEdges[i].source] );
			assert( in[simpleEdges[i].target] == out[simpleEdges[i].target] );
		}
	}

	long long penalties = 0;
	long long uturns = 0;
	qDebug() << "Test Importer:" << "forbid uturns:" << m_settings.forbidUTurn;
	qDebug() << "Test Importer:" << "uturn penalty:" << m_settings.uTurnPenalty;
	for ( unsigned i = 0; i < nodes; i++ ) {
		routingCoordinatesData << unsigned( 0 ) << unsigned( 0 );
		penaltyData << unsigned( in[i] ) << unsigned( out[i] );
		for ( int from = 0; from < in[i]; from++ ) {
			for ( int to = 0; to < out[i]; to++ ) {
				double penalty = 0;
				if ( from == to && from < bi[i] ) {
					uturns++;
					if ( m_settings.forbidUTurn )
						penalty = -1;
					else
						penalty = m_settings.uTurnPenalty;
				}
				penaltyData << penalty;
				penalties++;
			}
		}
	}
	qDebug() << "Test Importer:" << "wrote" << penalties << "penalties";
	qDebug() << "Test Importer:" << uturns << "uturns";

	return true;
}

bool TestImporter::SetIDMap( const std::vector< NodeID >& idMap )
{
	FileStream idMapData( fileInDirectory( m_outputDirectory, "Test Importer" ) + "_id_map" );

	if ( !idMapData.open( QIODevice::WriteOnly ) )
		return false;

	idMapData << unsigned( idMap.size() );
	for ( NodeID i = 0; i < ( NodeID ) idMap.size(); i++ )
		idMapData << idMap[i];

	return true;
}

bool TestImporter::GetIDMap( std::vector< NodeID >* idMap )
{
	FileStream idMapData( fileInDirectory( m_outputDirectory, "Test Importer" ) + "_id_map" );

	if ( !idMapData.open( QIODevice::ReadOnly ) )
		return false;

	unsigned numNodes;

	idMapData >> numNodes;
	idMap->resize( numNodes );

	for ( NodeID i = 0; i < ( NodeID ) numNodes; i++ ) {
		unsigned temp;
		idMapData >> temp;
		( *idMap )[i] = temp;
	}

	if ( idMapData.status() == QDataStream::ReadPastEnd )
		return false;

	return true;
}

bool TestImporter::SetEdgeIDMap( const std::vector< NodeID >& idMap )
{
	FileStream idMapData( fileInDirectory( m_outputDirectory, "Test Importer" ) + "_edge_id_map" );

	if ( !idMapData.open( QIODevice::WriteOnly ) )
		return false;

	idMapData << unsigned( idMap.size() );
	for ( NodeID i = 0; i < ( NodeID ) idMap.size(); i++ )
		idMapData << idMap[i];

	return true;
}

bool TestImporter::GetEdgeIDMap( std::vector< NodeID >* idMap )
{
	FileStream idMapData( fileInDirectory( m_outputDirectory, "Test Importer" ) + "_edge_id_map" );

	if ( !idMapData.open( QIODevice::ReadOnly ) )
		return false;

	unsigned numEdges;

	idMapData >> numEdges;
	idMap->resize( numEdges );

	for ( NodeID i = 0; i < ( NodeID ) numEdges; i++ ) {
		unsigned temp;
		idMapData >> temp;
		( *idMap )[i] = temp;
	}

	if ( idMapData.status() == QDataStream::ReadPastEnd )
		return false;

	return true;
}

bool TestImporter::GetRoutingEdges( std::vector< RoutingEdge >* data )
{
	FileStream edgesData( fileInDirectory( m_outputDirectory, "Test Importer" ) + "_mapped_edges" );

	if ( !edgesData.open( QIODevice::ReadOnly ) )
		return false;

	std::vector< int > nodeOutDegree;
	while ( true ) {
		unsigned source, target, nameID, refID, type;
		unsigned pathID, pathLength;
		unsigned addressID, addressLength;
		bool bidirectional;
		double seconds;
		qint8 edgeIDAtSource, edgeIDAtTarget;

		edgesData >> source >> target >> bidirectional >> seconds;
		edgesData >> nameID >> refID >> type;
		edgesData >> pathID >> pathLength;
		edgesData >> addressID >> addressLength;
		edgesData >> edgeIDAtSource >> edgeIDAtTarget;

		if ( edgesData.status() == QDataStream::ReadPastEnd )
			break;

		RoutingEdge edge;
		edge.source = source;
		edge.target = target;
		edge.bidirectional = bidirectional;
		edge.distance = seconds;
		edge.nameID = refID;
		edge.type = type;
		edge.pathID = pathID;
		edge.pathLength = pathLength;
		edge.edgeIDAtSource = edgeIDAtSource;
		edge.edgeIDAtTarget = edgeIDAtTarget;

		data->push_back( edge );

		if ( source >= nodeOutDegree.size() )
			nodeOutDegree.resize( source + 1, 0 );
		if ( target >= nodeOutDegree.size() )
			nodeOutDegree.resize( target + 1, 0 );
		nodeOutDegree[source]++;
		if ( bidirectional )
			nodeOutDegree[target]++;
	}

	for ( unsigned edge = 0; edge < data->size(); edge++ ) {
		int branches = nodeOutDegree[data->at( edge ).target];
		branches -= data->at( edge ).bidirectional ? 1 : 0;
		( *data )[edge].branchingPossible = branches > 1;
	}

	return true;
}

bool TestImporter::GetRoutingNodes( std::vector< RoutingNode >* data )
{
	FileStream routingCoordinatesData( fileInDirectory( m_outputDirectory, "Test Importer" ) + "_routing_coordinates" );

	if ( !routingCoordinatesData.open( QIODevice::ReadOnly ) )
		return false;

	while ( true ) {
		UnsignedCoordinate coordinate;
		routingCoordinatesData >> coordinate.x >> coordinate.y;
		if ( routingCoordinatesData.status() == QDataStream::ReadPastEnd )
			break;
		RoutingNode node;
		node.coordinate = coordinate;
		data->push_back( node );
	}

	return true;
}

bool TestImporter::GetRoutingEdgePaths( std::vector< RoutingNode >* data )
{
	FileStream edgePathsData( fileInDirectory( m_outputDirectory, "Test Importer" ) + "_paths" );

	if ( !edgePathsData.open( QIODevice::ReadOnly ) )
		return false;

	while ( true ) {
		UnsignedCoordinate coordinate;
		edgePathsData >> coordinate.x >> coordinate.y;
		if ( edgePathsData.status() == QDataStream::ReadPastEnd )
			break;
		RoutingNode node;
		node.coordinate = coordinate;
		data->push_back( node );
	}
	return true;
}

bool TestImporter::GetRoutingWayNames( std::vector< QString >* data )
{
	FileStream wayRefsData( fileInDirectory( m_outputDirectory, "Test Importer" ) + "_way_refs" );

	if ( !wayRefsData.open( QIODevice::ReadOnly ) )
		return false;

	while ( true ) {
		QString ref;
		wayRefsData >> ref;
		if ( wayRefsData.status() == QDataStream::ReadPastEnd )
			break;
		data->push_back( ref );
	}
	return true;
}

bool TestImporter::GetRoutingWayTypes( std::vector< QString >* data )
{
	FileStream wayTypesData( fileInDirectory( m_outputDirectory, "Test Importer" ) + "_way_types" );

	if ( !wayTypesData.open( QIODevice::ReadOnly ) )
		return false;

	while ( true ) {
		QString type;
		wayTypesData >> type;
		if ( wayTypesData.status() == QDataStream::ReadPastEnd )
			break;
		data->push_back( type );
	}
	return true;
}

bool TestImporter::GetRoutingPenalties( std::vector< char >* inDegree, std::vector< char >* outDegree, std::vector< double >* penalties )
{
	QString filename = fileInDirectory( m_outputDirectory, "Test Importer" );

	FileStream penaltyData( filename + "_penalties" );

	if ( !penaltyData.open( QIODevice::ReadOnly ) )
		return false;

	while ( true ) {
		int in, out;
		penaltyData >> in >> out;
		if ( penaltyData.status() == QDataStream::ReadPastEnd )
			break;
		unsigned oldPosition = penalties->size();
		for ( int i = 0; i < in; i++ ) {
			for ( int j = 0; j < out; j++ ) {
				double penalty;
				penaltyData >> penalty;
				penalties->push_back( penalty );
			}
		}
		if ( penaltyData.status() == QDataStream::ReadPastEnd ) {
			penalties->resize( oldPosition );
			qCritical() << "Test Importer: Corrupt Penalty Data";
			return false;
		}
		inDegree->push_back( in );
		outDegree->push_back( out );
	}

	return true;
}

bool TestImporter::GetAddressData( std::vector< Place >* /*dataPlaces*/, std::vector< Address >* /*dataAddresses*/, std::vector< UnsignedCoordinate >* /*dataWayBuffer*/, std::vector< QString >* /*addressNames*/ )
{
	return false;
}

bool TestImporter::GetBoundingBox( BoundingBox* box )
{
	FileStream boundingBoxData( fileInDirectory( m_outputDirectory, "Test Importer" ) + "_bounding_box" );

	if ( !boundingBoxData.open( QIODevice::ReadOnly ) )
		return false;

	GPSCoordinate minGPS, maxGPS;

	boundingBoxData >> minGPS.latitude >> minGPS.longitude >> maxGPS.latitude >> maxGPS.longitude;

	if ( boundingBoxData.status() == QDataStream::ReadPastEnd ) {
		qCritical() << "error reading bounding box";
		return false;
	}

	box->min = UnsignedCoordinate( minGPS );
	box->max = UnsignedCoordinate( maxGPS );
	if ( box->min.x > box->max.x )
		std::swap( box->min.x, box->max.x );
	if ( box->min.y > box->max.y )
		std::swap( box->min.y, box->max.y );

	return true;
}

void TestImporter::DeleteTemporaryFiles()
{
	QString filename = fileInDirectory( m_outputDirectory, "Test Importer" );
	QFile::remove( filename + "_address" );
	QFile::remove( filename + "_bounding_box" );
	QFile::remove( filename + "_edge_id_map" );
	QFile::remove( filename + "_id_map" );
	QFile::remove( filename + "_mapped_edges" );
	QFile::remove( filename + "_penalties" );
	QFile::remove( filename + "_routing_coordinates" );
	QFile::remove( filename + "_way_names" );
	QFile::remove( filename + "_way_refs" );
	QFile::remove( filename + "_way_types" );
}

// IConsoleSettings
QString TestImporter::GetModuleName()
{
	return GetName();
}

bool TestImporter::GetSettingsList( QVector< Setting >* settings )
{
	settings->push_back( Setting( "", "forbid-u-turn", "forbids u-turns", "" ) );
	settings->push_back( Setting( "", "u-turn-penalty", "penalized u-turns", "seconds" ) );

	return true;
}

bool TestImporter::SetSetting( int id, QVariant data )
{
	bool ok = true;
	switch( id ) {
	case 0:
		m_settings.forbidUTurn = true;
		break;
	case 1:
		m_settings.uTurnPenalty = data.toInt( &ok );
		break;
	default:
		return false;
	}

	return ok;
}

Q_EXPORT_PLUGIN2( testimporter, TestImporter )
