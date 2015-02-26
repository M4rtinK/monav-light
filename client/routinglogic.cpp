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

#include "routinglogic.h"
#include "descriptiongenerator.h"
#include "mapdata.h"
#include "utils/qthelpers.h"
#include "logger.h"
#ifndef NOQTMOBILE
#include "gpsdpositioninfosource.h"
#endif

#include <limits>
#include <algorithm>
#include <QtDebug>
#include <QSettings>
#include <QDir>

struct RoutingLogic::PrivateImplementation {
	GPSInfo gpsInfo;
	UnsignedCoordinate source;
	QVector< UnsignedCoordinate > waypoints;
	QVector< IRouter::Node > pathNodes;
	QVector< IRouter::Edge > pathEdges;
	double distance;
	double travelTime;
	DescriptionGenerator descriptionGenerator;
	QStringList labels;
	QStringList icons;
	bool linked;
#ifndef NOQTMOBILE
	// the current GPS source
	QGeoPositionInfoSource* gpsSource;
#endif
};

RoutingLogic::RoutingLogic() :
		d( new PrivateImplementation )
{
	d->linked = true;
	d->distance = -1;
	d->travelTime = -1;

	d->gpsInfo.altitude = -1;
	d->gpsInfo.groundSpeed = -1;
	d->gpsInfo.verticalSpeed = -1;
	d->gpsInfo.heading = -1;
	d->gpsInfo.horizontalAccuracy = -1;
	d->gpsInfo.verticalAccuracy = -1;

	QSettings settings( "MoNavClient" );
	settings.beginGroup( "Routing" );
	d->source.x = settings.value( "source.x", d->source.x ).toUInt();
	d->source.y = settings.value( "source.y", d->source.y ).toUInt();

#ifndef NOQTMOBILE
	d->gpsSource = QGeoPositionInfoSource::createDefaultSource( this );
	if ( d->gpsSource == NULL )
		d->gpsSource = GpsdPositionInfoSource::create( this );
	if ( d->gpsSource == NULL ) {
		qDebug() << "No GPS Sensor found! GPS Updates are not available";
	} else {
		// prevent QtMobility from cheating
		// documemenation states that it would provide updates as fast as possible if nothing was specified
		// nevertheless, it did provide only one every 5 seconds on the N900
		// with this setting on every second
		d->gpsSource->setUpdateInterval( 1000 );
		d->gpsSource->startUpdates();
		connect( d->gpsSource, SIGNAL(positionUpdated(QGeoPositionInfo)), this, SLOT(positionUpdated(QGeoPositionInfo)) );
	}
#endif

	for ( int i = 0; i < settings.value( "amountRoutepoints", 0 ).toInt(); i++ ) {
		UnsignedCoordinate coordinate;
		bool ok = true;
		coordinate.x = settings.value( QString( "routepointx%1" ).arg( i + 1 ), coordinate.x ).toUInt( &ok );
		if ( !ok )
			continue;
		coordinate.y = settings.value( QString( "routepointy%1" ).arg( i + 1 ), coordinate.y ).toUInt( &ok );
		if ( !ok )
			continue;
		if ( coordinate.IsValid() )
			d->waypoints.append( coordinate );
	}

	connect( this, SIGNAL(gpsInfoChanged()), Logger::instance(), SLOT(positionChanged()) );
	connect( MapData::instance(), SIGNAL(dataLoaded()), this, SLOT(dataLoaded()) );

	computeRoute();
	emit waypointsChanged();
}

RoutingLogic::~RoutingLogic()
{
	delete d;
}

void RoutingLogic::cleanup()
{
	disconnect();
#ifndef NOQTMOBILE
	if ( d->gpsSource != NULL )
		d->gpsSource->deleteLater();
#endif
	QSettings settings( "MoNavClient" );
	settings.beginGroup( "Routing" );
	settings.setValue( "source.x", d->source.x );
	settings.setValue( "source.y", d->source.y );
	settings.setValue( "amountRoutepoints", d->waypoints.size() );
	for ( int i = 0; i < d->waypoints.size(); i++ ){
		settings.setValue( QString( "routepointx%1" ).arg( i + 1 ), d->waypoints[i].x );
		settings.setValue( QString( "routepointy%1" ).arg( i + 1 ), d->waypoints[i].y );
	}
}

RoutingLogic* RoutingLogic::instance()
{
	static RoutingLogic routingLogic;
	return &routingLogic;
}

#ifndef NOQTMOBILE
void RoutingLogic::positionUpdated( const QGeoPositionInfo& update )
{
	if ( !update.isValid() )
		return;

	GPSCoordinate gps;
	gps.latitude = update.coordinate().latitude();
	gps.longitude = update.coordinate().longitude();
	d->gpsInfo.position = UnsignedCoordinate( gps );
	d->gpsInfo.altitude = update.coordinate().altitude();
	d->gpsInfo.timestamp = update.timestamp();
	if ( update.hasAttribute( QGeoPositionInfo::Direction ) )
		d->gpsInfo.heading = update.attribute( QGeoPositionInfo::Direction );
	if ( update.hasAttribute( QGeoPositionInfo::GroundSpeed ) )
		d->gpsInfo.groundSpeed = update.attribute( QGeoPositionInfo::GroundSpeed );
	if ( update.hasAttribute( QGeoPositionInfo::VerticalSpeed ) )
		d->gpsInfo.verticalSpeed = update.attribute( QGeoPositionInfo::VerticalSpeed );
	if ( update.hasAttribute( QGeoPositionInfo::HorizontalAccuracy ) )
		d->gpsInfo.horizontalAccuracy = update.attribute( QGeoPositionInfo::HorizontalAccuracy );
	if ( update.hasAttribute( QGeoPositionInfo::VerticalAccuracy ) )
		d->gpsInfo.verticalAccuracy = update.attribute( QGeoPositionInfo::VerticalAccuracy );

	if ( d->linked ) {
		d->source = d->gpsInfo.position;
		emit sourceChanged();
		computeRoute();
	}

	emit gpsInfoChanged();
}
#endif

QVector< UnsignedCoordinate > RoutingLogic::waypoints() const
{
	return d->waypoints;
}

UnsignedCoordinate RoutingLogic::source() const
{
	return d->source;
}

UnsignedCoordinate RoutingLogic::target() const
{
	if ( d->waypoints.empty() )
		return UnsignedCoordinate();
	return d->waypoints.last();
}

bool RoutingLogic::gpsLink() const
{
	return d->linked;
}

const RoutingLogic::GPSInfo RoutingLogic::gpsInfo() const
{
	return d->gpsInfo;
}

QVector< IRouter::Node > RoutingLogic::route() const
{
	return d->pathNodes;
}

void RoutingLogic::clear()
{
	d->waypoints.clear();
	computeRoute();
}

void RoutingLogic::instructions( QStringList* labels, QStringList* icons, int maxSeconds )
{
	d->descriptionGenerator.reset();
	d->descriptionGenerator.descriptions( &d->icons, &d->labels, d->pathNodes, d->pathEdges, maxSeconds );
	*labels = d->labels;
	*icons = d->icons;
}

void RoutingLogic::setWaypoints( QVector<UnsignedCoordinate> waypoints )
{
	bool changed = waypoints != d->waypoints;
	d->waypoints = waypoints;
	if ( changed )
	{
		computeRoute();
		emit waypointsChanged();
	}
}

void RoutingLogic::setWaypoint( int id, UnsignedCoordinate coordinate )
{
	if ( d->waypoints.size() <= id )
		d->waypoints.resize( id + 1 );
	d->waypoints[id] = coordinate;

	while ( !d->waypoints.empty() && !d->waypoints.back().IsValid() )
		d->waypoints.pop_back();

	computeRoute();

	emit waypointsChanged();
}

void RoutingLogic::setSource( UnsignedCoordinate coordinate )
{
	setGPSLink( false );
	d->source = coordinate;
	computeRoute();
	emit sourceChanged();
}

void RoutingLogic::setTarget( UnsignedCoordinate target )
{
	int index = d->waypoints.empty() ? 0 : d->waypoints.size() - 1;
	setWaypoint( index, target );
}

void RoutingLogic::setGPSLink( bool linked )
{
	if ( linked == d->linked )
		return;
	d->linked = linked;
	if ( d->gpsInfo.position.IsValid() ) {
		d->source = d->gpsInfo.position;
		emit sourceChanged();
		computeRoute();
	}
	emit gpsLinkChanged( d->linked );
}

void RoutingLogic::computeRoute()
{
	IGPSLookup* gpsLookup = MapData::instance()->gpsLookup();
	if ( gpsLookup == NULL )
		return;
	IRouter* router = MapData::instance()->router();
	if ( router == NULL )
		return;

	if ( !d->source.IsValid() ) {
		clearRoute();
		return;
	}

	QVector< UnsignedCoordinate > waypoints;
	int passedRoutepoint = 0;

	waypoints.push_back( d->source );
	for ( int i = 0; i < d->waypoints.size(); i++ ) {
		if ( d->waypoints[i].IsValid() )
			waypoints.push_back( d->waypoints[i] );
		if ( waypoints[0].ToGPSCoordinate().ApproximateDistance( d->waypoints[i].ToGPSCoordinate() ) < 50 && i == 0 ) {
			waypoints.remove( 1, waypoints.size() - 1 );
			passedRoutepoint = i + 1;
		}
	}

	if ( passedRoutepoint > 0 )
	{
		d->waypoints.remove( 0, passedRoutepoint );
		emit waypointsChanged();
	}

	if ( waypoints.size() < 2 ) {
		clearRoute();
		return;
	}

	QVector< IGPSLookup::Result > gps;

	for ( int i = 0; i < waypoints.size(); i++ ) {
		Timer time;
		IGPSLookup::Result result;
		bool found = gpsLookup->GetNearestEdge( &result, waypoints[i], 1000 );
		qDebug() << "GPS Lookup:" << time.elapsed() << "ms";

		if ( !found ) {
			clearRoute();
			return;
		}

		gps.push_back( result );
	}

	d->pathNodes.clear();
	d->pathEdges.clear();

	for ( int i = 1; i < waypoints.size(); i++ ) {
		QVector< IRouter::Node > nodes;
		QVector< IRouter::Edge > edges;
		double travelTime;

		Timer time;
		bool found = router->GetRoute( &travelTime, &nodes, &edges, gps[i - 1], gps[i] );
		qDebug() << "Routing:" << time.elapsed() << "ms";

		if ( found ) {
			if ( i == 1 ) {
				d->pathNodes = nodes;
				d->pathEdges = edges;
			} else {
				for ( int j = 1; j < nodes.size(); j++ )
					d->pathNodes.push_back( nodes[j] );
				for ( int j = 1; j < edges.size(); j++ )
					d->pathEdges.push_back( edges[j] );
			}
			d->travelTime += travelTime;
		} else {
			d->travelTime = -1;
			break;
		}
	}

	d->distance = waypoints.first().ToGPSCoordinate().ApproximateDistance( waypoints.last().ToGPSCoordinate() );

	emit routeChanged();
	emit instructionsChanged();
	emit distanceChanged( d->distance );
	emit travelTimeChanged( d->travelTime );
}

void RoutingLogic::clearRoute()
{
	d->distance = -1;
	d->travelTime = -1;
	d->pathEdges.clear();
	d->pathNodes.clear();
	d->icons.clear();
	d->labels.clear();
	emit routeChanged();
	emit instructionsChanged();
	emit distanceChanged( d->distance );
	emit travelTimeChanged( d->travelTime );
}

void RoutingLogic::dataLoaded()
{
	if ( !d->source.IsValid() )
	{
		const MapData::MapPackage& package = MapData::instance()->information();
		d->source.x = ( ( double ) package.max.x + package.min.x ) / 2;
		d->source.y = ( ( double ) package.max.y + package.min.y ) / 2;
		emit sourceChanged();
	}
	computeRoute();
}

void RoutingLogic::computeRoundtrip()
{
	IGPSLookup* gpsLookup = MapData::instance()->gpsLookup();
	if ( gpsLookup == NULL )
		return;
	IRouter* router = MapData::instance()->router();
	if ( router == NULL )
		return;

	Timer time;
	int num = d->waypoints.size() + 1;
	if ( num < 3 )
		return;

	QVector< IGPSLookup::Result > gps;

	// look up all GPS positions
	QVector< UnsignedCoordinate > waypoints;
	waypoints.push_back( d->source );
	waypoints += d->waypoints;

	for ( int i = 0; i < waypoints.size(); i++ ) {
		IGPSLookup::Result result;
		bool found = gpsLookup->GetNearestEdge( &result, waypoints[i], 1000 );

		if ( !found ) {
			clearRoute();
			return;
		}

		gps.push_back( result );
	}

	qDebug() << "GPS Mass Lookup:" << time.restart() << "ms";

	// compute all pair shortest paths
	QVector< int > distMatrix( num * num, 0 );
	for ( int from = 0; from < num; from++ ) {
		for ( int to = 0; to < num; to++ ) {
			// skip source == target
			if ( from == to )
				continue;

			double travelTime = 0;
			// lookup distance only
			bool found = router->GetRoute( &travelTime, NULL, NULL, gps[from], gps[to] );
			if ( !found )
				travelTime = std::numeric_limits< double >::max();

			distMatrix[from * num + to] = travelTime;
		}
	}

	qDebug() << "Routing Mass Computation:" << time.restart() << "ms";

	// first point is fixed, try all other permutations
	QVector< int > permutation;
	permutation.reserve( num + 1 );
	for ( int i = 0; i < num; i++ )
		permutation.push_back( i );
	permutation.push_back( 0 );

	// go through all permutations
	double bestDistance = std::numeric_limits< double >::max();
	QVector< int > bestPermutation;
	do {
		double currentDistance = 0;
		bool impossible = false;
		for ( int pos = 1; pos < num + 1; pos++ )
		{
			// calculate cost
			double distance = distMatrix[permutation[pos - 1] * num + permutation[pos]];
			if ( distance == std::numeric_limits< double >::max() )
			{
				impossible = true;
				break;
			}
			currentDistance += distance;
		}
		// permutation not possible? -> skip
		if ( impossible )
			continue;

		// found better permutation?
		if ( currentDistance < bestDistance )
		{
			bestDistance = currentDistance;
			bestPermutation = permutation;
		}
	} while( std::next_permutation( permutation.begin() + 1, permutation.end() - 1 ) ); // keep start and end fixed

	if ( bestPermutation.empty() )
	{
		qWarning() << "Failed to find Round Trip";
		return;
	}

	QVector< UnsignedCoordinate > newWaypoints;
	newWaypoints.reserve( num + 1 );
	for ( int pos = 0; pos < num + 1; pos++ )
		newWaypoints.push_back( waypoints[bestPermutation[pos]] );
	d->waypoints = newWaypoints;

	qDebug() << "Traveling Salesman Computation:" << time.restart() << "ms";

	computeRoute();
}

