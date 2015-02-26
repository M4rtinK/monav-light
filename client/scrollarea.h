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

#ifndef SCROLLAREA_H
#define SCROLLAREA_H

#include <QScrollArea>

// resizes the ScrollArea to always fit the contents in one direction and scroll in the other direction
// default orientation is Vertical, i.e., no horizontal scrollbar should appear
class ScrollArea : public QScrollArea
{

	Q_OBJECT

public:

	explicit ScrollArea( QWidget* parent = 0 );
	Qt::Orientation orientation();

signals:

public slots:

	void setOrientation( Qt::Orientation orientation );

protected:

	virtual void resizeEvent( QResizeEvent* event );
	virtual void mousePressEvent( QMouseEvent* event );
	virtual void mouseMoveEvent( QMouseEvent* event );

	Qt::Orientation m_orientation;

};

#endif // SCROLLAREA_H
