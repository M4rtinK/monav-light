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

#include "mrsettingsdialog.h"
#include "ui_mrsettingsdialog.h"
#include <QFileDialog>
#include <QFile>
#include <cassert>
#include <QtDebug>
#include <QSettings>

MRSettingsDialog::MRSettingsDialog( QWidget *parent ) :
	 QWidget( parent ),
	 m_ui( new Ui::MRSettingsDialog )
{
	 m_ui->setupUi(this);
	connectSlots();
}

MRSettingsDialog::~MRSettingsDialog()
{
	delete m_ui;
}

void MRSettingsDialog::connectSlots()
{
	connect( m_ui->fontBrowse, SIGNAL(clicked()), this, SLOT(browseFont()) );
	connect( m_ui->themeBrowse, SIGNAL(clicked()), this, SLOT(browseTheme()) );
	connect( m_ui->modulesBrowse, SIGNAL(clicked()), this, SLOT(browsePlugins()) );
}

void MRSettingsDialog::browseFont()
{
	QString dir = m_ui->fontEdit->text();
	dir = QFileDialog::getExistingDirectory(this, tr( "Open Font Directory" ), dir );
	if ( dir != "" )
		m_ui->fontEdit->setText( dir );
}

void MRSettingsDialog::browseTheme()
{
	QString file = m_ui->themeEdit->text();
	file = QFileDialog::getOpenFileName( this, tr("Enter OSM Theme Filename"), file, "*.xml" );
	if ( file != "" )
		m_ui->themeEdit->setText( file );
}

void MRSettingsDialog::browsePlugins()
{
	QString dir = m_ui->modulesEdit->text();
	dir = QFileDialog::getExistingDirectory(this, tr( "Open Mapnik Plugin Directory" ), dir );
	if ( dir != "" )
		m_ui->modulesEdit->setText( dir );
}

bool MRSettingsDialog::fillSettings( MapnikRenderer::Settings* settings ) {
	settings->fonts = m_ui->fontEdit->text();
	settings->theme = m_ui->themeEdit->text();
	settings->plugins = m_ui->modulesEdit->text();
	settings->tileSize = m_ui->tileSize->value();
	settings->metaTileSize = m_ui->metaTileSize->value();
	settings->fullZoom = m_ui->minZoom->value();
	settings->margin = m_ui->margin->value();
	settings->tileMargin = m_ui->tileMargin->value();
	settings->reduceColors = m_ui->colorReduction->isChecked();
	settings->deleteTiles = m_ui->removeTiles->isChecked();
	settings->pngcrush = m_ui->pngcrush->isChecked();

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

bool MRSettingsDialog::readSettings( const MapnikRenderer::Settings& settings )
{
	m_ui->fontEdit->setText( settings.fonts );
	m_ui->themeEdit->setText( settings.theme );
	m_ui->modulesEdit->setText( settings.plugins );
	m_ui->tileSize->setValue( settings.tileSize );
	m_ui->metaTileSize->setValue( settings.metaTileSize );
	m_ui->minZoom->setValue( settings.fullZoom );
	m_ui->margin->setValue( settings.margin );
	m_ui->tileMargin->setValue( settings.tileMargin );
	m_ui->colorReduction->setChecked( settings.reduceColors );
	m_ui->removeTiles->setChecked( settings.deleteTiles );
	m_ui->pngcrush->setChecked( settings.pngcrush );

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
