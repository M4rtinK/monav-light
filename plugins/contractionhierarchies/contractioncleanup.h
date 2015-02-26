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

#ifndef CONTRACTIONCLEANUP_H_INCLUDED
#define CONTRACTIONCLEANUP_H_INCLUDED

#include <QTime>
#include <queue>
#include <stack>
#include "contractor.h"

class ContractionCleanup {
	private:

		struct _HeapData {
			NodeID parent;
			_HeapData( NodeID p ) {
				parent = p;
			}
		};

		class _Node {

			public:

				NodeID id;
				int depth;
				NodeID mappedID;

				static bool CompareByID( const _Node& left, const _Node& right ) {
					return left.id < right.id;
				}

				static bool CompareByDepth( const _Node& left, const _Node& right ) {
					return left.depth < right.depth;
				}

				static bool CompareByRemappedID( const _Node& left, const _Node& right ) {
					return left.mappedID < right.mappedID;
				}

		};

		class _Comp {
			public:
				bool operator() ( const _Node& left, const _Node& right ) {
					return left.mappedID < right.mappedID;
				}
		};

		struct _Witness {
			int edge;
			int safe;
			NodeID affectedNode;

			bool operator< ( const _Witness& right ) const {
				if ( edge != right.edge )
					return edge < right.edge;
				if ( affectedNode != right.affectedNode )
					return affectedNode < right.affectedNode;
				return safe < right.safe;
			}

			bool operator== ( const _Witness& right ) const {
				return edge == right.edge && affectedNode == right.affectedNode;
			}
		};

		typedef BinaryHeap< NodeID, NodeID, int, _HeapData > _Heap;

	public:

		struct Edge {
			NodeID source;
			NodeID target;
			struct EdgeData {
				int distance : 29;
				bool shortcut : 1;
				bool forward : 1;
				bool backward : 1;
				union {
					NodeID middle;
					unsigned id;
				};
			} data;

			//sorts by source and other attributes
			static bool CompareBySource( const Edge& left, const Edge& right ) {
				if ( left.source != right.source )
					return left.source < right.source;
				int l = ( left.data.forward ? -1 : 0 ) + ( left.data.backward ? -1 : 0 );
				int r = ( right.data.forward ? -1 : 0 ) + ( right.data.backward ? -1 : 0 );
				if ( l != r )
					return l < r;
				if ( left.target != right.target )
					return left.target < right.target;
				return left.data.distance < right.data.distance;
			}

			//sorts by target
			static bool CompareByTarget( const Edge& left, const Edge& right ) {
				return left.target < right.target;
			}

			bool operator== ( const Edge& right ) const {
				return ( source == right.source && target == right.target && data.distance == right.data.distance && data.shortcut == right.data.shortcut && data.forward == right.data.forward && data.backward == right.data.backward && data.middle == right.data.middle );
			}
		};


		ContractionCleanup( int numNodes, const std::vector< Edge >& edges, const std::vector< Edge >& loops, const std::vector< Contractor::Witness >& witnessList ) {
			_graph = edges;
			_loops = loops;
			_witnessList = witnessList;
			_numNodes = numNodes;
			_heapForward = new _Heap( numNodes );
			_heapBackward = new _Heap( numNodes );
		}

		~ContractionCleanup() {

		}

		void Run() {

			double time = _Timestamp();

			RemoveUselessShortcuts();

			ReorderNodes();

			//DistributeWitnessData();

			time = _Timestamp() - time;
			qDebug( "Postprocessing Time: %lf s", time );
		}

		template< class OutputEdge >
		void GetData( std::vector< OutputEdge >* edges, std::vector< NodeID >* map ) {
			std::sort( _remap.begin(), _remap.end(), _Node::CompareByID );

			for ( NodeID node = 0; node < _numNodes; ++node )
				map->push_back( _remap[node].mappedID );

			convertEdges( edges, _graph );
			convertEdges( edges, _loops );

			std::sort( edges->begin(), edges->end() );
		}

	private:

		template< class OutputEdge >
		void convertEdges( std::vector< OutputEdge >* to, const std::vector< Edge >& from )
		{
			for ( unsigned edge = 0, endEdges = from.size(); edge != endEdges; ++edge ) {
				OutputEdge newEdge;
				newEdge.source = _remap[from[edge].source].mappedID;
				newEdge.target = _remap[from[edge].target].mappedID;
				assert( newEdge.source >= newEdge.target );
				newEdge.data.distance = from[edge].data.distance;
				newEdge.data.shortcut = from[edge].data.shortcut;
				if ( newEdge.data.shortcut )
					newEdge.data.middle = _remap[from[edge].data.middle].mappedID;
				else
					newEdge.data.id = from[edge].data.id;
				newEdge.data.forward = from[edge].data.forward;
				newEdge.data.backward = from[edge].data.backward;
				to->push_back( newEdge );
			}
		}

		class AllowForwardEdge {
			public:
				bool operator()( const Edge& data ) const {
					return data.data.forward;
				}
		};

		class AllowBackwardEdge {
			public:
				bool operator()( const Edge& data ) const {
					return data.data.backward;
				}
		};

		double _Timestamp() {
			static QTime timer;
			static bool first = true;
			if ( first )
			{
				first = false;
				timer.start();
			}
			return ( double ) timer.elapsed() / 1000;
		}

		void BuildOutgoingGraph() {
			//sort edges by source
			std::sort( _graph.begin(), _graph.end(), Edge::CompareBySource );
			_firstEdge.resize( _numNodes + 1 );
			_firstEdge[0] = 0;
			for ( NodeID i = 0, node = 0; i < ( NodeID ) _graph.size(); i++ ) {
				while ( _graph[i].source != node )
					_firstEdge[++node] = i;
				if ( i == ( NodeID ) _graph.size() - 1 )
					while ( node < _numNodes )
						_firstEdge[++node] = ( int ) _graph.size();
			}
		}

		void BuildIncomingGraph() {
			//sort edges and compute _firstEdge for the reverse _graph
			sort( _graph.begin(), _graph.end(), Edge::CompareByTarget );
			_firstEdge.resize( _numNodes + 1 );
			_firstEdge[0] = 0;
			for ( NodeID i = 0, node = 0; i < ( NodeID ) _graph.size(); i++ ) {
				while ( _graph[i].target != node )
					_firstEdge[++node] = i;
				if ( i == ( NodeID ) _graph.size() - 1 )
					while ( node < _numNodes )
						_firstEdge[++node] = ( int ) _graph.size();
			}
		}

		void RemoveUselessShortcuts() {

			qDebug( "Scanning for useless shortcuts" );
			BuildOutgoingGraph();
			int numUseless = 0;
			for ( unsigned i = 0; i < ( unsigned ) _graph.size(); i++ ) {

				for ( unsigned edge = _firstEdge[_graph[i].source]; edge < _firstEdge[_graph[i].source + 1]; ++edge ) {
					if ( _graph[edge].target != _graph[i].target )
						continue;
					if ( _graph[edge].data.distance < _graph[i].data.distance )
						continue;
					if ( edge == i )
						continue;
					if ( !_graph[edge].data.shortcut )
						continue;


					_graph[edge].data.forward &= !_graph[i].data.forward;
					_graph[edge].data.backward &= !_graph[i].data.backward;
				}

				if ( !_graph[i].data.forward && !_graph[i].data.backward )
					continue;

				//only remove shortcuts
				if ( !_graph[i].data.shortcut )
					continue;

				if ( _graph[i].data.forward ) {
					int result = _ComputeDistance( _graph[i].source, _graph[i].target );

					if ( result < _graph[i].data.distance ) {
						numUseless++;
						_graph[i].data.forward = false;
						//Contractor::Witness temp;
						//temp.source = _graph[i].source;
						//temp.target = _graph[i].target;
						//temp.middle = _graph[i].data.middle;
						//_witnessList.push_back( temp );
					}
				}
				if ( _graph[i].data.backward ) {
					int result = _ComputeDistance( _graph[i].target, _graph[i].source );

					if ( result < _graph[i].data.distance ) {
						numUseless++;
						_graph[i].data.backward = false;
						//Contractor::Witness temp;
						//temp.source = _graph[i].target;
						//temp.target = _graph[i].source;
						//temp.middle = _graph[i].data.middle;
						//_witnessList.push_back( temp );
					}
				}
			}
			qDebug( "Found %d useless shortcut directions", numUseless );

			qDebug( "Removing edges" );
			int usefull = 0;
			for ( int i = 0; i < ( int ) _graph.size(); i++ ) {
				if ( !_graph[i].data.forward && !_graph[i].data.backward /*&& _graph[i].data.shortcut */ ) // remove original edges which are too long
					continue;
				_graph[usefull] = _graph[i];
				usefull++;
			}
			qDebug( "Removed %d useless shortcuts", ( int ) _graph.size() - usefull );
			_graph.resize( usefull );
		}

		void ReorderNodes() {
			BuildOutgoingGraph();
			_remap.resize( _numNodes );
			for ( NodeID node = 0; node < _numNodes; ++node ) {
				_remap[node].id = node;
			}

			qDebug( "Compute Node Depth" );
			{
				std::queue< NodeID > q;
				std::vector< unsigned > inDegree( _numNodes, 0 );
				for ( int i = 0; i < ( int ) _graph.size(); i++ ) {
					inDegree[_graph[i].target]++;
				}
				for ( NodeID i = 0; i < _numNodes; i++ ) {
					if ( inDegree[i] == 0 )
						q.push( i );
				}
				assert( !q.empty() );
				NodeID lastNode = q.back();
				int depth = 0;
				while ( !q.empty() ) {
					NodeID node = q.front();
					q.pop();
					_remap[node].depth = depth;
					for ( unsigned i = _firstEdge[node], e = _firstEdge[node + 1]; i < e; i++ ) {
						const NodeID target = _graph[i].target;
						assert( inDegree[target] != 0 );
						inDegree[target]--;
						if ( inDegree[target] == 0 )
							q.push( target );
					}
					if ( node == lastNode ) {
						if ( !q.empty() )
							lastNode = q.back();
						depth++;
					}
				}
			}


			BuildIncomingGraph();
			qDebug( "Sort Nodes Topologically Depths First" );
			{
				std::stack< unsigned > s;
				std::vector< unsigned > outDegree( _numNodes, 0 );
				for ( int i = 0; i < ( int ) _graph.size(); i++ ) {
					outDegree[_graph[i].source]++;
				}
				for ( NodeID i = 0; i < _numNodes; i++ ) {
					if ( outDegree[i] == 0 )
						s.push( i );
				}
				NodeID newID = 0;
				while ( !s.empty() ) {
					unsigned n = s.top();
					s.pop();
					_remap[n].mappedID = newID++;
					for ( unsigned i = _firstEdge[n], e = _firstEdge[n + 1]; i < e; i++ ) {
						const NodeID target = _graph[i].source;
						assert( outDegree[target] != 0 );
						outDegree[target]--;
						if ( outDegree[target] == 0 )
							s.push( target );
					}
				}
				assert( newID == _numNodes );
			}

			qDebug( "Sort Nodes into buckets according to hierarchy depths" );
			{
				//sort by level
				sort( _remap.begin(), _remap.end(), _Node::CompareByDepth );

				//sort buckets by original id
				for ( NodeID i = 0; _numNodes - i > 16; ) {
					NodeID bucketSize = ( _numNodes - i ) * 15 / 16;
					NodeID position = i + bucketSize;
					while ( position + 1 < _numNodes && _remap[position].depth == _remap[position + 1].depth )
						++position;
					sort( _remap.begin() + i, _remap.begin() + position, _Node::CompareByRemappedID );
					i = position;
				}

				//sort by original id
				for ( NodeID i = 0; i < _numNodes; i++ ) {
					_remap[i].mappedID = i;
				}
				sort( _remap.begin(), _remap.end(), _Node::CompareByID );
			}

			//topological sorting
			qDebug( "Sort Nodes Topologically by computed order" );
			{
				std::priority_queue< _Node, std::vector< _Node >, _Comp > q;
				std::vector< unsigned > outDegree( _numNodes, 0 );
				for ( int i = 0; i < ( int ) _graph.size(); i++ ) {
					outDegree[_graph[i].source]++;
				}
				for ( NodeID i = 0; i < _numNodes; i++ ) {
					if ( outDegree[i] == 0 )
						q.push( _remap[i] );
				}
				NodeID newID = 0;
				while ( !q.empty() ) {
					_Node n = q.top();
					q.pop();
					_remap[n.id].mappedID = newID++;
					for ( int i = _firstEdge[n.id], e = _firstEdge[n.id + 1]; i < e; i++ ) {
						const NodeID target = _graph[i].source;
						assert( outDegree[target] != 0 );
						outDegree[target]--;
						if ( outDegree[target] == 0 )
							q.push( _remap[target] );
					}
				}
				assert( newID == _numNodes );
			}

			std::sort( _remap.begin(), _remap.end(), _Node::CompareByID );
		}

		void RemoveDuplicatedWitnesses() {
			qDebug( "Delete Duplicate Entries to save Memory" );
			if ( !_distributedWitnessData.empty() ) {
				int oldSize = _distributedWitnessData.size();
				sort( _distributedWitnessData.begin(), _distributedWitnessData.end() );
				_distributedWitnessData.resize( std::unique( _distributedWitnessData.begin(), _distributedWitnessData.end() ) - _distributedWitnessData.begin() );
				qDebug( "Deleted %d of %d" , ( int ) ( oldSize - _distributedWitnessData.size() ), oldSize );
			}
		}

		void DistributeWitnessData() {
			qDebug( "Distributing witness data" );
			BuildOutgoingGraph();
			int numDeletedWitness = 0;
			for ( unsigned i = _witnessList.size() - 1, e = _witnessList.size(); i < e; i-- ) {

				//save some memory by removing duplicate data during the processing.
				if ( i == e / 4 || i == e / 2 || i == e / 4 * 3 ) {
					RemoveDuplicatedWitnesses();
				}

				//witness obsolet?
				bool foundForwardEdge = false, foundBackwardEdge = false;
				int weight = 0;
				for ( int j = _firstEdge[_witnessList[i].middle], e = _firstEdge[_witnessList[i].middle + 1]; j < e; j++ ) {
					if ( _graph[j].target == _witnessList[i].source && _graph[j].data.backward ) {
						foundBackwardEdge = true;
						weight += _graph[j].data.distance;
					}
					if ( _graph[j].target == _witnessList[i].target && _graph[j].data.forward ) {
						foundForwardEdge = true;
						weight += _graph[j].data.distance;
					}
				}
				if ( !foundForwardEdge || !foundBackwardEdge ) {
					numDeletedWitness++;
					continue;
				}

				std::vector< NodeID > path;
				int result = _ComputeDistance( _witnessList[i].source, _witnessList[i].target, &path );

				for ( int j = 0; j < ( int ) path.size() - 1; j++ ) {
					int actDepth = _remap[path[j]].depth;
					int nextDepth = _remap[path[j + 1]].depth;
					assert( actDepth != nextDepth );
					_Witness temp;
					temp.affectedNode = _witnessList[i].middle;
					temp.safe = weight - result;
					assert( temp.safe > 0 );
					if ( nextDepth > actDepth ) {
						for ( unsigned edge = _firstEdge[path[j]], e = _firstEdge[path[j] + 1]; edge < e; edge++ ) {
							if ( _graph[edge].target == path[j + 1] && _graph[edge].data.forward ) {
								temp.edge = edge;
								break;
							}
							assert( edge != e - 1 );
						}
					} else {
						for ( unsigned edge = _firstEdge[path[j + 1]], e = _firstEdge[path[j + 1] + 1]; edge < e; edge++ ) {
							if ( _graph[edge].target == path[j] && _graph[edge].data.backward ) {
								temp.edge = edge;
								break;
							}
							assert( edge != e - 1 );
						}
					}
					_distributedWitnessData.push_back( temp );
				}

				path.clear();
			}
			std::vector< Contractor::Witness >().swap( _witnessList );

			RemoveDuplicatedWitnesses();

			qDebug( "%d Edge _Witness Entries, %lf per Edge", ( int ) _witnessList.size(), _witnessList.size() * 1.0f / _graph.size() );

			_witnessIndex.resize( _graph.size() + 1 );
			_witnessIndex[0] = 0;
			double safeFactor = 0;
			for ( int i = 0, position = 0; i < ( int ) _graph.size(); i++ ) {
				while ( _distributedWitnessData[position].edge == i && position < ( int ) _distributedWitnessData.size() ) {
					safeFactor += _distributedWitnessData[position].safe;
					position++;
				}
				_witnessIndex[i + 1] = position;
			}
			qDebug( "Average witness safty buffer: %lf", safeFactor / _distributedWitnessData.size() );

		}

		template< class EdgeAllowed > void _ComputeStep( _Heap* heapForward, _Heap* heapBackward, const EdgeAllowed& edgeAllowed, NodeID* middle, int* targetDistance ) {

			const NodeID node = heapForward->DeleteMin();
			const int distance = heapForward->GetKey( node );

			if ( heapBackward->WasInserted( node ) ) {
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
			for ( int edge = _firstEdge[node], endEdges = _firstEdge[node + 1]; edge != endEdges; ++edge ) {
				const NodeID to = _graph[edge].target;
				const int edgeWeight = _graph[edge].data.distance;
				assert( edgeWeight > 0 );
				const int toDistance = distance + edgeWeight;

				if ( edgeAllowed( _graph[edge] ) ) {
					//New Node discovered -> Add to Heap + Node Info Storage
					if ( !heapForward->WasInserted( to ) )
						heapForward->Insert( to, toDistance, node );

					//Found a shorter Path -> Update distance
					else if ( toDistance < heapForward->GetKey( to ) ) {
						heapForward->DecreaseKey( to, toDistance );
						//new parent
						heapForward->GetData( to ) = node;
					}
				}
			}
		}

		int _ComputeDistance( NodeID source, NodeID target, std::vector< NodeID >* path = NULL ) {
			_heapForward->Clear();
			_heapBackward->Clear();
			//insert source into heap
			_heapForward->Insert( source, 0, source );
			_heapBackward->Insert( target, 0, target );

			int targetDistance = std::numeric_limits< int >::max();
			NodeID middle = 0;
			AllowForwardEdge forward;
			AllowBackwardEdge backward;

			while ( _heapForward->Size() + _heapBackward->Size() > 0 ) {

				if ( _heapForward->Size() > 0 ) {
					_ComputeStep( _heapForward, _heapBackward, forward, &middle, &targetDistance );
				}

				if ( _heapBackward->Size() > 0 ) {
					_ComputeStep( _heapBackward, _heapForward, backward, &middle, &targetDistance );
				}

			}

			if ( targetDistance == std::numeric_limits< int >::max() )
				return std::numeric_limits< unsigned >::max();

			if ( path == NULL )
				return targetDistance;

			NodeID pathNode = middle;
			std::stack< NodeID > stack;

			while ( pathNode != source ) {
				NodeID parent = _heapForward->GetData( pathNode ).parent;
				stack.push( pathNode );
				pathNode = parent;
			}
			stack.push( pathNode );
			path->push_back( source );

			while ( stack.size() > 1 ) {
				const NodeID node = stack.top();
				stack.pop();
				_UnpackEdge( node, stack.top(), true, path );
			}

			pathNode = middle;
			while ( pathNode != target ) {
				NodeID parent = _heapBackward->GetData( pathNode ).parent;
				_UnpackEdge( parent, pathNode, false, path );
				pathNode = parent;
			}

			return targetDistance;
		}

		bool _UnpackEdge( const NodeID source, const NodeID target, bool forward, std::vector< NodeID >* path ) {
			int edge = _firstEdge[source];

			for ( int endEdge = _firstEdge[source + 1]; edge != endEdge; ++edge ) {
				if ( _graph[edge].target == target ) {
					if ( forward && _graph[edge].data.forward )
						break;
					if ( ( !forward ) && _graph[edge].data.backward )
						break;
				}
			}
			assert( edge != ( int ) _firstEdge[source + 1] );

			if ( !_graph[edge].data.shortcut ) {
				if ( forward )
					path->push_back( target );
				else
					path->push_back( source );
				return true;
			}

			const NodeID middle = _graph[edge].data.middle;

			if ( forward ) {
				_UnpackEdge( middle, source, false, path );
				_UnpackEdge( middle, target, true, path );
				return true;
			} else {
				_UnpackEdge( middle, target, false, path );
				_UnpackEdge( middle, source, true, path );
				return true;
			}
		}

		NodeID _numNodes;
		std::vector< Edge > _graph;
		std::vector< Edge > _loops;
		std::vector< unsigned > _firstEdge;
		std::vector< _Node > _remap;
		std::vector< Contractor::Witness > _witnessList;
		std::vector< _Witness > _distributedWitnessData;
		std::vector< unsigned > _witnessIndex;
		_Heap* _heapForward;
		_Heap* _heapBackward;
};

#endif // CONTRACTIONCLEANUP_H_INCLUDED
