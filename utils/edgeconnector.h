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

#ifndef EDGECONNECTOR_H
#define EDGECONNECTOR_H

#include "utils/coordinates.h"
#include <vector>
#include <QMultiHash>
#include <algorithm>

static uint qHash( const UnsignedCoordinate& key )
{
	return qHash( key.x ) ^ qHash( key.y );
}

template< class Node >
class EdgeConnector
{

public:

	struct Edge {
		Node source;
		Node target;
		bool reverseable;
	};

	static void run( std::vector< unsigned >* segments, std::vector< unsigned >* segmentDescriptions, std::vector< bool >* reversed, const std::vector< Edge >& edges )
	{
		// build node index
		QMultiHash< Node, unsigned > nodes;
		QMultiHash< Node, unsigned > backwardNodes;
		for ( unsigned i = 0; i < edges.size(); i++ ) {
			nodes.insert( edges[i].source, i );
			backwardNodes.insert( edges[i].target, i );
			if ( edges[i].reverseable ) {
				nodes.insert( edges[i].target, i );
				backwardNodes.insert( edges[i].source, i );
			}
		}

		std::vector< bool > used( edges.size(), false );
		reversed->resize( edges.size(), false );
		for ( unsigned i = 0; i < edges.size(); i++ ) {
			if ( used[i] )
				continue;

			unsigned lastSize = segmentDescriptions->size();

			segmentDescriptions->push_back( i );

			used[i] = true;
			removeEdge( &nodes, &backwardNodes, edges[i], i );

			// chain edges forward
			Node lastNode = edges[i].target;
			while ( nodes.contains( lastNode ) ) {
				unsigned nextEdge = nodes.value( lastNode );
				assert( !used[nextEdge] );

				if ( lastNode != edges[nextEdge].source ) {
					assert( lastNode == edges[nextEdge].target );
					assert( edges[nextEdge].reverseable );
					( *reversed )[nextEdge] = true;
					lastNode = edges[nextEdge].source;
				} else {
					lastNode = edges[nextEdge].target;
				}

				segmentDescriptions->push_back( nextEdge );

				used[nextEdge] = true;
				removeEdge( &nodes, &backwardNodes, edges[nextEdge], nextEdge );
			}

			// chain edges backward
			std::vector< unsigned > backwardsPath;
			lastNode = edges[i].source;
			while ( backwardNodes.contains( lastNode ) ) {
				unsigned nextEdge = backwardNodes.value( lastNode );
				assert( !used[nextEdge] );

				if ( lastNode != edges[nextEdge].target ) {
					assert( lastNode == edges[nextEdge].source );
					assert( edges[nextEdge].reverseable );
					( *reversed )[nextEdge] = true;
					lastNode = edges[nextEdge].target;
				} else {
					lastNode = edges[nextEdge].source;
				}

				backwardsPath.push_back( nextEdge );

				used[nextEdge] = true;
				removeEdge( &nodes, &backwardNodes, edges[nextEdge], nextEdge );
			}

			segmentDescriptions->insert( segmentDescriptions->begin() + lastSize, backwardsPath.rbegin(), backwardsPath.rend() );

			segments->push_back( segmentDescriptions->size() - lastSize );
		}

		assert( segmentDescriptions->size() == edges.size() );
	}

protected:

	static void removeEdge( QMultiHash< Node, unsigned >* nodes, QMultiHash< Node, unsigned >* backwardsNodes, const Edge& edge, unsigned value )
	{
		nodes->remove( edge.source, value );
		nodes->remove( edge.target, value );
		backwardsNodes->remove( edge.target, value );
		backwardsNodes->remove( edge.source, value );
	}

};

#endif // EDGECONNECTOR_H
