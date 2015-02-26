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

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "mapdata.h"
#include "mappackageswidget.h"
#include "mapmoduleswidget.h"
#include "overlaywidget.h"
#include "generalsettingsdialog.h"

#include "addressdialog.h"
#include "bookmarksdialog.h"

#include "routinglogic.h"
#include "routedescriptiondialog.h"
#include "gpsdialog.h"
#include "globalsettings.h"

#include <QtDebug>
#include <QInputDialog>
#include <QSettings>
#include <QGridLayout>
#include <QResizeEvent>
#include <QMessageBox>
#include <QStyle>
#include <QScrollArea>
#include <QSignalMapper>

#ifdef Q_WS_MAEMO_5
	#include "fullscreenexitbutton.h"
	#include <QtGui/QX11Info>
	#include <X11/Xlib.h>
	#include <X11/Xatom.h>
#endif

struct MainWindow::PrivateImplementation {

	enum Mode {
		Source, Target, POI, NoSelection
	};

	OverlayWidget* targetOverlay;
	OverlayWidget* sourceOverlay;
	OverlayWidget* gotoOverlay;
	OverlayWidget* settingsOverlay;
	OverlayWidget* toolsOverlay;

	QMenu* targetMenu;
	QMenu* sourceMenu;
	QMenu* gotoMenu;
	QMenu* settingsMenu;
	QMenu* toolsMenu;

	QSignalMapper* waypointMapper;

	int currentWaypoint;

	int maxZoom;
	bool fixed;
	Mode mode;

	void setupMenu();
	void resizeIcons();
};

MainWindow::MainWindow( QWidget* parent ) :
		QMainWindow( parent ),
		m_ui( new Ui::MainWindow )
{
	m_ui->setupUi( this );
	d = new PrivateImplementation;

	setupMenu();
	m_ui->zoomBar->hide();
	m_ui->infoWidget->hide();
	m_ui->tapMode->hide();
	m_ui->paintArea->setKeepPositionVisible( true );

	// ensure that we're painting our background
	setAutoFillBackground(true);

	d->mode = PrivateImplementation::NoSelection;
	d->fixed = false;

	QSettings settings( "MoNavClient" );
	// explicitly look for geometry as out own might not be initialized properly yet.
	if ( settings.contains( "geometry" ) )
		setGeometry( settings.value( "geometry" ).toRect() );
	GlobalSettings::loadSettings( &settings );

	resizeIcons();
	connectSlots();
	waypointsChanged();

#ifdef Q_WS_MAEMO_5
	grabZoomKeys( true );
	new FullScreenExitButton(this);
	showFullScreen();
#endif

	MapData* mapData = MapData::instance();

	if ( mapData->loadInformation() )
		mapData->loadLast();

	if ( !mapData->informationLoaded() ) {
		displayMapChooser();
	} else if ( !mapData->loaded() ) {
		displayModuleChooser();
	}
}

MainWindow::~MainWindow()
{
	QSettings settings( "MoNavClient" );
	settings.setValue( "geometry", geometry() );
	GlobalSettings::saveSettings( &settings );

	delete d;
	delete m_ui;
}

void MainWindow::connectSlots()
{
	MapData* mapData = MapData::instance();
	connect( m_ui->zoomBar, SIGNAL(valueChanged(int)), this, SLOT(setZoom(int)) );
	connect( m_ui->paintArea, SIGNAL(zoomChanged(int)), this, SLOT(setZoom(int)) );
	connect( m_ui->paintArea, SIGNAL(mouseClicked(ProjectedCoordinate)), this, SLOT(mouseClicked(ProjectedCoordinate)) );
	connect( m_ui->zoomIn, SIGNAL(clicked()), this, SLOT(addZoom()) );
	connect( m_ui->zoomOut, SIGNAL(clicked()), this, SLOT(subtractZoom()) );

	connect( m_ui->infoIcon1, SIGNAL(clicked()), this, SLOT(showInstructions()) );
	connect( m_ui->infoIcon2, SIGNAL(clicked()), this, SLOT(showInstructions()) );

	connect( mapData, SIGNAL(informationChanged()), this, SLOT(informationLoaded()) );
	connect( mapData, SIGNAL(dataLoaded()), this, SLOT(dataLoaded()) );
	connect( RoutingLogic::instance(), SIGNAL(instructionsChanged()), this, SLOT(instructionsChanged()) );
	connect( RoutingLogic::instance(), SIGNAL(waypointsChanged()), this, SLOT(waypointsChanged()) );
	connect( m_ui->lockButton, SIGNAL(clicked()), this, SLOT(toggleLocked()) );

	connect( m_ui->tools, SIGNAL(clicked()), this, SLOT(toolsMenu()) );
	connect( m_ui->show, SIGNAL(clicked()), this, SLOT(gotoMenu()) );
	connect( m_ui->settings, SIGNAL(clicked()), this, SLOT(settingsMenu()) );

	d->waypointMapper = new QSignalMapper( this );
	connect( d->waypointMapper, SIGNAL(mapped(int)), SLOT(setWaypointID(int)) );
	connect( m_ui->source, SIGNAL(clicked()), d->waypointMapper, SLOT(map()) );
	d->waypointMapper->setMapping( m_ui->source, -1 );
	connect( m_ui->source, SIGNAL(clicked()), this, SLOT(sourceMenu()) );
	connect( m_ui->target, SIGNAL(clicked()), d->waypointMapper, SLOT(map()) );
	d->waypointMapper->setMapping( m_ui->target, 0 );
	connect( m_ui->target, SIGNAL(clicked()), this, SLOT(targetMenu()) );

	connect( m_ui->tapMode, SIGNAL(clicked()), this, SLOT(setModeNoSelection()) );
}

void MainWindow::setupMenu()
{
	d->gotoMenu = new QMenu( tr( "Show" ), this );
	d->gotoMenu->addAction( QIcon( ":/images/oxygen/network-wireless.png" ), tr( "GPS-Location" ), this, SLOT(gotoGpsLocation()) );
	d->gotoMenu->addAction( QIcon( ":/images/source.png" ), tr( "Departure" ), this, SLOT(gotoSource()) );
	d->gotoMenu->addAction( QIcon( ":/images/target.png" ), tr( "Destination" ), this, SLOT(gotoTarget()) );
	d->gotoMenu->addAction( QIcon( ":/images/oxygen/bookmarks.png" ), tr( "Bookmark..." ), this, SLOT(gotoBookmark()) );
	d->gotoMenu->addAction( QIcon( ":/images/address.png" ), tr( "Address..." ), this, SLOT(gotoAddress()) );
	d->gotoMenu->addAction( QIcon( ":/images/oxygen/network-wireless.png" ), tr( "GPS-Coordinate..." ), this, SLOT(gotoGpsCoordinate()) );

	d->gotoOverlay = new OverlayWidget( this, tr( "Show" ) );
	d->gotoOverlay->addActions( d->gotoMenu->actions() );

	d->sourceMenu = new QMenu( tr( "Departure" ), this );
	d->sourceMenu->addAction( QIcon( ":/images/map.png" ), tr( "Tap on Map" ), this, SLOT(setModeSourceSelection()) );
	d->sourceMenu->addAction( QIcon( ":/images/oxygen/bookmarks.png" ), tr( "Bookmark" ), this, SLOT(sourceByBookmark()) );
	d->sourceMenu->addAction( QIcon( ":/images/address.png" ), tr( "Address" ), this, SLOT(sourceByAddress()) );
	d->sourceMenu->addAction( QIcon( ":/images/oxygen/network-wireless.png" ), tr( "GPS-Location" ), this, SLOT(sourceByGPS()) );

	d->sourceOverlay = new OverlayWidget( this, tr( "Departure" ) );
	d->sourceOverlay->addActions( d->sourceMenu->actions() );

	d->targetMenu = new QMenu( tr( "Destination" ), this );
	d->targetMenu->addAction( QIcon( ":/images/map.png" ), tr( "Tap on Map" ), this, SLOT(setModeTargetSelection()) );
	d->targetMenu->addAction( QIcon( ":/images/oxygen/bookmarks.png" ), tr( "Bookmark" ), this, SLOT(targetByBookmark()) );
	d->targetMenu->addAction( QIcon( ":/images/address.png" ), tr( "Address" ), this, SLOT(targetByAddress()) );
	d->targetMenu->addSeparator();
	d->targetMenu->addAction( QIcon( ":/images/oxygen/list-add.png" ), tr( "+Waypoint" ), this, SLOT(addRoutepoint()) );
	d->targetMenu->addAction( QIcon( ":/images/oxygen/list-remove.png" ), tr( "-Waypoint" ), this, SLOT(subductRoutepoint()) );

	d->targetOverlay = new OverlayWidget( this, tr( "Destination" ) );
	d->targetOverlay->addActions( d->targetMenu->actions() );

	d->settingsMenu = new QMenu( tr( "Settings" ), this );
	d->settingsMenu->addAction( QIcon( ":/images/oxygen/folder-tar.png" ), tr( "Map Packages" ), this, SLOT(displayMapChooser()) );
	d->settingsMenu->addAction( QIcon( ":/images/oxygen/folder-tar.png" ), tr( "Map Modules" ), this, SLOT(displayModuleChooser()) );
	d->settingsMenu->addAction( QIcon( ":/images/oxygen/preferences-system.png" ), tr( "General" ), this, SLOT(settingsGeneral()) );
	d->settingsMenu->addAction( QIcon( ":/images/oxygen/network-wireless.png" ), tr( "GPS Lookup" ), this, SLOT(settingsGPSLookup()) );
	d->settingsMenu->addAction( QIcon( ":/images/map.png" ), tr( "Renderer" ), this, SLOT(settingsRenderer()) );
	d->settingsMenu->addAction( QIcon( ":/images/route.png" ), tr( "Router" ), this, SLOT(settingsRouter()) );
	d->settingsMenu->addAction( QIcon( ":/images/address.png" ), tr( "Address Lookup" ), this, SLOT(settingsAddressLookup()) );
	d->settingsMenu->addAction( QIcon( ":/images/oxygen/hwinfo.png" ), tr( "GPS Receiver" ), this, SLOT(settingsGPS()) );

	d->settingsOverlay = new OverlayWidget( this, tr( "Settings" ) );
	d->settingsOverlay->addActions( d->settingsMenu->actions() );

	d->toolsMenu = new QMenu( tr( "Tools" ), this );
	d->toolsMenu->addAction( QIcon( ":/images/directions/roundabout.png" ), tr( "Compute Round Trip" ), this, SLOT(computeRoundTrip()) );
	d->toolsMenu->addAction( QIcon( ":/images/oxygen/bookmarks.png" ), tr( "Bookmarks" ), this, SLOT(bookmarks()) );

	d->toolsOverlay = new OverlayWidget( this, tr( "Tools" ) );
	d->toolsOverlay->addActions( d->toolsMenu->actions() );
}

void MainWindow::resizeIcons()
{
	// a bit hackish right now
	// find all suitable children and resize their icons
	// TODO find cleaner way
	int iconSize = GlobalSettings::iconSize();
	foreach ( QToolButton* button, this->findChildren< QToolButton* >() )
		button->setIconSize( QSize( iconSize, iconSize ) );
	foreach ( QToolBar* button, this->findChildren< QToolBar* >() )
		button->setIconSize( QSize( iconSize, iconSize ) );
	m_ui->waypointsWidget->setMinimumSize( m_ui->waypointsWidget->widget()->sizeHint() );
}

void MainWindow::showInstructions()
{
	RouteDescriptionWidget* widget = new RouteDescriptionWidget( this );
	int index = m_ui->stacked->addWidget( widget );
	m_ui->stacked->setCurrentIndex( index );
	connect( widget, SIGNAL(closed()), widget, SLOT(deleteLater()) );
}

void MainWindow::displayMapChooser()
{
	MapPackagesWidget* widget = new MapPackagesWidget();
	int index = m_ui->stacked->addWidget( widget );
	m_ui->stacked->setCurrentIndex( index );
	widget->show();
	setWindowTitle( "MoNav - Map Packages" );
	connect( widget, SIGNAL(mapChanged()), this, SLOT(mapLoaded()) );
}

void MainWindow::displayModuleChooser()
{
	MapModulesWidget* widget = new MapModulesWidget();
	int index = m_ui->stacked->addWidget( widget );
	m_ui->stacked->setCurrentIndex( index );
	widget->show();
	setWindowTitle( "MoNav - Map Modules" );
	connect( widget, SIGNAL(selected()), this, SLOT(modulesLoaded()) );
	connect( widget, SIGNAL(cancelled()), this, SLOT(modulesCancelled()) );
}

void MainWindow::mapLoaded()
{
	MapData* mapData = MapData::instance();
	if ( !mapData->informationLoaded() )
		return;

	if ( m_ui->stacked->count() <= 1 )
		return;

	QWidget* widget = m_ui->stacked->currentWidget();
	widget->deleteLater();

	if ( !mapData->loadLast() )
		displayModuleChooser();
}

void MainWindow::modulesCancelled()
{
	if ( m_ui->stacked->count() <= 1 )
		return;

	QWidget* widget = m_ui->stacked->currentWidget();
	widget->deleteLater();

	MapData* mapData = MapData::instance();
	if ( !mapData->loaded() ) {
		if ( !mapData->loadLast() )
			displayMapChooser();
	}
}

void MainWindow::modulesLoaded()
{
	if ( m_ui->stacked->count() <= 1 )
		return;

	QWidget* widget = m_ui->stacked->currentWidget();
	widget->deleteLater();
}

void MainWindow::informationLoaded()
{
	MapData* mapData = MapData::instance();
	if ( !mapData->informationLoaded() )
		return;

	this->setWindowTitle( "MoNav - " + mapData->information().name );
}

void MainWindow::dataLoaded()
{
	IRenderer* renderer = MapData::instance()->renderer();
	if ( renderer == NULL )
		return;

	d->maxZoom = renderer->GetMaxZoom();
	m_ui->zoomBar->setMaximum( d->maxZoom );
	m_ui->paintArea->setMaxZoom( d->maxZoom );
	setZoom( GlobalSettings::zoomMainMap());
	m_ui->paintArea->setVirtualZoom( GlobalSettings::magnification() );
	m_ui->paintArea->setCenter( RoutingLogic::instance()->source().ToProjectedCoordinate() );
	m_ui->paintArea->setKeepPositionVisible( true );

	this->setWindowTitle( "MoNav - " + MapData::instance()->information().name );
}

void MainWindow::instructionsChanged()
{
	if ( !d->fixed )
		return;

	QStringList label;
	QStringList icon;

	RoutingLogic::instance()->instructions( &label, &icon, 60 );

	m_ui->infoWidget->setHidden( label.empty() );

	if ( label.isEmpty() )
		return;

	m_ui->infoLabel1->setText( label[0] );
	m_ui->infoIcon1->setIcon( QIcon( icon[0] ) );

	m_ui->infoIcon2->setHidden( label.size() == 1 );
	m_ui->infoLabel2->setHidden( label.size() == 1 );

	if ( label.size() == 1 )
		return;

	m_ui->infoLabel2->setText( label[1] );
	m_ui->infoIcon2->setIcon( QIcon( icon[1] ) );
}

// SETTINGS

void MainWindow::settingsGeneral()
{
	GeneralSettingsDialog* window = new GeneralSettingsDialog;
	window->exec();
	delete window;
	resizeIcons();
	m_ui->paintArea->setVirtualZoom( GlobalSettings::magnification() );
}

void MainWindow::settingsRenderer()
{
	IRenderer* renderer = MapData::instance()->renderer();
	if ( renderer != NULL )
		renderer->ShowSettings();
}

void MainWindow::settingsRouter()
{
	IRouter* router = MapData::instance()->router();
	if ( router != NULL )
		router->ShowSettings();
}

void MainWindow::settingsGPSLookup()
{
	IGPSLookup* gpsLookup = MapData::instance()->gpsLookup();
	if( gpsLookup != NULL )
		gpsLookup->ShowSettings();
}

void MainWindow::settingsAddressLookup()
{
	IAddressLookup* addressLookup = MapData::instance()->addressLookup();
	if ( addressLookup != NULL )
		addressLookup->ShowSettings();
}

void MainWindow::settingsGPS()
{
	GPSDialog* window = new GPSDialog( this );
	window->exec();
	delete window;
}

// MENUES

void MainWindow::toolsMenu()
{
	if ( GlobalSettings::menuMode() == GlobalSettings::MenuPopup ) {
		QPoint position = m_ui->show->mapToGlobal( QPoint( m_ui->show->width() / 2, m_ui->show->height() / 2 ) );
		d->toolsMenu->exec( position );
	} else {
		d->toolsOverlay->show();
	}
}

void MainWindow::gotoMenu()
{
	if ( GlobalSettings::menuMode() == GlobalSettings::MenuPopup ) {
		QPoint position = m_ui->show->mapToGlobal( QPoint( m_ui->show->width() / 2, m_ui->show->height() / 2 ) );
		d->gotoMenu->exec( position );
	} else {
		d->gotoOverlay->show();
	}
}

void MainWindow::settingsMenu()
{
	if ( GlobalSettings::menuMode() == GlobalSettings::MenuPopup ) {
		QPoint position = m_ui->settings->mapToGlobal( QPoint( m_ui->settings->width() / 2, m_ui->settings->height() / 2 ) );
		d->settingsMenu->exec( position );
	} else {
		d->settingsOverlay->show();
	}
}

void MainWindow::sourceMenu()
{
	if ( GlobalSettings::menuMode() == GlobalSettings::MenuPopup ) {
		QPoint position = m_ui->source->mapToGlobal( QPoint( m_ui->source->width() / 2, m_ui->source->height() / 2 ) );
		d->sourceMenu->exec( position );
	} else {
		d->sourceOverlay->show();
	}
}
void MainWindow::targetMenu()
{
	if ( GlobalSettings::menuMode() == GlobalSettings::MenuPopup ) {
		QPoint position = m_ui->target->mapToGlobal( QPoint( m_ui->target->width() / 2, m_ui->target->height() / 2 ) );
		d->targetMenu->exec( position );
	} else {
		d->targetOverlay->show();
	}
}

// EVENTS

void MainWindow::resizeEvent( QResizeEvent* event )
{
	QBoxLayout* box = qobject_cast< QBoxLayout* >( m_ui->infoWidget->layout() );
	assert ( box != NULL );
	if ( event->size().width() > event->size().height() )
		box->setDirection( QBoxLayout::LeftToRight );
	else
		box->setDirection( QBoxLayout::TopToBottom );
}

#ifdef Q_WS_MAEMO_5
void MainWindow::grabZoomKeys( bool grab )
{
	if ( !winId() ) {
		qWarning() << "Can't grab keys unless we have a window id";
		return;
	}

	unsigned long val = ( grab ) ? 1 : 0;
	Atom atom = XInternAtom( QX11Info::display(), "_HILDON_ZOOM_KEY_ATOM", False );
	if ( !atom ) {
		qWarning() << "Unable to obtain _HILDON_ZOOM_KEY_ATOM. This will only work on a Maemo 5 device!";
		return;
	}

	XChangeProperty ( QX11Info::display(), winId(), atom, XA_INTEGER, 32, PropModeReplace, reinterpret_cast< unsigned char* >( &val ), 1 );
}

void MainWindow::keyPressEvent( QKeyEvent* event )
{
	switch (event->key()) {
	case Qt::Key_F7:
		addZoom();
		event->accept();
		break;

	case Qt::Key_F8:
		this->subtractZoom();
		event->accept();
		break;
	}

	QWidget::keyPressEvent(event);
}
#endif

// SLOTS

void MainWindow::setModeSourceSelection()
{
	m_ui->paintArea->setKeepPositionVisible( false );
	d->mode = PrivateImplementation::Source;
	m_ui->waypointsWidget->setVisible( false );
	m_ui->menuWidget->setVisible( false );
	m_ui->lockButton->setVisible( false );
	m_ui->tapMode->setVisible( true );
}

void MainWindow::setModeTargetSelection()
{
	m_ui->paintArea->setKeepPositionVisible( false );
	d->mode = PrivateImplementation::Target;
	m_ui->waypointsWidget->setVisible( false );
	m_ui->menuWidget->setVisible( false );
	m_ui->lockButton->setVisible( false );
	m_ui->tapMode->setVisible( true );
}

void MainWindow::setModeNoSelection()
{
	d->mode = PrivateImplementation::NoSelection;
	m_ui->waypointsWidget->setVisible( true );
	m_ui->menuWidget->setVisible( true );
	m_ui->lockButton->setVisible( true );
	m_ui->tapMode->setVisible( false );
}

void MainWindow::mouseClicked( ProjectedCoordinate clickPos )
{
	UnsignedCoordinate coordinate( clickPos );
	if ( d->mode == PrivateImplementation::Source ) {
		RoutingLogic::instance()->setSource( coordinate );
	} else if ( d->mode == PrivateImplementation::Target ) {
		RoutingLogic::instance()->setWaypoint( d->currentWaypoint, coordinate );
	} else if ( d->mode == PrivateImplementation::POI ){
		//m_selected = coordinate;
		//m_ui->paintArea->setPOI( coordinate );
		//return;
	}

	//m_mode = None; // might be contra-productiv for some use cases. E.g., many new users just want to click around the map and wonder about the blazingly fast routing *g*
}

void MainWindow::gotoGpsLocation()
{
	const RoutingLogic::GPSInfo& gpsInfo = RoutingLogic::instance()->gpsInfo();
	if ( !gpsInfo.position.IsValid() )
		return;
	GPSCoordinate gps( gpsInfo.position.ToGPSCoordinate().latitude, gpsInfo.position.ToGPSCoordinate().longitude );
	m_ui->paintArea->setCenter( ProjectedCoordinate( gps ) );
	m_ui->paintArea->setKeepPositionVisible( true );

	IRenderer* renderer = MapData::instance()->renderer();
	if ( renderer == NULL )
		return;
	setZoom( renderer->GetMaxZoom() - 5 );
}

void MainWindow::gotoSource()
{
	UnsignedCoordinate coordinate = RoutingLogic::instance()->source();
	if ( !coordinate.IsValid() )
		return;
	m_ui->paintArea->setCenter( coordinate.ToProjectedCoordinate() );
	m_ui->paintArea->setKeepPositionVisible( true );

	IRenderer* renderer = MapData::instance()->renderer();
	if ( renderer == NULL )
		return;
	setZoom( renderer->GetMaxZoom() - 5 );
}

void MainWindow::gotoTarget()
{
	UnsignedCoordinate coordinate = RoutingLogic::instance()->target();
	if ( !coordinate.IsValid() )
		return;
	m_ui->paintArea->setCenter( coordinate.ToProjectedCoordinate() );
	m_ui->paintArea->setKeepPositionVisible( false );

	IRenderer* renderer = MapData::instance()->renderer();
	if ( renderer == NULL )
		return;
	setZoom( renderer->GetMaxZoom() - 5 );
}

void MainWindow::gotoBookmark()
{
	UnsignedCoordinate result;
	if ( !BookmarksDialog::showBookmarks( &result, this ) )
		return;
	m_ui->paintArea->setCenter( result.ToProjectedCoordinate() );
	m_ui->paintArea->setKeepPositionVisible( false );

	IRenderer* renderer = MapData::instance()->renderer();
	if ( renderer == NULL )
		return;
	setZoom( renderer->GetMaxZoom() - 5 );
}

void MainWindow::gotoAddress()
{
	if ( MapData::instance()->addressLookup() == NULL )
		return;
	UnsignedCoordinate result;
	if ( !AddressDialog::getAddress( &result, this, true ) )
		return;
	m_ui->paintArea->setCenter( result.ToProjectedCoordinate() );
	m_ui->paintArea->setKeepPositionVisible( false );

	IRenderer* renderer = MapData::instance()->renderer();
	if ( renderer == NULL )
		return;
	setZoom( renderer->GetMaxZoom() - 5 );
}

void MainWindow::gotoGpsCoordinate()
{
	bool ok = false;
	double latitude = QInputDialog::getDouble( this, "Enter Coordinate", "Enter Latitude", 0, -90, 90, 5, &ok );
	if ( !ok )
		return;
	double longitude = QInputDialog::getDouble( this, "Enter Coordinate", "Enter Longitude", 0, -180, 180, 5, &ok );
	if ( !ok )
		return;
	GPSCoordinate gps( latitude, longitude );
	m_ui->paintArea->setCenter( ProjectedCoordinate( gps ) );
	m_ui->paintArea->setKeepPositionVisible( false );

	IRenderer* renderer = MapData::instance()->renderer();
	if ( renderer == NULL )
		return;
	setZoom( renderer->GetMaxZoom() - 5 );
}

void MainWindow::sourceByBookmark()
{
	UnsignedCoordinate result;
	if ( !BookmarksDialog::showBookmarks( &result, this ) )
		return;
	RoutingLogic::instance()->setSource( result );
	m_ui->paintArea->setCenter( result.ToProjectedCoordinate() );
}

void MainWindow::sourceByAddress()
{
	if ( MapData::instance()->addressLookup() == NULL )
		return;
	UnsignedCoordinate result;
	if ( !AddressDialog::getAddress( &result, this ) )
		return;
	RoutingLogic::instance()->setSource( result );
	m_ui->paintArea->setCenter( result.ToProjectedCoordinate() );
}

void MainWindow::sourceByGPS()
{
	RoutingLogic::instance()->setGPSLink( true );
}

void MainWindow::setWaypointID( int id )
{
	d->currentWaypoint = id;
}

void MainWindow::targetByBookmark()
{
	UnsignedCoordinate result;
	if ( !BookmarksDialog::showBookmarks( &result, this ) )
		return;
	RoutingLogic::instance()->setWaypoint( d->currentWaypoint, result );
	m_ui->paintArea->setCenter( result.ToProjectedCoordinate() );
}

void MainWindow::targetByAddress()
{
	if ( MapData::instance()->addressLookup() == NULL )
		return;
	UnsignedCoordinate result;
	if ( !AddressDialog::getAddress( &result, this ) )
		return;
	RoutingLogic::instance()->setWaypoint( d->currentWaypoint, result );
	m_ui->paintArea->setCenter( result.ToProjectedCoordinate() );
}

void MainWindow::subductRoutepoint()
{
	RoutingLogic* routingLogic = RoutingLogic::instance();
	QVector< UnsignedCoordinate > waypoints = routingLogic->waypoints();
	if ( d->currentWaypoint >= waypoints.size() )
		return;
	waypoints.remove( d->currentWaypoint );
	routingLogic->setWaypoints( waypoints );
}

void MainWindow::addRoutepoint()
{
	RoutingLogic* routingLogic = RoutingLogic::instance();
	QVector< UnsignedCoordinate > waypoints = routingLogic->waypoints();
	if ( waypoints.empty() )
		waypoints.resize( d->currentWaypoint );
	waypoints.insert( d->currentWaypoint, UnsignedCoordinate() );
	routingLogic->setWaypoints( waypoints );
}

void MainWindow::bookmarks()
{
	UnsignedCoordinate result;
	if ( !BookmarksDialog::showBookmarks( &result, this ) )
		return;

	m_ui->paintArea->setCenter( result.ToProjectedCoordinate() );
}

void MainWindow::addZoom()
{
	setZoom( GlobalSettings::zoomMainMap() + 1 );
}

void MainWindow::subtractZoom()
{
	setZoom( GlobalSettings::zoomMainMap() - 1 );
}

void MainWindow::setZoom( int zoom )
{
	IRenderer* renderer = MapData::instance()->renderer();
	if ( renderer == NULL )
		return;
	if( zoom > renderer->GetMaxZoom() )
		zoom = renderer->GetMaxZoom();
	if( zoom < 0 )
		zoom = 0;

	m_ui->zoomBar->setValue( zoom );
	m_ui->paintArea->setZoom( zoom );
	GlobalSettings::setZoomMainMap( zoom );
}

void MainWindow::toggleLocked()
{
	d->fixed = !d->fixed;
	m_ui->paintArea->setFixed( d->fixed );
	if ( d->fixed ) {
		m_ui->lockButton->setIcon( QIcon( ":images/oxygen/emblem-locked.png") );
		instructionsChanged();
	} else {
		m_ui->lockButton->setIcon( QIcon( ":images/oxygen/emblem-unlocked.png") );
		m_ui->infoWidget->hide();
	}
}

void MainWindow::waypointsChanged()
{
	QList< QToolButton* > waypointButtons = findChildren< QToolButton* >( "waypoint" );
	foreach( QToolButton* button, waypointButtons )
		button->deleteLater();
	QVector< UnsignedCoordinate > waypoints = RoutingLogic::instance()->waypoints();
	for ( int i = 0; i < waypoints.size() - 1; i++ )
	{
		int id = waypoints.size() - 1 - i;
		QToolButton* button = new QToolButton( NULL );
		if ( QFile::exists( QString( ":/images/waypoint%1.png" ).arg( id ) ) )
			button->setIcon( QIcon( QString( ":/images/waypoint%1.png" ).arg( id ) ) );
		else
			button->setIcon( QIcon( ":/images/target.png" ) );
		button->setIconSize( QSize( GlobalSettings::iconSize(), GlobalSettings::iconSize() ) );
		button->setAutoRaise( true );
		button->setObjectName( "waypoint" );
		m_ui->scrollAreaWidgetContents->layout()->addWidget( button );
		d->waypointMapper->setMapping( button, id - 1 );
		connect( button, SIGNAL(clicked()), d->waypointMapper, SLOT(map()) );
		connect( button, SIGNAL(clicked()), this, SLOT(targetMenu()) );
		button->show();
	}
	if ( !waypoints.empty() )
		d->waypointMapper->setMapping( m_ui->target, waypoints.size() - 1 );
	else
		d->waypointMapper->setMapping( m_ui->target, 0 );
	m_ui->scrollAreaWidgetContents->layout()->addWidget( m_ui->source );
}

void MainWindow::computeRoundTrip()
{
	if ( RoutingLogic::instance()->waypoints().size() > 5 )
	{
		int choice = QMessageBox::question( this, "Round Trip", "Computing the round trip might take a while if you selected a large amount of waypoints. Compute anyway?", QMessageBox::Yes, QMessageBox::No );
		if ( choice == 0 )
			return;
	}
	RoutingLogic::instance()->computeRoundtrip();
}
