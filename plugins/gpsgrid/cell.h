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

#ifndef CELL_H
#define CELL_H

#include "utils/config.h"
#include "utils/coordinates.h"
#include "utils/bithelpers.h"
#include "utils/edgeconnector.h"
#include <vector>
#include <algorithm>

namespace gg
{

	class Cell {

	public:

		struct Edge {
			unsigned pathID;
			unsigned short pathLength;
			unsigned short edgeID;
			NodeID source;
			NodeID target;
			bool bidirectional;
			bool operator==( const Edge& right ) const {
				if ( source != right.source )
					return false;
				if ( target != right.target )
					return false;
				if ( bidirectional != right.bidirectional )
					return false;
				if ( edgeID != right.edgeID )
					return false;
				if ( pathLength != right.pathLength )
					return false;
				return true;
			}
		};

		std::vector< Edge > edges;
		std::vector< UnsignedCoordinate > coordinates;

		bool operator==( const Cell& right ) const
		{
			if ( edges.size() != right.edges.size() )
				return false;
			for ( int i = 0; i < ( int ) edges.size(); i++ ) {
				if ( !( edges[i] == right.edges[i] ) )
					return false;
				for ( unsigned path = 0; path < edges[i].pathLength; path++ ) {
					if ( coordinates[path + edges[i].pathID].x != right.coordinates[path + right.edges[i].pathID].x )
						return false;
					if ( coordinates[path + edges[i].pathID].y != right.coordinates[path + right.edges[i].pathID].y )
						return false;
				}
			}

			return true;
		}

		size_t write( unsigned char* buffer, UnsignedCoordinate min, UnsignedCoordinate max )
		{
			unsigned char* const oldBuffer = buffer;

			NodeID minID = std::numeric_limits< NodeID >::max();
			NodeID maxID = 0;
			unsigned short maxEdgeID = 0;
			unsigned short maxPathLength = 0;

			{
				// build edge connector data structures
				std::vector< EdgeConnector< unsigned >::Edge > connectorEdges;
				std::vector< unsigned > resultSegments;
				std::vector< unsigned > resultSegmentDescriptions;
				std::vector< bool > resultReversed;

				for ( unsigned i = 0; i < ( unsigned ) edges.size(); i++ ) {
					EdgeConnector< unsigned >::Edge newEdge;
					newEdge.source = edges[i].source;
					newEdge.target = edges[i].target;
					newEdge.reverseable = edges[i].bidirectional;
					connectorEdges.push_back( newEdge );
				}

				EdgeConnector< unsigned >::run( &resultSegments, &resultSegmentDescriptions, &resultReversed, connectorEdges );

				for ( unsigned i = 0; i < ( unsigned ) edges.size(); i++ ) {
					if ( resultReversed[i] ) {
						std::swap( edges[i].source, edges[i].target );
						std::reverse( coordinates.begin() + edges[i].pathID, coordinates.begin() + edges[i].pathID + edges[i].pathLength );
					}
				}

				std::vector< Edge > reorderedEdges;
				for ( unsigned i = 0; i < ( unsigned ) resultSegmentDescriptions.size(); i++ )
					reorderedEdges.push_back( edges[resultSegmentDescriptions[i]] );

				edges.swap( reorderedEdges );
			}

			std::vector< NodeID > nodes;
			for ( std::vector< Edge >::iterator i = edges.begin(), e = edges.end(); i != e; ++i ) {
				minID = std::min( minID, i->source );
				minID = std::min( minID, i->target );
				maxID = std::max( maxID, i->source );
				maxID = std::max( maxID, i->target );
				maxID = std::max( maxID, i->target );
				maxEdgeID = std::max( maxEdgeID, i->edgeID );
				assert( i->pathLength > 1 );
				maxPathLength = std::max( maxPathLength, ( unsigned short ) ( i->pathLength - 2 ) );

				nodes.push_back( i->source );
				nodes.push_back( i->target );
			}

			std::sort( nodes.begin(), nodes.end() );
			nodes.resize( std::unique( nodes.begin(), nodes.end() ) - nodes.begin() );

			const char xBits = bits_needed( max.x - min.x );
			const char yBits = bits_needed( max.y - min.y );
			const char idBits = bits_needed( maxID - minID );
			const char edgeIDBits = bits_needed( maxEdgeID );
			const char pathLengthBits = bits_needed( maxPathLength );

			int offset = 0;
			write_unaligned_unsigned( &buffer, edges.size(), 32, &offset );
			write_unaligned_unsigned( &buffer, minID, 32, &offset );
			write_unaligned_unsigned( &buffer, idBits, 6, &offset );
			write_unaligned_unsigned( &buffer, edgeIDBits, 6, &offset );
			write_unaligned_unsigned( &buffer, pathLengthBits, 6, &offset );

			NodeID lastTarget = std::numeric_limits< NodeID >::max();
			std::vector< UnsignedCoordinate > wayBuffer;
			for ( std::vector< Edge >::const_iterator i = edges.begin(), e = edges.end(); i != e; i++ ) {
				bool reuse = lastTarget == i->source;

				write_unaligned_unsigned( &buffer, reuse ? 1 : 0, 1, &offset );
				if ( !reuse )
					write_unaligned_unsigned( &buffer, i->source - minID, idBits, &offset );
				write_unaligned_unsigned( &buffer, i->target - minID, idBits, &offset );
				write_unaligned_unsigned( &buffer, i->edgeID, edgeIDBits, &offset );
				write_unaligned_unsigned( &buffer, i->bidirectional ? 1 : 0, 1, &offset );
				write_unaligned_unsigned( &buffer, i->pathLength - 2, pathLengthBits, &offset );

				assert( i->pathLength >= 2 );
				for ( int pathID = 1; pathID < i->pathLength - 1; pathID++ )
					wayBuffer.push_back( coordinates[pathID + i->pathID] );

				lastTarget = i->target;
			}

			std::vector< UnsignedCoordinate > nodeCoordinates( nodes.size() );
			for ( std::vector< Edge >::iterator i = edges.begin(), e = edges.end(); i != e; ++i ) {
                unsigned sourcePos = std::lower_bound( nodes.begin(), nodes.end(), i->source ) - nodes.begin();
                unsigned targetPos = std::lower_bound( nodes.begin(), nodes.end(), i->target ) - nodes.begin();
				nodeCoordinates[sourcePos] = coordinates[i->pathID];
				nodeCoordinates[targetPos] = coordinates[i->pathID + i->pathLength - 1];
			}

			for ( std::vector< UnsignedCoordinate >::const_iterator i = nodeCoordinates.begin(), iend = nodeCoordinates.end(); i != iend; i++ ) {
				writeCoordinate( &buffer, &offset, i->x, min.x, max.x, xBits);
				writeCoordinate( &buffer, &offset, i->y, min.y, max.y, yBits);
			}

			for ( std::vector< UnsignedCoordinate >::const_iterator i = wayBuffer.begin(), iend = wayBuffer.end(); i != iend; i++ ) {
				writeCoordinate( &buffer, &offset, i->x, min.x, max.x, xBits);
				writeCoordinate( &buffer, &offset, i->y, min.y, max.y, yBits);
			}

			buffer += ( offset + 7 ) / 8;

			return buffer - oldBuffer;
		}

		size_t  read( const unsigned char* buffer, UnsignedCoordinate min, UnsignedCoordinate max ) {
			const unsigned char* oldBuffer = buffer;

			int offset = 0;

			const char xBits = bits_needed( max.x - min.x );
			const char yBits = bits_needed( max.y - min.y );

			unsigned numEdges = read_unaligned_unsigned( &buffer, 32, &offset );
			unsigned minID = read_unaligned_unsigned( &buffer, 32, &offset );
			unsigned idBits = read_unaligned_unsigned( &buffer, 6, &offset );
			unsigned edgeIDBits = read_unaligned_unsigned( &buffer, 6, &offset );
			unsigned pathLengthBits = read_unaligned_unsigned( &buffer, 6, &offset );

			std::vector< NodeID > nodes;
			NodeID lastTarget = std::numeric_limits< NodeID >::max();
			for ( unsigned i = 0; i < numEdges; i++ ) {
				Edge edge;
				bool reuse = read_unaligned_unsigned( &buffer, 1, &offset ) == 1;
				if ( reuse )
					edge.source = lastTarget;
				else
					edge.source = read_unaligned_unsigned( &buffer, idBits, &offset ) + minID;
				edge.target = read_unaligned_unsigned( &buffer, idBits, &offset ) + minID;
				edge.edgeID = read_unaligned_unsigned( &buffer, edgeIDBits, &offset );
				edge.bidirectional = read_unaligned_unsigned( &buffer, 1, &offset ) == 1;
				edge.pathLength = read_unaligned_unsigned( &buffer, pathLengthBits, &offset );

				edges.push_back( edge );

				nodes.push_back( edge.source );
				nodes.push_back( edge.target );

				lastTarget = edge.target;
			}
			assert( edges.size() != 0 );

			std::sort( nodes.begin(), nodes.end() );
			nodes.resize( std::unique( nodes.begin(), nodes.end() ) - nodes.begin() );

			std::vector< UnsignedCoordinate > nodeCoordinates( nodes.size() );
			for ( std::vector< UnsignedCoordinate >::iterator i = nodeCoordinates.begin(), iend = nodeCoordinates.end(); i != iend; i++ ) {
				readCoordinate( &buffer, &offset, &i->x, min.x, max.x, xBits);
				readCoordinate( &buffer, &offset, &i->y, min.y, max.y, yBits);
			}

			for ( std::vector< Edge >::iterator i = edges.begin(), iend = edges.end(); i != iend; i++ ) {
                unsigned sourcePos = std::lower_bound( nodes.begin(), nodes.end(), i->source ) - nodes.begin();
                unsigned targetPos = std::lower_bound( nodes.begin(), nodes.end(), i->target ) - nodes.begin();
				if ( coordinates.size() == 0 || coordinates.back().x != nodeCoordinates[sourcePos].x || coordinates.back().y != nodeCoordinates[sourcePos].y )
					coordinates.push_back( nodeCoordinates[sourcePos] );
				i->pathID = coordinates.size() - 1;
				for ( int path = 0; path < i->pathLength; path++ ) {
					UnsignedCoordinate coordinate;
					readCoordinate( &buffer, &offset, &coordinate.x, min.x, max.x, xBits);
					readCoordinate( &buffer, &offset, &coordinate.y, min.y, max.y, yBits);
					coordinates.push_back( coordinate );
				}
				coordinates.push_back( nodeCoordinates[targetPos] );

				i->pathLength += 2;
			}

			buffer += ( offset + 7 ) / 8;

			return buffer - oldBuffer;
		}

	protected:

		void writeCoordinate( unsigned char** buffer, int* offset, unsigned value, unsigned min, unsigned max, char bits )
		{
			bool inside = !( value < min || value > max );
			write_unaligned_unsigned( buffer, inside ? 1 : 0, 1, offset );
			if ( inside )
				write_unaligned_unsigned( buffer, value - min, bits, offset );
			else
				write_unaligned_unsigned( buffer, value, 32, offset );
		}

		void readCoordinate( const unsigned char** buffer, int* offset, unsigned* value, unsigned min, unsigned /*max*/, char bits )
		{
			bool inside = read_unaligned_unsigned( buffer, 1, offset ) == 1;
			if ( inside )
				*value = read_unaligned_unsigned( buffer, bits, offset ) + min;
			else
				*value = read_unaligned_unsigned( buffer, 32, offset );
		}

	};
}

#endif // CELL_H
