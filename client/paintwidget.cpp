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

#include "paintwidget.h"
#include "ui_paintwidget.h"
#include "utils/qthelpers.h"
#include "mapdata.h"
#include "routinglogic.h"
#include "globalsettings.h"

#include <QPainter>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QtDebug>
#include <algorithm>

#ifdef Q_WS_MAEMO_5
#include <QtDBus/QtDBus>
#include "mce/dbus-names.h"
#endif

PaintWidget::PaintWidget(QWidget *parent) :
	QWidget( parent ),
	m_ui( new Ui::PaintWidget )
{
	m_ui->setupUi( this );
	if ( MapData::instance()->loaded() ) {
		setAttribute( Qt::WA_OpaquePaintEvent, true );
		setAttribute( Qt::WA_NoSystemBackground, true );
	}
	m_lastMouseX = 0;
	m_lastMouseY = 0;
	m_wheelDelta = 0;
	m_fixed = false;
	setKeepPositionVisible( false );
	m_mouseDown = false;
	m_drag = false;
	m_request.zoom = 0;
	m_request.center = RoutingLogic::instance()->source().ToProjectedCoordinate();

	dataLoaded();
	sourceChanged();
	waypointsChanged();
	trackChanged();
	routeChanged();

	connect( MapData::instance(), SIGNAL(dataLoaded()), this, SLOT(dataLoaded()) );
	connect( RoutingLogic::instance(), SIGNAL(sourceChanged()), this, SLOT(sourceChanged()) );
	connect( RoutingLogic::instance(), SIGNAL(routeChanged()), this, SLOT(routeChanged()) );
	connect( RoutingLogic::instance(), SIGNAL(waypointsChanged()), this, SLOT(waypointsChanged()) );
	connect( Logger::instance(), SIGNAL(trackChanged()), this, SLOT(trackChanged()) );
}

PaintWidget::~PaintWidget()
{
	delete m_ui;
}

void PaintWidget::dataLoaded()
{
	IRenderer* renderer = MapData::instance()->renderer();
	if ( renderer == NULL )
		return;

	setAttribute( Qt::WA_OpaquePaintEvent, true );
	setAttribute( Qt::WA_NoSystemBackground, true );
	renderer->SetUpdateSlot( this, SLOT(update()) );
	update();
}

void PaintWidget::setFixed( bool fixed )
{
	m_fixed = fixed;
	if (m_fixed)
		setKeepPositionVisible( true );
	update();
}

void PaintWidget::setKeepPositionVisible( bool visibility )
{
	m_keepPositionVisible = visibility;
}

void PaintWidget::setCenter( const ProjectedCoordinate c )
{
	m_request.center = c;
	update();
}

void PaintWidget::setZoom( int z )
{
	m_request.zoom = z;
	update();
}

void PaintWidget::setMaxZoom( int z )
{
	m_maxZoom = z;
}

void PaintWidget::sourceChanged()
{
	m_request.position = RoutingLogic::instance()->source();
	m_request.heading = RoutingLogic::instance()->gpsInfo().heading;
	update();
	#ifdef Q_WS_MAEMO_5
	QDBusConnection::systemBus().call(QDBusMessage::createMethodCall(MCE_SERVICE,  MCE_REQUEST_PATH,MCE_REQUEST_IF, MCE_PREVENT_BLANK_REQ));
	#endif
}

void PaintWidget::waypointsChanged()
{
	m_request.target = RoutingLogic::instance()->target();
	QVector< UnsignedCoordinate > waypoints = RoutingLogic::instance()->waypoints();
	if ( waypoints.size() > 0 && RoutingLogic::instance()->target() == waypoints.back() )
		waypoints.pop_back();
	m_request.waypoints = waypoints;
	update();
}

void PaintWidget::setPOIs( QVector< UnsignedCoordinate > p )
{
	m_request.POIs = p;
	update();
}

void PaintWidget::setPOI( UnsignedCoordinate p )
{
	m_request.POIs = QVector< UnsignedCoordinate >( 1, p );
	update();
}

void PaintWidget::routeChanged()
{
	m_request.route = RoutingLogic::instance()->route();
	update();
}

void PaintWidget::trackChanged()
{
	setTracklogPolygons(Logger::instance()->polygonEndpointsTracklog(), Logger::instance()->polygonCoordsTracklog());
}

void PaintWidget::setStreetPolygons( QVector< int > polygonEndpointsStreet, QVector< UnsignedCoordinate > polygonCoordsStreet )
{
	m_request.polygonEndpointsStreet = polygonEndpointsStreet;
	m_request.polygonCoordsStreet = polygonCoordsStreet;
	if ( isVisible() )
		update();
}

void PaintWidget::setTracklogPolygons( QVector< int > polygonEndpointsTracklog, QVector< UnsignedCoordinate > polygonCoordsTracklog )
{
	m_request.polygonEndpointsTracklog = polygonEndpointsTracklog;
	m_request.polygonCoordsTracklog = polygonCoordsTracklog;
	if ( isVisible() )
		update();
}

void PaintWidget::setVirtualZoom( int z )
{
	m_request.virtualZoom = z;
	if ( isVisible() )
		update();
}

void PaintWidget::mousePressEvent( QMouseEvent* event )
{
	event->accept();
	if ( m_fixed )
		return;
	if ( event->button() != Qt::LeftButton )
		return;
	m_startMouseX = m_lastMouseX = event->x();
	m_startMouseY = m_lastMouseY = event->y();
	m_mouseDown = true;
	m_drag = false;
}

void PaintWidget::mouseMoveEvent( QMouseEvent* event )
{
	event->accept();
	if ( m_fixed || !m_mouseDown )
		return;
	if ( ( event->buttons() & Qt::LeftButton ) == 0 )
		return;
	int minDiff = 7;
#ifdef Q_WS_MAEMO_5
	minDiff = 15;
#endif
	if ( abs( event->x() - m_startMouseX ) + abs( event->y() - m_startMouseY ) > minDiff )
		m_drag = true;
	if ( !m_drag )
		return;

	IRenderer* renderer = MapData::instance()->renderer();
	if ( renderer == NULL )
		return;

	m_request.center = renderer->Move( event->x() - m_lastMouseX, event->y() - m_lastMouseY, m_request );
	m_lastMouseX = event->x();
	m_lastMouseY = event->y();

	setKeepPositionVisible( false );

	update();
}

void PaintWidget::mouseReleaseEvent( QMouseEvent* event )
{
	event->accept();
	m_mouseDown = false;
	if ( m_fixed )
		return;
	if ( m_drag )
		return;
	if ( event->button() != Qt::LeftButton )
		return;

	IRenderer* renderer = MapData::instance()->renderer();
	if ( renderer == NULL )
		return;

	emit mouseClicked( renderer->PointToCoordinate( event->x() - width() / 2, event->y() - height() / 2, m_request ) );
}

void PaintWidget::wheelEvent( QWheelEvent * event )
{
	IRenderer* renderer = MapData::instance()->renderer();
	if ( renderer == NULL )
		return;

	// 15 degrees is a full mousewheel "click"
	int numDegrees = event->delta() / 8 + m_wheelDelta;
	int numSteps = numDegrees / 15;
	m_wheelDelta = numDegrees % 15;

	// limit zoom
	int newZoom = m_request.zoom + numSteps;
	if ( newZoom < 0 )
		newZoom = 0;
	if ( newZoom > m_maxZoom )
		newZoom = m_maxZoom;

	// avoid looping event calls
	if ( newZoom == m_request.zoom )
		return;

	// zoom in/out on current mouse position
	m_request.center = renderer->Move( width() / 2 - event->x(), height() / 2 - event->y(), m_request );
	m_request.zoom = newZoom;
	m_request.center = renderer->Move( event->x() - width() / 2, event->y() - height() / 2, m_request );

	emit zoomChanged( newZoom );
	update();
	event->accept();
}

void PaintWidget::paintEvent( QPaintEvent* )
{
	if ( !isVisible() )
		return;

	IRenderer* renderer = MapData::instance()->renderer();
	if ( renderer == NULL )
		return;

	if ( m_fixed ){
		m_request.center = m_request.position.ToProjectedCoordinate();

		if ( GlobalSettings::autoRotation() )
		{
			//gradually change the screen rotation to match the heading
			double diff = m_request.rotation + m_request.heading;
			while ( diff <= -180 )
				diff += 360;
			while ( diff >= 180 )
				diff -=360;
			//to filter out noise stop when close enough
			if ( diff > 0 )
				diff = std::max( 0.0, diff - 15 );
			if ( diff < 0 )
				diff = std::min( 0.0, diff + 15 );
			m_request.rotation -= diff / 2;

			//normalize
			while ( m_request.rotation < 0 )
				m_request.rotation += 360;
			while ( m_request.rotation >= 360 )
				m_request.rotation -= 360;

			int radius = height() * 0.25;
			m_request.center = renderer->PointToCoordinate( 0, -radius, m_request );
		} else {
			m_request.rotation = 0;
		}
	} else {
		m_request.rotation = 0;
	}

	if (m_keepPositionVisible)
		m_request.center = m_request.position.ToProjectedCoordinate();

	QPainter painter( this );
	Timer time;
	renderer->Paint( &painter, m_request );
	qDebug() << "Rendering:" << time.elapsed() << "ms";
}

void PaintWidget::contextMenuEvent(QContextMenuEvent *event )
{
	emit contextMenu( event->globalPos() );
}
