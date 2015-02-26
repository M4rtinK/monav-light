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

#include "mappackageswidget.h"
#include "ui_mappackageswidget.h"
#include "serverinputdialog.h"
#include "mapdata.h"

#include <QResizeEvent>
#include <QShowEvent>
#include <QSettings>
#include <QStringList>
#include <QDir>
#include <QProgressDialog>
#include <QFileDialog>
#include <QtDebug>
#include <QMessageBox>

struct MapPackagesWidget::PrivateImplementation {

	int selected;

	QString path;
	QVector< MapData::MapPackage > maps;
	QVector< ServerLogic::Server > servers;
	QVector< QDomElement > elements;
	ServerLogic *serverLogic;
	QProgressDialog *progress;
	QString progressDetails;

	void populateInstalled( QListWidget* list );
	void populateUpdatable( QListWidget* list );
	void startPackageDownload();
	void highlightButton( QPushButton* button, bool highlight );
	void showFinishDialog();
};

MapPackagesWidget::MapPackagesWidget( QWidget* parent ) :
	QWidget( parent ),
	m_ui( new Ui::MapPackagesWidget )
{
	m_ui->setupUi( this );
	d = new PrivateImplementation;

	d->serverLogic = NULL;
	d->progress = NULL;
	QString progressDetails = "";

	QSettings settings( "MoNavClient" );
	settings.beginGroup( "MapPackages" );
	d->path = settings.value( "path" ).toString();
	bool worldMap = settings.value( "worldmap", true ).toBool();
	m_ui->installedList->setVisible( !worldMap );
	m_ui->worldMap->setVisible( worldMap );
	m_ui->switchSelection->setChecked( worldMap );

	int entries = settings.beginReadArray( "servers" );
	for ( int i = 0; i < entries; i++ ) {
		settings.setArrayIndex( i );
		ServerLogic::Server server;
		server.name = settings.value( "name" ).toString();
		server.url = settings.value( "url" ).toUrl();
		d->servers.push_back( server );
		m_ui->server->addItem( server.name, server.url );
	}
	settings.endArray();
	m_ui->server->setCurrentIndex( settings.value( "server", -1 ).toInt() );
		// TODO: INSERT DEFAULT SERVER
	m_ui->loadList->setEnabled( m_ui->server->count() > 0 );

	connect( m_ui->changeDirectory, SIGNAL(clicked()), this, SLOT(directory()) );
	connect( m_ui->load, SIGNAL(clicked()), this, SLOT(load()) );
	connect( m_ui->installedList, SIGNAL(itemSelectionChanged()), this, SLOT(mapSelectionChanged()) );
	connect( m_ui->worldMap, SIGNAL(clicked(int)), this, SLOT(selected(int)) );

	connect( m_ui->backUD, SIGNAL(clicked()), this, SLOT(changeTab()) );
	connect( m_ui->check, SIGNAL(clicked()), this, SLOT(check()) );
	connect( m_ui->update, SIGNAL(clicked()), this, SLOT(update()) );
	connect( m_ui->updateList, SIGNAL(itemSelectionChanged()), this, SLOT(updateSelectionChanged()) );

	connect( m_ui->backDL, SIGNAL(clicked()), this, SLOT(changeTab()) );
	connect( m_ui->server, SIGNAL(currentIndexChanged(int)), this, SLOT(serverIndexChanged(int)));
	connect( m_ui->addServer, SIGNAL(clicked()), this, SLOT(editServerList()) );
	connect( m_ui->loadList, SIGNAL(clicked()), this, SLOT(downloadList()) );
	connect( m_ui->loadPackages, SIGNAL(clicked()), this, SLOT(downloadPackages()) );
	connect( m_ui->downloadList, SIGNAL(itemSelectionChanged()), this, SLOT(downloadSelectionChanged()) );

	d->populateInstalled( m_ui->installedList );
	d->highlightButton( m_ui->changeDirectory, m_ui->installedList->count() == 0 );
	m_ui->worldMap->setMaps( d->maps );
	m_ui->worldMap->setHighlight( d->selected );
}

MapPackagesWidget::~MapPackagesWidget()
{
	QSettings settings( "MoNavClient" );
	settings.beginGroup( "MapPackages" );
	settings.setValue( "path", d->path );
	settings.setValue( "worldmap", m_ui->switchSelection->isChecked() );
	settings.setValue( "server", m_ui->server->currentIndex());
	settings.beginWriteArray( "servers", d->servers.size() );
	for ( int i = 0; i < d->servers.size(); i++ ) {
		settings.setArrayIndex( i );
		settings.setValue( "name", d->servers[i].name );
		settings.setValue( "url", d->servers[i].url );
	}
	settings.endArray();

	if(d->serverLogic != NULL)
		delete d->serverLogic;
	if(d->progress != NULL)
		delete d->progress;
	delete d;
	delete m_ui;
}

void MapPackagesWidget::resizeEvent ( QResizeEvent* /*event*/ )
{
	// TODO CHANGE ORIENTATION
}

void MapPackagesWidget::showEvent( QShowEvent* /*event*/ )
{
	if ( !QFile::exists( d->path ) ) {
		//QMessageBox::information( this, "Data Directory", "Before proceeding be sure to select a valid data directory", "Ok" );
	}
}

void MapPackagesWidget::selected( int id )
{
	m_ui->installedList->item( id )->setSelected( true );
}

void MapPackagesWidget::changeTab( int tabIndex )
{
		m_ui->tabs->setCurrentIndex( tabIndex );
}

void MapPackagesWidget::serverIndexChanged( int newIndex )
{
		m_ui->loadList->setEnabled( newIndex >= 0 );
}

void MapPackagesWidget::mapSelectionChanged()
{
	bool selected = m_ui->installedList->selectedItems().size() == 1;
	if ( selected )
		m_ui->worldMap->setHighlight( m_ui->installedList->selectedItems().first()->data( Qt::UserRole ).toInt() );
	m_ui->load->setEnabled( selected );
	m_ui->deleteMap->setEnabled( selected );
}

void MapPackagesWidget::updateSelectionChanged()
{
	bool selected = m_ui->updateList->selectedItems().size() > 0;
	m_ui->update->setEnabled( selected );
}

void MapPackagesWidget::downloadSelectionChanged()
{
	bool selected = m_ui->downloadList->selectedItems().size() > 0;
	m_ui->loadPackages->setEnabled( selected );
}

void MapPackagesWidget::load()
{
	QList< QListWidgetItem* > items = m_ui->installedList->selectedItems();
	if ( items.size() != 1 ) {
		qDebug() << "Error: only one map should be selected";
		return;
	}

	int index = items.first()->data( Qt::UserRole ).toInt();

	MapData* mapData = MapData::instance();
	mapData->setPath( d->maps[index].path );
	if ( !mapData->loadInformation() )
		return;

	emit mapChanged();
}

void MapPackagesWidget::directory()
{
	QString newDir = QFileDialog::getExistingDirectory( this, "MoNav Data Directory", d->path );
	if ( newDir.isEmpty() || newDir == d->path )
		return;

	d->path = newDir;
	d->populateInstalled( m_ui->installedList );
	d->highlightButton( m_ui->changeDirectory, m_ui->installedList->count() == 0 );
	m_ui->worldMap->setMaps( d->maps );
	m_ui->worldMap->setHighlight( d->selected );
}

void MapPackagesWidget::setupNetworkAccess()
{
	d->serverLogic = new ServerLogic();

	connect( d->serverLogic, SIGNAL( loadedList() ), this, SLOT( populateServerPackageList() ) );
	connect( d->serverLogic, SIGNAL( loadedPackage( QString ) ), this, SLOT( handleProgress( QString ) ) );
	connect( d->serverLogic, SIGNAL( loadedPackage( QString ) ), d->serverLogic, SLOT( loadPackage() ) );
	connect( d->serverLogic, SIGNAL( checkedPackage( QString ) ), this, SLOT( handleProgress( QString ) ) );
	connect( d->serverLogic, SIGNAL( checkedPackage( QString ) ), d->serverLogic, SLOT( checkPackage() ) );
	connect( d->serverLogic, SIGNAL( error( ServerLogic::ERROR_TYPE, QString ) ), this, SLOT( cleanUp( ServerLogic::ERROR_TYPE, QString ) ) );

	d->serverLogic->connectNetworkManager();
}

void MapPackagesWidget::downloadList()
{
	QString name = d->servers[m_ui->server->currentIndex()].name;
	QUrl url = d->servers[m_ui->server->currentIndex()].url;

	QMessageBox dlMsg;
	dlMsg.setText( "Downloading package list from " + name + " (" + url.toString() + ")." );
	dlMsg.setInformativeText( "Downloading the package list requires an active internet connection. Proceed?" );
	dlMsg.setStandardButtons( QMessageBox::No | QMessageBox::Yes );
	dlMsg.setDefaultButton( QMessageBox::Yes );

	if( dlMsg.exec() != QMessageBox::Yes )
		return;

	if( d->serverLogic == NULL )
		setupNetworkAccess();

	d->progressDetails = "";
	d->serverLogic->setOp( ServerLogic::LIST_DL );
	d->serverLogic->setStatus( ServerLogic::NO_ERROR );
	d->serverLogic->loadPackageList( url );
}

void MapPackagesWidget::check()
{
	QList< ServerLogic::PackageInfo > packagesInstalled;

	ServerLogic::PackageInfo package;
	for( int i=0; i < d->maps.size(); i++ )
	{
        QDir dir( QDir::cleanPath( d->maps.at( i ).path ) );

		QSettings mapInfo( dir.filePath( "MoNav.ini" ), QSettings::IniFormat );
		QString mapName = mapInfo.value( "name" ).toString();

		QSettings serverInfo( dir.filePath( "Server.ini" ), QSettings::IniFormat );
		package.server = serverInfo.value( "server" ).toString();
		package.timestamp = serverInfo.value( "timestamp").toUInt();
        package.dir = dir.path() + '/' + "MoNav.ini";
		package.path = mapName;

		if( !package.server.isEmpty() )
			packagesInstalled.push_back( package );

		QStringList modules = dir.entryList( QDir::Dirs | QDir::NoDotAndDotDot );
		for( int j=0; j < modules.size(); j++ )
		{
			dir.cd( modules[j] );

			QSettings moduleInfo( dir.filePath( "Server.ini" ), QSettings::IniFormat );
			package.server = moduleInfo.value( "server" ).toString();
			package.timestamp = moduleInfo.value( "timestamp").toUInt();
            package.dir = dir.path();
			package.path = mapName;

			if( !package.server.isEmpty() )
				packagesInstalled.push_back( package );

			dir.cdUp();
		}
	}

	if( packagesInstalled.isEmpty() )
		return;

	qSort( packagesInstalled.begin(), packagesInstalled.end() );

	QString checkServer = packagesInstalled.first().server;
	QString checkInfo = "Server " + checkServer +":\n";
	foreach ( const ServerLogic::PackageInfo& pInfo, packagesInstalled )
	{
		if( pInfo.server != checkServer )
		{
			checkServer = pInfo.server;
			checkInfo.append( "\nServer " + checkServer +":\n");
		}

		checkInfo.append( pInfo.dir + '\n');
	}

	QMessageBox dlMsg;
	dlMsg.setText( "Checking installed packages for updates." );
	dlMsg.setInformativeText( "Update Check requires an active internet connection to retrieve server package list. Proceed?" );
	dlMsg.setDetailedText( checkInfo );
	dlMsg.setStandardButtons( QMessageBox::No | QMessageBox::Yes );
	dlMsg.setDefaultButton( QMessageBox::Yes );

	if( dlMsg.exec() != QMessageBox::Yes )
		return;

	if( d->serverLogic == NULL )
		setupNetworkAccess();
	else
	{
		d->serverLogic->clearUpdatablePackages();
		d->serverLogic->clearPackagesToLoad();
	}

	d->serverLogic->addPackagesToLoad( packagesInstalled );

	if( d->progress != NULL )
		delete d->progress;
	d->progress = new QProgressDialog( packagesInstalled.first().path, "Abort", 0, packagesInstalled.size(), this );
	d->progress->setWindowModality( Qt::WindowModal );
	d->progress->setValue( 0 );
	d->progress->show();
	d->progressDetails = "";

	connect( d->serverLogic, SIGNAL( loadedList() ), d->serverLogic, SLOT( checkPackage() ) );
	disconnect( d->serverLogic, SIGNAL( loadedList() ), this, SLOT( populateServerPackageList() ) );

	d->serverLogic->setOp( ServerLogic::PACKAGE_CHK );
	d->serverLogic->setStatus( ServerLogic::NO_ERROR );
	d->serverLogic->loadPackageList( packagesInstalled.first().server + "packageList.xml" );

	return;
}

void MapPackagesWidget::update()
{
	QList< QListWidgetItem* > selected = m_ui->updateList->selectedItems();

		if( selected.isEmpty() )
			return;

	QList< ServerLogic::PackageInfo > packagesToUpdate;

	foreach ( QListWidgetItem* item, selected )
	{
		packagesToUpdate.append( d->serverLogic->updatablePackages()[ item->data( Qt::UserRole ).toInt() ] );
		item->setSelected( false );
	}

	if( packagesToUpdate.isEmpty() )
		return;

	d->serverLogic->clearPackagesToLoad();
	d->serverLogic->addPackagesToLoad( packagesToUpdate );

	d->startPackageDownload();

}

void MapPackagesWidget::downloadPackages()
{
	QList< QTreeWidgetItem* > selected = m_ui->downloadList->selectedItems();

	QString serverName = d->servers[ m_ui->server->currentIndex() ].name;
	QString serverPath = d->serverLogic->packageList().documentElement().attribute( "path" );

	QList< ServerLogic::PackageInfo > packagesToLoad;
	foreach ( QTreeWidgetItem* item, selected )
	{
		int elementIndex =  item->data( 0, Qt::UserRole ).toInt();

		QDomElement map = d->elements[ elementIndex ];
		while( map.tagName() != "map" )
			map = map.parentNode().toElement();
		QString mapName = map.attribute( "name" );

        QString baseDir = d->path.isEmpty() ? "" : d->path + '/';
        QString localDir = baseDir + serverName + '/' + mapName + '/';

		ServerLogic::PackageInfo pInfo;
		pInfo.server = serverPath;
		pInfo.dir = localDir;

		if( !QFile( localDir + "MoNav.ini" ).exists() )
		{
			pInfo.path = map.attribute( "path" );
			pInfo.size = map.attribute( "size" ).toLongLong();
			pInfo.timestamp = map.attribute( "timestamp" ).toUInt();

			if( !packagesToLoad.contains( pInfo ) )
				packagesToLoad.push_back( pInfo );
		}

		QDomElement element = d->elements[ elementIndex ];
		while ( true )
		{
			while( !element.firstChild().isText() )
				element = element.firstChild().toElement();

			pInfo.path = element.text();
			pInfo.size = element.attribute( "size" ).toLongLong();
			pInfo.timestamp = element.attribute( "timestamp" ).toUInt();

			if( !packagesToLoad.contains( pInfo ) )
				packagesToLoad.push_back( pInfo );

			while( element.nextSiblingElement().isNull() && element != d->elements[ elementIndex ] )
				element = element.parentNode().toElement();

			if( element == d->elements[ elementIndex ] )
				break;

			element = element.nextSiblingElement();
		}

		item->setSelected( false );
	}

	if( packagesToLoad.isEmpty() )
		return;

	d->serverLogic->addPackagesToLoad( packagesToLoad );

	d->startPackageDownload();
}

void MapPackagesWidget::editServerList()
{
	ServerInputDialog* dialog = new ServerInputDialog( d->servers, this );

	if( dialog->exec() == QDialog::Accepted )
	{
		dialog->writeServerSettings( &d->servers );

		m_ui->server->clear();
		for(int i=0; i < d->servers.size(); i++)
			m_ui->server->addItem( d->servers[i].name, d->servers[i].url );
	}

	delete dialog;
}

void MapPackagesWidget::populateServerPackageList()
{
	m_ui->downloadList->clear();
	d->elements.clear();

	if( d->serverLogic->packageList().isNull() )
	{
		d->serverLogic->setStatus( ServerLogic::LIST_DL_ERROR );
		d->showFinishDialog();
		return;
	}

	QDomElement element = d->serverLogic->packageList().documentElement().firstChildElement();
	QTreeWidgetItem *parent = NULL;

	int id = 0;
	while( !element.isNull() )
	{
		d->elements.push_back( element );

		QString name = element.hasAttribute( "name" ) ? element.attribute( "name" ) : element.tagName();

		QTreeWidgetItem *item = new QTreeWidgetItem( parent, QStringList( name ) );

		QString status = "";
		if( element.firstChild().isText() )
		{
			QString modulePath = element.firstChild().toText().data();
			QString moduleName = modulePath.remove( 0, modulePath.lastIndexOf( '/' ) + 1 ).replace( ".mmm", "" );
			QString mapName = element.parentNode().parentNode().toElement().attribute( "name" );
			QString dirName =
					d->path + "/" + m_ui->server->currentText() + "/" + mapName + "/" + moduleName;

			if( QDir( dirName ).exists() )
				status = "installed";
		}

		item->setText( 1, status);
		item->setData( 0, Qt::UserRole, id );

		m_ui->downloadList->addTopLevelItem( item );

		++id;

		if( !element.firstChildElement().isNull() )
		{
			element = element.firstChildElement();
			parent = item;
		}

		else
		{
			while( element.nextSiblingElement().isNull() && !element.parentNode().isNull() )
			{
				element = element.parentNode().toElement();
				parent = parent != NULL ? parent->parent() : NULL;
			}

			element = element.nextSiblingElement();
		}
	}

	qDebug() << "loaded package list";
}

void MapPackagesWidget::PrivateImplementation::startPackageDownload()
{
	QString dlInfo = "";
	qint64 sizeTotal = 0;
	foreach ( const ServerLogic::PackageInfo& pInfo, serverLogic->packagesToLoad() )
	{
		dlInfo.append( pInfo.server + pInfo.path + QString( " - %1 byte\n" ).arg( pInfo.size ) );
		sizeTotal += pInfo.size;
	}

	QMessageBox dlMsg;
	//dlMsg.setText( "Downloading packages from " + serverName + " ( " + serverPath + " )"  );
	dlMsg.setText( "Downloading packages." );
	QString sizeInfo = QString( "Total size is %1 byte. Proceed?" ).arg( sizeTotal );
	dlMsg.setInformativeText( "Downloading the packages requires an active internet connection. " + sizeInfo );
	dlMsg.setDetailedText( dlInfo );
	dlMsg.setStandardButtons( QMessageBox::No | QMessageBox::Yes );
	dlMsg.setDefaultButton( QMessageBox::Yes );

	if( dlMsg.exec() != QMessageBox::Yes )
	{
		serverLogic->clearPackagesToLoad();
		return;
	}

	if( progress != NULL )
		delete progress;
	progress = new QProgressDialog(
				serverLogic->packagesToLoad().first().path,
				"Abort download",
				0,
				serverLogic->packagesToLoad().size() );
	progress->setWindowModality( Qt::WindowModal );
	progress->setValue( 0 );
	progress->show();
	progressDetails = "";

	serverLogic->setOp( ServerLogic::PACKAGE_DL );
	serverLogic->setStatus( ServerLogic::NO_ERROR );
	serverLogic->loadPackage();
}

void MapPackagesWidget::PrivateImplementation::populateInstalled( QListWidget* list )
{
	list->clear();
	maps.clear();
	selected = -1;

	MapData* mapData = MapData::instance();
	if ( !mapData->searchForMapPackages( path, &maps, 2 ) )
		return;

	for ( int i = 0; i < maps.size(); i++ ) {
		QListWidgetItem* item = new QListWidgetItem( maps[i].name );
		item->setData( Qt::UserRole, i );
		list->addItem( item );
		if ( maps[i].path == mapData->path() ) {
			item->setSelected( true );
			selected = i;
		}
	}
}

void MapPackagesWidget::PrivateImplementation::populateUpdatable( QListWidget* list )
{
	list->clear();

	ServerLogic::PackageInfo package;
	for( int i=0; i < serverLogic->updatablePackages().size(); i++ )
	{
		package = serverLogic->updatablePackages().at( i );
		QString module = package.path.right( package.path.size() - package.path.lastIndexOf( '/') - 1 );
		module.truncate( module.lastIndexOf( '.') );
		QString map = package.path.left( package.path.lastIndexOf( '/') );

		QListWidgetItem *item = new QListWidgetItem( map + " - " + module );
		item->setData( Qt::UserRole, i );
		list->addItem( item );
	}
}

void MapPackagesWidget::PrivateImplementation::highlightButton( QPushButton* button, bool highlight )
{
	QFont font = button->font();
	font.setBold( highlight );
	font.setUnderline( highlight );
	button->setFont( font );
}

void MapPackagesWidget::PrivateImplementation::showFinishDialog()
{
	ServerLogic::OPERATION_TYPE operation = serverLogic->getOp();
	QMessageBox dlMsg;

	switch( operation )
	{
		case ServerLogic::PACKAGE_CHK:
		{
			dlMsg.setText( "Package Update Check" );
			dlMsg.setDetailedText( progressDetails );
			break;
		}
		case ServerLogic::PACKAGE_DL:
		{
			dlMsg.setText( "Package Download & Extract" );
			dlMsg.setDetailedText( progressDetails );
			break;
		}
		case ServerLogic::LIST_DL:
		{
			dlMsg.setText( "Server Package List Download" );
			dlMsg.setDetailedText( progressDetails );
			break;
		}
		default:
		{
			dlMsg.setText( "Operation" );
			dlMsg.setDetailedText( progressDetails );
		}
	}

	if( serverLogic->getStatus() != ServerLogic::NO_ERROR )
		dlMsg.setInformativeText( "Errors occurred.\nSee details for more info.");
	else
		dlMsg.setInformativeText( "Finished");

	dlMsg.setStandardButtons( QMessageBox::Ok );
	dlMsg.setDefaultButton( QMessageBox::Ok );

	dlMsg.exec();
}


void MapPackagesWidget::handleProgress( QString text )
{
	if( d->progress == NULL || d->progress->wasCanceled() )
		return;

	qDebug() << text;
	d->progress->setLabelText( text );
	d->progressDetails.append( text + '\n' );

	int newProgressValue = d->progress->value() + 1;
	d->progress->setValue( newProgressValue );

	if( newProgressValue == d->progress->maximum() )
	{
		ServerLogic::OPERATION_TYPE operation = d->serverLogic->getOp();

		switch( operation )
		{
			case ServerLogic::PACKAGE_DL:
			{
				d->populateInstalled( m_ui->installedList );
				if( m_ui->updateList->count() > 0 )
					d->populateUpdatable( m_ui->updateList );
				break;
			}

			case ServerLogic::PACKAGE_CHK:
			{
				disconnect( d->serverLogic, SIGNAL( loadedList() ), d->serverLogic, SLOT( checkPackage() ) );
				connect( d->serverLogic, SIGNAL( loadedList() ), this, SLOT( populateServerPackageList() ) );
				d->populateUpdatable( m_ui->updateList );
				break;
			}

			default:
				break;
		}

		d->showFinishDialog();
	}
}

void MapPackagesWidget::cleanUp( ServerLogic::ERROR_TYPE type, QString message )
{
	d->progressDetails.append( message );

	if( type == ServerLogic::LIST_DL_ERROR )
		return;

	if( d->progress != NULL )
	{
		d->progress->cancel();
		d->progress->deleteLater();
		d->progress = NULL;
	}

	d->showFinishDialog();
}
