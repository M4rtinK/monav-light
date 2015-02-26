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

#ifndef WORLDMAPCHOOSER_H
#define WORLDMAPCHOOSER_H

#include <QWidget>
#include "mapdata.h"

class WorldMapChooser : public QWidget
{
	Q_OBJECT

public:

	explicit WorldMapChooser( QWidget* parent = 0 );
	~WorldMapChooser();

	void setMaps( QVector< MapData::MapPackage > maps );

public slots:

	void setHighlight( int id );

signals:

	void clicked( int id );

protected:

	void resizeEvent( QResizeEvent* event );
	void showEvent( QShowEvent* );
	void hideEvent( QHideEvent* );
	void mouseReleaseEvent( QMouseEvent* event );
	void paintEvent( QPaintEvent* );

private:

	struct PrivateImplementation;
	PrivateImplementation*d ;
};

#endif // WORLDMAPCHOOSER_H
