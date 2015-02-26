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

#include "orsettingsdialog.h"
#include "ui_orsettingsdialog.h"
#include <cassert>
#include <QtDebug>

ORSettingsDialog::ORSettingsDialog( QWidget *parent ) :
		QWidget( parent ),
		m_ui( new Ui::ORSettingsDialog )
{
	m_ui->setupUi(this);
}

ORSettingsDialog::~ORSettingsDialog()
{
	delete m_ui;
}

bool ORSettingsDialog::readSettings( const OSMRenderer::Settings& settings )
{
	int index = 0;
	for ( int zoom = 0; zoom < 19; zoom++ ) {
		QString name = QString( "zoom%1" ).arg( zoom );
		QCheckBox* checkbox = findChild< QCheckBox* >( name );
		assert( checkbox != NULL );

		bool included = false;
		if ( index < ( int ) settings.zoomLevels.size() && settings.zoomLevels[index] == zoom ) {
			included = true;
			index++;
		}

		checkbox->setChecked( included );
	}
	return true;
}

bool ORSettingsDialog::fillSettings( OSMRenderer::Settings* settings )
{
	if ( settings == NULL )
		return false;
	settings->zoomLevels.clear();
	for ( int zoom = 0; zoom < 19; zoom++ ) {
		QString name = QString( "zoom%1" ).arg( zoom );
		QCheckBox* checkbox = findChild< QCheckBox* >( name );
		assert( checkbox != NULL );
		if ( checkbox->isChecked() )
			settings->zoomLevels.push_back( zoom );
	}
	return true;
}

