#include "Bookmarks.h"

#include "utils/coordinates.h"
#include "client/routinglogic.h"

#include <QSettings>

Bookmarks::Bookmarks(QObject *parent) : QObject(parent) {

}

Bookmarks::~Bookmarks() {
	
}

void Bookmarks::addBookmark(QString name) {
	QSettings settings( "MoNavClient" );
	settings.beginGroup( "Bookmarks" );

	QStringList names = settings.value( "names" ).toStringList();
	names.push_back( name );
	settings.setValue( "names", names );

	UnsignedCoordinate pos = RoutingLogic::instance()->target();
	settings.setValue( QString( "%1.coordinates.x" ).arg( names.size() - 1 ), pos.x );
	settings.setValue( QString( "%1.coordinates.y" ).arg( names.size() - 1 ), pos.y );
}

void Bookmarks::setBookmark(int index) {
	QSettings settings( "MoNavClient" );
	settings.beginGroup( "Bookmarks" );
	
	UnsignedCoordinate pos;
	pos.x = settings.value( QString( "%1.coordinates.x" ).arg( index ), 0 ).toUInt();
	pos.y = settings.value( QString( "%1.coordinates.y" ).arg( index ), 0 ).toUInt();
	
	RoutingLogic::instance()->setTarget( pos );
}

void Bookmarks::delBookmark(int index) {
	QSettings settings( "MoNavClient" );
	settings.beginGroup( "Bookmarks" );

	QStringList names = settings.value( "names" ).toStringList();
	
	for ( int i = index; i < names.size() - 1; i++ ) {
		UnsignedCoordinate pos;
		pos.x = settings.value( QString( "%1.coordinates.x" ).arg( i + 1 ), 0 ).toUInt();
		pos.y = settings.value( QString( "%1.coordinates.y" ).arg( i + 1 ), 0 ).toUInt();
		settings.setValue( QString( "%1.coordinates.x" ).arg( i ), pos.x );
		settings.setValue( QString( "%1.coordinates.y" ).arg( i ), pos.y );
	}
	settings.remove( QString( "%1.coordinates.x" ).arg( names.size() - 1 ) );
	settings.remove( QString( "%1.coordinates.y" ).arg( names.size() - 1 ) );
	
	names.removeAt(index);
	settings.setValue( "names", names );
}

QStringList Bookmarks::getBookmarks() {
	QSettings settings( "MoNavClient" );
	settings.beginGroup( "Bookmarks" );
	QStringList names = settings.value( "names" ).toStringList();
	return names;
}
