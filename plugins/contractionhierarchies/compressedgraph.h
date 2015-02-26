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

#ifndef COMPRESSEDGRAPH_H
#define COMPRESSEDGRAPH_H

#include "interfaces/irouter.h"
#include "utils/coordinates.h"
#include "utils/bithelpers.h"
#include "blockcache.h"
#include <QString>
#include <QFile>
#include <algorithm>
#include <vector>

class CompressedGraph {

public :

		typedef unsigned NodeIterator;

protected:

	//TYPES

	struct Block {
		struct Settings {
			// adress blocks from the adjacent blocks array
			unsigned char blockBits;
			// address an entry in the adjacent blocks array
			//unsigned char adjacentBlockBits; ==> can be computed from adjacentBlockcount bitsNeeded( count - 1 )
			// address an internal node with a shortcut's middle
			//unsigned char internalBits; ==> can be computed from nodeCount bitsNeeded( count - 1 );
			// address an external node in another block
			unsigned char externalBits;
			// address the first edge of a node
			unsigned char firstEdgeBits;
			// bits used for the short weight class
			unsigned char shortWeightBits;
			// bits uses for the long weight class
			unsigned char longWeightBits;
			// bits used for the difference ( x - minX )
			unsigned char xBits;
			// bits used for the difference ( y - minY )
			unsigned char yBits;
			// minimal x value
			unsigned minX;
			// minimal y value
			unsigned minY;
			// #nodes => used for the size of firstEdges
			unsigned nodeCount;
			// #adjacent blocks => used for the size of adjacentBlocks
			unsigned adjacentBlockCount;
		} settings;

		unsigned char adjacentBlockBits;
		unsigned char internalBits;

		unsigned edges;
		unsigned adjacentBlocks;
		unsigned firstEdges;
		unsigned nodeCoordinates;

		unsigned id;
		const unsigned char* buffer;

		void load( unsigned id, const unsigned char* buffer )
		{
			CompressedGraph::loadBlock( this, id, buffer );
		}
	};

	struct PathBlock {

		struct DataItem {
			unsigned a;
			unsigned b;

			DataItem()
			{
				a = b = 0;
			}

			DataItem( const IRouter::Node& node )
			{
				assert( bits_needed( node.coordinate.x ) < 32 );
				a = node.coordinate.x << 1;
				a |= 1;
				b = node.coordinate.y;
			}

			DataItem( const IRouter::Edge& description )
			{
				a = description.name;
				a <<= 1;
				a |= description.branchingPossible ? 1 : 0;
				a <<= 1;
				b = description.type;
				b <<= 16;
				b |= description.length;
				b <<= 8;
				b |= encode_integer< 4, 4 >( description.seconds );
			}

			bool isNode() const
			{
				return ( a & 1 ) == 1;
			}

			bool isEdge() const
			{
				return ( a & 1 ) == 0;
			}

			IRouter::Node toNode()
			{
				IRouter::Node node;
				node.coordinate = UnsignedCoordinate( a >> 1, b );
				return node;
			}

			IRouter::Edge toEdge()
			{
				IRouter::Edge edge;
				edge.name = a >> 2;
				edge.branchingPossible = ( a & 2 ) == 2;
				edge.type = b >> 24;
				edge.length = ( b >> 8 ) & ( ( 1u << 16 ) -1 );
				edge.seconds = decode_integer< 4, 4 >( b & 255 );
				return edge;
			}
		};

		unsigned id;
		const unsigned char* buffer;

		void load( unsigned id, const unsigned char* buffer )
		{
			CompressedGraph::loadPathBlock( this, id, buffer );
		}

	};

public:

	// TYPES

	struct Edge {
		NodeIterator source;
		NodeIterator target;
		struct Data {
			unsigned distance;
			bool shortcut : 1;
			bool forward : 1;
			bool backward : 1;
			bool unpacked : 1;
			bool reversed : 1;
			union {
				NodeIterator middle;
				unsigned id;
			};
			unsigned path;
		} data;

		bool operator<( const Edge& right ) const {
			if ( source != right.source )
				return source < right.source;
			int l = ( data.forward ? -1 : 0 ) + ( data.backward ? 1 : 0 );
			int r = ( right.data.forward ? -1 : 0 ) + ( right.data.backward ? 1 : 0 );
			if ( l != r )
				return l < r;
			if ( target != right.target )
				return target < right.target;
			return data.distance < right.data.distance;
		}

	};

	class EdgeIterator {

		friend class CompressedGraph;

	public:

		EdgeIterator()
		{
		}

		bool hasEdgesLeft()
		{
			return m_position < m_end;
		}

		NodeIterator target() const { return m_target; }
		bool forward() const { return m_data.forward; }
		bool backward() const { return m_data.backward; }
		bool shortcut() const { return m_data.shortcut; }
		bool unpacked() const { return m_data.unpacked; }
		NodeIterator middle() const { return m_data.middle; }
		unsigned distance() const { return m_data.distance; }
		IRouter::Edge description() const { return IRouter::Edge( m_data.description.nameID, m_data.description.branchingPossible, m_data.description.type, 1, ( m_data.distance + 5 ) / 10 ); }
#ifdef NDEBUG
	private:
#endif

		EdgeIterator( unsigned source, const Block& block, unsigned position, unsigned end ) :
				m_block( &block ), m_source( source ), m_position( position ), m_end( end )
		{
		}

		const Block* m_block;
		NodeIterator m_target;
		NodeIterator m_source;
		unsigned m_position;
		unsigned m_end;

		struct EdgeData {
			unsigned distance;
			bool shortcut : 1;
			bool forward : 1;
			bool backward : 1;
			bool unpacked : 1;
			bool reversed : 1;
			union {
				NodeIterator middle;
				struct {
					unsigned nameID : 30;
					bool branchingPossible : 1;
					unsigned type;
				} description;
			};
			unsigned path;
		} m_data;
	};

	// FUNCTIONS

	CompressedGraph()
	{
		m_loaded = false;
	}

	~CompressedGraph()
	{
		if ( m_loaded )
			unloadGraph();
	}

	bool loadGraph( QString filename, unsigned cacheSize )
	{
		if ( m_loaded )
			unloadGraph();
		QFile settingsFile( filename + "_config" );
		if ( !settingsFile.open( QIODevice::ReadOnly ) ) {
			qCritical() << "failed to open file:" << settingsFile.fileName();
			return false;
		}
		m_settings.read( settingsFile );
		if ( !m_blockCache.load( filename + "_edges", cacheSize / m_settings.blockSize / 2 + 1, m_settings.blockSize ) )
			return false;
		if ( !m_pathCache.load( filename + "_paths", cacheSize / m_settings.blockSize / 2 + 1, m_settings.blockSize ) )
			return false;
		m_loaded = true;
		return true;
	}

	void unloadGraph()
	{
		m_blockCache.unload();
		m_pathCache.unload();
	}

	EdgeIterator edges( NodeIterator node )
	{
		unsigned blockID = nodeToBlock( node );
		unsigned internal = nodeToInternal( node );
		const Block* block = getBlock( blockID );
		return unpackFirstEdges( *block, internal );
	}

	EdgeIterator findEdge( NodeIterator source, NodeIterator target, unsigned id )
	{
		if ( source < target )
			std::swap( source, target );
		EdgeIterator e = edges( source );
		while ( e.hasEdgesLeft() ) {
			unpackNextEdge( &e );
			if ( e.target() != target )
				continue;
			if ( e.shortcut() )
				continue;
			if ( id != 0 ) {
				id--;
				continue;
			}

			return e;
		}
		assert( false );
		return e;
	}

	void unpackNextEdge( EdgeIterator* edge )
	{
		const Block& block = *edge->m_block;
		EdgeIterator::EdgeData& edgeData = edge->m_data;
		const unsigned char* buffer = block.buffer + ( edge->m_position >> 3 );
		int offset = edge->m_position & 7;

		// forward + backward flag
		bool forwardAndBackward = read_unaligned_unsigned( &buffer, 1, &offset ) != 0;
		if ( forwardAndBackward ) {
			edgeData.forward = true;
			edgeData.backward = true;
		} else {
			edgeData.forward = read_unaligned_unsigned( &buffer, 1, &offset ) != 0;
			edgeData.backward = !edgeData.forward;
		}

		// target
		bool internalTarget = read_unaligned_unsigned( &buffer, 1, &offset ) != 0;
		if ( internalTarget ) {
			unsigned target = read_unaligned_unsigned( &buffer, bits_needed( edge->m_source ), &offset );
			edge->m_target = nodeFromDescriptor( block.id, target );
		} else {
			unsigned adjacentBlock = read_unaligned_unsigned( &buffer, block.adjacentBlockBits, &offset );
			unsigned target = read_unaligned_unsigned( &buffer, block.settings.externalBits, &offset );
			unsigned adjacentBlockPosition = block.adjacentBlocks + adjacentBlock * block.settings.blockBits;
			unsigned targetBlock = read_unaligned_unsigned( block.buffer + ( adjacentBlockPosition >> 3 ), block.settings.blockBits, adjacentBlockPosition & 7 );
			edge->m_target = nodeFromDescriptor( targetBlock, target );
		}

		// weight
		bool longWeight = block.settings.shortWeightBits == block.settings.longWeightBits;
		if ( !longWeight )
			longWeight = read_unaligned_unsigned( &buffer, 1, &offset ) != 0;
		edgeData.distance = read_unaligned_unsigned( &buffer, longWeight ? block.settings.longWeightBits : block.settings.shortWeightBits, &offset );

		// unpacked
		edgeData.unpacked = read_unaligned_unsigned( &buffer, 1, &offset ) != 0;
		if ( edgeData.unpacked ) {
			if ( forwardAndBackward )
				edgeData.reversed = read_unaligned_unsigned( &buffer, 1, &offset ) != 0;
			else
				edgeData.reversed = edgeData.backward;
			edgeData.path = read_unaligned_unsigned( &buffer, m_settings.pathBits, &offset );
		}

		// shortcut
		edgeData.shortcut = read_unaligned_unsigned( &buffer, 1, &offset ) != 0;
		if ( edgeData.shortcut ) {
			if ( !edgeData.unpacked ) {
				unsigned middle = read_unaligned_unsigned( &buffer, block.internalBits, &offset );
				edgeData.middle = nodeFromDescriptor( block.id, middle );
			}
		}

		// edge description
		if ( !edgeData.shortcut && !edgeData.unpacked ) {
			edgeData.description.type = read_unaligned_unsigned( &buffer, m_settings.typeBits, &offset );
			edgeData.description.nameID = read_unaligned_unsigned( &buffer, m_settings.nameBits, &offset );
			edgeData.description.branchingPossible = read_unaligned_unsigned( &buffer, 1, &offset );
		}

		edge->m_position = ( buffer - block.buffer ) * 8 + offset;
	}

	IRouter::Node node( NodeIterator node )
	{
		unsigned blockID = nodeToBlock( node );
		unsigned internal = nodeToInternal( node );
		const Block* block = getBlock( blockID );
		IRouter::Node result;
		unpackCoordinates( *block, internal, &result.coordinate );
		return result;
	}

	unsigned numberOfNodes() const
	{
		return m_settings.numberOfNodes;
	}

	unsigned numberOfEdges() const
	{
		return m_settings.numberOfEdges;
	}

	template< class T, class S >
	void path( const EdgeIterator& edge, T path, S edges, bool forward )
	{
		assert( edge.unpacked() );
		unsigned pathBegin = path->size();
		unsigned edgesBegin = edges->size();
		int increase = edge.m_data.reversed ? -1 : 1;

		IRouter::Node targetNode = node( edge.target() );
		unsigned pathID = edge.m_data.path;

		if ( !forward ) {
			PathBlock::DataItem data = unpackPath( pathID );
			assert( data.isNode() );
			path->push_back( data.toNode().coordinate );
		}

		pathID += increase;

		while( true ) {
			PathBlock::DataItem data = unpackPath( pathID );
			if ( data.isEdge() ) {
				edges->push_back( data.toEdge() );
				pathID += increase;
				continue;
			}
			assert( data.isNode() );
			IRouter::Node node = data.toNode();
			if ( node.coordinate.x == targetNode.coordinate.x && node.coordinate.y == targetNode.coordinate.y )
				break;
			path->push_back( node.coordinate );
			pathID += increase;
		}

		if ( forward ) {
			path->push_back( targetNode.coordinate );
		} else {
			std::reverse( path->begin() + pathBegin, path->end() );
			std::reverse( edges->begin() + edgesBegin, edges->end() );
		}

		assert( edges->size() != ( int ) edgesBegin ); // at least one edge description has to be present
	}

protected:

	// TYPES

	struct GlobalSettings {
		unsigned blockSize;
		unsigned char internalBits;
		unsigned char pathBits;
		unsigned char typeBits;
		unsigned char nameBits;
		unsigned numberOfNodes;
		unsigned numberOfEdges;

		void read( QFile& in )
		{
			in.read( ( char* ) &blockSize, sizeof( blockSize ) );
			in.read( ( char* ) &internalBits, sizeof( internalBits ) );
			in.read( ( char* ) &pathBits, sizeof( pathBits ) );
			in.read( ( char* ) &typeBits, sizeof( typeBits ) );
			in.read( ( char* ) &nameBits, sizeof( nameBits ) );
			in.read( ( char* ) &numberOfNodes, sizeof( numberOfNodes ) );
			in.read( ( char* ) &numberOfEdges, sizeof( numberOfEdges ) );
		}

		void write( QFile& out )
		{
			out.write( ( const char* ) &blockSize, sizeof( blockSize ) );
			out.write( ( const char* ) &internalBits, sizeof( internalBits ) );
			out.write( ( const char* ) &pathBits, sizeof( pathBits ) );
			out.write( ( const char* ) &typeBits, sizeof( typeBits ) );
			out.write( ( const char* ) &nameBits, sizeof( nameBits ) );
			out.write( ( const char* ) &numberOfNodes, sizeof( numberOfNodes ) );
			out.write( ( const char* ) &numberOfEdges, sizeof( numberOfEdges ) );
		}
	};

	struct nodeDescriptor {
		unsigned block;
		unsigned node;
	};

	// FUNCTIONS

	PathBlock::DataItem unpackPath( unsigned position ) {
		unsigned blockID = position / ( m_settings.blockSize / 8 );
		unsigned internal = ( position % ( m_settings.blockSize / 8 ) ) * 8;
		const PathBlock* block = getPathBlock( blockID );
		PathBlock::DataItem data;
		data.a = *( ( unsigned* ) ( block->buffer + internal ) );
		data.b = *( ( unsigned* ) ( block->buffer + internal + 4 ) );
		return data;
	}

	void unpackCoordinates( const Block& block, unsigned node, UnsignedCoordinate* result )
	{
		unsigned position = block.nodeCoordinates + ( block.settings.xBits + block.settings.yBits ) * node;
		const unsigned char* buffer = block.buffer + ( position >> 3 );
		int offset = position & 7;
		result->x = read_unaligned_unsigned( &buffer, block.settings.xBits, &offset ) + block.settings.minX;
		result->y = read_unaligned_unsigned( buffer, block.settings.yBits, offset ) + block.settings.minY;
	}

	EdgeIterator unpackFirstEdges( const Block& block, unsigned node )
	{
		unsigned position = block.firstEdges + block.settings.firstEdgeBits * node;
		const unsigned char* buffer = block.buffer + ( position >> 3 );
		int offset = position & 7;
		unsigned begin = read_unaligned_unsigned( &buffer, block.settings.firstEdgeBits, &offset );
		unsigned end = read_unaligned_unsigned( buffer, block.settings.firstEdgeBits, offset );
		return EdgeIterator( node, block, begin + block.edges, end + block.edges );
	}

	const Block* getBlock( unsigned block )
	{
		return m_blockCache.getBlock( block );
	}

	const PathBlock* getPathBlock( unsigned block )
	{
		return m_pathCache.getBlock( block );
	}

	unsigned nodeToBlock( NodeIterator node )
	{
		return node >> m_settings.internalBits;
	}

	unsigned nodeToInternal( NodeIterator node )
	{
		return read_bits( node, m_settings.internalBits );
	}

	NodeIterator nodeFromDescriptor( nodeDescriptor node )
	{
		NodeIterator result = ( node.block << m_settings.internalBits ) | node.node;
		assert( nodeToBlock( result ) == node.block );
		assert( nodeToInternal( result ) == node.node );
		return result;
	}

	NodeIterator nodeFromDescriptor( unsigned block, unsigned node )
	{
		NodeIterator result = ( block << m_settings.internalBits ) | node;
		assert( nodeToBlock( result ) == block );
		assert( nodeToInternal( result ) == node );
		return result;
	}

	static void loadBlock( Block* block, unsigned blockID, const unsigned char* blockBuffer )
	{
		const unsigned char* buffer = blockBuffer;
		int offset = 0;

		// read settings
		block->settings.blockBits = read_unaligned_unsigned( &buffer, 8, &offset );
		block->settings.externalBits = read_unaligned_unsigned( &buffer, 8, &offset );
		block->settings.firstEdgeBits = read_unaligned_unsigned( &buffer, 8, &offset );
		block->settings.shortWeightBits = read_unaligned_unsigned( &buffer, 8, &offset );
		block->settings.longWeightBits = read_unaligned_unsigned( &buffer, 8, &offset );
		block->settings.xBits = read_unaligned_unsigned( &buffer, 8, &offset );
		block->settings.yBits = read_unaligned_unsigned( &buffer, 8, &offset );
		block->settings.minX = read_unaligned_unsigned( &buffer, 32, &offset );
		block->settings.minY = read_unaligned_unsigned( &buffer, 32, &offset );
		block->settings.nodeCount = read_unaligned_unsigned( &buffer, 32, &offset );
		block->settings.adjacentBlockCount = read_unaligned_unsigned( &buffer, 32, &offset );

		// set other values
		block->internalBits = bits_needed( block->settings.nodeCount - 1 );
		block->adjacentBlockBits = bits_needed( block->settings.adjacentBlockCount - 1 );
		block->id = blockID;
		block->buffer = blockBuffer;

		// compute offsets
		block->nodeCoordinates = ( buffer - blockBuffer ) * 8 + offset;
		block->adjacentBlocks = block->nodeCoordinates + ( block->settings.xBits + block->settings.yBits ) * block->settings.nodeCount;
		block->firstEdges = block->adjacentBlocks + block->settings.blockBits * block->settings.adjacentBlockCount;
		block->edges = block->firstEdges + block->settings.firstEdgeBits * ( block->settings.nodeCount + 1 );
	}

	static void loadPathBlock( PathBlock* block, unsigned blockID, const unsigned char* blockBuffer )
	{
		block->id = blockID;
		block->buffer = blockBuffer;
	}

	// VARIABLES

	GlobalSettings m_settings;
	BlockCache< Block > m_blockCache;
	BlockCache< PathBlock > m_pathCache;
	bool m_loaded;
};

#endif // COMPRESSEDGRAPH_H
