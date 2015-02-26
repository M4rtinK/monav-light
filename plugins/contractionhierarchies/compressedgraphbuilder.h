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

#ifndef COMPRESSEDGRAPHBUILDER_H
#define COMPRESSEDGRAPHBUILDER_H

#include "interfaces/iimporter.h"
#include "compressedgraph.h"
#include "utils/qthelpers.h"
#include <limits>
#include <QHash>
#include <QtDebug>

class CompressedGraphBuilder : public CompressedGraph {

public:

	typedef IImporter::RoutingEdge OriginalEdge;

	CompressedGraphBuilder( unsigned blockSize, std::vector< IRouter::Node >& inputNodes, std::vector< Edge >& inputEdges, std::vector< OriginalEdge >& originalEdges, std::vector< IRouter::Node >& edgePaths )
	{
		m_settings.blockSize = blockSize;
		m_settings.numberOfNodes = inputNodes.size();
		m_settings.numberOfEdges = inputEdges.size();
		m_nodes.swap( inputNodes );
		m_edges.swap( inputEdges );
		m_originalEdges.swap( originalEdges );
		m_edgePaths.swap( edgePaths );
		m_blockBuffer = new unsigned char[blockSize];
	}

	~CompressedGraphBuilder()
	{
		delete m_blockBuffer;
	}

	bool run( QString filename, std::vector< unsigned >* remap)
	{
		return createGraph( filename, remap );
	}

private:

	// STRUCTS

	struct BlockBuilder: public Block {
		unsigned baseSize;
		unsigned maxX;
		unsigned maxY;
		unsigned firstNode;

		unsigned size;

		unsigned internalShortcutCount;
		unsigned shortcutCount;
		unsigned externalEdgeCount;
		unsigned internalEdgeTargetSize;
		unsigned edgeCount;
		unsigned singleDirectionCount;
		unsigned singleDirectionUnpackedCount;
		unsigned unpackedEdgeCount;

		std::vector< unsigned > weightDistribution;
		QHash< unsigned, unsigned > internalShortcutTargets;
		QHash< unsigned, unsigned > internalDirectedShortcutTargets;
		QHash< unsigned, unsigned > adjacentBlocks;
	};

	struct Statistics {
		long long adjacentBlocks;
		long long noWeightSplit;

		long long forwardOnly;
		long long backwardOnly;
		long long externalTarget;
		long long externalMiddle;
		long long reversed;
		long long shortcuts;
		long long unpackedEdges;

		long long xSize;
		long long ySize;
		long long firstEdgeSize;

		long long directionSize;
		long long weightSize;
		long long unpackedSize;
		long long internalMiddleSize;
		long long externalTargetSize;
		long long internalTargetSize;

		Statistics()
		{
			memset( this, 0, sizeof( Statistics ) );
		}

		void print( const CompressedGraphBuilder& graph )
		{
			qDebug() << "statistics:";

			qDebug() << "adjacentBlocks:" << adjacentBlocks;
			qDebug() << "noWeightSplit :" << noWeightSplit;

			qDebug() << "forwardOnly       :" << forwardOnly;
			qDebug() << "backwardOnly      :" << backwardOnly;
			qDebug() << "externalTarget    :" << externalTarget;
			qDebug() << "externalMiddle    :" << externalMiddle;
			qDebug() << "reversed          :" << reversed;
			qDebug() << "shortcuts         :" << shortcuts;
			qDebug() << "unpacked edges    :" << unpackedEdges;

			qDebug() << "xSize        :" << xSize / 8 / 1024 / 1024 << "MB /" << graph.m_nodes.size() * 32 / 8 / 1024 / 1024 << "MB";
			qDebug() << "ySize        :" << ySize / 8 / 1024 / 1024 << "MB /" << graph.m_nodes.size() * 32 / 8 / 1024 / 1024 << "MB";
			qDebug() << "firstEdgeSize:" << firstEdgeSize / 8 / 1024 / 1024 << "MB /" << graph.m_nodes.size() * 32 / 8 / 1024 / 1024 << "MB";

			qDebug() << "directionSize      :" << directionSize / 8 / 1024 / 1024 << "MB /" << graph.m_edges.size() * 2 / 8 / 1024 / 1024 << "MB";
			qDebug() << "weightSize         :" << weightSize / 8 / 1024 / 1024 << "MB /" << graph.m_edges.size() * 32 / 8 / 1024 / 1024 << "MB";
			qDebug() << "internalMiddleSize :" << internalMiddleSize / 8 / 1024 / 1024 << "MB /" << ( shortcuts - externalMiddle ) * 32 / 8 / 1024 / 1024 << "MB";
			qDebug() << "externalTargetSize :" << externalTargetSize / 8 / 1024 / 1024 << "MB /" << externalTarget * 32 / 8 / 1024 / 1024 << "MB";
			qDebug() << "internalTargetSize :" << internalTargetSize / 8 / 1024 / 1024 << "MB /" << ( graph.m_edges.size() - externalTarget ) * 32 / 8 / 1024 / 1024 << "MB";
			qDebug() << "unpackedSize       :" << unpackedSize / 8 / 1024 / 1024 << "MB /" << unpackedEdges * 32 / 8 / 1024 / 1024 << "MB";
			qDebug() << "edgeDescriptionSize:" << ( graph.m_edges.size() - shortcuts - unpackedEdges ) * ( graph.m_settings.typeBits + graph.m_settings.nameBits + 1 ) / 8 / 1024 / 1024
					<< "MB /" << ( graph.m_edges.size() - shortcuts - unpackedEdges ) * ( 32 + 32 + 32 ) / 8 / 1024 / 1024 << "MB";
		}

		void addBlock( const CompressedGraphBuilder& graph )
		{
			const BlockBuilder& block = graph.m_block;
			adjacentBlocks += block.settings.adjacentBlockCount;
			if ( block.settings.shortWeightBits == block.settings.longWeightBits )
				noWeightSplit++;

			xSize += block.settings.xBits * block.settings.nodeCount;
			ySize += block.settings.yBits * block.settings.nodeCount;
			firstEdgeSize += block.settings.firstEdgeBits * block.settings.nodeCount;

			externalTarget += block.externalEdgeCount;
			externalTargetSize += block.externalEdgeCount * ( 1 + bits_needed( block.settings.adjacentBlockCount - 1 ) + block.settings.externalBits );
			externalTargetSize += block.settings.adjacentBlockCount * block.settings.blockBits;
			internalTargetSize += ( block.edgeCount - block.externalEdgeCount ) + block.internalEdgeTargetSize;
			externalMiddle += block.shortcutCount - block.internalShortcutCount;
			internalMiddleSize += block.internalShortcutCount * ( 1 + block.internalBits );
			directionSize += block.edgeCount;
			shortcuts += block.shortcutCount;
			unpackedEdges += block.unpackedEdgeCount;
			unpackedSize += block.edgeCount * 1;
			unpackedSize += block.unpackedEdgeCount * (  1 + graph.m_settings.pathBits ) - block.singleDirectionUnpackedCount;

			for ( int i = 0; i < block.settings.shortWeightBits; i++ )
				weightSize += block.settings.shortWeightBits * block.weightDistribution[i];
			for ( int i = block.settings.shortWeightBits; i < block.settings.longWeightBits; i++ )
				weightSize += block.settings.longWeightBits * block.weightDistribution[i];

			for ( unsigned i = 0; i < block.settings.nodeCount; i++ ) {
				unsigned node = i + block.firstNode;
				for ( unsigned e = graph.m_firstEdges[node]; e < graph.m_firstEdges[node + 1]; e++ ) {
					if ( graph.m_edges[e].data.forward && !graph.m_edges[e].data.backward ) {
						forwardOnly++;
						directionSize++;
					}
					if ( !graph.m_edges[e].data.forward && graph.m_edges[e].data.backward ) {
						backwardOnly++;
						directionSize++;
					}
					if ( graph.m_edges[e].data.shortcut ) {
						if ( graph.m_nodeIDs[node].block != graph.m_nodeIDs[graph.m_edges[e].data.middle].block ) {
							if ( graph.m_edges[e].data.reversed )
								reversed++;
						}
					}
				}
			}
		}
	};

	friend class Statistics;

	// FUNCTIONS

	// Adds a new builder block starting at node
	void initBlock( unsigned blockID, unsigned firstNode )
	{
		m_block.id = blockID;
		// Settings + 64Bit buffer
		m_block.baseSize = sizeof( Block::Settings ) + 4;
		m_block.maxX = 0;
		m_block.maxY = 0;
		m_block.firstNode = firstNode;

		m_block.settings.blockBits = bits_needed( blockID - 1 );
		m_block.settings.externalBits = 0;
		m_block.settings.firstEdgeBits = 0;
		m_block.settings.shortWeightBits = 0;
		m_block.settings.longWeightBits = 0;
		m_block.settings.xBits = 0;
		m_block.settings.yBits = 0;
		m_block.settings.minX = std::numeric_limits< unsigned >::max();
		m_block.settings.minY = std::numeric_limits< unsigned >::max();
		m_block.settings.adjacentBlockCount = 0;
		m_block.settings.nodeCount = 0;

		m_block.internalShortcutCount = 0;
		m_block.shortcutCount = 0;
		m_block.externalEdgeCount = 0;
		m_block.internalEdgeTargetSize = 0;
		m_block.edgeCount = 0;
		m_block.singleDirectionCount = 0;
		m_block.singleDirectionUnpackedCount = 0;
		m_block.unpackedEdgeCount = 0;
		m_block.weightDistribution.clear();
		m_block.weightDistribution.resize( 33, 0 );
		m_block.internalShortcutTargets.clear();
		m_block.internalDirectedShortcutTargets.clear();
		m_block.adjacentBlocks.clear();
	}

	bool addNode ( unsigned node ) {
		m_block.settings.nodeCount++;

		m_block.settings.minX = std::min( m_nodes[node].coordinate.x, m_block.settings.minX );
		m_block.settings.minY = std::min( m_nodes[node].coordinate.y, m_block.settings.minY );
		m_block.maxX = std::max( m_nodes[node].coordinate.x, m_block.maxX );
		m_block.maxY = std::max( m_nodes[node].coordinate.y, m_block.maxY );

		m_block.settings.xBits = bits_needed( m_block.maxX - m_block.settings.minX );
		m_block.settings.yBits = bits_needed( m_block.maxY - m_block.settings.minY );

		unsigned internalTargetBits = bits_needed( node - m_block.firstNode ); // loops + previous nodes
		for ( unsigned i = m_firstEdges[node]; i < m_firstEdges[node + 1]; i++ ) {
			const Edge& edge = m_edges[i];

			if ( !edge.data.forward || !edge.data.backward )
				m_block.singleDirectionCount++;

			unsigned char weightBits = bits_needed( edge.data.distance );
			m_block.weightDistribution[weightBits]++;
			m_block.settings.longWeightBits = std::max( weightBits, m_block.settings.longWeightBits );

			if ( edge.target >= m_block.firstNode ) {
				m_block.internalEdgeTargetSize += internalTargetBits; // we only need to address previous nodes due to DAG property
			} else {
				m_block.externalEdgeCount++;
				int block = m_nodeIDs[edge.target].block;
				if ( !m_block.adjacentBlocks.contains( block ) ) {
					m_block.adjacentBlocks.insert( block, m_block.adjacentBlocks.size() );
					m_block.settings.externalBits = std::max( m_externalBits[block], m_block.settings.externalBits );
				}
			}

			if ( edge.data.shortcut ) {
				m_block.shortcutCount++;
				m_block.unpackedEdgeCount++;
				m_block.internalShortcutTargets[edge.data.middle]++;
				if ( !edge.data.forward || !edge.data.backward ) {
					m_block.singleDirectionUnpackedCount++;
					m_block.internalDirectedShortcutTargets[edge.data.middle]++;
				}
			} else {
				if ( mustUnpack( i ) ) {
					m_block.unpackedEdgeCount++;
					if ( !edge.data.forward || !edge.data.backward )
						m_block.singleDirectionUnpackedCount++;
				}
			}
		}

		m_block.edgeCount += m_firstEdges[node + 1] - m_firstEdges[node];
		m_block.internalShortcutCount += m_block.internalShortcutTargets[node];
		m_block.unpackedEdgeCount -= m_block.internalShortcutTargets[node];
		m_block.singleDirectionUnpackedCount -= m_block.internalDirectedShortcutTargets[node];
		m_block.settings.adjacentBlockCount = m_block.adjacentBlocks.size();

		int size = 0;
		size += m_block.edgeCount * ( 1 + 1 + 1 ); // forward / backward + shortcut + target ( external vs internal ) flags
		size += m_block.singleDirectionCount; // backward & forward takes up only 1 bit
		size += m_block.externalEdgeCount * ( bits_needed( m_block.settings.adjacentBlockCount - 1 ) + m_block.settings.externalBits ); // external targets
		size += m_block.internalEdgeTargetSize; // internal targets
		size += m_block.edgeCount * ( 1 ); // unpacked flag
		size += m_block.unpackedEdgeCount * ( m_settings.pathBits + 1 ); // unpacked + reversed flag
		size -= m_block.singleDirectionUnpackedCount; // directed edges do not need a reverse flag
		size += m_block.internalShortcutCount * bits_needed( node - m_block.firstNode ); // internal middle
		unsigned shortWeightsCount = 0;
		unsigned minimumWeightSize = std::numeric_limits< unsigned >::max();
		for ( int bits = 0; bits <= m_block.settings.longWeightBits; bits++ ) {
			shortWeightsCount += m_block.weightDistribution[bits];
			unsigned weightSize = shortWeightsCount * bits + ( m_block.edgeCount - shortWeightsCount ) * m_block.settings.longWeightBits;
			if ( bits != m_block.settings.longWeightBits )
				weightSize += m_block.edgeCount; // long vs short flag
			if ( weightSize < minimumWeightSize ) {
				minimumWeightSize = weightSize;
				m_block.settings.shortWeightBits = bits;
			}
		}
		size += minimumWeightSize;
		size += ( m_block.edgeCount - m_block.internalShortcutCount - m_block.unpackedEdgeCount ) * ( m_settings.typeBits + m_settings.nameBits + 1 );
		// size == edge block size => compute firstEdgeBits
		m_block.settings.firstEdgeBits = bits_needed( size );
		size += m_block.settings.xBits * m_block.settings.nodeCount; // x coordinate
		size += m_block.settings.yBits * m_block.settings.nodeCount; // y coordinate
		size += m_block.settings.firstEdgeBits * ( m_block.settings.nodeCount + 1 ); // first edge entries
		size += m_block.settings.blockBits * m_block.settings.adjacentBlockCount; // adjacent blocks entries

		m_block.size = size;

		if ( ( size + 7 ) / 8 + m_block.baseSize > m_settings.blockSize ) {
			if ( m_block.settings.nodeCount == 1 ) {
				qCritical() << "ERROR: a node requires more space than a single block can suffice\n"
						<< "block:" << m_block.id << "node:" << node << "size:" << size << "maxSize:" << m_settings.blockSize << "\n"
						<< "try increasing the block size";
				exit( -1 ); // TODO return gracefully!
			}
			return false;
		}

		//remap nodes
		m_nodeIDs[node].block = m_block.id;
		m_nodeIDs[node].node = node - m_block.firstNode;

		return true;
	}

	void writeBlock( QFile& blockFile )
	{
		m_block.internalBits = bits_needed( m_block.settings.nodeCount - 1 );

		memset( m_blockBuffer, 0, m_settings.blockSize );
		unsigned char* buffer = m_blockBuffer;
		int offset = 0;

		// write settings
		write_unaligned_unsigned( &buffer, m_block.settings.blockBits, 8, &offset );
		write_unaligned_unsigned( &buffer, m_block.settings.externalBits, 8, &offset );
		write_unaligned_unsigned( &buffer, m_block.settings.firstEdgeBits, 8, &offset );
		write_unaligned_unsigned( &buffer, m_block.settings.shortWeightBits, 8, &offset );
		write_unaligned_unsigned( &buffer, m_block.settings.longWeightBits, 8, &offset );
		write_unaligned_unsigned( &buffer, m_block.settings.xBits, 8, &offset );
		write_unaligned_unsigned( &buffer, m_block.settings.yBits, 8, &offset );
		write_unaligned_unsigned( &buffer, m_block.settings.minX, 32, &offset );
		write_unaligned_unsigned( &buffer, m_block.settings.minY, 32, &offset );
		write_unaligned_unsigned( &buffer, m_block.settings.nodeCount, 32, &offset );
		write_unaligned_unsigned( &buffer, m_block.settings.adjacentBlockCount, 32, &offset );

		unsigned char* beginBuffer = buffer;
		int beginOffset = offset;

		// write coordinates
		for ( unsigned i = 0; i < m_block.settings.nodeCount; i++ ) {
			unsigned node = i + m_block.firstNode;
			write_unaligned_unsigned( &buffer, m_nodes[node].coordinate.x - m_block.settings.minX, m_block.settings.xBits, &offset );
			write_unaligned_unsigned( &buffer, m_nodes[node].coordinate.y - m_block.settings.minY, m_block.settings.yBits, &offset );
		}

		// write adjacent blocks
		std::vector< unsigned > adjacentBlocks( m_block.settings.adjacentBlockCount );
		for ( QHash< unsigned, unsigned >::const_iterator i = m_block.adjacentBlocks.begin(), iend = m_block.adjacentBlocks.end(); i != iend; i++ )
			adjacentBlocks[i.value()] = i.key();
		for ( unsigned i = 0; i < m_block.settings.adjacentBlockCount; i++ )
			write_unaligned_unsigned( &buffer, adjacentBlocks[i], m_block.settings.blockBits, &offset );
		m_block.adjacentBlockBits = bits_needed( m_block.settings.adjacentBlockCount - 1 );

		// reserve space for first edge bits
		unsigned char* firstEdgesBuffer = buffer;
		int firstEdgesOffset = offset;
		offset += m_block.settings.firstEdgeBits * ( m_block.settings.nodeCount + 1 );
		buffer += offset >> 3;
		offset &= 7;

		// write edges
		std::vector< unsigned > firstEdges;
		firstEdges.push_back( 0 );
		unsigned char* edgesBegin = buffer;
		unsigned edgesBeginOffset = offset;
		for ( unsigned i = 0; i < m_block.settings.nodeCount; i++ ) {
			unsigned node = i + m_block.firstNode;
			for ( unsigned edge = m_firstEdges[node]; edge < m_firstEdges[node + 1]; edge++ ) {
				// forward + backward flags
				bool directed = !m_edges[edge].data.forward || !m_edges[edge].data.backward;
				if ( !directed )
					write_unaligned_unsigned( &buffer, 1, 1, &offset );
				else {
					write_unaligned_unsigned( &buffer, 0, 1, &offset );
					write_unaligned_unsigned( &buffer, m_edges[edge].data.forward ? 1 : 0, 1, &offset );
				}
				assert( m_edges[edge].data.forward || m_edges[edge].data.backward );

				// target
				if ( m_nodeIDs[node].block == m_nodeIDs[m_edges[edge].target].block ) {
					write_unaligned_unsigned( &buffer, 1, 1, &offset );
					write_unaligned_unsigned( &buffer, m_edges[edge].target - m_block.firstNode, bits_needed( i ), &offset );
				} else {
					write_unaligned_unsigned( &buffer, 0, 1, &offset );
					unsigned targetBlock = m_nodeIDs[m_edges[edge].target].block;
					write_unaligned_unsigned( &buffer, m_block.adjacentBlocks[targetBlock], m_block.adjacentBlockBits, &offset );
					write_unaligned_unsigned( &buffer, m_nodeIDs[m_edges[edge].target].node, m_block.settings.externalBits, &offset );
				}

				// weight
				bool longWeight = m_edges[edge].data.distance >= ( 1u << m_block.settings.shortWeightBits );
				if ( m_block.settings.shortWeightBits != m_block.settings.longWeightBits )
					write_unaligned_unsigned( &buffer, longWeight ? 1 : 0, 1, &offset );
				write_unaligned_unsigned( &buffer, m_edges[edge].data.distance, longWeight ? m_block.settings.longWeightBits : m_block.settings.shortWeightBits, &offset );

				// unpacked
				bool unpacked = false;
				if ( m_edges[edge].data.shortcut ) {
					if ( m_edges[edge].data.middle - m_block.firstNode >= m_block.settings.nodeCount )
						unpacked = true;
				} else {
					unpacked = mustUnpack( edge );
				}
				write_unaligned_unsigned( &buffer, unpacked ? 1 : 0, 1, &offset );
				if ( unpacked ) {
					assert( m_edges[edge].data.unpacked );
					if ( !directed )
						write_unaligned_unsigned( &buffer, m_edges[edge].data.reversed ? 1 : 0, 1, &offset );
					write_unaligned_unsigned( &buffer, m_edges[edge].data.path, m_settings.pathBits, &offset );
				}

				// shortcut
				write_unaligned_unsigned( &buffer, m_edges[edge].data.shortcut ? 1 : 0, 1, &offset );
				if ( m_edges[edge].data.shortcut ) {
					if ( !unpacked )
						write_unaligned_unsigned( &buffer, m_edges[edge].data.middle - m_block.firstNode, m_block.internalBits, &offset );
				}

				// edge description
				if ( !m_edges[edge].data.shortcut && !unpacked ) {
					unsigned originalID = m_edges[edge].data.id;
					write_unaligned_unsigned( &buffer, m_originalEdges[originalID].type, m_settings.typeBits, &offset );
					write_unaligned_unsigned( &buffer, m_originalEdges[originalID].nameID, m_settings.nameBits, &offset );
					write_unaligned_unsigned( &buffer, m_originalEdges[originalID].branchingPossible ? 1 : 0, 1, &offset );
				}
			}
			firstEdges.push_back( ( buffer - edgesBegin ) * 8 + offset - edgesBeginOffset );
		}

		// write first edges in reserved space
		for ( unsigned i = 0; i < m_block.settings.nodeCount + 1; i++ )
			write_unaligned_unsigned( &firstEdgesBuffer, firstEdges[i], m_block.settings.firstEdgeBits, &firstEdgesOffset );

#ifndef NDEBUG
		testBlock( firstEdges );
#endif

		assert( ( unsigned ) ( buffer - m_blockBuffer ) < m_settings.blockSize );
		assert( ( unsigned ) ( ( buffer - beginBuffer ) * 8 + offset - beginOffset ) == m_block.size );
		blockFile.write( ( const char* ) m_blockBuffer, m_settings.blockSize );
	}

#ifndef NDEBUG
	void testBlock( const std::vector< unsigned >& firstEdges )
	{
		Block block;
		block.load( m_block.id, m_blockBuffer );

		// check coordinates
		for ( unsigned i = 0; i < m_block.settings.nodeCount; i++ ) {
			UnsignedCoordinate coordinate;
			unpackCoordinates( block, i, &coordinate );
			assert( coordinate.x == m_nodes[i + m_block.firstNode].coordinate.x );
			assert( coordinate.y == m_nodes[i + m_block.firstNode].coordinate.y );
		}

		// check first edges
		for ( unsigned i = 0; i < m_block.settings.nodeCount; i++ ) {
			EdgeIterator edge = unpackFirstEdges( block, i );
			assert( edge.m_position == firstEdges[i] + block.edges );
			assert( edge.m_end == firstEdges[i + 1] + block.edges );
		}

		// check edges
		for ( unsigned i = 0; i < m_block.settings.nodeCount; i++ ) {
			EdgeIterator edge = unpackFirstEdges( block, i );
			for ( unsigned e = m_firstEdges[i + m_block.firstNode]; e < m_firstEdges[i + m_block.firstNode + 1]; e++ ) {
				assert( edge.hasEdgesLeft() );
				unpackNextEdge( &edge );
				assert( nodeFromDescriptor( m_nodeIDs[m_edges[e].target] ) == edge.target() );
				assert( m_edges[e].data.forward == edge.forward() );
				assert( m_edges[e].data.backward == edge.backward() );
				assert( m_edges[e].data.shortcut == edge.shortcut() );
				if ( m_edges[e].data.shortcut ) {
					assert ( mustUnpack( e ) == ( edge.unpacked() ) );
					if ( edge.unpacked() )
						assert( m_edges[e].data.path == edge.m_data.path );
					else
						assert( nodeFromDescriptor( m_nodeIDs[m_edges[e].data.middle] ) == edge.middle() );
				}
				if ( !edge.shortcut() && !edge.unpacked() ) {
					unsigned originalID = m_edges[e].data.id;
					IRouter::Edge description = edge.description();
					assert( description.type == m_originalEdges[originalID].type );
					assert( description.name == m_originalEdges[originalID].nameID );
					assert( description.length == 1 );
				}
				assert( m_edges[e].data.distance == edge.distance() );
			}
		}
	}
#endif

	// build edge index
	void buildIndex()
	{
		for ( unsigned i = 0; i < m_edges.size(); i++ ) {
			//edges belong to a new node? -> update index
			while ( m_firstEdges.size() <= m_edges[i].source )
				m_firstEdges.push_back( i );
		}
		while ( m_firstEdges.size() <= m_nodes.size() )
			m_firstEdges.push_back( m_edges.size() );
	}

	// unpack all external shortcuts
	void unpackAllNecessary( QFile* pathFile, bool pretend = false )
	{
		for ( std::vector< Edge >::iterator i = m_edges.begin(), iend = m_edges.end(); i != iend; i++ )
			i->data.unpacked = false;
		m_unpackBuffer.clear();
		m_unpackBufferOffset = 0;

		unsigned numberOfUnpackable = 0;
		unsigned numberOfUnpacked = 0;

		qDebug( "Computing path data" );
		for ( unsigned i = 0; i < m_edges.size(); i++ ) {

			//do not unpack internal shortcuts
			if ( !pretend && !mustUnpack( i ) )
				continue;
			if ( pretend && !mustPretend( i ) )
				continue;

			numberOfUnpackable++;

			//path already fully unpacked ( covered by some higher level shortcut )?
			if ( m_edges[i].data.unpacked )
				continue;

			//get unpacked path
			if ( m_edges[i].data.forward ) {
				m_unpackBuffer.push_back( m_nodes[m_edges[i].source] );
				unpackPath ( m_edges[i].source, m_edges[i].target, true, i );
			} else {
				m_unpackBuffer.push_back( m_nodes[m_edges[i].target] );
				unpackPath ( m_edges[i].source, m_edges[i].target, false, i );
			}
			numberOfUnpacked++;
			if ( !pretend ) {
				for ( unsigned i = 0; i < m_unpackBuffer.size(); i++ ) {
					pathFile->write( ( const char* ) &m_unpackBuffer[i].a, sizeof( unsigned ) );
					pathFile->write( ( const char* ) &m_unpackBuffer[i].b, sizeof( unsigned ) );
				}
			}
			m_unpackBufferOffset += m_unpackBuffer.size();
			m_unpackBuffer.clear();
		}
		if ( pretend )
			qDebug() << "max path index:" << m_unpackBufferOffset;
		else
			qDebug( "unpacked edges: %lf %%", 100.0f * numberOfUnpacked / numberOfUnpackable );
	}

	// unpacks a shortcut and updates the pointers for all shortcuts encountered
	void unpackPath( unsigned source, unsigned target, bool forward, unsigned edgeID = std::numeric_limits< unsigned >::max() ) {
		unsigned shortestEdgeID = edgeID;
		if ( edgeID == std::numeric_limits< unsigned >::max() ) {
			unsigned distance = std::numeric_limits< unsigned >::max();
			for ( unsigned edgeID = m_firstEdges[source]; edgeID < m_firstEdges[source + 1]; edgeID++ ) {
				const Edge& edge = m_edges[edgeID];
				if ( edge.target != target )
					continue;
				if (  forward && !edge.data.forward )
					continue;
				if ( !forward && !edge.data.backward )
					continue;
				if ( edge.data.distance > distance )
					continue;
				distance = edge.data.distance;
				shortestEdgeID = edgeID;
			}
		}
		Edge& shortestEdge = m_edges[shortestEdgeID];

		shortestEdge.data.reversed = !forward;
		shortestEdge.data.unpacked = true;

		if ( !shortestEdge.data.shortcut ) {
			const IImporter::RoutingEdge originalEdge = m_originalEdges[shortestEdge.data.id];
			bool reversed = originalEdge.target != target;
			if ( forward ) {
				//point at source node
				shortestEdge.data.path = m_unpackBuffer.size() - 1 + m_unpackBufferOffset;

				unpackEdge( originalEdge, reversed );
				m_unpackBuffer.push_back ( m_nodes[target] );
			} else {
				unpackEdge( originalEdge, !reversed );
				m_unpackBuffer.push_back ( m_nodes[source] );

				//point at target
				shortestEdge.data.path = m_unpackBuffer.size() - 1 + m_unpackBufferOffset;
			}
			return;
		}

		unsigned middle = shortestEdge.data.middle;

		//unpack the nodes in the right order
		if ( forward ) {
			//point at source
			shortestEdge.data.path = m_unpackBuffer.size() - 1 + m_unpackBufferOffset;

			unpackPath ( middle, source, false );
			unpackPath ( middle, target, true );
		} else {
			unpackPath ( middle, target, false );
			unpackPath ( middle, source, true );

			//point at target
			shortestEdge.data.path = m_unpackBuffer.size() - 1 + m_unpackBufferOffset;
		}
	}

	void unpackEdge( const IImporter::RoutingEdge& edge, bool reversed )
	{
		m_unpackBuffer.push_back( PathBlock::DataItem( IRouter::Edge( edge.nameID, edge.branchingPossible, edge.type, edge.pathLength + 1, edge.distance + 0.5 ) ) );

		if ( edge.pathLength == 0 )
			return;

		unsigned end = edge.pathID + edge.pathLength;

		if ( !reversed ) {
			for ( unsigned i = edge.pathID; i < end; i++ )
				m_unpackBuffer.push_back( m_edgePaths[i] );
		} else {
			for ( unsigned i = end - 1; i > edge.pathID; i-- )
				m_unpackBuffer.push_back( m_edgePaths[i] );
			m_unpackBuffer.push_back( m_edgePaths[edge.pathID] );
		}
	}

	bool mustUnpack( unsigned edgeID )
	{
		const Edge& edge = m_edges[edgeID];
		//do not unpack internal shortcuts
		if ( edge.data.shortcut )
			return m_nodeIDs[edge.source].block != m_nodeIDs[edge.data.middle].block;
		const IImporter::RoutingEdge& originalEdge = m_originalEdges[edge.data.id];
		return originalEdge.pathLength != 0;
	}

	bool mustPretend( unsigned edgeID )
	{
		const Edge& edge = m_edges[edgeID];
		//do not unpack internal shortcuts
		if ( edge.data.shortcut )
			return true;
		const IImporter::RoutingEdge& originalEdge = m_originalEdges[edge.data.id];
		return originalEdge.pathLength != 0;
	}

	bool createGraph( QString filename, std::vector< unsigned >* remap )
	{
		assert( m_nodes.size() == remap->size() );
		qDebug() << "creating compressed graph with" << m_nodes.size() <<  "nodes and" << m_edges.size() << "edges";
		Timer time;

		buildIndex();
		m_nodeIDs.resize( m_nodes.size() );
		qDebug() << "build node index:" << time.restart() << "ms";

		unpackAllNecessary( NULL, true );
		qDebug() << "computed max path index:" << time.restart() << "ms";
		m_settings.pathBits = bits_needed( m_unpackBufferOffset );

		unsigned short maxType = 0;
		unsigned maxName = 0;
		for ( unsigned edge = 0; edge < m_originalEdges.size(); edge++ ) {
			maxType = std::max( maxType, m_originalEdges[edge].type );
			maxName = std::max( maxName, m_originalEdges[edge].nameID );
		}
		m_settings.typeBits = bits_needed( maxType );
		m_settings.nameBits = bits_needed( maxName );

		// compute mapping nodes -> blocks
		unsigned blocks = 0;
		initBlock( 0, 0 );
		for ( unsigned node = 0; node < m_nodes.size(); node++ ) {
			if ( !addNode( node ) ) {
				m_externalBits.push_back( bits_needed( node - 1 - m_block.firstNode ) ); // never negative <= at least one node
				initBlock( ++blocks, node );
				addNode( node ); // cannot fail -> exit( -1 ) in addNode otherwise
			}
		}
		assert( blocks == m_externalBits.size() );
		// account for the last block
		blocks++;
		m_externalBits.push_back( bits_needed( m_nodes.size() - 1 - m_block.firstNode ) );
		qDebug() << "computed block layout:" << time.restart() << "ms";

		m_settings.internalBits = 0;
		for ( unsigned block = 0; block < blocks; block++ )
			m_settings.internalBits = std::max( m_externalBits[block], m_settings.internalBits );

		// unpack shortcuts
		QFile pathFile( filename + "_paths" );
		if ( !openQFile( &pathFile, QIODevice::WriteOnly ) )
			return false;
		unpackAllNecessary( &pathFile );
		qDebug() << "unpacked shortcuts:" << time.restart() << "ms";

		// recompute block settings and write
		QFile blockFile( filename + "_edges" );
		if ( !openQFile( &blockFile, QIODevice::WriteOnly ) )
			return false;
		for ( unsigned block = 0, node = 0; block < blocks; block++ ) {
			initBlock( block, node );
			while ( node < m_nodes.size() && m_nodeIDs[node].block == block )
				addNode( node++ );  // should never fail
			writeBlock( blockFile );
			m_statistics.addBlock( *this );
		}
		qDebug() << "wrote blocks" << time.restart() << "ms";

		// write config
		QFile configFile( filename + "_config" );
		if ( !openQFile( &configFile, QIODevice::WriteOnly ) )
			return false;
		m_settings.write( configFile );

		qDebug( "used Settings:" );
		qDebug( "\tblock size: %d", m_settings.blockSize );
		qDebug( "\tinternal bits: %d", m_settings.internalBits );
		qDebug( "\tpath bits: %d", m_settings.pathBits );
		qDebug( "\tblocks: %d", blocks );
		qDebug( "\tpath blocks: %d", m_unpackBufferOffset / ( m_settings.blockSize / 8 ) );
		qDebug( "\tblock space: %lld Mb" , blockFile.size() / 1024 / 1024 );
		qDebug( "\tpath block space: %lld Mb" , pathFile.size() / 1024 / 1024 );
		qDebug( "\tmax internal ID: %u", nodeFromDescriptor( m_nodeIDs.back() ) );

		m_statistics.print( *this );

		for ( unsigned i = 0; i < m_nodes.size(); i++ )
			( *remap )[i] = nodeFromDescriptor( m_nodeIDs[( *remap )[i]] );

		return true;
	}

	//VARIABLES
	std::vector< IRouter::Node > m_nodes;
	std::vector< unsigned > m_firstEdges;
	std::vector< Edge > m_edges;
	std::vector< nodeDescriptor > m_nodeIDs;
	std::vector< OriginalEdge > m_originalEdges;
	std::vector< IRouter::Node > m_edgePaths;
	std::vector< unsigned char > m_externalBits;
	std::vector< PathBlock::DataItem > m_unpackBuffer;
	unsigned m_unpackBufferOffset;
	BlockBuilder m_block;
	unsigned char* m_blockBuffer;

	Statistics m_statistics;
};

#endif // COMPRESSEDGRAPHBUILDER_H
