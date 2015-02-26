/*
Copyright 2010 Christoph Eckert ce@christeck.de

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

#ifndef LOGGER_H
#define LOGGER_H

#include <iostream>
#include <QObject>
#include <QDebug>
#include <QSettings>
#include <QDateTime>
#include <QFileInfo>
#include <QDir>
#include <QFile>
#include "../utils/coordinates.h"
#include "routinglogic.h"

class Logger : public QObject
{

Q_OBJECT

public:

	static Logger* instance();
	~Logger();
	bool loggingEnabled();
	void setLoggingEnabled(bool);
	QString directory();
	void setDirectory(QString);

	QVector< int > polygonEndpointsTracklog();
	QVector< UnsignedCoordinate > polygonCoordsTracklog();

public slots:

	void positionChanged();
	void initialize();
	void clearTracklog();

	// destroys the object
	void cleanup();

signals:
	void trackChanged();

protected:

	explicit Logger( QObject* parent = 0 );
	bool readGpxLog();
	bool writeGpxLog();
	QFile m_logFile;
	QDateTime m_lastFlushTime;
	bool m_loggingEnabled;
	QString m_tracklogPath;
	QString m_tracklogPrefix;
	QVector<RoutingLogic::GPSInfo> m_gpsInfoBuffer;
};

#endif // LOGGER_H
