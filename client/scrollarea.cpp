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

#include "scrollarea.h"
#include <QApplication>
#include <QStyle>
#include <QResizeEvent>
#include <QMouseEvent>
#include <QtDebug>

ScrollArea::ScrollArea( QWidget* parent ) :
	QScrollArea( parent )
{
	m_orientation = Qt::Vertical;
}

Qt::Orientation ScrollArea::orientation()
{
	return m_orientation;
}

void ScrollArea::setOrientation( Qt::Orientation orientation )
{
	m_orientation = orientation;
}

void ScrollArea::mousePressEvent( QMouseEvent* event )
{
	event->accept();
}

void ScrollArea::mouseMoveEvent( QMouseEvent* event )
{
	event->accept();
}

void ScrollArea::resizeEvent( QResizeEvent* event )
{
	QScrollArea::resizeEvent( event );

	if ( widget() == NULL )
		return;

	int widgetWidth = widget()->sizeHint().width() + 2 * frameWidth();
	int widgetHeight = widget()->sizeHint().height() + 2 * frameWidth();

	if ( m_orientation == Qt::Vertical ) {
		if ( widgetHeight > height() ) {
			widgetWidth += QApplication::style()->pixelMetric( QStyle::PM_ScrollBarExtent );
			widgetWidth += QApplication::style()->pixelMetric( QStyle::PM_ScrollView_ScrollBarSpacing );
		}
		setFixedWidth( widgetWidth );
		setMaximumHeight( widgetHeight );
	} else {
		if ( widgetWidth > width() ) {
			widgetHeight += QApplication::style()->pixelMetric( QStyle::PM_ScrollBarExtent );
			widgetHeight += QApplication::style()->pixelMetric( QStyle::PM_ScrollView_ScrollBarSpacing );
		}
		setFixedHeight( widgetHeight );
		setMaximumWidth( widgetWidth );
	}
}
