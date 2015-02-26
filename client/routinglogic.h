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

#ifndef ROUTINGLOGIC_H
#define ROUTINGLOGIC_H

#include "utils/coordinates.h"
#include "interfaces/irouter.h"

#include <QObject>
#include <QStringList>
#include <QVector>
#include <QDateTime>
#include <limits>

#ifndef NOQTMOBILE
#include <QGeoPositionInfoSource>
#endif

class RoutingLogic : public QObject
{

	Q_OBJECT

public:

	// GPS information
	struct GPSInfo {
		UnsignedCoordinate position;
		double altitude;
		double heading;
		double groundSpeed;
		double verticalSpeed;
		double horizontalAccuracy;
		double verticalAccuracy;
		QDateTime timestamp;
	};

	// return the instance of RoutingLogic
	static RoutingLogic* instance();
	// the waypoints used to calculate the route, excluding the source
	QVector< UnsignedCoordinate > waypoints() const;
	// source
	UnsignedCoordinate source() const;
	// target == last waypoint
	UnsignedCoordinate target() const;
	// route description via its nodes' coordinate
	QVector< IRouter::Node > route() const;
	// is the source linked to the GPS reciever?
	bool gpsLink() const;
	// GPS information
	const GPSInfo gpsInfo() const;
	// clears the waypoints/ target / route
	void clear();
	// driving instruction for the current route
	void instructions( QStringList* labels, QStringList* icons, int maxSeconds = std::numeric_limits< int >::max() );

signals:

	void instructionsChanged();
	void routeChanged();
	void distanceChanged( double meter );
	void travelTimeChanged( double seconds );
	void waypointReached( int id );
	void waypointsChanged();
	void gpsInfoChanged();
	void gpsLinkChanged( bool linked );
	void sourceChanged();

public slots:

	// sets the waypoints
	void setWaypoints( QVector< UnsignedCoordinate > waypoints );
	// sets a waypoint. If the coordinate is not valid the waypoint id is removed
	void setWaypoint( int id, UnsignedCoordinate coordinate );
	// sets the source coordinate
	void setSource( UnsignedCoordinate coordinate );
	// sets the target coordine == last waypoint. Inserte a waypoint if necessary.
	void setTarget( UnsignedCoordinate target );
	// links / unlinks GPS and source coordinate
	void setGPSLink( bool linked );

	// computes a roundtrip visiting all waypoints
	// starting at the first one
	// and returning to the first one
	void computeRoundtrip();

	// destroys this object
	void cleanup();

protected:

	RoutingLogic();
	~RoutingLogic();
	void computeRoute();
	void updateInstructions();
	void clearRoute();

	struct PrivateImplementation;
	PrivateImplementation* const d;

protected slots:

	void dataLoaded();

#ifndef NOQTMOBILE
	void positionUpdated( const QGeoPositionInfo& update );
#endif
};

#endif // ROUTINGLOGIC_H
