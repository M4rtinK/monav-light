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

#include "placechooser.h"
#include "ui_placechooser.h"
#include "interfaces/irenderer.h"
#include "mapdata.h"
#include "globalsettings.h"

struct PlaceChooser::PrivateImplementation {
	QVector< UnsignedCoordinate > places;
	int place;
	UnsignedCoordinate selected;
};

PlaceChooser::PlaceChooser( QWidget* parent ) :
		QDialog( parent ),
		m_ui( new Ui::PlaceChooser )
{
	m_ui->setupUi( this );
	d = new PrivateImplementation;

	m_ui->zoomBar->hide();

	connect( m_ui->zoomBar, SIGNAL(valueChanged(int)), this, SLOT(setZoom(int)) );
	connect( m_ui->paintArea, SIGNAL(zoomChanged(int)), this, SLOT(setZoom(int)) );
	connect( m_ui->previousButton, SIGNAL(clicked()), this, SLOT(previousPlace()) );
	connect( m_ui->nextButton, SIGNAL(clicked()), this, SLOT(nextPlace()) );
	connect( m_ui->headerLabel, SIGNAL(clicked()), this, SLOT(accept()) );
	connect( m_ui->zoomIn, SIGNAL(clicked()), this, SLOT(addZoom()) );
	connect( m_ui->zoomOut, SIGNAL(clicked()), this, SLOT(subtractZoom()) );

	int iconSize = GlobalSettings::iconSize();
	QSize size( iconSize, iconSize );
	m_ui->previousButton->setIconSize( size );
	m_ui->nextButton->setIconSize( size );
	m_ui->zoomIn->setIconSize( size );
	m_ui->zoomOut->setIconSize( size );

	IRenderer* renderer = MapData::instance()->renderer();
	if ( renderer == NULL )
		return;

	int maxZoom = renderer->GetMaxZoom();
	m_ui->zoomBar->setMaximum( maxZoom );
	m_ui->paintArea->setMaxZoom( maxZoom );
	setZoom( GlobalSettings::zoomPlaceChooser());
	m_ui->paintArea->setVirtualZoom( GlobalSettings::magnification() );
}

PlaceChooser::~PlaceChooser()
{
	delete d;
	delete m_ui;
}

void PlaceChooser::addZoom()
{
	setZoom( GlobalSettings::zoomPlaceChooser() + 1 );
}

void PlaceChooser::subtractZoom()
{
	setZoom( GlobalSettings::zoomPlaceChooser() - 1 );
}

void PlaceChooser::setZoom( int zoom )
{
	IRenderer* renderer = MapData::instance()->renderer();
	if ( renderer == NULL )
		return;
	if( zoom > renderer->GetMaxZoom() )
		zoom = renderer->GetMaxZoom();
	if( zoom < 0 )
		zoom = 0;

	m_ui->zoomBar->setValue( zoom );
	m_ui->paintArea->setZoom( zoom );
	GlobalSettings::setZoomPlaceChooser( zoom );
}

void PlaceChooser::nextPlace()
{
	d->place = ( d->place + 1 ) % d->places.size();
	m_ui->headerLabel->setText( QString( tr( "Choose City (%1/%2)" ) ).arg( d->place + 1 ).arg( d->places.size() ) );
	m_ui->paintArea->setCenter( d->places[d->place].ToProjectedCoordinate() );
}

void PlaceChooser::previousPlace()
{
	d->place = ( d->place + d->places.size() - 1 ) % d->places.size();
	m_ui->headerLabel->setText( QString( tr( "Choose City (%1/%2)" ) ).arg( d->place + 1 ).arg( d->places.size() ) );
	m_ui->paintArea->setCenter( d->places[d->place].ToProjectedCoordinate() );
}

int PlaceChooser::selectPlaces( QVector< UnsignedCoordinate > places, QWidget* p )
{
	if ( places.size() == 0 )
		return -1;

	PlaceChooser* window = new PlaceChooser( p );
	window->d->places = places;
	window->d->place = 0;

	window->m_ui->headerLabel->setText( QString( tr( "Choose City (%1/%2)" ) ).arg( 1 ).arg( places.size() ) );
	window->m_ui->paintArea->setCenter( places.first().ToProjectedCoordinate() );
	window->m_ui->paintArea->setPOIs( places );

	int value = window->exec();
	int id = -1;
	if ( value == Accepted )
		id = window->d->place;
	delete window;

	return id;
}
