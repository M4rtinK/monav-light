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

#include "worldmapchooser.h"

#include <algorithm>
#include <QPixmap>
#include <QPainter>
#include <QResizeEvent>
#include <QDebug>
#include <QMouseEvent>
#include <QDir>
#include <QSvgRenderer>

struct WorldMapChooser::PrivateImplementation {
	QVector< MapData::MapPackage > maps;
	double minX;
	double minY;
	double maxX;
	double maxY;
	QRectF viewPort;
	int highlight;
	QVector< QRect > rects;

	QString background;
	QPixmap image;
	QSvgRenderer svg;
};

WorldMapChooser::WorldMapChooser( QWidget* parent ) :
		QWidget( parent )
{
	d = new PrivateImplementation;
	d->minX = 0;
	d->minY = 0;
	d->maxX = 1;
	d->maxY = 1;

	d->svg.load( QString( ":/images/world/world-optimized.svgz" ) );
}

WorldMapChooser::~WorldMapChooser()
{
	delete d;
}

void WorldMapChooser::showEvent( QShowEvent* )
{
	QResizeEvent event( size(), size() );
}

void WorldMapChooser::hideEvent( QHideEvent* )
{
	// save memory
	d->image = QPixmap();
}

void WorldMapChooser::setHighlight( int id )
{
	d->highlight = id;
	update();
}

void WorldMapChooser::mouseReleaseEvent( QMouseEvent* event )
{
	if ( event->button() != Qt::LeftButton ) {
		event->ignore();
		return;
	}

	QVector< int > result;
	for ( int i = 0; i < d->rects.size(); i++ ) {
		if ( d->rects[i].contains( event->pos() ) ) {
			result.push_back( i );
		}
	}
	if ( result.size() > 0 ) {
		int next = 0;
		if ( result.contains( d->highlight ) ) {
			next = result.indexOf( d->highlight );
			next = ( next + 1 ) % result.size();
		}
		d->highlight = result[next];
		emit clicked( d->highlight );
		update();
	}

	event->accept();
}

void WorldMapChooser::setMaps( QVector<MapData::MapPackage> maps )
{
	d->maps = maps;
	if ( maps.size() == 0 ) {
		d->minX = 0;
		d->minY = 0;
		d->maxX = 1;
		d->maxY = 1;
	} else {
		d->minX = std::numeric_limits< double >::max();
		d->minY = std::numeric_limits< double >::max();
		d->maxX = -std::numeric_limits< double >::max();
		d->maxY = -std::numeric_limits< double >::max();

		for ( int i = 0; i < maps.size(); i++ ) {
			GPSCoordinate min = maps[i].min.ToGPSCoordinate();
			GPSCoordinate max = maps[i].max.ToGPSCoordinate();
			d->minX = std::min( min.longitude / 360.0 + 0.5, d->minX );
			d->maxX = std::max( max.longitude / 360.0 + 0.5, d->maxX );
			d->minY = std::min( -min.latitude / 180.0 + 0.5, d->minY );
			d->maxY = std::max( -max.latitude / 180.0 + 0.5, d->maxY );
		}

		double rangeX = d->maxX - d->minX;
		double rangeY = d->maxY - d->minY;
		if ( rangeX == 0 )
			rangeX = 1;
		if ( rangeY == 0 )
			rangeY = 1;

		d->minX = std::max( d->minX - rangeX * 0.1, 0.0 );
		d->minY = std::max( d->minY - rangeY * 0.1, 0.0 );
		d->maxX = std::min( d->maxX + rangeX * 0.1, 1.0 );
		d->maxY = std::min( d->maxY + rangeY * 0.1, 1.0 );
	}

	QResizeEvent event( size(), size() );
	resizeEvent( &event );
}

void WorldMapChooser::resizeEvent( QResizeEvent* event )
{
	int width = event->size().width();
	int height = event->size().height();

	d->viewPort.setLeft( d->minX );
	d->viewPort.setTop( d->minY );
	d->viewPort.setRight( d->maxX );
	d->viewPort.setBottom( d->maxY );

	if ( d->viewPort.width() / d->viewPort.height() > ( double ) width / height ) {
		double move = d->viewPort.width() / width * height - d->viewPort.height();
		d->viewPort.setTop( d->viewPort.top() - move / 2 );
		d->viewPort.setBottom( d->viewPort.bottom() + move / 2 );
	} else if ( d->viewPort.width() / d->viewPort.height() < ( double ) width / height ) {
		double move = d->viewPort.height() * width / height - d->viewPort.width();
		d->viewPort.setLeft( d->viewPort.left() - move / 2 );
		d->viewPort.setRight( d->viewPort.right() + move / 2 );
	}

	d->rects.clear();

	d->image = QPixmap( width, height );
	if ( !d->image.isNull() && d->image.width() != 0 && d->image.height() != 0 )
	{
		d->image.fill( Qt::transparent );
		QPainter painter( &d->image );
		QRectF svgSize = d->svg.viewBoxF();
		QRectF viewPort = QRectF( d->viewPort.left() * svgSize.width() + svgSize.left(), d->viewPort.top() * svgSize.height() + svgSize.top(), d->viewPort.width() * svgSize.width(), d->viewPort.height() * svgSize.height() );
		d->svg.setViewBox( viewPort );
		d->svg.render( &painter );
		d->svg.setViewBox( svgSize );
		for ( int i = 0; i < d->maps.size(); i++ ) {
			GPSCoordinate min = d->maps[i].min.ToGPSCoordinate();
			GPSCoordinate max = d->maps[i].max.ToGPSCoordinate();
			int left = ( min.longitude / 360.0 + 0.5 - d->viewPort.left() ) * width / d->viewPort.width();
			int top = ( -min.latitude / 180.0 + 0.5 - d->viewPort.top() ) * height / d->viewPort.height();
			int right = ( max.longitude / 360.0 + 0.5 - d->viewPort.left() ) * width / d->viewPort.width();
			int bottom = ( -max.latitude / 180.0 + 0.5 - d->viewPort.top() ) * height / d->viewPort.height();
			QRect rect( left, top, right - left, bottom - top );
			d->rects.push_back( rect );
		}
	}
}

void WorldMapChooser::paintEvent( QPaintEvent* /*event*/ )
{
	QPainter painter( this );
	if ( !d->image.isNull() )
		painter.drawPixmap( 0, 0, d->image );
	for ( int i = 0; i < d->rects.size(); i++ )
	{
		if ( d->highlight == i )
			painter.setBrush( QColor( 128, 128, 128, 128 ) );
		else
			painter.setBrush( Qt::NoBrush );
		painter.drawRect( d->rects[i] );
	}
}
