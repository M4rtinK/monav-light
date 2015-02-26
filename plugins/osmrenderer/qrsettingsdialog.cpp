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

#include "qrsettingsdialog.h"
#include "ui_qrsettingsdialog.h"
#include <cassert>
#include <QtDebug>
#include <QFileDialog>

QRSettingsDialog::QRSettingsDialog( QWidget *parent ) :
		QWidget( parent ),
		m_ui( new Ui::QRSettingsDialog )
{
	m_ui->setupUi(this);
	connectSlots();
}

QRSettingsDialog::~QRSettingsDialog()
{
	delete m_ui;
}

void QRSettingsDialog::connectSlots()
{
	connect(m_ui->inputBrowse, SIGNAL(clicked()), this, SLOT(browseInput()));
	connect(m_ui->rulesFileBrowse, SIGNAL(clicked()), this, SLOT(browseRulesFile()));
}

bool QRSettingsDialog::readSettings( const QtileRenderer::Settings& settings )
{
	m_ui->inputEdit->setText( settings.inputFile );
	m_ui->rulesFileEdit->setText( settings.rulesFile );
	return true;
}

bool QRSettingsDialog::fillSettings( QtileRenderer::Settings* settings )
{
	if ( settings == NULL )
		return false;
	settings->inputFile = m_ui->inputEdit->text();
	settings->rulesFile = m_ui->rulesFileEdit->text();
	return true;
}

void QRSettingsDialog::browseInput()
{
	QString file = m_ui->inputEdit->text();
	file = QFileDialog::getOpenFileName(this, tr( "Open input file" ), file);
	if ( file != "" )
		m_ui->inputEdit->setText( file );
}

void QRSettingsDialog::browseRulesFile()
{
	QString file = m_ui->rulesFileEdit->text();
	file = QFileDialog::getOpenFileName(this, tr( "Open rules file" ), file);
	if ( file != "" )
		m_ui->rulesFileEdit->setText( file );
}
