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

#include "overlaywidget.h"
#include <QMouseEvent>
#include <QCursor>

OverlayWidget::OverlayWidget( QWidget *parent, QString title ) :
	QWidget( parent )
{
	m_mouseDown = false;
	m_grid = new QGridLayout( this );
	m_grid->setColumnStretch( 0, 0 );
	m_grid->setColumnStretch( 1, 0 );
	m_grid->setColumnStretch( 2, 0 );
	m_grid->setRowStretch( 0, 0 );
	m_grid->setRowStretch( 1, 0 );
	m_grid->setRowStretch( 2, 0 );

	m_scrollArea = new ScrollArea;
	m_scrollArea->setOrientation( Qt::Horizontal );

	m_centralWidget = new QToolBar( title );
	m_centralWidget->setAutoFillBackground( true );
	m_centralWidget->setToolButtonStyle( Qt::ToolButtonTextUnderIcon );
	m_centralWidget->setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed );
	m_scrollArea->setWidget( m_centralWidget );
	m_scrollArea->setWidgetResizable( true );

	m_grid->addWidget( m_scrollArea, 1, 1 );

	hide();

	QPalette pal = palette();
	pal.setColor( QPalette::Window, QColor( 0, 0, 0, 128 ) );
	setPalette( pal );

	setAutoFillBackground( true );

	connect( m_centralWidget, SIGNAL(actionTriggered(QAction*)), this, SLOT(hide()) );
}

void OverlayWidget::addAction( QAction *action )
{
	m_centralWidget->addAction( action );
}

void OverlayWidget::addActions( QList< QAction* > actions )
{
	m_centralWidget->addActions( actions );
}

QList< QAction* > OverlayWidget::actions() const
{
	return m_centralWidget->actions();
}

void OverlayWidget::hideEvent( QHideEvent* event )
{
	if ( event->spontaneous() || parentWidget() == NULL )
		return;
	parentWidget()->removeEventFilter( this );
}

void OverlayWidget::showEvent( QShowEvent* event )
{
	if ( event->spontaneous() || parentWidget() == NULL )
		return;
	raise();
	setFixedSize( parentWidget()->size() );
	setOrientation();
	parentWidget()->installEventFilter( this );
	QCursor::setPos( mapToGlobal( m_scrollArea->pos() ) );
}

bool OverlayWidget::eventFilter( QObject* obj, QEvent* ev )
{
	if ( obj != parent() )
		 return QWidget::eventFilter( obj, ev );

	QWidget *parent = parentWidget();

	if ( ev->type() == QEvent::Resize ) {
		setFixedSize( parent->size() );
		setOrientation();
	}

	return QWidget::eventFilter( obj, ev );
}

void OverlayWidget::mousePressEvent( QMouseEvent* event )
{
	event->accept();
	if ( event->button() == Qt::LeftButton )
		m_mouseDown = true;
}

void OverlayWidget::mouseReleaseEvent( QMouseEvent* event )
{
	event->accept();
	if ( !m_mouseDown )
		return;

	if ( event->button() == Qt::LeftButton ) {
		m_mouseDown = false;
		hide();
	}
}

void OverlayWidget::setOrientation()
{
	if ( width() > height() ) {
		m_centralWidget->setOrientation( Qt::Horizontal );
		m_scrollArea->setOrientation( Qt::Horizontal );
	} else {
		m_centralWidget->setOrientation( Qt::Vertical );
		m_scrollArea->setOrientation( Qt::Vertical );
	}
	m_centralWidget->setFixedSize( m_centralWidget->sizeHint() );
}
