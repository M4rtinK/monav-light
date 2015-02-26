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

#ifndef OVERLAYWIDGET_H
#define OVERLAYWIDGET_H

#include "scrollarea.h"
#include <QWidget>
#include <QGridLayout>
#include <QToolBar>

class OverlayWidget : public QWidget
{

	Q_OBJECT

public:

	explicit OverlayWidget( QWidget *parent = 0, QString title = "" );
	void addAction( QAction *action );
	void addActions( QList< QAction* > actions );
	QList< QAction* > actions() const;

signals:

public slots:


protected:

	virtual void mousePressEvent( QMouseEvent* event );
	virtual void mouseReleaseEvent( QMouseEvent* event );
	bool eventFilter( QObject *obj, QEvent* ev );
	virtual void hideEvent( QHideEvent* event );
	virtual void showEvent( QShowEvent* event );
	void setOrientation();

	QToolBar* m_centralWidget;
	ScrollArea* m_scrollArea;
	QGridLayout* m_grid;
	bool m_mouseDown;
};

#endif // OVERLAYWIDGET_H
