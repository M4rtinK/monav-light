/****************************************************************************
** GpsdPositionInfoSource
** (c) 2011 by Till Harbaum <till@harbaum.org>
** This code is public domain, do what you want with it
****************************************************************************/

// Based infos from:
// http://gpsd.berlios.de/client-howto.html
// http://doc.qt.nokia.com/qtmobility-1.0-tp/qgeopositioninfosource.html

#include <QDebug>
#include <QByteArray>
#include <QStringList>
#include <math.h>

#include "gpsdpositioninfosource.h"
#include "json.h"

#ifndef NAN
#define NAN (0.0/0.0)
#endif
#ifndef NOQTMOBILE
qreal GpsdPositionInfoSource::getReal(const QMap<QString, QVariant> &map, const QString &name) {
  // make sure that key exists
  if(!map.contains(name)) return NAN;

  QVariant variant = map.value(name);

  // make sure value is variant of correkt type
  if(QVariant::String != variant.type()) 
    return NAN;

  // extract string and convert to float
  return variant.toString().toFloat();
}

void GpsdPositionInfoSource::setAttribute(QGeoPositionInfo &info, 
  QGeoPositionInfo::Attribute attr, const QMap<QString, QVariant> &map, const QString &name) {

  qreal value = getReal(map, name);
  if(!isnan(value)) info.setAttribute(attr, value);
}

void GpsdPositionInfoSource::parse(const QString &str) {
  bool ok;

  // feed reply into json parser
  
  // json is a QString containing the data to convert
  QVariant result = Json::parse(str, ok);
  if(!ok) {
	 qDebug() << "GpsdPositionInfoSource::parse:" << "Json deconding failed.";
    return;
  }
  
  // we expect a qvariantmap
  if(QVariant::Map != result.type()) {
	 qDebug() << "GpsdPositionInfoSource::parse:" << "Unexpected result type:" << result.type();
    return;
  } 
  
  QMap<QString, QVariant> map = result.toMap();
  
  // extract reply class
  QString classStr = map.value("class").toString();
  
  if(!classStr.compare("VERSION")) {
    qDebug() << "Connected to GPSD:";
    qDebug() << "Release:" << map.value("release").toString();
    qDebug() << "Revision:" << map.value("rev").toString();
    qDebug() << "Protocol version:" << 
      map.value("proto_major").toString() + "." + 
      map.value("proto_minor").toString();

  } else if(!classStr.compare("TPV")) {
    // TPV is the most interesting string for us
    m_lastKnown = QGeoPositionInfo();

    int mode = map.value("mode").toInt();
    if(mode > 0) {
      QGeoCoordinate coo(getReal(map, "lat"), getReal(map, "lon"));
      if(mode == 3)
	coo.setAltitude(getReal(map, "alt"));

      m_lastKnown.setCoordinate(coo);
    }

    setAttribute(m_lastKnown, QGeoPositionInfo::Direction, map, "track");
    setAttribute(m_lastKnown, QGeoPositionInfo::VerticalSpeed, map, "climb");
    setAttribute(m_lastKnown, QGeoPositionInfo::GroundSpeed, map, "speed");
    setAttribute(m_lastKnown, QGeoPositionInfo::VerticalAccuracy, map, "epv");

    // horizontal error in lat or long
    qreal epx = getReal(map, "epx");
    qreal epy = getReal(map, "epy");

    if(!isnan(epx) && !isnan(epy))
      m_lastKnown.setAttribute(QGeoPositionInfo::HorizontalAccuracy, epx>epy?epx:epy);

    QDateTime time;
    time.setTime_t(getReal(map, "time"));
    m_lastKnown.setTimestamp(time);

    emit positionUpdated( m_lastKnown );
  }
}

void GpsdPositionInfoSource::readData() {

  // split reply into seperate strings at newline
  QStringList data = QString::fromUtf8(m_tcpSocket->readAll()).split('\n');

  for(int i=0;i<data.size();i++)
    if(!data[i].trimmed().isEmpty())
      parse(data[i].trimmed());
}

void GpsdPositionInfoSource::displayError(QAbstractSocket::SocketError) {
  qDebug() << "GpsdPositionInfoSource::displayError:" << "Error: " << m_tcpSocket->errorString();
}

GpsdPositionInfoSource::GpsdPositionInfoSource(QObject *parent)
  : QGeoPositionInfoSource(parent) {
  qDebug() << "GpsdPositionInfoSource";

  // connect to gpsd
  m_tcpSocket = new QTcpSocket(this);

  connect(m_tcpSocket, SIGNAL(readyRead()), this, SLOT(readData()));
  connect(m_tcpSocket, SIGNAL(error(QAbstractSocket::SocketError)),
	  this, SLOT(displayError(QAbstractSocket::SocketError)));

  m_tcpSocket->connectToHost("localhost", 2947);
}

GpsdPositionInfoSource::~GpsdPositionInfoSource() {
  qDebug() << "GpsdPositionInfoSource::~GpsdPositionInfoSource";

  m_tcpSocket->close();
}

void GpsdPositionInfoSource::setUpdateInterval(int msec) {
  int interval = msec;
  if (interval != 0)
    interval = qMax(msec, minimumUpdateInterval());
  QGeoPositionInfoSource::setUpdateInterval(interval);
}

void GpsdPositionInfoSource::startUpdates() {
  // request info from gpsd
  QString request = "?WATCH={\"enable\":true,\"json\":true}";
  m_tcpSocket->write(request.toUtf8());
}

void GpsdPositionInfoSource::stopUpdates() {
  // ask gpsd to stop sending data
  QString request = "?WATCH={\"enable\":false}";
  m_tcpSocket->write(request.toUtf8());
}

void GpsdPositionInfoSource::requestUpdate(int) {
  emit positionUpdated( m_lastKnown );
}

QGeoPositionInfo GpsdPositionInfoSource::lastKnownPosition(bool) const {
  // the bool value does not matter since we only use satellite positioning
  return m_lastKnown;
}

QGeoPositionInfoSource::PositioningMethods GpsdPositionInfoSource::supportedPositioningMethods() const {
  return SatellitePositioningMethods;
}

int GpsdPositionInfoSource::minimumUpdateInterval() const {
	return 1000;
}

GpsdPositionInfoSource * GpsdPositionInfoSource::create(QObject *parent)
{
	GpsdPositionInfoSource* source = new GpsdPositionInfoSource( parent );
	if ( !source->m_tcpSocket->waitForConnected( 100 ) )
	{
		qDebug() << "Could not connect to gpsd after 100ms";
		source->deleteLater();
		return NULL;
	}
	return source;
}

QGeoPositionInfoSource::Error GpsdPositionInfoSource::error() const {
	return QGeoPositionInfoSource::UnknownSourceError;
}
#endif
