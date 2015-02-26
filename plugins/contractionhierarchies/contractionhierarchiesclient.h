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

#ifndef CONTRACTIONHIERARCHIESCLIENT_H
#define CONTRACTIONHIERARCHIESCLIENT_H

#include <QObject>
#include <QStringList>
#include "interfaces/irouter.h"
#include "binaryheap.h"
#include "compressedgraph.h"
#include <queue>

class ContractionHierarchiesClient : public QObject, public IRouter
{
	Q_OBJECT
	Q_INTERFACES( IRouter )
public:
	ContractionHierarchiesClient();
	virtual ~ContractionHierarchiesClient();

	virtual QString GetName();
	virtual void SetInputDirectory( const QString& dir );
	virtual void ShowSettings();
	virtual bool IsCompatible( int fileFormatVersion );
	virtual bool LoadData();
	virtual bool UnloadData();
	virtual bool GetRoute( double* distance, QVector< Node>* pathNodes, QVector< Edge >* pathEdges, const IGPSLookup::Result& source, const IGPSLookup::Result& target );
	virtual bool GetName( QString* result, unsigned name );
	virtual bool GetNames( QVector< QString >* result, QVector< unsigned > names );
	virtual bool GetType( QString* result, unsigned type );
	virtual bool GetTypes( QVector< QString >* result, QVector< unsigned > types );

protected:
	struct HeapData {
		CompressedGraph::NodeIterator parent;
		bool stalled: 1;
		HeapData( CompressedGraph::NodeIterator p ) {
			parent = p;
			stalled = false;
		}
	};

	class AllowForwardEdge {
		public:
			bool operator()( bool forward, bool /*backward*/ ) const {
				return forward;
			}
	};

	class AllowBackwardEdge {
		public:
			bool operator()( bool /*forward*/, bool backward ) const {
				return backward;
			}
	};

	typedef CompressedGraph::NodeIterator NodeIterator;
	typedef CompressedGraph::EdgeIterator EdgeIterator;
	typedef BinaryHeap< NodeIterator, int, int, HeapData, MapStorage< NodeIterator, unsigned > > Heap;

	CompressedGraph m_graph;
	const char* m_names;
	QFile m_namesFile;
	Heap* m_heapForward;
	Heap* m_heapBackward;
	std::queue< NodeIterator > m_stallQueue;
	QString m_directory;
	QStringList m_types;

	template< class EdgeAllowed, class StallEdgeAllowed >
	void computeStep( Heap* heapForward, Heap* heapBackward, const EdgeAllowed& edgeAllowed, const StallEdgeAllowed& stallEdgeAllowed, NodeIterator* middle, int* targetDistance );
	int computeRoute( const IGPSLookup::Result& source, const IGPSLookup::Result& target, QVector< Node>* pathNodes, QVector< Edge >* pathEdges );
	bool unpackEdge( const NodeIterator source, const NodeIterator target, bool forward, QVector< Node>* pathNodes, QVector< Edge >* pathEdges );

};

#endif // CONTRACTIONHIERARCHIESCLIENT_H
