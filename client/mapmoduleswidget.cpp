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

#include "mapdata.h"

#include "mapmoduleswidget.h"
#include "ui_mapmoduleswidget.h"

#include <QMessageBox>
#include <QDir>
#include <QProgressDialog>
#include <QtConcurrentRun>
#include <QFuture>
#include <QFutureWatcher>

struct MapModulesWidget::PrivateImplementation
{
	QVector< MapData::Module > rendering;
	QVector< MapData::Module > routing;
	QVector< MapData::Module > addressLookup;
};

MapModulesWidget::MapModulesWidget( QWidget* parent ) :
		QWidget( parent ),
		m_ui( new Ui::MapModulesWidget )
{
	m_ui->setupUi( this );
	d = new PrivateImplementation;

	connect( m_ui->select, SIGNAL(clicked()), this, SLOT(select()) );
	connect( m_ui->cancel, SIGNAL(clicked()), this, SIGNAL(cancelled()) );
	populateData();
}

MapModulesWidget::~MapModulesWidget()
{
	delete d;
	delete m_ui;
}

void MapModulesWidget::populateData()
{
	MapData* mapData = MapData::instance();
	m_ui->routing->clear();
	m_ui->rendering->clear();
	m_ui->addressLookup->clear();

	QDir dir( mapData->path() );
	dir.setNameFilters( QStringList( "*.mmm" ) );
	QStringList packedModules = dir.entryList( QDir::Files, QDir::Name );

	if ( packedModules.size() > 0 ) {
		int button = QMessageBox::question( NULL, "Packed Map Modules", "Found packed map modules, do you want to unpack them?", "Unpack", "Ignore", "Delete" );
		if ( button == 0 ) {
			QProgressDialog progress;
			progress.setWindowTitle( "MoNav - Unpacking" );
			progress.setWindowModality( Qt::ApplicationModal );
			progress.setMaximum( packedModules.size() );
			for ( int i = 0; i < packedModules.size(); i++ ) {
				QString filename = dir.filePath( packedModules[i] );
				QFuture< bool > future = QtConcurrent::run( MapData::unpackModule, filename );
				QFutureWatcher< bool > watcher;
				connect( &watcher, SIGNAL(finished()), &progress, SLOT(accept()) );
				watcher.setFuture( future );
				progress.setLabelText( "Unpacking Module: " + packedModules[i] );
				progress.setValue( i );
				int result = progress.exec();
				if ( result == QProgressDialog::Rejected ) {
					future.waitForFinished();
					break;
				}

				if ( !future.result() ) {
					int button = QMessageBox::question( NULL, "Packed Map Modules", "Failed to unpack module: " + packedModules[i], "Ignore", "Delete" );
					if ( button == 1 ) {
						QFile::remove( dir.filePath( packedModules[i] ) );
					}
				}
			}
			mapData->loadInformation();
		} else if ( button == 3 ) {
			foreach( QString filename, packedModules ) {
				QFile::remove( filename );
			}
		}
	}

	if ( !mapData->informationLoaded() )
		return;

	d->routing = mapData->modules( MapData::Routing );
	d->rendering = mapData->modules( MapData::Rendering );
	d->addressLookup = mapData->modules( MapData::AddressLookup );

	for ( int i = 0; i < d->routing.size(); i++ )
		m_ui->routing->addItem( d->routing[i].name );
	for ( int i = 0; i < d->rendering.size(); i++ )
		m_ui->rendering->addItem( d->rendering[i].name );
	for ( int i = 0; i < d->addressLookup.size(); i++ )
		m_ui->addressLookup->addItem( d->addressLookup[i].name );

	QString lastRouting;
	QString lastRendering;
	QString lastAddressLookup;
	mapData->lastModules( &lastRouting, &lastRendering, &lastAddressLookup );
	int routing = m_ui->routing->findText( lastRouting );
	int rendering = m_ui->rendering->findText( lastRendering );
	int addressLookup = m_ui->addressLookup->findText( lastAddressLookup );
	if ( routing != -1 )
		m_ui->routing->setCurrentIndex( routing );
	if ( rendering != -1 )
		m_ui->rendering->setCurrentIndex( rendering );
	if ( addressLookup != -1 )
		m_ui->addressLookup->setCurrentIndex( addressLookup );
}

void MapModulesWidget::select()
{
	int routingIndex = m_ui->routing->currentIndex();
	int renderingIndex = m_ui->rendering->currentIndex();
	int addressLookupIndex = m_ui->addressLookup->currentIndex();
	if ( routingIndex == -1 )
		return;
	if ( renderingIndex == -1 )
		return;
	if ( addressLookupIndex == -1 )
		return;

	MapData* mapData = MapData::instance();
	if ( mapData->load( d->routing[routingIndex], d->rendering[renderingIndex], d->addressLookup[addressLookupIndex] ) )
		emit selected();

	return;
}
