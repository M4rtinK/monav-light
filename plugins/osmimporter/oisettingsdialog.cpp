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

#include "oisettingsdialog.h"
#include "ui_oisettingsdialog.h"
#include "speedprofiledialog.h"
#include "utils/qthelpers.h"

#include <QFileDialog>
#include <QInputDialog>
#include <QtDebug>
#include <QSettings>
#include <cassert>
#include <QDir>

OISettingsDialog::OISettingsDialog( QWidget *parent ) :
	QWidget(parent),
	m_ui(new Ui::OISettingsDialog)
{
	m_ui->setupUi(this);

	QDir dir( ":/speed profiles" );
	dir.setNameFilters( QStringList( "*.spp" ) );
	QStringList files = dir.entryList( QDir::Files );
	for ( int i = 0; i < files.size(); i++ ) {
		m_ui->speedProfileChooser->addItem( files[i] );
		m_speedProfiles.push_back( dir.filePath( files[i] ) );
	}

	connectSlots();
}

void OISettingsDialog::connectSlots()
{
	connect( m_ui->speedProfileChooser, SIGNAL(currentIndexChanged(int)), this, SLOT(currentIndexChanged()) );
	connect( m_ui->languages, SIGNAL(currentRowChanged(int)), this, SLOT(currentLanguageChanged(int)) );
	connect( m_ui->addLanguage, SIGNAL(clicked()), this, SLOT(addLanguage()) );
	connect( m_ui->deleteLanguage, SIGNAL(clicked()), this, SLOT(deleteLanguage()) );
	connect( m_ui->edit, SIGNAL(clicked()), this, SLOT(edit()) );
	connect( m_ui->speedProfile, SIGNAL(textChanged(QString)), this, SLOT(filenameChanged(QString)) );
}

OISettingsDialog::~OISettingsDialog()
{
	delete m_ui;
}

void OISettingsDialog::filenameChanged( QString text )
{
	QPalette pal;
	if ( !QFile::exists( text ) )
		pal.setColor( QPalette::Text, Qt::red );
	m_ui->speedProfile->setPalette( pal );
}

void OISettingsDialog::currentLanguageChanged ( int currentRow )
{
	m_ui->deleteLanguage->setEnabled( currentRow != -1 );
}

void OISettingsDialog::addLanguage()
{
	QListWidgetItem* item = new QListWidgetItem( "name", m_ui->languages, 0 );
	item->setFlags( item->flags () | Qt::ItemIsEditable );
}

void OISettingsDialog::deleteLanguage()
{
	delete m_ui->languages->takeItem( m_ui->languages->currentRow() );
}

void OISettingsDialog::currentIndexChanged()
{
	int index = m_ui->speedProfileChooser->currentIndex();
	if ( index == -1 )
		return;
	m_ui->speedProfile->setText( m_speedProfiles[index] );
}

bool OISettingsDialog::readSettings( const OSMImporter::Settings& settings )
{
	m_ui->languages->clear();
	m_ui->languages->addItems( settings.languageSettings );
	m_ui->speedProfile->setText( settings.speedProfile );

	int index = m_speedProfiles.indexOf( settings.speedProfile );
	if ( index != -1 ) {
		m_ui->speedProfileChooser->setCurrentIndex( index );
	}
	return true;
}

bool OISettingsDialog::fillSettings( OSMImporter::Settings* settings ) const
{
	settings->languageSettings.clear();
	for ( int i = 0; i < m_ui->languages->count(); i++ )
		settings->languageSettings.push_back( m_ui->languages->item( i )->text() );
	settings->speedProfile = m_ui->speedProfile->text();
	return true;
}

void OISettingsDialog::edit()
{
	SpeedProfileDialog* dialog = new SpeedProfileDialog( this, m_ui->speedProfile->text() );
	dialog->exec();
	delete dialog;
}
