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

#include "contractionhierarchiesclient.h"
#include "utils/qthelpers.h"
#include <QtDebug>
#include <stack>
#ifndef NOGUI
	#include <QMessageBox>
#endif

ContractionHierarchiesClient::ContractionHierarchiesClient()
{
	m_heapForward = NULL;
	m_heapBackward = NULL;
}

ContractionHierarchiesClient::~ContractionHierarchiesClient()
{
	UnloadData();
}


QString ContractionHierarchiesClient::GetName()
{
	return "Contraction Hierarchies";
}

void ContractionHierarchiesClient::SetInputDirectory( const QString& dir )
{
	m_directory = dir;
}

void ContractionHierarchiesClient::ShowSettings()
{
#ifndef NOGUI
	QMessageBox::information( NULL, "Settings", "No settings available" );
#endif
}

bool ContractionHierarchiesClient::UnloadData()
{
	if ( m_heapForward != NULL )
		delete m_heapForward;
	m_heapForward = NULL;
	if ( m_heapBackward != NULL )
		delete m_heapBackward;
	m_heapBackward = NULL;
	m_types.clear();
	m_graph.unloadGraph();

	return true;
}

bool ContractionHierarchiesClient::IsCompatible( int fileFormatVersion )
{
	if ( fileFormatVersion == 1 )
		return true;
	return false;
}

bool ContractionHierarchiesClient::LoadData()
{
	QString filename = fileInDirectory( m_directory,"Contraction Hierarchies" );
	UnloadData();

	if ( !m_graph.loadGraph( filename, 1024 * 1024 * 4 ) )
		return false;

	m_namesFile.setFileName( filename + "_names" );
	if ( !openQFile( &m_namesFile, QIODevice::ReadOnly ) )
		return false;
	m_names = ( const char* ) m_namesFile.map( 0, m_namesFile.size() );
	if ( m_names == NULL )
		return false;
	m_namesFile.close();

	m_heapForward = new Heap( m_graph.numberOfNodes() );
	m_heapBackward = new Heap( m_graph.numberOfNodes() );

	QFile typeFile( filename + "_types" );
	if ( !openQFile( &typeFile, QIODevice::ReadOnly ) )
		return false;

	QByteArray buffer = typeFile.readAll();
	QString types = QString::fromUtf8( buffer.constData() );
	m_types = types.split( ';' );

	return true;
}

bool ContractionHierarchiesClient::GetRoute( double* distance, QVector< Node>* pathNodes, QVector< Edge >* pathEdges, const IGPSLookup::Result& source, const IGPSLookup::Result& target )
{
	assert( distance != NULL );
	m_heapForward->Clear();
	m_heapBackward->Clear();

	*distance = computeRoute( source, target, pathNodes, pathEdges );
	if ( *distance == std::numeric_limits< int >::max() )
		return false;

	// is it shorter to drive along the edge?
	if ( target.source == source.source && target.target == source.target && source.edgeID == target.edgeID ) {
		EdgeIterator targetEdge = m_graph.findEdge( target.source, target.target, target.edgeID );
		double onEdgeDistance = fabs( target.percentage - source.percentage ) * targetEdge.distance();
		if ( onEdgeDistance < *distance ) {
			if ( ( targetEdge.forward() && targetEdge.backward() ) || source.percentage < target.percentage ) {
				if ( pathNodes != NULL && pathEdges != NULL )
				{
					pathNodes->clear();
					pathEdges->clear();
					pathNodes->push_back( source.nearestPoint );

					QVector< Node > tempNodes;
					if ( targetEdge.unpacked() )
						m_graph.path( targetEdge, &tempNodes, pathEdges, target.target == targetEdge.target() );
					else
						pathEdges->push_back( targetEdge.description() );

					if ( target.previousWayCoordinates < source.previousWayCoordinates ) {
						for ( unsigned pathID = target.previousWayCoordinates; pathID < source.previousWayCoordinates; pathID++ )
							pathNodes->push_back( tempNodes[pathID - 1] );
						std::reverse( pathNodes->begin() + 1, pathNodes->end() );
					} else {
						for ( unsigned pathID = source.previousWayCoordinates; pathID < target.previousWayCoordinates; pathID++ )
							pathNodes->push_back( tempNodes[pathID - 1] );
					}

					pathNodes->push_back( target.nearestPoint );
					pathEdges->front().length = pathNodes->size() - 1;
				}

				*distance = onEdgeDistance;
			}
		}
	}

	*distance /= 10;
	return true;
}

bool ContractionHierarchiesClient::GetName( QString* result, unsigned name )
{
	*result =  QString::fromUtf8( m_names + name );
	return true;
}

bool ContractionHierarchiesClient::GetNames( QVector< QString >* result, QVector< unsigned > names )
{
	result->resize( names.size() );
	for ( int i = 0; i < names.size(); i++ )
		( *result )[i] = QString::fromUtf8( m_names + names[i] );
	return true;
}

bool ContractionHierarchiesClient::GetType( QString* result, unsigned type )
{
	*result = m_types[type];
	return true;
}

bool ContractionHierarchiesClient::GetTypes( QVector< QString >* result, QVector< unsigned > types )
{
	result->resize( types.size() );
	for ( int i = 0; i < types.size(); i++ )
		( *result )[i] = m_types[types[i]];
	return true;
}

template< class EdgeAllowed, class StallEdgeAllowed >
void ContractionHierarchiesClient::computeStep( Heap* heapForward, Heap* heapBackward, const EdgeAllowed& edgeAllowed, const StallEdgeAllowed& stallEdgeAllowed, NodeIterator* middle, int* targetDistance ) {

	const NodeIterator node = heapForward->DeleteMin();
	const int distance = heapForward->GetKey( node );

	if ( heapForward->GetData( node ).stalled )
		return;

	if ( heapBackward->WasInserted( node ) && !heapBackward->GetData( node ).stalled ) {
		const int newDistance = heapBackward->GetKey( node ) + distance;
		if ( newDistance < *targetDistance ) {
			*middle = node;
			*targetDistance = newDistance;
		}
	}

	if ( distance > *targetDistance ) {
		heapForward->DeleteAll();
		return;
	}
	for ( EdgeIterator edge = m_graph.edges( node ); edge.hasEdgesLeft(); ) {
		m_graph.unpackNextEdge( &edge );
		const NodeIterator to = edge.target();
		const int edgeWeight = edge.distance();
		assert( edgeWeight > 0 );
		const int toDistance = distance + edgeWeight;

		if ( stallEdgeAllowed( edge.forward(), edge.backward() ) && heapForward->WasInserted( to ) ) {
			const int shorterDistance = heapForward->GetKey( to ) + edgeWeight;
			if ( shorterDistance < distance ) {
				//perform a bfs starting at node
				//only insert nodes when a sub-optimal path can be proven
				//insert node into the stall queue
				heapForward->GetKey( node ) = shorterDistance;
				heapForward->GetData( node ).stalled = true;
				m_stallQueue.push( node );

				while ( !m_stallQueue.empty() ) {
					//get node from the queue
					const NodeIterator stallNode = m_stallQueue.front();
					m_stallQueue.pop();
					const int stallDistance = heapForward->GetKey( stallNode );

					//iterate over outgoing edges
					for ( EdgeIterator stallEdge = m_graph.edges( stallNode ); stallEdge.hasEdgesLeft(); ) {
						m_graph.unpackNextEdge( &stallEdge );
						//is edge outgoing/reached/stalled?
						if ( !edgeAllowed( stallEdge.forward(), stallEdge.backward() ) )
							continue;
						const NodeIterator stallTo = stallEdge.target();
						if ( !heapForward->WasInserted( stallTo ) )
							continue;
						if ( heapForward->GetData( stallTo ).stalled == true )
							continue;

						const int stallToDistance = stallDistance + stallEdge.distance();
						//sub-optimal path found -> insert stallTo
						if ( stallToDistance < heapForward->GetKey( stallTo ) ) {
							if ( heapForward->WasRemoved( stallTo ) )
								heapForward->GetKey( stallTo ) = stallToDistance;
							else
								heapForward->DecreaseKey( stallTo, stallToDistance );

							m_stallQueue.push( stallTo );
							heapForward->GetData( stallTo ).stalled = true;
						}
					}
				}
				break;
			}
		}

		if ( edgeAllowed( edge.forward(), edge.backward() ) ) {
			//New Node discovered -> Add to Heap + Node Info Storage
			if ( !heapForward->WasInserted( to ) )
				heapForward->Insert( to, toDistance, node );

			//Found a shorter Path -> Update distance
			else if ( toDistance <= heapForward->GetKey( to ) ) {
				heapForward->DecreaseKey( to, toDistance );
				//new parent + unstall
				heapForward->GetData( to ).parent = node;
				heapForward->GetData( to ).stalled = false;
			}
		}
	}
}

int ContractionHierarchiesClient::computeRoute( const IGPSLookup::Result& source, const IGPSLookup::Result& target, QVector< Node>* pathNodes, QVector< Edge >* pathEdges ) {
	EdgeIterator sourceEdge = m_graph.findEdge( source.source, source.target, source.edgeID );
	unsigned sourceWeight = sourceEdge.distance();
	EdgeIterator targetEdge = m_graph.findEdge( target.source, target.target, target.edgeID );
	unsigned targetWeight = targetEdge.distance();

	//insert source into heap
	m_heapForward->Insert( source.target, sourceWeight - sourceWeight * source.percentage, source.target );
	if ( sourceEdge.backward() && sourceEdge.forward() && source.target != source.source )
		m_heapForward->Insert( source.source, sourceWeight * source.percentage, source.source );

	//insert target into heap
	m_heapBackward->Insert( target.source, targetWeight * target.percentage, target.source );
	if ( targetEdge.backward() && targetEdge.forward() && target.target != target.source )
		m_heapBackward->Insert( target.target, targetWeight - targetWeight * target.percentage, target.target );

	int targetDistance = std::numeric_limits< int >::max();
	NodeIterator middle = ( NodeIterator ) 0;
	AllowForwardEdge forward;
	AllowBackwardEdge backward;

	while ( m_heapForward->Size() + m_heapBackward->Size() > 0 ) {

		if ( m_heapForward->Size() > 0 )
			computeStep( m_heapForward, m_heapBackward, forward, backward, &middle, &targetDistance );

		if ( m_heapBackward->Size() > 0 )
			computeStep( m_heapBackward, m_heapForward, backward, forward, &middle, &targetDistance );

	}

	if ( targetDistance == std::numeric_limits< int >::max() )
		return std::numeric_limits< int >::max();

	// abort early if the path description is not requested
	if ( pathNodes == NULL || pathEdges == NULL )
		return targetDistance;

	std::stack< NodeIterator > stack;
	NodeIterator pathNode = middle;
	while ( true ) {
		NodeIterator parent = m_heapForward->GetData( pathNode ).parent;
		stack.push( pathNode );
		if ( parent == pathNode )
			break;
		pathNode = parent;
	}

	pathNodes->push_back( source.nearestPoint );
	bool reverseSourceDescription = pathNode != source.target;
	if ( source.source == source.target && sourceEdge.backward() && sourceEdge.forward() && source.percentage < 0.5 )
		reverseSourceDescription = !reverseSourceDescription;
	if ( sourceEdge.unpacked() ) {
		bool unpackSourceForward = source.target != sourceEdge.target() ? reverseSourceDescription : !reverseSourceDescription;
		m_graph.path( sourceEdge, pathNodes, pathEdges, unpackSourceForward );
		if ( reverseSourceDescription ) {
			pathNodes->remove( 1, pathNodes->size() - 1 - source.previousWayCoordinates );
		} else {
			pathNodes->remove( 1, source.previousWayCoordinates - 1 );
		}
	} else {
		pathNodes->push_back( m_graph.node( pathNode ) );
		pathEdges->push_back( sourceEdge.description() );
	}
	pathEdges->front().length = pathNodes->size() - 1;
	pathEdges->front().seconds *= reverseSourceDescription ? source.percentage : 1 - source.percentage;

	while ( stack.size() > 1 ) {
		const NodeIterator node = stack.top();
		stack.pop();
		unpackEdge( node, stack.top(), true, pathNodes, pathEdges );
	}

	pathNode = middle;
	while ( true ) {
		NodeIterator parent = m_heapBackward->GetData( pathNode ).parent;
		if ( parent == pathNode )
			break;
		unpackEdge( parent, pathNode, false, pathNodes, pathEdges );
		pathNode = parent;
	}

	int begin = pathNodes->size();
	bool reverseTargetDescription = pathNode != target.source;
	if ( target.source == target.target && targetEdge.backward() && targetEdge.forward() && target.percentage > 0.5 )
		reverseTargetDescription = !reverseTargetDescription;
	if ( targetEdge.unpacked() ) {
		bool unpackTargetForward = target.target != targetEdge.target() ? reverseTargetDescription : !reverseTargetDescription;
		m_graph.path( targetEdge, pathNodes, pathEdges, unpackTargetForward );
		if ( reverseTargetDescription ) {
			pathNodes->resize( pathNodes->size() - target.previousWayCoordinates );
		} else {
			pathNodes->resize( begin + target.previousWayCoordinates - 1 );
		}
	} else {
		pathEdges->push_back( targetEdge.description() );
	}
	pathNodes->push_back( target.nearestPoint );
	pathEdges->back().length = pathNodes->size() - begin;
	pathEdges->back().seconds *= reverseTargetDescription ? 1 - target.percentage : target.percentage;

	return targetDistance;
}

bool ContractionHierarchiesClient::unpackEdge( const NodeIterator source, const NodeIterator target, bool forward, QVector< Node >* pathNodes, QVector< Edge >* pathEdges ) {
	EdgeIterator shortestEdge;

	unsigned distance = std::numeric_limits< unsigned >::max();
	for ( EdgeIterator edge = m_graph.edges( source ); edge.hasEdgesLeft(); ) {
		m_graph.unpackNextEdge( &edge );
		if ( edge.target() != target )
			continue;
		if ( forward && !edge.forward() )
			continue;
		if ( !forward && !edge.backward() )
			continue;
		if ( edge.distance() > distance )
			continue;
		distance = edge.distance();
		shortestEdge = edge;
	}

	if ( shortestEdge.unpacked() ) {
		m_graph.path( shortestEdge, pathNodes, pathEdges, forward );
		return true;
	}

	if ( !shortestEdge.shortcut() ) {
		pathEdges->push_back( shortestEdge.description() );
		if ( forward )
			pathNodes->push_back( m_graph.node( target ).coordinate );
		else
			pathNodes->push_back( m_graph.node( source ).coordinate );
		return true;
	}

	const NodeIterator middle = shortestEdge.middle();

	if ( forward ) {
		unpackEdge( middle, source, false, pathNodes, pathEdges );
		unpackEdge( middle, target, true, pathNodes, pathEdges );
		return true;
	} else {
		unpackEdge( middle, target, false, pathNodes, pathEdges );
		unpackEdge( middle, source, true, pathNodes, pathEdges );
		return true;
	}
}

Q_EXPORT_PLUGIN2( contractionhierarchiesclient, ContractionHierarchiesClient )

