#include "MapPackages.h"

#include "client/worldmapchooser.h"

#include <QSettings>
#include <QGuiApplication>
#include <QQuickView>

MapPackages::MapPackages(QObject *parent) : QObject(parent) {

}

MapPackages::~MapPackages() {

}

void MapPackages::loadMapPackages() {
	QSettings settings( "MoNavClient" );
	settings.beginGroup( "MapPackages" );
	QString path = settings.value( "path", "/home/nemo/monavmaps" ).toString();
	qDebug() << "searching map packages from:" << path;
	
	maps.clear();

	MapData* mapData = MapData::instance();
	if ( !mapData->searchForMapPackages( path, &maps, 2 ) )
		return;
		
	int selected = -1;
	QStringList list;
	
	for ( int i = 0; i < maps.size(); i++ ) {
		list << maps[i].name;
		if ( maps[i].path == mapData->path() ) {
			selected = i;
		}
	}
	
	WorldMapChooser *worldmapchooser = 0;
	QGuiApplication *app = qobject_cast<QGuiApplication *>(QCoreApplication::instance());
	for (unsigned i = 0; i < app->topLevelWindows().size(); i++) {
		QQuickView *view = qobject_cast<QQuickView *>(app->topLevelWindows()[i]);
		if (!view) {
			continue;
		}
		QObject *object = view->rootObject();
		if (!object) {
			continue;
		}
		worldmapchooser = object->findChild<WorldMapChooser *>("worldmapchooser");
	}
	
	if (worldmapchooser) {
		worldmapchooser->setMaps( maps );
		worldmapchooser->setHighlight( selected );
	}
	
	QStringList routinglist;
	QStringList renderinglist;
	QStringList addresslookuplist;
	
	QVector<MapData::Module> routingmodules = mapData->modules(MapData::Routing);
	for (int i = 0; i < routingmodules.size(); i++) {
		qDebug() << routingmodules[i].name;
		routinglist << routingmodules[i].name;
	}
	
	QVector<MapData::Module> renderingmodules = mapData->modules(MapData::Rendering);
	for (int i = 0; i < renderingmodules.size(); i++) {
		qDebug() << renderingmodules[i].name;
		renderinglist << renderingmodules[i].name;
	}
	
	QVector<MapData::Module> addresslookupmodules = mapData->modules(MapData::AddressLookup);
	for (int i = 0; i < addresslookupmodules.size(); i++) {
		qDebug() << addresslookupmodules[i].name;
		addresslookuplist << addresslookupmodules[i].name;
	}
	
	emit mapPackagesLoaded(list, selected, routinglist, renderinglist, addresslookuplist);
}

void MapPackages::changeMapPackage(int selected) {
	MapData* mapData = MapData::instance();
	mapData->setPath( maps[selected].path );
	if ( !mapData->loadInformation() )
		return;
		
	loadMapPackages();
}

void MapPackages::loadMapModules(int routingindex, int renderingindex, int addresslookupindex) {
	qDebug() << "loading map modules...";
	MapData* mapData = MapData::instance();
	
	if (mapData->modules(MapData::Routing).size() == 0 || mapData->modules(MapData::Rendering).size() == 0 || mapData->modules(MapData::AddressLookup).size() == 0) {
		qDebug() << "some map modules could not be found!";
		return;
	}
	
	MapData::Module routing = mapData->modules(MapData::Routing)[routingindex];
	MapData::Module rendering = mapData->modules(MapData::Rendering)[renderingindex];
	MapData::Module addresslookup = mapData->modules(MapData::AddressLookup)[addresslookupindex];
	
	if (!mapData->load(routing, rendering, addresslookup)) {
		qDebug() << "loading map modules failed!";
	}
}

QStringList MapPackages::getCurrentModuleNames() {
	MapData* mapData = MapData::instance();
	QStringList list;
	QString lastRouting;
	QString lastRendering;
	QString lastAddressLookup;
	mapData->lastModules( &lastRouting, &lastRendering, &lastAddressLookup );
	list << lastRouting << lastRendering << lastAddressLookup;
	return list;
}
