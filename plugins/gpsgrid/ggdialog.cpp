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

#include <QSettings>
#include "ggdialog.h"
#include "ui_ggdialog.h"

GGDialog::GGDialog( QWidget *parent ) :
		QWidget( parent ),
		ui( new Ui::GGDialog )
{
	ui->setupUi(this);
}

GGDialog::~GGDialog()
{
	delete ui;
}

bool GGDialog::getSettings( Settings* settings )
{
	if ( settings == NULL )
		return false;
	return true;
}

bool GGDialog::loadSettings( QSettings* settings )
{
	settings->beginGroup( "GPSGrid" );
	settings->endGroup();
	return true;
}

bool GGDialog::saveSettings( QSettings* settings )
{
	settings->beginGroup( "GPSGrid" );
	settings->endGroup();
	return true;
}
