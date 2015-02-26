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

#include "pluginmanager.h"
#include "utils/qthelpers.h"
#include "preprocessingwindow.h"
#include "ui_preprocessingwindow.h"
#include "utils/logwindow.h"

#include <omp.h>
#include <QFileDialog>
#include <QSettings>
#include <QtDebug>
#include <QMessageBox>
#include <QDesktopServices>
#include <QCloseEvent>

PreprocessingWindow::PreprocessingWindow( QWidget* parent, QString configFile ) :
	QMainWindow( parent ),
	m_ui( new Ui::PreprocessingWindow )
{
	m_ui->setupUi(this);

	setWindowIcon( QIcon( ":/images/target.png" ) );

	LogWindow* log = new LogWindow();
	log->setWindowIcon( QIcon( ":/images/target.png" ) );
	log->show();

	connectSlots();

	PluginManager* pluginManager = PluginManager::instance();
	pluginManager->loadPlugins();

	m_ui->importerComboBox->addItems( pluginManager->importerPlugins() );
	m_ui->routerComboBox->addItems( pluginManager->routerPlugins() );
	m_ui->rendererComboBox->addItems( pluginManager->rendererPlugins() );
	m_ui->gpsLookupComboBox->addItems( pluginManager->gpsLookupPlugins() );
	m_ui->addressLookupComboBox->addItems( pluginManager->addressLookupPlugins() );

	foreach( QObject* plugin, pluginManager->plugins() ) {
		IGUISettings* guiSettings = qobject_cast< IGUISettings* >( plugin );
		if ( guiSettings != NULL ) {
			QWidget* settings = NULL;
			if ( guiSettings->GetSettingsWindow( &settings ) ) {
				m_settings.push_back( settings );
				m_settingPlugins.push_back( guiSettings );
				m_ui->pageList->addItem( settings->windowTitle() );
				m_ui->pages->addWidget( settings );
			}
		}
	}

	loadSettings( configFile );
}

bool PreprocessingWindow::readSettingsWidgets()
{
	for ( int i = 0; i < m_settings.size(); i++ ) {
		if ( !m_settingPlugins[i]->ReadSettingsWindow( m_settings[i] ) )
			return false;
	}
	return true;
}

bool PreprocessingWindow::fillSettingsWidgets()
{
	for ( int i = 0; i < m_settings.size(); i++ ) {
		if ( !m_settingPlugins[i]->FillSettingsWindow( m_settings[i] ) )
			return false;
	}
	return true;
}

bool PreprocessingWindow::saveSettings( QString configFile )
{
	if ( !readSettingsWidgets() )
		return false;

	QSettings* settings;
	if ( configFile.isEmpty() ) {
		settings = new QSettings( "MoNav" );
	} else {
		settings = new QSettings( configFile, QSettings::IniFormat );
		settings->clear();
	}

	if ( !PluginManager::instance()->saveSettings( settings ) )
		return false;

	settings->setValue( "importer", m_ui->importerComboBox->currentText() );
	settings->setValue( "router", m_ui->routerComboBox->currentText() );
	settings->setValue( "renderer", m_ui->rendererComboBox->currentText() );
	settings->setValue( "gpsLookup", m_ui->gpsLookupComboBox->currentText() );
	settings->setValue( "addressLookup", m_ui->addressLookupComboBox->currentText() );

	if ( !configFile.isEmpty() )
		return true;

	settings->beginGroup( "GUI" );

	settings->setValue( "threads", m_ui->threads->value() );

	settings->setValue( "routingModuleName", m_ui->routingName->text() );
	settings->setValue( "renderingModuleName", m_ui->renderingName->text() );
	settings->setValue( "addressLookupModuleName", m_ui->addressLookupName->text() );

	settings->setValue( "geometry", saveGeometry() );
	settings->setValue( "splitter", m_ui->splitter->saveState() );

	delete settings;
	return true;
}

bool PreprocessingWindow::loadSettings( QString configFile )
{
	QSettings* settings;
	if ( configFile.isEmpty() )
		settings = new QSettings( "MoNav" );
	else
		settings = new QSettings( configFile, QSettings::IniFormat );

	PluginManager* pluginManager = PluginManager::instance();
	if ( !pluginManager->loadSettings( settings ) )
		return false;

	if ( !fillSettingsWidgets() )
		return false;

	m_ui->input->setText( pluginManager->inputFile() );
	m_ui->output->setText( pluginManager->outputDirectory() );

	m_ui->name->setText( pluginManager->name() );

	m_ui->packaging->setChecked( pluginManager->packaging() );
	m_ui->dictionarySize->setValue( pluginManager->dictionarySize() );
	m_ui->blockSize->setValue( pluginManager->blockSize() );

	QString importerName = settings->value( "importer", m_ui->importerComboBox->currentText() ).toString();
	QString routerName = settings->value( "router", m_ui->routerComboBox->currentText() ).toString();
	QString rendererName = settings->value( "renderer", m_ui->rendererComboBox->currentText() ).toString();
	QString gpsLookupName = settings->value( "gpsLookup", m_ui->gpsLookupComboBox->currentText() ).toString();
	QString addressLookupName = settings->value( "addressLookup", m_ui->addressLookupComboBox->currentText() ).toString();

	m_ui->importerComboBox->setCurrentIndex( m_ui->importerComboBox->findText( importerName ) );
	m_ui->routerComboBox->setCurrentIndex( m_ui->routerComboBox->findText( routerName ) );
	m_ui->rendererComboBox->setCurrentIndex( m_ui->rendererComboBox->findText( rendererName ) );
	m_ui->gpsLookupComboBox->setCurrentIndex( m_ui->gpsLookupComboBox->findText( gpsLookupName ) );
	m_ui->addressLookupComboBox->setCurrentIndex( m_ui->addressLookupComboBox->findText( addressLookupName ) );

	delete settings;

	if ( !configFile.isEmpty() )
		return true;

	settings = new QSettings( "MoNav" );

	settings->beginGroup( "GUI" );

	m_ui->routingName->setText( settings->value( "routingModuleName" ).toString() );
	m_ui->renderingName->setText( settings->value( "renderingModuleName" ).toString() );
	m_ui->addressLookupName->setText( settings->value( "addressLookupModuleName" ).toString() );

	m_ui->threads->setMaximum( omp_get_max_threads() );
	m_ui->threads->setValue( settings->value( "threads", omp_get_max_threads() ).toInt() );

	restoreGeometry( settings->value( "geometry" ).toByteArray() );

	QList< int > splitterSizes;
	splitterSizes.push_back( 0 );
	splitterSizes.push_back( 1 );
	m_ui->splitter->setSizes( splitterSizes );
	m_ui->splitter->restoreState( settings->value( "splitter" ).toByteArray() );

	return true;
}

void PreprocessingWindow::closeEvent( QCloseEvent* event )
{
	if ( !saveSettings() ) {
		if ( QMessageBox::information( this, "Save Settings", "Failed To Save Settings", QMessageBox::Cancel | QMessageBox::Close ) == QMessageBox::Cancel ) {
			event->ignore();
			return;
		}
	}
	event->accept();
	QApplication::closeAllWindows();
	//PluginManager::instance()->unloadPlugins();
	return;
}

PreprocessingWindow::~PreprocessingWindow()
{
	delete m_ui;
}

void PreprocessingWindow::connectSlots()
{
	PluginManager* pluginManager = PluginManager::instance();
	connect( pluginManager, SIGNAL(finished(bool)), this, SLOT(taskFinished(bool)) );
	connect( m_ui->packaging, SIGNAL(toggled(bool)), pluginManager, SLOT(setPackaging(bool)) );
	connect( m_ui->dictionarySize, SIGNAL(valueChanged(int)), pluginManager, SLOT(setDictionarySize(int)) );
	connect( m_ui->blockSize, SIGNAL(valueChanged(int)), pluginManager, SLOT(setBlockSize(int)) );

	connect( m_ui->inputBrowse, SIGNAL(clicked()), this, SLOT(inputBrowse()) );
	connect( m_ui->input, SIGNAL(textChanged(QString)), this, SLOT(inputChanged(QString)) );

	connect( m_ui->outputBrowse, SIGNAL(clicked()), this, SLOT(outputBrowse()) );
	connect( m_ui->output, SIGNAL(textChanged(QString)), this, SLOT(outputChanged(QString)) );

	connect( m_ui->name, SIGNAL(textChanged(QString)), this, SLOT(nameChanged(QString)) );

	connect( m_ui->threads, SIGNAL(valueChanged(int)), this, SLOT(threadsChanged(int)) );

	connect( m_ui->saveSettings, SIGNAL(clicked()), this, SLOT(saveSettingsToFile()) );
	connect( m_ui->loadSettings, SIGNAL(clicked()), this, SLOT(loadSettingsFromFile()) );

	connect( m_ui->importerPreprocessButton, SIGNAL(clicked()), this, SLOT(importerPreprocessing()) );
	connect( m_ui->rendererPreprocessButton, SIGNAL(clicked()), this, SLOT(rendererPreprocessing()) );
	connect( m_ui->routerPreprocessButton, SIGNAL(clicked()), this, SLOT(routerPreprocessing()) );
	connect( m_ui->addressLookupPreprocessButton, SIGNAL(clicked()), this, SLOT(addressLookupPreprocessing()) );
	connect( m_ui->deleteTemporaryButton, SIGNAL(clicked()), this, SLOT(deleteTemporary()) );
	connect( m_ui->writeConfigButton, SIGNAL(clicked()), this, SLOT(writeConfig()) );

	connect( m_ui->allPreprocessButton, SIGNAL(clicked()), this, SLOT(preprocessAll()) );
	connect( m_ui->daemonPreprocessButton, SIGNAL(clicked()), this, SLOT(preprocessDaemon()) );
}

void PreprocessingWindow::saveSettingsToFile()
{
	QString filename = QFileDialog::getSaveFileName( this, "Config File", QString(), "*.ini" );
	if ( filename.isEmpty() )
		return;
	if ( !filename.endsWith( ".ini" ) )
		filename += ".ini";
	if ( !saveSettings( filename ) )
		qCritical() << "Failed To Save Settings";
}

void PreprocessingWindow::loadSettingsFromFile()
{
	QString filename = QFileDialog::getOpenFileName( this, "Config File", QString(), "*.ini" );
	if ( filename.isEmpty() )
		return;
	if ( !loadSettings( filename ) )
		qCritical() << "Failed to load settings";
}

void PreprocessingWindow::inputBrowse()
{
	QString file = m_ui->input->text();
	file = QFileDialog::getOpenFileName( this, tr( "Open Input File" ), file );
	if ( !file.isEmpty() )
		m_ui->input->setText( file );
}

void PreprocessingWindow::outputBrowse()
{
	QString dir = m_ui->output->text();
	dir = QFileDialog::getExistingDirectory(this, tr( "Open Output Directory" ), dir );
	if ( !dir.isEmpty() )
		m_ui->output->setText( dir );
}

void PreprocessingWindow::inputChanged( QString text )
{
	QPalette pal;
	if ( !QFile::exists( text ) )
		pal.setColor( QPalette::Text, Qt::red );
	m_ui->input->setPalette( pal );

	PluginManager::instance()->setInputFile( text );
}

void PreprocessingWindow::outputChanged( QString text )
{
	QPalette pal;
	if ( !QFile::exists( text ) )
		pal.setColor( QPalette::Text, Qt::red );
	m_ui->output->setPalette( pal );

	PluginManager::instance()->setOutputDirectory( text );
}

void PreprocessingWindow::nameChanged( QString text )
{
	PluginManager::instance()->setName( text );
}

void PreprocessingWindow::taskFinished( bool result )
{
	if ( result == false ) {
		qCritical() << "Task failed, aborting";
		m_tasks.clear();
		setEnabled( true );
		return;
	}
	switch( m_tasks.front() ) {
	case TaskImporting:
		m_ui->importerLabel->setPixmap( QPixmap( result ? ":/images/ok.png" : ":/images/notok.png" ) );
		break;
	case TaskRouting:
		m_ui->routerLabel->setPixmap( QPixmap( result ? ":/images/ok.png" : ":/images/notok.png" ) );
		break;
	case TaskRendering:
		m_ui->rendererLabel->setPixmap( QPixmap( result ? ":/images/ok.png" : ":/images/notok.png" ) );
		break;
	case TaskAddressLookup:
		m_ui->addressLookupLabel->setPixmap( QPixmap( result ? ":/images/ok.png" : ":/images/notok.png" ) );
		break;
	case TaskConfig:
		m_ui->configLabel->setPixmap( QPixmap( result ? ":/images/ok.png" : ":/images/notok.png" ) );
		break;
	case TaskDeleteTemporary:
		m_ui->deleteTemporaryLabel->setPixmap( QPixmap( result ? ":/images/ok.png" : ":/images/notok.png" ) );
		break;
	}

	m_tasks.pop_front();
	nextTask();
}

void PreprocessingWindow::nextTask()
{
	if ( m_tasks.isEmpty() ) {
		setEnabled( true );
		return;
	}

	setEnabled( false );

	switch ( m_tasks.front() ) {
	case TaskImporting:
		if ( !PluginManager::instance()->processImporter( m_ui->importerComboBox->currentText(), true ) )
			taskFinished( false );
		break;
	case TaskRouting:
		if ( !PluginManager::instance()->processRoutingModule(
				m_ui->routingName->text(),
				m_ui->importerComboBox->currentText(),
				m_ui->routerComboBox->currentText(),
				m_ui->gpsLookupComboBox->currentText(),
				true ) )
			taskFinished( false );
		break;
	case TaskRendering:
		if ( !PluginManager::instance()->processRenderingModule(
				m_ui->renderingName->text(),
				m_ui->importerComboBox->currentText(),
				m_ui->rendererComboBox->currentText(),
				true ) )
			taskFinished( false );
		break;
	case TaskAddressLookup:
		if ( !PluginManager::instance()->processAddressLookupModule(
				m_ui->addressLookupName->text(),
				m_ui->importerComboBox->currentText(),
				m_ui->addressLookupComboBox->currentText(),
				true ) )
			taskFinished( false );
		break;
	case TaskConfig:
		if ( !PluginManager::instance()->writeConfig( m_ui->importerComboBox->currentText() ) )
			taskFinished( false );
		else
			taskFinished( true );
		break;
	case TaskDeleteTemporary:
		if ( !PluginManager::instance()->deleteTemporaryFiles() )
			taskFinished( false );
		else
			taskFinished( true );
		break;
	}
}

void PreprocessingWindow::threadsChanged( int threads )
{
	omp_set_num_threads( threads );
}

void PreprocessingWindow::importerPreprocessing()
{
	if ( !readSettingsWidgets() )
		return;

	m_tasks.push_back( TaskImporting );
	nextTask();
}

void PreprocessingWindow::rendererPreprocessing()
{
	if ( !readSettingsWidgets() )
		return;

	m_tasks.push_back( TaskRendering );
	nextTask();
}

void PreprocessingWindow::routerPreprocessing()
{
	if ( !readSettingsWidgets() )
		return;

	m_tasks.push_back( TaskRouting );
	nextTask();
}

void PreprocessingWindow::addressLookupPreprocessing()
{
	if ( !readSettingsWidgets() )
		return;

	m_tasks.push_back( TaskAddressLookup );
	nextTask();
}

void PreprocessingWindow::writeConfig()
{
	if ( !readSettingsWidgets() )
		return;

	m_tasks.push_back( TaskConfig );
	nextTask();
}

void PreprocessingWindow::deleteTemporary()
{
	if ( !readSettingsWidgets() )
		return;

	m_tasks.push_back( TaskDeleteTemporary );
	nextTask();
}

void PreprocessingWindow::preprocessAll()
{
	if ( !readSettingsWidgets() )
		return;

	m_tasks.push_back( TaskImporting );
	m_tasks.push_back( TaskRouting );
	m_tasks.push_back( TaskRendering );
	m_tasks.push_back( TaskAddressLookup );
	m_tasks.push_back( TaskConfig );
	m_tasks.push_back( TaskDeleteTemporary );
	nextTask();
}

void PreprocessingWindow::preprocessDaemon()
{
	if ( !readSettingsWidgets() )
		return;

	m_tasks.push_back( TaskImporting );
	m_tasks.push_back( TaskRouting );
	m_tasks.push_back( TaskConfig );
	m_tasks.push_back( TaskDeleteTemporary );
	nextTask();
}

