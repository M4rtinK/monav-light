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

#include "interfaces/iimporter.h"
#ifndef NOGUI
#include "ggdialog.h"
#endif
#include "gpsgrid.h"
#include "cell.h"
#include "table.h"
#include "utils/intersection.h"
#include "utils/qthelpers.h"
#include <vector>

#include <QFile>
#include <algorithm>

GPSGrid::GPSGrid()
{
}

GPSGrid::~GPSGrid()
{
}

QString GPSGrid::GetName()
{
	return "GPS Grid";
}

int GPSGrid::GetFileFormatVersion()
{
	return 1;
}

GPSGrid::Type GPSGrid::GetType()
{
	return GPSLookup;
}

bool GPSGrid::LoadSettings( QSettings* /*settings*/ )
{
	return true;
}

bool GPSGrid::SaveSettings( QSettings* /*settings*/ )
{
	return true;
}

bool GPSGrid::Preprocess( IImporter* importer, QString dir )
{
	QString filename = fileInDirectory( dir, "GPSGrid" );

	QFile gridFile( filename + "_grid" );
	QFile configFile( filename + "_config" );

	if ( !openQFile( &gridFile, QIODevice::WriteOnly ) )
		return false;
	if ( !openQFile( &configFile, QIODevice::WriteOnly ) )
		return false;

	std::vector< IImporter::RoutingNode > inputNodes;
	std::vector< IImporter::RoutingEdge > inputEdges;
	std::vector< unsigned > nodeIDs;
	std::vector< unsigned > edgeIDs;
	std::vector< IImporter::RoutingNode > edgePaths;

	if ( !importer->GetRoutingNodes( &inputNodes ) )
		return false;
	if ( !importer->GetRoutingEdges( &inputEdges ) )
		return false;
	if ( !importer->GetIDMap( &nodeIDs ) )
		return false;
	if ( !importer->GetEdgeIDMap( &edgeIDs ) )
		return false;
	if ( !importer->GetRoutingEdgePaths( &edgePaths ) )
		return false;

	static const int width = 32 * 32 * 32;

	std::vector< GridImportEdge > grid;

	Timer time;
	for ( std::vector< IImporter::RoutingEdge >::const_iterator i = inputEdges.begin(); i != inputEdges.end(); ++i ) {
		std::vector< UnsignedCoordinate > path;
		path.push_back( inputNodes[i->source].coordinate );
		for ( unsigned pathID = 0; pathID < i->pathLength; pathID++ )
			path.push_back( edgePaths[pathID + i->pathID].coordinate );
		path.push_back( inputNodes[i->target].coordinate );

		std::vector< std::pair< unsigned, unsigned > > gridCells;
		for ( unsigned segment = 1; segment < path.size(); segment++ ) {
			ProjectedCoordinate sourceCoordinate = path[segment - 1].ToProjectedCoordinate();
			ProjectedCoordinate targetCoordinate = path[segment].ToProjectedCoordinate();
			sourceCoordinate.x *= width;
			sourceCoordinate.y *= width;
			targetCoordinate.x *= width;
			targetCoordinate.y *= width;

			NodeID minYGrid = floor( sourceCoordinate.y );
			NodeID minXGrid = floor( sourceCoordinate.x );
			NodeID maxYGrid = floor( targetCoordinate.y );
			NodeID maxXGrid = floor( targetCoordinate.x );

			if ( minYGrid > maxYGrid )
				std::swap( minYGrid, maxYGrid );
			if ( minXGrid > maxXGrid )
				std::swap( minXGrid, maxXGrid );

			for ( NodeID yGrid = minYGrid; yGrid <= maxYGrid; ++yGrid ) {
				for ( NodeID xGrid = minXGrid; xGrid <= maxXGrid; ++xGrid ) {
					if ( !clipEdge( sourceCoordinate, targetCoordinate, ProjectedCoordinate( xGrid, yGrid ), ProjectedCoordinate( xGrid + 1, yGrid + 1 ) ) )
						continue;

					gridCells.push_back( std::pair< unsigned, unsigned >( xGrid, yGrid ) );
				}
			}
		}
		std::sort( gridCells.begin(), gridCells.end() );
		gridCells.resize( std::unique( gridCells.begin(), gridCells.end() ) - gridCells.begin() );

		GridImportEdge clippedEdge;
		clippedEdge.edge = i - inputEdges.begin();

		for ( unsigned cell = 0; cell < gridCells.size(); cell++ ) {
			clippedEdge.x = gridCells[cell].first;
			clippedEdge.y = gridCells[cell].second;
			grid.push_back( clippedEdge );
		}
	}
	qDebug() << "GPS Grid: distributed edges:" << time.restart() << "ms";
	qDebug() << "GPS Grid: overhead:" << grid.size() - inputEdges.size() << "duplicated edges";
	qDebug() << "GPS Grid: overhead:" << ( grid.size() - inputEdges.size() ) * 100 / inputEdges.size() << "% duplicated edges";

	std::sort( grid.begin(), grid.end() );
	qDebug() << "GPS Grid: sorted edges:" << time.restart() << "ms";

	std::vector< gg::GridIndex > tempIndex;
	qint64 position = 0;
	for ( std::vector< GridImportEdge >::const_iterator edge = grid.begin(); edge != grid.end(); ) {
		gg::Cell cell;
		gg::GridIndex entry;
		entry.x = edge->x;
		entry.y = edge->y;
		entry.position = position;

		tempIndex.push_back( entry );
		do {
			const IImporter::RoutingEdge& originalEdge = inputEdges[edge->edge];
			gg::Cell::Edge newEdge;
			newEdge.source = nodeIDs[originalEdge.source];
			newEdge.target = nodeIDs[originalEdge.target];
			newEdge.edgeID = edgeIDs[edge->edge];
			newEdge.bidirectional = originalEdge.bidirectional;
			newEdge.pathID = cell.coordinates.size();
			newEdge.pathLength = 2 + originalEdge.pathLength;
			cell.coordinates.push_back( inputNodes[originalEdge.source].coordinate );
			for ( unsigned pathID = 0; pathID < originalEdge.pathLength; pathID++ )
				cell.coordinates.push_back( edgePaths[pathID + originalEdge.pathID].coordinate );
			cell.coordinates.push_back( inputNodes[originalEdge.target].coordinate );
			cell.edges.push_back( newEdge );
			edge++;
		} while ( edge != grid.end() && edge->x == entry.x && edge->y == entry.y );

		ProjectedCoordinate min( ( double ) entry.x / width, ( double ) entry.y / width );
		ProjectedCoordinate max( ( double ) ( entry.x + 1 ) / width, ( double ) ( entry.y + 1 ) / width );

		unsigned maxSize = cell.edges.size() * sizeof( gg::Cell::Edge ) * 2 + cell.coordinates.size() * sizeof( UnsignedCoordinate ) * 2 + 100;
		unsigned char* buffer = new unsigned char[maxSize];
		memset( buffer, 0, maxSize );
		int size = cell.write( buffer, UnsignedCoordinate( min ), UnsignedCoordinate( max ) );
		assert( size < ( int ) maxSize );

#ifndef NDEBUG
		gg::Cell unpackCell;
		unpackCell.read( buffer, UnsignedCoordinate( min ), UnsignedCoordinate( max ) );
		assert( unpackCell == cell );
#endif

		gridFile.write( ( const char* ) &size, sizeof( size ) );
		gridFile.write( ( const char* ) buffer, size );

		delete[] buffer;
		position += size + sizeof( size );
	}
	qDebug() << "GPS Grid: wrote cells:" << time.restart() << "ms";

	gg::Index::Create( filename + "_index", tempIndex );
	qDebug() << "GPS Grid: created index:" << time.restart() << "ms";

	return true;
}

bool GPSGrid::clipHelper( double directedProjection, double directedDistance, double* tMinimum, double* tMaximum ) {
	if ( directedProjection == 0 ) {
		if ( directedDistance < 0 )
			return false;
	} else {
		double amount = directedDistance / directedProjection;
		if ( directedProjection < 0 ) {
			if ( amount > *tMaximum )
				return false;
			else if ( amount > *tMinimum )
				*tMinimum = amount;
		} else {
			if ( amount < *tMinimum )
				return false;
			else if ( amount < *tMaximum )
				*tMaximum = amount;
		}
	}
	return true;
}

bool GPSGrid::clipEdge( ProjectedCoordinate source, ProjectedCoordinate target, ProjectedCoordinate min, ProjectedCoordinate max ) {
	const double xDiff = target.x - source.x;
	const double yDiff = target.y - source.y;
	double tMinimum = 0, tMaximum = 1;

	if ( clipHelper( -xDiff, source.x - min.x, &tMinimum, &tMaximum ) )
		if ( clipHelper( xDiff, max.x - source.x, &tMinimum, &tMaximum ) )
			if ( clipHelper( -yDiff, source.y - min.y, &tMinimum, &tMaximum ) )
				if ( clipHelper( yDiff, max.y - source.y, &tMinimum, &tMaximum ) )
					return true;
	return false;
}

#ifndef NOGUI
bool GPSGrid::GetSettingsWindow( QWidget** window )
{
	*window = new GGDialog();
	return true;
}

bool GPSGrid::FillSettingsWindow( QWidget* /*window*/ )
{
	return true;
}

bool GPSGrid::ReadSettingsWindow( QWidget* /*window*/ )
{
	return true;
}
#endif

Q_EXPORT_PLUGIN2(gpsgrid, GPSGrid)
