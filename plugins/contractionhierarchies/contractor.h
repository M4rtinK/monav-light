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

#ifndef CONTRACTOR_H_INCLUDED
#define CONTRACTOR_H_INCLUDED
#include <vector>

#ifndef _OPENMP
#define omp_get_thread_num() (0)
#define omp_get_max_threads() (1)
#else
#include <omp.h>
#endif
#include <limits>
#include "utils/qthelpers.h"
#include "dynamicgraph.h"
#include "binaryheap.h"
#include "utils/config.h"

class Contractor {

	public:

		struct Witness {
			NodeID source;
			NodeID target;
			NodeID middle;
		};

	private:

		struct _EdgeData {
			unsigned distance;
			unsigned originalEdges : 29;
			bool shortcut : 1;
			bool forward : 1;
			bool backward : 1;
			union {
				NodeID middle; // shortcut
				unsigned id; // original edge
			};
		} data;

		struct _HeapData {
			bool target;
			_HeapData()
			{
				target = false;
			}
			_HeapData( bool t )
			{
				target = t;
			}
		};

		typedef DynamicGraph< _EdgeData > _DynamicGraph;
		typedef BinaryHeap< NodeID, NodeID, unsigned, _HeapData > _Heap;
		typedef _DynamicGraph::InputEdge _ImportEdge;

		struct _ThreadData {
			_Heap heap;
			std::vector< _ImportEdge > insertedEdges;
			std::vector< Witness > witnessList;
			std::vector< NodeID > neighbours;
			_ThreadData( NodeID nodes ): heap( nodes ) {
			}
		};

		struct _PriorityData {
			int depth;
			NodeID bias;
			_PriorityData() {
				depth = 0;
			}
		};

		struct _ContractionInformation {
			int edgesDeleted;
			int edgesAdded;
			int originalEdgesDeleted;
			int originalEdgesAdded;
			_ContractionInformation() {
				edgesAdded = edgesDeleted = originalEdgesAdded = originalEdgesDeleted = 0;
			}
		};

		struct _NodePartitionor {
			bool operator()( std::pair< NodeID, bool > nodeData ) {
				return !nodeData.second;
			}
		};

		struct _LogItem {
			unsigned iteration;
			NodeID nodes;
			double contraction;
			double independent;
			double inserting;
			double removing;
			double updating;

			_LogItem() {
				iteration = nodes = contraction = independent = inserting = removing = updating = 0;
			}

			double GetTotalTime() const {
				return contraction + independent + inserting + removing + updating;
			}

			void PrintStatistics() const {
				qDebug( "%d\t%d\t%lf\t%lf\t%lf\t%lf\t%lf", iteration, nodes, independent, contraction, inserting, removing, updating );
			}
		};

		class _LogData {
			public:

				std::vector < _LogItem > iterations;

				unsigned GetNIterations() {
					return ( unsigned ) iterations.size();
				}

				_LogItem GetSum() const {
					_LogItem sum;
					sum.iteration = ( unsigned ) iterations.size();

					for ( int i = 0, e = ( int ) iterations.size(); i < e; ++i ) {
						sum.nodes += iterations[i].nodes;
						sum.contraction += iterations[i].contraction;
						sum.independent += iterations[i].independent;
						sum.inserting += iterations[i].inserting;
						sum.removing += iterations[i].removing;
						sum.updating += iterations[i].updating;
					}

					return sum;
				}

				void PrintHeader() const {
					qDebug( "Iteration\tNodes\tIndependent\tContraction\tInserting\tRemoving\tUpdating" );
				}

				void PrintSummary() const {
					PrintHeader();
					GetSum().PrintStatistics();
				}

				void Print() const {
					PrintHeader();
					for ( int i = 0, e = ( int ) iterations.size(); i < e; ++i )
						iterations[i].PrintStatistics();
				}

				void Insert( const _LogItem& data ) {
					iterations.push_back( data );
				}

		};

	public:

		template< class InputEdge >
		Contractor( int nodes, const std::vector< InputEdge >& inputEdges ) {
			std::vector< _ImportEdge > edges;
			edges.reserve( 2 * inputEdges.size() );
			int skippedLargeEdges = 0;
			for ( typename std::vector< InputEdge >::const_iterator i = inputEdges.begin(), e = inputEdges.end(); i != e; ++i ) {
				_ImportEdge edge;
				edge.source = i->source;
				edge.target = i->target;
				edge.data.distance = std::max( i->distance * 10.0 + 0.5, 1.0 );
				if ( edge.data.distance > 24 * 60 * 60 * 10 ) {
					skippedLargeEdges++;
					continue;
				}
				edge.data.shortcut = false;
				edge.data.id = i - inputEdges.begin();
				edge.data.forward = true;
				edge.data.backward = i->bidirectional;
				edge.data.originalEdges = 1;

				if ( edge.data.distance < 1 ) {
					qDebug() << edge.source << edge.target << edge.data.forward << edge.data.backward << edge.data.distance << edge.data.id << i->distance;
				}

				if ( edge.source == edge.target ) {
					_loops.push_back( edge );
					continue;
				}

				edges.push_back( edge );
				std::swap( edge.source, edge.target );
				edge.data.forward = i->bidirectional;
				edge.data.backward = true;
				edges.push_back( edge );
			}
			if ( skippedLargeEdges != 0 )
				qDebug( "Skipped %d edges with too large edge weight", skippedLargeEdges );
			std::sort( edges.begin(), edges.end() );

			_graph = new _DynamicGraph( nodes, edges );

			std::vector< _ImportEdge >().swap( edges );
		}

		~Contractor() {
			delete _graph;
		}

		void Run() {
			const NodeID numberOfNodes = _graph->GetNumberOfNodes();
			_LogData log;

			int maxThreads = omp_get_max_threads();
			std::vector < _ThreadData* > threadData;
			for ( int threadNum = 0; threadNum < maxThreads; ++threadNum ) {
				threadData.push_back( new _ThreadData( numberOfNodes ) );
			}
			qDebug( "%d nodes, %d edges", numberOfNodes, _graph->GetNumberOfEdges() );
			qDebug( "using %d threads", maxThreads );

			NodeID levelID = 0;
			NodeID iteration = 0;
			std::vector< std::pair< NodeID, bool > > remainingNodes( numberOfNodes );
			std::vector< double > nodePriority( numberOfNodes );
			std::vector< _PriorityData > nodeData( numberOfNodes );

			//initialize the variables
		#pragma omp parallel for schedule ( guided )
			for ( int x = 0; x < ( int ) numberOfNodes; ++x )
				remainingNodes[x].first = x;
			std::random_shuffle( remainingNodes.begin(), remainingNodes.end() );
			for ( int x = 0; x < ( int ) numberOfNodes; ++x )
				nodeData[remainingNodes[x].first].bias = x;

			qDebug( "Initialise Elimination PQ... " );
			_LogItem statistics0;
			statistics0.updating = _Timestamp();
			statistics0.iteration = 0;
		#pragma omp parallel
			{
				_ThreadData* data = threadData[omp_get_thread_num()];
		#pragma omp for schedule ( guided )
				for ( int x = 0; x < ( int ) numberOfNodes; ++x ) {
					nodePriority[x] = _Evaluate( data, &nodeData[x], x );
				}
			}
			qDebug( "done" );

			statistics0.updating = _Timestamp() - statistics0.updating;
			log.Insert( statistics0 );

			log.PrintHeader();
			statistics0.PrintStatistics();

			while ( levelID < numberOfNodes ) {
				_LogItem statistics;
				statistics.iteration = iteration++;
				const int last = ( int ) remainingNodes.size();

				//determine independent node set
				double timeLast = _Timestamp();
		#pragma omp parallel
				{
					_ThreadData* const data = threadData[omp_get_thread_num()];
					#pragma omp for schedule ( guided )
					for ( int i = 0; i < last; ++i ) {
						const NodeID node = remainingNodes[i].first;
						remainingNodes[i].second = _IsIndependent( nodePriority, nodeData, data, node );
					}
				}
				_NodePartitionor functor;
				const std::vector < std::pair < NodeID, bool > >::const_iterator first = stable_partition( remainingNodes.begin(), remainingNodes.end(), functor );
				const int firstIndependent = first - remainingNodes.begin();
				statistics.nodes = last - firstIndependent;
				statistics.independent += _Timestamp() - timeLast;
				timeLast = _Timestamp();

				//contract independent nodes
		#pragma omp parallel
				{
					_ThreadData* const data = threadData[omp_get_thread_num()];
		#pragma omp for schedule ( guided ) nowait
					for ( int position = firstIndependent ; position < last; ++position ) {
						NodeID x = remainingNodes[position].first;
						_Contract< false > ( data, x );
						nodePriority[x] = -1;
					}
					std::sort( data->insertedEdges.begin(), data->insertedEdges.end() );
				}
				statistics.contraction += _Timestamp() - timeLast;
				timeLast = _Timestamp();

				#pragma omp parallel
				{
					_ThreadData* const data = threadData[omp_get_thread_num()];
				#pragma omp for schedule ( guided ) nowait
					for ( int position = firstIndependent ; position < last; ++position ) {
						NodeID x = remainingNodes[position].first;
						_DeleteIncommingEdges( data, x );
					}
				}
				statistics.removing += _Timestamp() - timeLast;
				timeLast = _Timestamp();

				//insert new edges
				for ( int threadNum = 0; threadNum < maxThreads; ++threadNum ) {
					_ThreadData& data = *threadData[threadNum];
					for ( int i = 0; i < ( int ) data.insertedEdges.size(); ++i ) {
						const _ImportEdge& edge = data.insertedEdges[i];
						_graph->InsertEdge( edge.source, edge.target, edge.data );
					}
					std::vector< _ImportEdge >().swap( data.insertedEdges );
				}
				statistics.inserting += _Timestamp() - timeLast;
				timeLast = _Timestamp();

				//update priorities
		#pragma omp parallel
				{
					_ThreadData* const data = threadData[omp_get_thread_num()];
		#pragma omp for schedule ( guided ) nowait
					for ( int position = firstIndependent ; position < last; ++position ) {
						NodeID x = remainingNodes[position].first;
						_UpdateNeighbours( &nodePriority, &nodeData, data, x );
					}
				}
				statistics.updating += _Timestamp() - timeLast;
				timeLast = _Timestamp();

				//output some statistics
				statistics.PrintStatistics();
				//qDebug( wxT( "Printed" ) );

				//remove contracted nodes from the pool
				levelID += last - firstIndependent;
				remainingNodes.resize( firstIndependent );
				std::vector< std::pair< NodeID, bool > >( remainingNodes ).swap( remainingNodes );
				log.Insert( statistics );
			}

			for ( int threadNum = 0; threadNum < maxThreads; threadNum++ ) {
				_witnessList.insert( _witnessList.end(), threadData[threadNum]->witnessList.begin(), threadData[threadNum]->witnessList.end() );
				delete threadData[threadNum];
			}

			log.PrintSummary();
			qDebug( "Total Time: %lf s", log.GetSum().GetTotalTime() );

		}

		template< class Edge >
		void GetEdges( std::vector< Edge >* edges ) {
			NodeID numberOfNodes = _graph->GetNumberOfNodes();
			for ( NodeID node = 0; node < numberOfNodes; ++node ) {
				for ( _DynamicGraph::EdgeIterator edge = _graph->BeginEdges( node ), endEdges = _graph->EndEdges( node ); edge != endEdges; ++edge ) {
					const NodeID target = _graph->GetTarget( edge );
					const _EdgeData& data = _graph->GetEdgeData( edge );
					Edge newEdge;
					newEdge.source = node;
					newEdge.target = target;
					newEdge.data.distance = data.distance;
					newEdge.data.shortcut = data.shortcut;
					if ( data.shortcut )
						newEdge.data.middle = data.middle;
					else
						newEdge.data.id = data.id;
					newEdge.data.forward = data.forward;
					newEdge.data.backward = data.backward;
					edges->push_back( newEdge );
				}
			}
		}

		template< class Edge >
		void GetLoops( std::vector< Edge >* edges ) {
			for ( unsigned i = 0; i < _loops.size(); i++ ) {
				Edge newEdge;
				newEdge.source = _loops[i].source;
				newEdge.target = _loops[i].target;
				newEdge.data.distance = _loops[i].data.distance;
				newEdge.data.shortcut = _loops[i].data.shortcut;
				newEdge.data.id = _loops[i].data.id;
				newEdge.data.forward = _loops[i].data.forward;
				newEdge.data.backward = _loops[i].data.backward;
				edges->push_back( newEdge );
			}
		}

		void GetWitnessList( std::vector< Witness >& list ) {
			list = _witnessList;
		}

	private:

		double _Timestamp() {
			static Timer timer;
			return ( double ) timer.elapsed() / 1000;
		}

		bool _ConstructCH( _DynamicGraph* _graph );

		void _Dijkstra( const unsigned maxDistance, const unsigned numTargets, const int maxNodes, _ThreadData* const data ){

			_Heap& heap = data->heap;

			int nodes = 0;
			unsigned targetsFound = 0;
			while ( heap.Size() > 0 ) {
				const NodeID node = heap.DeleteMin();
				const unsigned distance = heap.GetKey( node );
				if ( nodes++ > maxNodes )
					return;
				//Destination settled?
				if ( distance > maxDistance )
					return;
				if ( heap.GetData( node ).target ) {
					targetsFound++;
					if ( targetsFound >= numTargets )
						return;
				}

				//iterate over all edges of node
				for ( _DynamicGraph::EdgeIterator edge = _graph->BeginEdges( node ), endEdges = _graph->EndEdges( node ); edge != endEdges; ++edge ) {
					const _EdgeData& data = _graph->GetEdgeData( edge );
					if ( !data.forward )
						continue;
					const NodeID to = _graph->GetTarget( edge );
					const unsigned toDistance = distance + data.distance;

					//New Node discovered -> Add to Heap + Node Info Storage
					if ( !heap.WasInserted( to ) )
						heap.Insert( to, toDistance, _HeapData() );

					//Found a shorter Path -> Update distance
					else if ( toDistance < heap.GetKey( to ) ) {
						heap.DecreaseKey( to, toDistance );
					}
				}
			}
		}

		double _Evaluate( _ThreadData* const data, _PriorityData* const nodeData, NodeID node ){
			_ContractionInformation stats;

			//perform simulated contraction
			_Contract< true > ( data, node, &stats );

			// Result will contain the priority
			double result;
			if ( stats.edgesDeleted == 0 || stats.originalEdgesDeleted == 0 )
				result = 1 * nodeData->depth;
			else
				result =  2 * ((( double ) stats.edgesAdded ) / stats.edgesDeleted ) + 1 * ((( double ) stats.originalEdgesAdded ) / stats.originalEdgesDeleted ) + 1 * nodeData->depth;
			assert( result >= 0 );
			return result;
		}


		template< bool Simulate > bool _Contract( _ThreadData* const data, NodeID node, _ContractionInformation* const stats = NULL ) {
			_Heap& heap = data->heap;
			//std::vector< Witness >& witnessList = data->witnessList;
			int insertedEdgesSize = data->insertedEdges.size();
			std::vector< _ImportEdge >& insertedEdges = data->insertedEdges;

			for ( _DynamicGraph::EdgeIterator inEdge = _graph->BeginEdges( node ), endInEdges = _graph->EndEdges( node ); inEdge != endInEdges; ++inEdge ) {
				const _EdgeData& inData = _graph->GetEdgeData( inEdge );
				const NodeID source = _graph->GetTarget( inEdge );
				if ( Simulate ) {
					assert( stats != NULL );
					stats->edgesDeleted++;
					stats->originalEdgesDeleted += inData.originalEdges;
				}
				if ( !inData.backward )
					continue;

				heap.Clear();
				heap.Insert( source, 0, _HeapData() );
				if ( node != source )
					heap.Insert( node, inData.distance, _HeapData() );
				unsigned maxDistance = 0;
				unsigned numTargets = 0;

				for ( _DynamicGraph::EdgeIterator outEdge = _graph->BeginEdges( node ), endOutEdges = _graph->EndEdges( node ); outEdge != endOutEdges; ++outEdge ) {
					const _EdgeData& outData = _graph->GetEdgeData( outEdge );
					if ( !outData.forward )
						continue;
					const NodeID target = _graph->GetTarget( outEdge );
					const unsigned pathDistance = inData.distance + outData.distance;
					maxDistance = std::max( maxDistance, pathDistance );
					if ( !heap.WasInserted( target ) ) {
						heap.Insert( target, pathDistance, _HeapData( true ) );
						numTargets++;
					} else if ( pathDistance < heap.GetKey( target ) ) {
						heap.DecreaseKey( target, pathDistance );
					}
				}

				if ( Simulate )
					_Dijkstra( maxDistance, numTargets, 500, data );
				else
					_Dijkstra( maxDistance, numTargets, 1000, data );

				for ( _DynamicGraph::EdgeIterator outEdge = _graph->BeginEdges( node ), endOutEdges = _graph->EndEdges( node ); outEdge != endOutEdges; ++outEdge ) {
					const _EdgeData& outData = _graph->GetEdgeData( outEdge );
					if ( !outData.forward )
						continue;
					const NodeID target = _graph->GetTarget( outEdge );
					const int pathDistance = inData.distance + outData.distance;
					const int distance = heap.GetKey( target );

					if ( pathDistance <= distance ) {
						if ( Simulate ) {
							assert( stats != NULL );
							stats->edgesAdded += 2;
							stats->originalEdgesAdded += 2 * ( outData.originalEdges + inData.originalEdges );
						} else {
							_ImportEdge newEdge;
							newEdge.source = source;
							newEdge.target = target;
							newEdge.data.distance = pathDistance;
							newEdge.data.forward = true;
							newEdge.data.backward = false;
							newEdge.data.middle = node;
							newEdge.data.shortcut = true;
							newEdge.data.originalEdges = outData.originalEdges + inData.originalEdges;
							insertedEdges.push_back( newEdge );
							std::swap( newEdge.source, newEdge.target );
							newEdge.data.forward = false;
							newEdge.data.backward = true;
							insertedEdges.push_back( newEdge );
						}
					}
					/*else if ( !Simulate ) {
						Witness witness;
						witness.source = source;
						witness.target = target;
						witness.middle = node;
						witnessList.push_back( witness );
					}*/
				}
			}

			if ( !Simulate ) {
				for ( int i = insertedEdgesSize, iend = insertedEdges.size(); i < iend; i++ ) {
					bool found = false;
					for ( int other = i + 1 ; other < iend ; ++other ) {
						if ( insertedEdges[other].source != insertedEdges[i].source )
							continue;
						if ( insertedEdges[other].target != insertedEdges[i].target )
							continue;
						if ( insertedEdges[other].data.distance != insertedEdges[i].data.distance )
							continue;
						if ( insertedEdges[other].data.shortcut != insertedEdges[i].data.shortcut )
							continue;
						insertedEdges[other].data.forward |= insertedEdges[i].data.forward;
						insertedEdges[other].data.backward |= insertedEdges[i].data.backward;
						found = true;
						break;
					}
					if ( !found )
						insertedEdges[insertedEdgesSize++] = insertedEdges[i];
				}
				insertedEdges.resize( insertedEdgesSize );
			}

			return true;
		}

		bool _DeleteIncommingEdges( _ThreadData* const data, NodeID node ) {
			std::vector< NodeID >& neighbours = data->neighbours;
			neighbours.clear();

			//find all neighbours
			for ( _DynamicGraph::EdgeIterator e = _graph->BeginEdges( node ) ; e < _graph->EndEdges( node ) ; ++e ) {
				const NodeID u = _graph->GetTarget( e );
				if ( u == node )
					continue;
				neighbours.push_back( u );
			}
			//eliminate duplicate entries ( forward + backward edges )
			std::sort( neighbours.begin(), neighbours.end() );
			neighbours.resize( std::unique( neighbours.begin(), neighbours.end() ) - neighbours.begin() );

			for ( int i = 0, e = ( int ) neighbours.size(); i < e; ++i ) {
				const NodeID u = neighbours[i];
				_graph->DeleteEdgesTo( u, node );
			}

			return true;
		}

		bool _UpdateNeighbours( std::vector< double >* priorities, std::vector< _PriorityData >* const nodeData, _ThreadData* const data, NodeID node ) {
			std::vector< NodeID >& neighbours = data->neighbours;
			neighbours.clear();

			//find all neighbours
			for ( _DynamicGraph::EdgeIterator e = _graph->BeginEdges( node ) ; e < _graph->EndEdges( node ) ; ++e ) {
				const NodeID u = _graph->GetTarget( e );
				if ( u == node )
					continue;
				neighbours.push_back( u );
				( *nodeData )[u].depth = std::max(( *nodeData )[node].depth + 1, ( *nodeData )[u].depth );
			}
			//eliminate duplicate entries ( forward + backward edges )
			std::sort( neighbours.begin(), neighbours.end() );
			neighbours.resize( std::unique( neighbours.begin(), neighbours.end() ) - neighbours.begin() );

			for ( int i = 0, e = ( int ) neighbours.size(); i < e; ++i ) {
				const NodeID u = neighbours[i];
				( *priorities )[u] = _Evaluate( data, &( *nodeData )[u], u );
			}

			return true;
		}

		bool _IsIndependent( const std::vector< double >& priorities, const std::vector< _PriorityData >& nodeData, _ThreadData* const data, NodeID node ) {
			const double priority = priorities[node];

			std::vector< NodeID >& neighbours = data->neighbours;
			neighbours.clear();

			for ( _DynamicGraph::EdgeIterator e = _graph->BeginEdges( node ) ; e < _graph->EndEdges( node ) ; ++e ) {
				const NodeID target = _graph->GetTarget( e );
				const double targetPriority = priorities[target];
				assert( targetPriority >= 0 );
				//found a neighbour with lower priority?
				if ( priority > targetPriority )
					return false;
				//tie breaking
				if ( priority == targetPriority && nodeData[node].bias < nodeData[target].bias )
					return false;
				neighbours.push_back( target );
			}

			std::sort( neighbours.begin(), neighbours.end() );
			neighbours.resize( std::unique( neighbours.begin(), neighbours.end() ) - neighbours.begin() );

			//examine all neighbours that are at most 2 hops away
			for ( std::vector< NodeID >::const_iterator i = neighbours.begin(), lastNode = neighbours.end(); i != lastNode; ++i ) {
				const NodeID u = *i;

				for ( _DynamicGraph::EdgeIterator e = _graph->BeginEdges( u ) ; e < _graph->EndEdges( u ) ; ++e ) {
					const NodeID target = _graph->GetTarget( e );

					const double targetPriority = priorities[target];
					assert( targetPriority >= 0 );
					//found a neighbour with lower priority?
					if ( priority > targetPriority )
						return false;
					//tie breaking
					if ( priority == targetPriority && nodeData[node].bias < nodeData[target].bias )
						return false;
				}
			}

			return true;
		}


		_DynamicGraph* _graph;
		std::vector< Witness > _witnessList;
		std::vector< _ImportEdge > _loops;
};

#endif // CONTRACTOR_H_INCLUDED
