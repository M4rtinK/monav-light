/****************************************************************************
** GpsdPositionInfoSource
** (c) 2011 by Till Harbaum <till@harbaum.org>
** This code is public domain, do what you want with it
****************************************************************************/

#ifndef GPSDPOSITIONINFOSOURCE_H
#define GPSDPOSITIONINFOSOURCE_H

#include <QObject>
#include <QTcpSocket>
#ifndef NOQTMOBILE
#include <QGeoPositionInfoSource>

QTM_USE_NAMESPACE

class GpsdPositionInfoSource : public QGeoPositionInfoSource {
    Q_OBJECT

 public:
	static GpsdPositionInfoSource* create(QObject* parent = NULL );
  ~GpsdPositionInfoSource();

  void setUpdateInterval(int msec);
  
  QGeoPositionInfo lastKnownPosition(bool fromSatellitePositioningMethodsOnly = false) const;
  PositioningMethods supportedPositioningMethods() const;
  int minimumUpdateInterval() const;
  
 public slots:
  void startUpdates();
  void stopUpdates();
  void requestUpdate(int timeout = 0);

 private slots:
  void readData(); 
  void displayError(QAbstractSocket::SocketError);

 private:
  GpsdPositionInfoSource(QObject *parent = 0);

  qreal getReal(const QMap<QString, QVariant> &, const QString &);
  void setAttribute(QGeoPositionInfo &, QGeoPositionInfo::Attribute, const QMap<QString, QVariant> &, const QString &);
  void parse(const QString &);
  QTcpSocket *m_tcpSocket;
  QGeoPositionInfo m_lastKnown;
};
#endif
#endif
