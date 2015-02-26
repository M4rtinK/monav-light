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

#ifndef GPSGRID_H
#define GPSGRID_H

#include "interfaces/ipreprocessor.h"
#include "interfaces/iguisettings.h"
#include "utils/coordinates.h"

class GPSGrid :
		public QObject,
#ifndef NOGUI
		public IGUISettings,
#endif
		public IPreprocessor
{
	Q_OBJECT
	Q_INTERFACES( IPreprocessor )
	Q_PLUGIN_METADATA(IID "monav.IPreprocessor/1.2")
#ifndef NOGUI
	Q_INTERFACES( IGUISettings )
	Q_PLUGIN_METADATA(IID "monav.IGUISettings/1.0")
#endif

public:

	GPSGrid();
	virtual ~GPSGrid();

	// IPreprocessor
	virtual QString GetName();
	virtual int GetFileFormatVersion();
	virtual Type GetType();
	virtual bool LoadSettings( QSettings* settings );
	virtual bool SaveSettings( QSettings* settings );
	virtual bool Preprocess( IImporter* importer, QString dir );

#ifndef NOGUI
	// IGUISettings
	virtual bool GetSettingsWindow( QWidget** window );
	virtual bool FillSettingsWindow( QWidget* window );
	virtual bool ReadSettingsWindow( QWidget* window );
#endif

protected:

	struct GridImportEdge {
		unsigned edge;
		int x;
		int y;
		bool operator<( const GridImportEdge& right ) const {
			if ( x != right.x )
				return x < right.x;
			return y < right.y;
		}
	};

	bool clipHelper( double directedProjection, double directedDistance, double* tMinimum, double* tMaximum );
	bool clipEdge( ProjectedCoordinate source, ProjectedCoordinate target, ProjectedCoordinate min, ProjectedCoordinate max );
};

#endif // GPSGRID_H
