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

#include "contractionhierarchies.h"
#include "compressedgraphbuilder.h"
#include "contractor.h"
#include "contractioncleanup.h"
#include "utils/qthelpers.h"
#ifndef NOGUI
#include "chsettingsdialog.h"
#endif

#include <QSettings>

ContractionHierarchies::ContractionHierarchies()
{
}

ContractionHierarchies::~ContractionHierarchies()
{
}

QString ContractionHierarchies::GetName()
{
	return "Contraction Hierarchies";
}

bool ContractionHierarchies:: LoadSettings( QSettings* settings )
{
	settings->beginGroup( "ContractionHierarchies" );
	bool ok = false;
	m_settings.blockSize = settings->value( "blockSize", 12 ).toInt( &ok );
	settings->endGroup();
	return ok;
}

bool ContractionHierarchies::SaveSettings( QSettings* settings )
{
	settings->beginGroup( "ContractionHierarchies" );
	settings->setValue( "blockSize", m_settings.blockSize );
	settings->endGroup();
	return true;
}

int ContractionHierarchies::GetFileFormatVersion()
{
	return 1;
}

ContractionHierarchies::Type ContractionHierarchies::GetType()
{
	return Router;
}

bool ContractionHierarchies::Preprocess( IImporter* importer, QString dir )
{
	QString filename = fileInDirectory( dir, "Contraction Hierarchies" );

	std::vector< IImporter::RoutingNode > inputNodes;
	std::vector< IImporter::RoutingEdge > inputEdges;

	if ( !importer->GetRoutingNodes( &inputNodes ) )
		return false;
	if ( !importer->GetRoutingEdges( &inputEdges ) )
		return false;

	unsigned numEdges = inputEdges.size();
	unsigned numNodes = inputNodes.size();

	Contractor* contractor = new Contractor( numNodes, inputEdges );
	std::vector< IImporter::RoutingEdge >().swap( inputEdges );
	contractor->Run();

	std::vector< Contractor::Witness > witnessList;
	contractor->GetWitnessList( witnessList );

	std::vector< ContractionCleanup::Edge > contractedEdges;
	std::vector< ContractionCleanup::Edge > contractedLoops;
	contractor->GetEdges( &contractedEdges );
	contractor->GetLoops( &contractedLoops );
	delete contractor;

	ContractionCleanup* cleanup = new ContractionCleanup( inputNodes.size(), contractedEdges, contractedLoops, witnessList );
	std::vector< ContractionCleanup::Edge >().swap( contractedEdges );
	std::vector< ContractionCleanup::Edge >().swap( contractedLoops );
	std::vector< Contractor::Witness >().swap( witnessList );
	cleanup->Run();

	std::vector< CompressedGraph::Edge > edges;
	std::vector< NodeID > map;
	cleanup->GetData( &edges, &map );
	delete cleanup;

	{
		std::vector< unsigned > edgeIDs( numEdges );
		for ( unsigned edge = 0; edge < edges.size(); edge++ ) {
			if ( edges[edge].data.shortcut )
				continue;
			unsigned id = 0;
			unsigned otherEdge = edge;
			while ( true ) {
				if ( otherEdge == 0 )
					break;
				otherEdge--;
				if ( edges[otherEdge].source != edges[edge].source )
					break;
				if ( edges[otherEdge].target != edges[edge].target )
					continue;
				if ( edges[otherEdge].data.shortcut )
					continue;
				id++;
			}
			edgeIDs[edges[edge].data.id] = id;
		}
		importer->SetEdgeIDMap( edgeIDs );
	}

	std::vector< IRouter::Node > nodes( numNodes );
	for ( std::vector< IImporter::RoutingNode >::const_iterator i = inputNodes.begin(), iend = inputNodes.end(); i != iend; i++ )
		nodes[map[i - inputNodes.begin()]].coordinate = i->coordinate;
	std::vector< IImporter::RoutingNode >().swap( inputNodes );

	std::vector< IRouter::Node > pathNodes;
	{
		std::vector< IImporter::RoutingNode > edgePaths;
		if ( !importer->GetRoutingEdgePaths( &edgePaths ) )
			return false;
		pathNodes.resize( edgePaths.size() );
		for ( unsigned i = 0; i < edgePaths.size(); i++ )
			pathNodes[i].coordinate = edgePaths[i].coordinate;
	}

	if ( !importer->GetRoutingEdges( &inputEdges ) )
		return false;

	{
		std::vector< QString > inputNames;
		if ( !importer->GetRoutingWayNames( &inputNames ) )
			return false;

		QFile nameFile( filename + "_names" );
		if ( !openQFile( &nameFile, QIODevice::WriteOnly ) )
			return false;

		std::vector< unsigned > nameMap( inputNames.size() );
		for ( unsigned name = 0; name < inputNames.size(); name++ ) {
			nameMap[name] = nameFile.pos();
			QByteArray buffer = inputNames[name].toUtf8();
			buffer.push_back( ( char ) 0 );
			nameFile.write( buffer );
		}

		nameFile.close();
		nameFile.open( QIODevice::ReadOnly );
		const char* test = ( const char* ) nameFile.map( 0, nameFile.size() );
		for ( unsigned name = 0; name < inputNames.size(); name++ ) {
			QString testName = QString::fromUtf8( test + nameMap[name] );
			assert( testName == inputNames[name] );
		}

		for ( unsigned edge = 0; edge < numEdges; edge++ )
			inputEdges[edge].nameID = nameMap[inputEdges[edge].nameID];
	}

	{
		std::vector< QString > inputTypes;
		if ( !importer->GetRoutingWayTypes( &inputTypes ) )
			return false;

		QFile typeFile( filename + "_types" );
		if ( !openQFile( &typeFile, QIODevice::WriteOnly ) )
			return false;

		QStringList typeList;
		for ( unsigned type = 0; type < inputTypes.size(); type++ )
			typeList.push_back( inputTypes[type] );

		typeFile.write( typeList.join( ";" ).toUtf8() );
	}

	for ( std::vector< IImporter::RoutingEdge >::iterator i = inputEdges.begin(), iend = inputEdges.end(); i != iend; i++ ) {
		i->source = map[i->source];
		i->target = map[i->target];
	}

	CompressedGraphBuilder* builder = new CompressedGraphBuilder( 1u << m_settings.blockSize, nodes, edges, inputEdges, pathNodes );
	if ( !builder->run( filename, &map ) )
		return false;
	delete builder;

	importer->SetIDMap( map );

	return true;
}

#ifndef NOGUI
bool ContractionHierarchies::GetSettingsWindow( QWidget** window )
{
	*window = new CHSettingsDialog();
	return true;
}

bool ContractionHierarchies::FillSettingsWindow( QWidget* window )
{
	CHSettingsDialog* settings = qobject_cast< CHSettingsDialog* >( window );
	if ( settings == NULL )
		return false;
	return settings->readSettings( m_settings );
}

bool ContractionHierarchies::ReadSettingsWindow( QWidget* window )
{
	CHSettingsDialog* settings = qobject_cast< CHSettingsDialog* >( window );
	if ( settings == NULL )
		return false;
	return settings->fillSettings( &m_settings );
}

#endif

// IConsoleSettings
QString ContractionHierarchies::GetModuleName()
{
	return GetName();
}

bool ContractionHierarchies::GetSettingsList( QVector< Setting >* settings )
{
	settings->push_back( Setting( "", "block-size", "sets block size of compressed graph to 2^x", "integer > 7" ) );
	return true;
}

bool ContractionHierarchies::SetSetting( int id, QVariant data )
{
	bool ok = true;
	switch( id ) {
	case 0:
		m_settings.blockSize = data.toInt( &ok );
		break;
	default:
		return false;
	}

	return ok;
}

Q_EXPORT_PLUGIN2( contractionhierarchies, ContractionHierarchies )
