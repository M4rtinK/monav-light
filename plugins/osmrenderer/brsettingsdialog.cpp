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

#include "brsettingsdialog.h"
#include "ui_brsettingsdialog.h"
#include <QSettings>
#include <QtDebug>

BRSettingsDialog::BRSettingsDialog(QWidget *parent) :
	QDialog(parent),
	m_ui(new Ui::BRSettingsDialog)
{
	m_ui->setupUi(this);
	QSettings settings( "MoNavClient" );
	settings.beginGroup( "Renderer Base" );
	m_ui->antiAliasing->setChecked( settings.value( "antiAliasing", true ).toBool() );
	m_ui->hqAntiAliasing->setChecked( settings.value( "hqAntiAliasing", false ).toBool() );
	m_ui->filtering->setChecked( settings.value( "filtering", false ).toBool() );
	m_ui->cacheSize->setValue( settings.value( "cacheSize", 1 ).toInt() );
}

BRSettingsDialog::~BRSettingsDialog()
{
	QSettings settings( "MoNavClient" );
	settings.beginGroup( "Renderer Base" );
	settings.setValue( "antiAliasing", m_ui->antiAliasing->isChecked() );
	settings.setValue( "hqAntiAliasing", m_ui->hqAntiAliasing->isChecked() );
	settings.setValue( "filtering", m_ui->filtering->isChecked() );
	settings.setValue( "cacheSize", m_ui->cacheSize->value() );
	delete m_ui;
}

bool BRSettingsDialog::getSettings( Settings* settings )
{
	if ( settings == NULL )
		return false;
	settings->antiAliasing = m_ui->antiAliasing->isChecked();
	settings->hqAntiAliasing = m_ui->hqAntiAliasing->isChecked();
	settings->filter = m_ui->filtering->isChecked();
	settings->cacheSize = m_ui->cacheSize->value();
	return true;
}

void BRSettingsDialog::setAdvanced( QDialog* dialog )
{
	m_ui->advanced->setEnabled( dialog != NULL );
	m_ui->advanced->disconnect( SIGNAL(clicked()) );
	if ( dialog != NULL )
		connect( m_ui->advanced, SIGNAL(clicked()), dialog, SLOT(exec()) );
}
