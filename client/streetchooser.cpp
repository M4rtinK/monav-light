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

#include "streetchooser.h"
#include "ui_streetchooser.h"

#include "interfaces/irenderer.h"
#include "mapdata.h"
#include "globalsettings.h"

struct StreetChooser::PrivateImplementation {
	UnsignedCoordinate coordinate;
};

StreetChooser::StreetChooser( QWidget* parent ) :
		QDialog( parent ),
		m_ui( new Ui::StreetChooser )
{
	m_ui->setupUi( this );
	d = new PrivateImplementation;

	m_ui->zoomBar->hide();

	connect( m_ui->zoomBar, SIGNAL(valueChanged(int)), this, SLOT(setZoom(int)) );
	connect( m_ui->paintArea, SIGNAL(zoomChanged(int)), this, SLOT(setZoom(int)) );
	connect( m_ui->headerLabel, SIGNAL(clicked()), this, SLOT(accept()) );
	connect( m_ui->zoomIn, SIGNAL(clicked()), this, SLOT(addZoom()) );
	connect( m_ui->zoomOut, SIGNAL(clicked()), this, SLOT(subtractZoom()) );
	connect( m_ui->paintArea, SIGNAL(mouseClicked(ProjectedCoordinate)), this, SLOT(mouseClicked(ProjectedCoordinate)) );

	int iconSize = GlobalSettings::iconSize();
	QSize size( iconSize, iconSize );
	m_ui->zoomIn->setIconSize( size );
	m_ui->zoomOut->setIconSize( size );

	IRenderer* renderer = MapData::instance()->renderer();
	if ( renderer == NULL )
		return;

	int maxZoom = renderer->GetMaxZoom();
	m_ui->zoomBar->setMaximum( maxZoom );
	m_ui->paintArea->setMaxZoom( maxZoom );
	setZoom( GlobalSettings::zoomStreetChooser());
	m_ui->paintArea->setVirtualZoom( GlobalSettings::magnification() );
}

StreetChooser::~StreetChooser()
{
	delete d;
	delete m_ui;
}

void StreetChooser::addZoom()
{
	setZoom( GlobalSettings::zoomStreetChooser() + 1 );
}

void StreetChooser::subtractZoom()
{
	setZoom( GlobalSettings::zoomStreetChooser() - 1 );
}

void StreetChooser::setZoom( int zoom )
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
	GlobalSettings::setZoomStreetChooser( zoom );
}

void StreetChooser::mouseClicked( ProjectedCoordinate clickPos )
{
	UnsignedCoordinate coordinate( clickPos );
	d->coordinate = coordinate;
	m_ui->paintArea->setPOI( coordinate );
	return;
}

bool StreetChooser::selectStreet( UnsignedCoordinate* result, QVector< int >segmentLength, QVector< UnsignedCoordinate > coordinates, QWidget* p )
{
	if ( result == NULL )
		return false;
	if ( segmentLength.size() == 0 )
		return false;
	if ( coordinates.size() == 0 )
		return false;

	StreetChooser* window = new StreetChooser( p );
	ProjectedCoordinate center = coordinates[coordinates.size()/2].ToProjectedCoordinate();
	window->m_ui->paintArea->setCenter( center );
	window->m_ui->paintArea->setStreetPolygons( segmentLength, coordinates );
	UnsignedCoordinate centerPos( center );
	window->d->coordinate = centerPos;
	window->m_ui->paintArea->setPOI( centerPos );

	int value = window->exec();

	if ( value == Accepted )
		*result = window->d->coordinate;
	delete window;

	return value == Accepted;
}

