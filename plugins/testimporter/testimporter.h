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

#ifndef OSMIMPORTER_H
#define OSMIMPORTER_H

#include "interfaces/iimporter.h"
#include "interfaces/iguisettings.h"
#include "interfaces/iconsolesettings.h"

#include <QObject>

class TestImporter :
		public QObject,
		public IConsoleSettings,
		public IImporter
{
	Q_OBJECT
	Q_INTERFACES( IImporter )
	Q_PLUGIN_METADATA(IID "monav.IImporter/1.0")
	Q_INTERFACES( IConsoleSettings )
	Q_PLUGIN_METADATA(IID "monav.IConsoleSettings/1.0")

public:

	struct Settings {
		bool forbidUTurn;
		int uTurnPenalty;
	};

	TestImporter();
	virtual ~TestImporter();

	// IPreprocessor
	virtual QString GetName();
	virtual void SetOutputDirectory( const QString& dir );
	virtual bool LoadSettings( QSettings* settings );
	virtual bool SaveSettings( QSettings* settings );
	virtual bool Preprocess( QString filename );
	virtual bool SetIDMap( const std::vector< NodeID >& idMap );
	virtual bool GetIDMap( std::vector< NodeID >* idMap );
	virtual bool SetEdgeIDMap( const std::vector< NodeID >& idMap );
	virtual bool GetEdgeIDMap( std::vector< NodeID >* idMap );
	virtual bool GetRoutingEdges( std::vector< RoutingEdge >* data );
	virtual bool GetRoutingEdgePaths( std::vector< RoutingNode >* data );
	virtual bool GetRoutingNodes( std::vector< RoutingNode >* data );
	virtual bool GetRoutingWayNames( std::vector< QString >* data );
	virtual bool GetRoutingWayTypes( std::vector< QString >* data );
	virtual bool GetRoutingPenalties( std::vector< char >* inDegree, std::vector< char >* outDegree, std::vector< double >* penalties );
	virtual bool GetAddressData( std::vector< Place >* dataPlaces, std::vector< Address >* dataAddresses, std::vector< UnsignedCoordinate >* dataWayBuffer, std::vector< QString >* addressNames );
	virtual bool GetBoundingBox( BoundingBox* box );
	virtual void DeleteTemporaryFiles();

	// IConsoleSettings
	virtual QString GetModuleName();
	virtual bool GetSettingsList( QVector< Setting >* settings );
	virtual bool SetSetting( int id, QVariant data );

protected:

	struct SimpleEdge {
		unsigned source;
		unsigned target;
		double distance;
		bool forward;
		bool backward;
		bool bidirectional;

		bool operator<( const SimpleEdge& right ) const {
			if ( bidirectional != right.bidirectional )
				return bidirectional;
			if ( source != right.source )
				return source < right.source;
			if ( target != right.target )
				return target < right.target;
			return distance < right.distance;
		}

		static bool sortToMerge( const SimpleEdge& left, const SimpleEdge& right ) {
			if ( left.source != right.source )
				return left.source < right.source;
			if ( left.target != right.target )
				return left.target < right.target;
			if ( left.distance != right.distance )
				return left.distance < right.distance;
			return false;
		}

		bool operator==( const SimpleEdge& right ) const {
			return source == right.source &&
					target == right.target &&
					distance == right.distance &&
					bidirectional == right.bidirectional;
		}
	};

	bool parseDDSG( QString filename );

	Settings m_settings;
	QString m_outputDirectory;
};

#endif // OSMIMPORTER_H
