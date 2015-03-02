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

#ifndef PAINTWIDGET_H
#define PAINTWIDGET_H

#ifndef SAILFISH
#include <QWidget>
#else
#include <QQuickPaintedItem>
#endif
#include "interfaces/irenderer.h"
#include "interfaces/irouter.h"
#include "logger.h"

#ifndef SAILFISH
namespace Ui {
	class PaintWidget;
}

class PaintWidget : public QWidget {
#else

class PaintWidget : public QQuickPaintedItem {
#endif
	Q_OBJECT
	Q_PROPERTY(qint32 startMouseX MEMBER m_startMouseX)
	Q_PROPERTY(qint32 startMouseY MEMBER m_startMouseY)
public:
#ifndef SAILFISH
	PaintWidget(QWidget *parent = 0);
#else
	PaintWidget();
#endif
	~PaintWidget();
	void paint(QPainter *painter);

public slots:

	void setFixed( bool f );
	void setKeepPositionVisible( bool visibility );
	void setZoom( int z );
	void setMaxZoom( int z );
	void setCenter( const ProjectedCoordinate c );
	void setPOIs( QVector< UnsignedCoordinate > p );
	void setPOI( UnsignedCoordinate p );
	void setStreetPolygons( QVector< int > polygonEndpointsStreet, QVector< UnsignedCoordinate > polygonCoordsStreet );
	void setTracklogPolygons( QVector< int > polygonEndpointsTracklog, QVector< UnsignedCoordinate > polygonCoordsTracklog );
	void setVirtualZoom( int z );
	void setSource(qint16 x, quint16 y);
	void setDestination(qint16 x, quint16 y);
#ifdef SAILFISH
	void update();
#endif

	void routeChanged();
	void trackChanged();
	void waypointsChanged();
	void sourceChanged();
	void dataLoaded();

signals:

	void zoomChanged( int z );
	void mouseClicked( ProjectedCoordinate clickPos );
	void contextMenu( QPoint globalPos );

protected:
	void paintEvent( QPaintEvent* );
	void mouseMoveEvent( QMouseEvent * event );
	void mousePressEvent( QMouseEvent * event );
	void mouseReleaseEvent( QMouseEvent* event );
	QPointF calcAverage(QMap<int, QPointF> &points);
	Q_INVOKABLE void press(QVariantList list, QVariantList idlist);
	Q_INVOKABLE void move(QVariantList list, QVariantList idlist);
	Q_INVOKABLE void release(QVariantList list, QVariantList idlist);
	void wheelEvent( QWheelEvent * event );
	void contextMenuEvent( QContextMenuEvent *event) ;

	IRenderer::PaintRequest m_request;

	int m_maxZoom;
	int m_lastMouseX;
	int m_lastMouseY;
	int m_startMouseX;
	int m_startMouseY;
	bool m_drag;
	bool m_mouseDown;
	int m_wheelDelta;
	bool m_fixed;
	bool m_keepPositionVisible;
	qreal m_startZoom;

#ifndef SAILFISH
	Ui::PaintWidget* m_ui;
#endif
	QMap<int, QPointF> touch_start_pos;
	QMap<int, QPointF> touch_current_pos;
};

#endif // PAINTWIDGET_H
