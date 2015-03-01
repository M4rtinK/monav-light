#include <QQuickView>
#include <QQmlContext>
#include <QGuiApplication>

#include <sailfishapp.h>

#include "client/paintwidget.h"
#include "client/worldmapchooser.h"

#include "Mainview.h"
#include "TapMenu.h"
#include "MapPackages.h"
#include "Bookmarks.h"

Q_IMPORT_PLUGIN( MapnikRendererClient );
Q_IMPORT_PLUGIN( ContractionHierarchiesClient );
Q_IMPORT_PLUGIN( GPSGridClient );
Q_IMPORT_PLUGIN( UnicodeTournamentTrieClient );
Q_IMPORT_PLUGIN( OSMRendererClient );
Q_IMPORT_PLUGIN( QtileRendererClient );

int main(int argc, char **argv)
{
	QGuiApplication *app = SailfishApp::application(argc, argv);
	
    qmlRegisterType<Mainview>("harbour.monav", 1, 0, "Mainview");
    qmlRegisterType<TapMenu>("harbour.monav", 1, 0, "TapMenu");
    qmlRegisterType<MapPackages>("harbour.monav", 1, 0, "MapPackages");
    qmlRegisterType<Bookmarks>("harbour.monav", 1, 0, "Bookmarks");
    qmlRegisterType<PaintWidget>("harbour.monav", 1, 0, "PaintWidget");
    qmlRegisterType<WorldMapChooser>("harbour.monav", 1, 0, "WorldMapChooser");

	QQuickView *view = SailfishApp::createView();
	view->setSource(SailfishApp::pathTo("qml/Main.qml"));
	view->show();
	view->setResizeMode(QQuickView::SizeRootObjectToView);
	QObject *object = (QObject *) view->rootObject();
	
	Mainview *mainview = object->findChild<Mainview *>("mainview");
	mainview->init();
	
	app->connect( app, SIGNAL(aboutToQuit()), MapData::instance(), SLOT(cleanup()) );
	app->connect( app, SIGNAL(aboutToQuit()), RoutingLogic::instance(), SLOT(cleanup()) );
	app->connect( app, SIGNAL(aboutToQuit()), Logger::instance(), SLOT(cleanup()) );
	
	app->connect( app, SIGNAL(aboutToQuit()), mainview, SLOT(cleanup()) );

	return app->exec();
}
