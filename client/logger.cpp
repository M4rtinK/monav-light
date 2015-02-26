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

#include "logger.h"
#include "routinglogic.h"
#include "utils/qthelpers.h"

Logger* Logger::instance()
{
	static Logger logger;
	return &logger;
}


Logger::Logger( QObject* parent ) :
	QObject( parent )
{
	initialize();
	readGpxLog();
}


Logger::~Logger()
{
}

void Logger::cleanup()
{
	writeGpxLog();
}

void Logger::initialize()
{
	m_lastFlushTime = QDateTime::currentDateTime();

	QSettings settings( "MoNavClient" );
	m_loggingEnabled = settings.value( "LoggingEnabled", false ).toBool();
	m_tracklogPath = settings.value( "LogFilePath", QDir::homePath() ).toString();

	m_tracklogPrefix = tr( "MoNav Track" );
	QString tracklogFilename = m_tracklogPrefix;

	QDateTime currentDateTime = QDateTime::currentDateTime();
	tracklogFilename.append( currentDateTime.toString( " yyyy-MM-dd" ) );
	tracklogFilename.append( ".gpx" );
	m_logFile.setFileName( fileInDirectory( m_tracklogPath, tracklogFilename ) );
}


QVector< int > Logger::polygonEndpointsTracklog()
{
	QVector<int> endpoints;
	bool append = false;
	int invalidElements = 0;
	for (int i = 0; i < m_gpsInfoBuffer.size(); i++)
	{
		if( !m_gpsInfoBuffer.at(i).position.IsValid() )
		{
			invalidElements++;
			append = true;
			continue;
		}
		if (append == true || endpoints.size() == 0)
		{
			endpoints.append(i+1-invalidElements);
			append = false;
			continue;
		}
		endpoints.pop_back();
		endpoints.append(i+1-invalidElements);
	}

	return endpoints;
}


QVector< UnsignedCoordinate > Logger::polygonCoordsTracklog()
{
	QVector<UnsignedCoordinate> coordinates;
	for (int i = 0; i < m_gpsInfoBuffer.size(); i++)
	{
		if( m_gpsInfoBuffer.at(i).position.IsValid() )
			coordinates.append(m_gpsInfoBuffer.at(i).position);
	}
	return coordinates;
}


void Logger::positionChanged()
{
	if ( !m_loggingEnabled )
		return;

	const RoutingLogic::GPSInfo& gpsInfo = RoutingLogic::instance()->gpsInfo();
	if ( !gpsInfo.position.IsValid() )
		return;
	m_gpsInfoBuffer.append(gpsInfo);
	int flushSecondsPassed = m_lastFlushTime.secsTo( QDateTime::currentDateTime() );
	if ( flushSecondsPassed >= 300 )
		writeGpxLog();
	emit trackChanged();
}


bool Logger::writeGpxLog()
{

	QString backupFilename = m_logFile.fileName().remove( m_logFile.fileName().size() -4, 4 ).append( "-bck.gpx" );
	if ( m_logFile.exists() && m_logFile.exists(backupFilename))
		m_logFile.remove( backupFilename );

	if ( !m_logFile.open( QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate ) ){
		m_loggingEnabled = false;
		qDebug() << "Logger: Cannot write " << m_logFile.fileName() << ". Logging disabled.";
		return false;
	}

	QString trackName = m_tracklogPrefix;
	QDateTime currentDateTime = QDateTime::currentDateTime();
	trackName.append( currentDateTime.toString( " yyyy-MM-dd" ) );
	trackName.prepend("  <name>");
	trackName.append("</name>\n");

	QTextStream gpxStream(&m_logFile);
	gpxStream << QString("<?xml version=\"1.0\"?>\n").toUtf8();
	gpxStream << QString("<gpx version=\"1.0\" creator=\"MoNav Tracklogger\" xmlns=\"http://www.topografix.com/GPX/1/0\">\n").toUtf8();
	gpxStream << QString("  <trk>\n").toUtf8();
	gpxStream << trackName;

	bool insideTracksegment = false;
	for (int i = 0; i < m_gpsInfoBuffer.size(); i++)
	{
			if (!m_gpsInfoBuffer.at(i).position.IsValid() && insideTracksegment)
		{
			gpxStream << "    </trkseg>\n";
			insideTracksegment = false;
			continue;
		}
		if (!m_gpsInfoBuffer.at(i).position.IsValid() && !insideTracksegment)
		{
			continue;
		}
		if (m_gpsInfoBuffer.at(i).position.IsValid() && !insideTracksegment)
		{
			gpxStream << "    <trkseg>\n";
			insideTracksegment = true;
		}
		if (m_gpsInfoBuffer.at(i).position.IsValid() && insideTracksegment)
		{
			QString lat = QString::number(m_gpsInfoBuffer.at(i).position.ToGPSCoordinate().latitude).prepend("      <trkpt lat=\"").append("\"");
			QString lon = QString::number(m_gpsInfoBuffer.at(i).position.ToGPSCoordinate().longitude).prepend(" lon=\"").append("\">\n");
			QString ele = QString::number(m_gpsInfoBuffer.at(i).altitude).prepend("        <ele>").append("</ele>\n");
			QString time = m_gpsInfoBuffer.at(i).timestamp.toString( "yyyy-MM-ddThh:mm:ss" ).prepend("        <time>").append("</time>\n");
			gpxStream << lat.toUtf8();
			gpxStream << lon.toUtf8();
			if (!ele.contains("nan"))
				gpxStream << ele.toUtf8();
			gpxStream << time.toUtf8();
			gpxStream << QString("      </trkpt>\n").toUtf8();
		}
	}
	if (insideTracksegment)
	{
		gpxStream << QString("    </trkseg>\n").toUtf8();
	}
	gpxStream << QString("  </trk>\n").toUtf8();
	gpxStream << QString("</gpx>\n").toUtf8();
	m_logFile.close();
	m_lastFlushTime = QDateTime::currentDateTime();
	return true;
}


bool Logger::readGpxLog()
{
	m_gpsInfoBuffer.clear();

	if ( !m_logFile.open( QIODevice::ReadOnly | QIODevice::Text ) )
	{
		return false;
	}

	QString lineBuffer;
	QString latString;
	QString lonString;
	QString eleString;
	QString timeString;
	QStringList tempList;
	bool insideTrackpoint = false;
	while ( !m_logFile.atEnd() )
	{
		lineBuffer = m_logFile.readLine();
		lineBuffer = lineBuffer.simplified();
		if (!insideTrackpoint)
		{
			latString = "";
			lonString = "";
			eleString = "";
			timeString = "";
		}
		if (lineBuffer.contains("<trkpt"))
		{
			insideTrackpoint = true;
			tempList = lineBuffer.split("\"");
			latString = tempList.at(1);
			lonString = tempList.at(3);
		}
		if (lineBuffer.contains("<ele>"))
		{
			lineBuffer = lineBuffer.remove("<ele>");
			lineBuffer = lineBuffer.remove("</ele>");
			eleString = lineBuffer;
		}
		if (lineBuffer.contains("<time>"))
		{
			lineBuffer = lineBuffer.remove("<time>");
			lineBuffer = lineBuffer.remove("</time>");
			timeString = lineBuffer;
		}
		if (lineBuffer.contains("</trkpt>"))
		{
			RoutingLogic::GPSInfo gpsInfo;
			gpsInfo.position = UnsignedCoordinate( GPSCoordinate() );
			gpsInfo.altitude = -1;
			gpsInfo.groundSpeed = -1;
			gpsInfo.verticalSpeed = -1;
			gpsInfo.heading = -1;
			gpsInfo.horizontalAccuracy = -1;
			gpsInfo.verticalAccuracy = -1;
			QDateTime invalidTime;
			gpsInfo.timestamp = invalidTime;
			gpsInfo.position = UnsignedCoordinate( GPSCoordinate( latString.toDouble(), lonString.toDouble() ) );
			gpsInfo.altitude = eleString.toDouble();
			gpsInfo.timestamp = QDateTime::fromString( timeString, "yyyy-MM-ddTHH:mm:ss" );
			m_gpsInfoBuffer.append(gpsInfo);
		}
		if (lineBuffer.contains("</trkseg>"))
		{
			RoutingLogic::GPSInfo gpsInfo;
			gpsInfo.position = UnsignedCoordinate( GPSCoordinate() );
			gpsInfo.altitude = -1;
			gpsInfo.groundSpeed = -1;
			gpsInfo.verticalSpeed = -1;
			gpsInfo.heading = -1;
			gpsInfo.horizontalAccuracy = -1;
			gpsInfo.verticalAccuracy = -1;
			QDateTime invalidTime;
			gpsInfo.timestamp = invalidTime;
			m_gpsInfoBuffer.append(gpsInfo);
		}
	}
	m_logFile.close();
	emit trackChanged();
	return true;
}


void Logger::clearTracklog()
{
	m_gpsInfoBuffer.clear();
	writeGpxLog();
	initialize();
	readGpxLog();
}


bool Logger::loggingEnabled()
{
	return m_loggingEnabled;
}


void Logger::setLoggingEnabled(bool enable)
{
	// Avoid a new logfile is created in case nothing changed.
	if (m_loggingEnabled == enable)
		return;
	QSettings settings( "MoNavClient" );
	settings.setValue("LoggingEnabled", enable);
	writeGpxLog();
	initialize();
	readGpxLog();
}


QString Logger::directory()
{
	return m_tracklogPath;
}


void Logger::setDirectory(QString directory)
{
	// Avoid a new logfile is created in case nothing changed.
	if (m_tracklogPath == directory)
		return;
	QSettings settings( "MoNavClient" );
	settings.setValue("LogFilePath", directory);
	writeGpxLog();
	initialize();
	readGpxLog();
}

