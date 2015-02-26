/****************************************************************************
**
** Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Mobility Components.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

/*
This has been hacked to provide working gps updates on android for use in
MoNav. GPS info is not exposed in the NDK, and can only be accessed by the
java loader. The java loader therefore registers for GPS, and calls the jni
function Java_com_nokia_qt_android_QtApplication_g_1position_1updated
each time it gets a gps update.

No other functions of QtLocation are supported! Updates occur constantly, not
just when StartUpdates is called.
*/

#include <QMetaType>
#include "qgeopositioninfosource_android.h"
#include "com_nokia_qt_QtApplication.h"

#ifdef ANDROID_LOG_GPS
#include <math.h>
#endif

QTM_BEGIN_NAMESPACE

// ========== QGeoPositionInfoSourceAndroid ==========
QList<QGeoPositionInfoSourceAndroid*> QGeoPositionInfoSourceAndroid::allSources;

QGeoPositionInfoSourceAndroid::QGeoPositionInfoSourceAndroid(QObject *parent)
        : QGeoPositionInfoSource(parent)
{
    allSources.push_back(this);
    qRegisterMetaType<QGeoPositionInfo>("QGeoPositionInfo");

    //connect(infoThread, SIGNAL(dataUpdated(GPS_POSITION)), this, SLOT(dataUpdated(GPS_POSITION)));
    //connect(infoThread, SIGNAL(updateTimeout()), this, SIGNAL(updateTimeout()));
}

QGeoPositionInfoSourceAndroid::~QGeoPositionInfoSourceAndroid()
{
    allSources.removeAll(this);
}

QGeoPositionInfoSource::PositioningMethods QGeoPositionInfoSourceAndroid::supportedPositioningMethods() const
{
    return QGeoPositionInfoSource::SatellitePositioningMethods;
}

QGeoPositionInfo QGeoPositionInfoSourceAndroid::lastKnownPosition(bool) const
{
    return lastPosition;
}

void QGeoPositionInfoSourceAndroid::setUpdateInterval(int msec)
{
    // If msec is 0 we send updates as data becomes available, otherwise we force msec to be equal
    // to or larger than the minimum update interval.
    if (msec != 0 && msec < MinimumUpdateInterval)
        msec = MinimumUpdateInterval;

//    infoThread->setUpdateInterval(msec);
    QGeoPositionInfoSource::setUpdateInterval(msec);
}

int QGeoPositionInfoSourceAndroid::minimumUpdateInterval() const
{
    return MinimumUpdateInterval;
}

void QGeoPositionInfoSourceAndroid::startUpdates()
{
//    infoThread->startUpdates();
}

void QGeoPositionInfoSourceAndroid::stopUpdates()
{
//    infoThread->stopUpdates();
}

void QGeoPositionInfoSourceAndroid::requestUpdate(int timeout)
{
    // A timeout of 0 means to use the default timeout, which is handled by the QGeoInfoThreadAndroid
    // instance, otherwise if timeout is less than the minimum update interval we emit a
    // updateTimeout signal
//    if (timeout < minimumUpdateInterval() && timeout != 0)
//        emit updateTimeout();
//    else
//        infoThread->requestUpdate(timeout);
}

void QGeoPositionInfoSourceAndroid::dataUpdated(const QGeoPositionInfo &data)
{
    if(!data.isValid()) return;
    lastPosition = data;
    emit positionUpdated(data);
}

#include "moc_qgeopositioninfosource_android.cpp"
QTM_END_NAMESPACE

JNIEXPORT void JNICALL Java_com_nokia_qt_android_QtApplication_g_1position_1updated
  (JNIEnv *self, jclass var2, jdouble lat, jdouble lon, jdouble altitude, jdouble bearing, jdouble speed)
{
    QtMobility::QGeoCoordinate coord(lat, lon, altitude);
    QtMobility::QGeoPositionInfo info(coord, QDateTime::currentDateTime());
    info.setAttribute(QtMobility::QGeoPositionInfo::Direction, (int) bearing);
    info.setAttribute(QtMobility::QGeoPositionInfo::GroundSpeed, (int) speed);
    for(QList<QtMobility::QGeoPositionInfoSourceAndroid*>::iterator i = QtMobility::QGeoPositionInfoSourceAndroid::allSources.begin();
        i!=QtMobility::QGeoPositionInfoSourceAndroid::allSources.end(); i++) {
        (*i)->dataUpdated(info);
    }

#ifdef ANDROID_LOG_GPS
    static FILE *fp=NULL;
    if(!fp) fp = fopen("/data/local/gps.log", "a");
    if(!fp) return;
    fprintf(fp, "$GPRMC,%s,A,%.3f,%s,%.3f,%s,%f,%f,%s,0,W\n",
        QDateTime::currentDateTime().toString("hhmmss").toAscii().constData(),
        fabs(trunc(lat)*100 + (lat-trunc(lat))*60), lat>0? "N" : "S",
        fabs(trunc(lon)*100 + (lon-trunc(lon))*60), lon>0? "E" : "W",
        (double) speed, (double) bearing,
        QDateTime::currentDateTime().toString("ddMMyy").toAscii().constData());
#endif
}
