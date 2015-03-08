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

#include <QtDebug>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QByteArray>
#
#include "routingdaemon.h"

Q_IMPORT_PLUGIN( ContractionHierarchiesClient )
Q_IMPORT_PLUGIN( GPSGridClient )

int main( int argc, char** argv )
{
        /*
        if ( argc == 2 && argv[1] == QString( "--help" ) ) {
		qDebug() << "usage:" << argv[0];
		qDebug() << "\tstarts the service";
		qDebug() << "usage:" << argv[0] << "-i | -install";
		qDebug() << "\tinstalls the service";
		qDebug() << "usage:" << argv[0] << "-u | -uninstall";
		qDebug() << "\tuninstalls the service";
		qDebug() << "usage:" << argv[0] << "-t | -terminate";
		qDebug() << "\tterminates the service";
		qDebug() << "usage:" << argv[0] << "-v | -version";
		qDebug() << "\tdisplays version and status";
		return 1;
        }*/

	RoutingDaemon *monavInstance = new RoutingDaemon(argc, argv);
        qDebug() << "monav starting";
        qDebug() << "parsing json from argv";
        qDebug() << argv[1];
        QString requestString;
        requestString = QString(argv[1]);
        qDebug() << requestString.toUtf8();
        QJsonParseError parseError;
        QJsonDocument routingRequest = QJsonDocument::fromJson(requestString.toUtf8(), &parseError);
        qDebug() << "json parsed";
        if (parseError.error != QJsonParseError::NoError) {
            qDebug() << "JSON parsing failed";
            qDebug() << parseError.errorString();
            qDebug() << parseError.offset;
            return 1;
        }
        if (routingRequest.isEmpty()) {
            qDebug() << "the JSON document is empty";
            return 2;
        }
        if (!routingRequest.object()["waypoints"].isArray() || routingRequest.object()["waypoints"].toArray().count() <= 1 ) {
            qDebug() << "you need to provide at least 2 waypoints for routing";
            return 3;
       	}
        if (!routingRequest.object().contains("dataDirectory")) {
            qDebug() << "you need to provide a path to an offline routing data directory";
            return 4;
        }

        // TODO: check if the jeson document has dataDirectory and at least
        //       2 waypoints

        qDebug() << routingRequest.toJson();
        qDebug() << "computing route";

        QJsonObject result = monavInstance->route(routingRequest.object());
        qDebug() << "result:";
	QJsonDocument resultDocument = QJsonDocument(result);
        QTextStream(stdout) << resultDocument.toJson(QJsonDocument::Compact) << endl;

        return 0;
}

