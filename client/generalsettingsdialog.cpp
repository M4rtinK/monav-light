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

#include <QMessageBox>

#include "generalsettingsdialog.h"
#include "ui_generalsettingsdialog.h"
#include "globalsettings.h"
#include "logger.h"


GeneralSettingsDialog::GeneralSettingsDialog( QWidget* parent ) :
		QDialog( parent ),
		m_ui( new Ui::GeneralSettingsDialog )
{
	m_ui->setupUi( this );
	// Windows Mobile Window Flags
	setWindowFlags( windowFlags() & ( ~Qt::WindowOkButtonHint ) );
	setWindowFlags( windowFlags() | Qt::WindowCancelButtonHint );

	m_ui->iconSize->setValue( GlobalSettings::iconSize() );
	if ( GlobalSettings::menuMode() == GlobalSettings::MenuPopup )
		m_ui->popup->setChecked( true );
	else
		m_ui->overlay->setChecked( true );
	m_ui->magnification->setValue( GlobalSettings::magnification() );

	connect( m_ui->defaultIconSize, SIGNAL(clicked()), this, SLOT(setDefaultIconSize()) );

	m_ui->checkBoxMapRotation->setChecked( GlobalSettings::autoRotation() );
	m_ui->checkBoxLogging->setChecked(Logger::instance()->loggingEnabled());
	m_ui->lineEditPathLogging->setText(Logger::instance()->directory());

	connect( m_ui->pushButtonPathLogging, SIGNAL(clicked()), this, SLOT(selectPathLogging()) );
	connect( m_ui->pushButtonClearTracklog, SIGNAL(clicked()), this, SLOT(confirmClearTracklog()) );

	QSettings settings( "MoNavClient" );
	settings.beginGroup( "GeneralSettingsDialog" );
	restoreGeometry( settings.value( "geometry" ).toByteArray() );
	m_ui->settingsList->setCurrentIndex( settings.value( "currentPage", 0 ).toInt() );
}

GeneralSettingsDialog::~GeneralSettingsDialog()
{
	QSettings settings( "MoNavClient" );
	settings.beginGroup( "GeneralSettingsDialog" );
	settings.setValue( "geometry", saveGeometry() );
	settings.setValue( "currentPage", m_ui->settingsList->currentIndex() );
	delete m_ui;
}

int GeneralSettingsDialog::exec()
{
	int value = QDialog::exec();
	fillSettings();
	return value;
}

void GeneralSettingsDialog::fillSettings() const
{
	GlobalSettings::setIconSize( m_ui->iconSize->value() );
	GlobalSettings::setMagnification( m_ui->magnification->value() );
	if ( m_ui->overlay->isChecked() )
		GlobalSettings::setMenuMode( GlobalSettings::MenuOverlay );
	else
		GlobalSettings::setMenuMode( GlobalSettings::MenuPopup );

	GlobalSettings::setAutoRotation( m_ui->checkBoxMapRotation->isChecked() );
	Logger::instance()->setLoggingEnabled(m_ui->checkBoxLogging->isChecked());
	Logger::instance()->setDirectory(m_ui->lineEditPathLogging->text());
}

void GeneralSettingsDialog::setDefaultIconSize()
{
	GlobalSettings::setDefaultIconsSize();
	m_ui->iconSize->setValue( GlobalSettings::iconSize() );
}

void GeneralSettingsDialog::selectPathLogging()
{
	QString path = m_ui->lineEditPathLogging->text();
	path = QFileDialog::getExistingDirectory (this, tr("Select Logging Directory"), path, QFileDialog::ShowDirsOnly);
	m_ui->lineEditPathLogging->setText(path);
}

void GeneralSettingsDialog::confirmClearTracklog()
{
	QMessageBox messageBox;
	messageBox.setWindowTitle( tr( "Clear Tracklog" ) );
	messageBox.setText("This will discard the current tracklog.");
	messageBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
	messageBox.setDefaultButton(QMessageBox::Cancel);
	int returnValue = messageBox.exec();
	if ( returnValue == QMessageBox::Ok )
		Logger::instance()->clearTracklog();
}
