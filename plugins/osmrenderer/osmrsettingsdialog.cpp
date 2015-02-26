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

#include "osmrsettingsdialog.h"
#include "ui_osmrsettingsdialog.h"
#include <QSettings>
#include <cassert>

OSMRSettingsDialog::OSMRSettingsDialog( QWidget* parent ) :
		QDialog( parent ),
		m_ui( new Ui::OSMRSettingsDialog )
{
	m_ui->setupUi( this );
	QSettings settings( "MoNavClient" );
	settings.beginGroup( "OSM Renderer" );
	m_ui->custom->setChecked( settings.value( "custom", false ).toBool() );
	m_ui->customServer->setText( settings.value( "customServer", "http://tile.openstreetmap.org/%1/%2/%3.png" ).toString() );
	m_ui->server->addItem( "OpenStreetMap ( Mapnik )", "http://tile.openstreetmap.org/%1/%2/%3.png" );
	m_ui->server->addItem( "OpenStreetMap ( Osmarenderer )", "http://tah.openstreetmap.org/Tiles/tile/%1/%2/%3.png" );
	m_ui->server->addItem( "OpenStreetMap ( Cycle Map )", "http://andy.sandbox.cloudmade.com/tiles/cycle/%1/%2/%3.png" );
	m_ui->server->setCurrentIndex( settings.value( "index", 0 ).toInt() );

	connect( m_ui->server, SIGNAL(currentIndexChanged(int)), this, SLOT(serverChanged(int)) );
}

OSMRSettingsDialog::~OSMRSettingsDialog()
{
	QSettings settings( "MoNavClient" );
	settings.beginGroup( "OSM Renderer" );
	settings.setValue( "custom", m_ui->custom->isChecked() );
	settings.setValue( "customServer", m_ui->customServer->text() );
	settings.setValue( "index", m_ui->server->currentIndex() );
	delete m_ui;
}

bool OSMRSettingsDialog::getSettings( Settings* settings )
{
	assert( settings != NULL );
	settings->tileURL = m_ui->customServer->text();
	return true;
}

void OSMRSettingsDialog::serverChanged( int index )
{
	if ( index == -1 )
		m_ui->customServer->clear();
	m_ui->customServer->setText( m_ui->server->itemData( index ).toString() );
}
