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

#include "logwindow.h"
#include "ui_logwindow.h"
#include "utils/log.h"

#include <QSettings>

LogWindow::LogWindow( QWidget* parent ) :
	QMainWindow( parent ),
	m_ui( new Ui::LogWindow )
{
	m_ui->setupUi( this );
	setAttribute( Qt::WA_DeleteOnClose, true );

	QSettings settings( "MoNav" );
	settings.beginGroup( "Log GUI" );
	restoreGeometry( settings.value( "geometry", saveGeometry() ).toByteArray() );

	QFont font( "Monospace" );
	font.setStyleHint( QFont::TypeWriter );
	m_ui->text->setFont( font );

	connect( Log::instance(), SIGNAL(newLogItem(QString)), this, SLOT(addItem(QString)) );
	connect( m_ui->actionEnable, SIGNAL(toggled(bool)), Log::instance(), SLOT(setEnabled(bool)) );
	connect( m_ui->actionLevel, SIGNAL(toggled(bool)), Log::instance(), SLOT(setLogType(bool)) );
	connect( m_ui->actionTime, SIGNAL(toggled(bool)), Log::instance(), SLOT(setLogTime(bool)) );
	connect( m_ui->actionClear, SIGNAL(triggered()), this, SLOT(clear()) );
	connect( m_ui->actionClose, SIGNAL(triggered()), this, SLOT(close()) );
	m_ui->actionFatal->setEnabled( false );
	m_ui->actionCritical->setEnabled( false );
	m_ui->actionSystem->setEnabled( false );
	m_ui->actionVerbose->setEnabled( false );
	m_ui->actionWarning->setEnabled( false );
}

LogWindow::~LogWindow()
{
	QSettings settings( "MoNav" );
	settings.beginGroup( "Log GUI" );
	settings.setValue( "geometry", saveGeometry() );
	delete m_ui;
}

void LogWindow::addItem(QString text)
{
	text = text.replace( '[', "[<font color=green>" );
	text = text.replace( ']', "</font>]" );
	m_ui->text->append( text );
}

void LogWindow::clear()
{
	m_ui->text->clear();
}
