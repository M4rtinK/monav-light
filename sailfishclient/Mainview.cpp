#include "Mainview.h"

#include "client/globalsettings.h"
#include "client/mapdata.h"
#include "client/paintwidget.h"

#include <QSettings>
#include <QGuiApplication>
#include <QQuickView>
#include <QtDBus>

Mainview::Mainview(QObject *parent) : QObject(parent) {
	activation_change_event_filter = new ActivationChangeEventFilter(this);
	navigation = false;
}

Mainview::~Mainview() {
	delete activation_change_event_filter;
}

template <typename T> T Mainview::findObject(QString name) {
	T object = 0;
	QGuiApplication *app = qobject_cast<QGuiApplication *>(QCoreApplication::instance());
	for (unsigned i = 0; i < app->topLevelWindows().size(); i++) {
		QQuickView *view = qobject_cast<QQuickView *>(app->topLevelWindows()[i]);
		if (!view) {
			continue;
		}
		QObject *root = view->rootObject();
		if (!root) {
			continue;
		}
		object = root->findChild<T>(name);
	}
	return object;
}

void Mainview::init() {
	QSettings settings( "MoNavClient" );
	GlobalSettings::loadSettings( &settings );
	
	connectSlots();
	
	MapData* mapData = MapData::instance();

	if ( mapData->loadInformation() )
		mapData->loadLast();
		
	if ( !mapData->informationLoaded() || !mapData->loaded() ) {
		emit mapDataLoadFailed();
	}
}

void Mainview::connectSlots() {
	MapData* mapData = MapData::instance();
	
	connect( mapData, SIGNAL(dataLoaded()), this, SLOT(dataLoaded()) );
	
	connect( RoutingLogic::instance(), SIGNAL(instructionsChanged()), this, SLOT(instructionsChanged()) );

	connect(activation_change_event_filter, SIGNAL(applicationActive()), this, SLOT(updateDisplayBlankingPause()));
	QCoreApplication::instance()->installEventFilter(activation_change_event_filter);
}

void Mainview::cleanup() {
	QSettings settings( "MoNavClient" );
	GlobalSettings::saveSettings( &settings );
}

void Mainview::dataLoaded() {
	IRenderer* renderer = MapData::instance()->renderer();
	if ( renderer == NULL )
		return;
	
	PaintWidget *paintwidget = findObject<PaintWidget *>("paintwidget");
	
	if (!paintwidget) {
		return;
	}

	int maxZoom = renderer->GetMaxZoom();
	paintwidget->setMaxZoom( maxZoom );
	setZoom( GlobalSettings::zoomMainMap() );
	paintwidget->setVirtualZoom( GlobalSettings::magnification() );
	paintwidget->setCenter( RoutingLogic::instance()->source().ToProjectedCoordinate() );
	paintwidget->setKeepPositionVisible( true );
}

void Mainview::addZoom()
{
	setZoom( GlobalSettings::zoomMainMap() + 1 );
}

void Mainview::subtractZoom()
{
	setZoom( GlobalSettings::zoomMainMap() - 1 );
}

void Mainview::setNavigation(bool navigation) {
	PaintWidget *paintwidget = findObject<PaintWidget *>("paintwidget");
	
	if (paintwidget) {
		paintwidget->setFixed(navigation);
	}
	
	this->navigation = navigation;
	updateDisplayBlankingPause();
}

void Mainview::instructionsChanged()
{
	/*if ( !d->fixed )
		return;*/

	QStringList label;
	QStringList icon;

	RoutingLogic::instance()->instructions( &label, &icon, 60 );
	
	emit instructionsLoaded(label, icon);
}

void Mainview::updateDisplayBlankingPause() {
	QDBusConnection bus = QDBusConnection::systemBus();
	QDBusInterface *interface = new QDBusInterface("com.nokia.mce", "/com/nokia/mce/request", "com.nokia.mce.request", bus, this);
	if (navigation) {
		interface->call("req_display_blanking_pause");
	} else {
		interface->call("req_display_cancel_blanking_pause");
	}
}

void Mainview::checkMapData() {
	MapData* mapData = MapData::instance();
	if ( !mapData->informationLoaded() || !mapData->loaded() ) {
		emit mapDataLoadFailed();
	}
}

void Mainview::pageActivating() {
	PaintWidget *paintwidget = findObject<PaintWidget *>("paintwidget");
	
	if (paintwidget) {
		paintwidget->update();
	}
}

void Mainview::setZoom( int zoom ) {
	IRenderer* renderer = MapData::instance()->renderer();
	if ( renderer == NULL )
		return;
	if( zoom > renderer->GetMaxZoom() )
		zoom = renderer->GetMaxZoom();
	if( zoom < 0 )
		zoom = 0;

	PaintWidget *paintwidget = findObject<PaintWidget *>("paintwidget");
	
	if (!paintwidget) {
		return;
	}

	paintwidget->setZoom( zoom );
	paintwidget->setVirtualZoom( 1 );
	GlobalSettings::setZoomMainMap( zoom );
}
