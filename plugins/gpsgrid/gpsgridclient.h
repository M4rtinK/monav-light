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

#ifndef GPSGRIDCLIENT_H
#define GPSGRIDCLIENT_H

#include <QObject>
#include <QFile>
#include "interfaces/igpslookup.h"
#include "cell.h"
#include "table.h"
#include <QCache>

class GPSGridClient : public QObject, public IGPSLookup
{
	Q_OBJECT
	Q_INTERFACES( IGPSLookup )
public:
	GPSGridClient();
	virtual ~GPSGridClient();

	virtual QString GetName();
	virtual void SetInputDirectory( const QString& dir );
	virtual void ShowSettings();
	virtual bool IsCompatible( int fileFormatVersion );
	virtual bool LoadData();
	virtual bool UnloadData();
	virtual bool GetNearestEdge( Result* result, const UnsignedCoordinate& coordinate, double radius, double headingPenalty, double heading );

signals:

public slots:

protected:

	double gridDistance2( UnsignedCoordinate* nearestPoint, double* percentage, const UnsignedCoordinate source, const UnsignedCoordinate target, const UnsignedCoordinate& coordinate );
	double gridDistance2( const UnsignedCoordinate& min, const UnsignedCoordinate& max, const UnsignedCoordinate& coordinate );
	bool checkCell( Result* result, QVector< UnsignedCoordinate >* path, NodeID gridX, NodeID gridY, const UnsignedCoordinate& coordinate, double gridRadius2, double gridHeadingPenalty2 = 0, double heading = 0);

	long long cacheSize;
	QString directory;
	QFile* gridFile;
	QCache< qint64, gg::Cell > cache;
	gg::Index* index;
};

#endif // GPSGRIDCLIENT_H
