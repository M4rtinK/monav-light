/*
Copyright 2011  Thomas Miedema thomasmiedema@gmail.com

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

#ifndef ROUTINGSERVER_H
#define ROUTINGSERVER_H

#include "routingcommon.h"

#include <QTcpServer>
#include <QTcpSocket>

// http://doc.qt.nokia.com/solutions/4/qtservice/qtservice-example-server.html
class RoutingServer : public QTcpServer, public RoutingCommon<QTcpSocket>  {

	Q_OBJECT

public:

	RoutingServer(quint16 port, QObject* parent = 0) : QTcpServer(parent) {
		listen(QHostAddress::Any, port);
	}

	void incomingConnection(int socket)
	{
		// When a new client connects, the server constructs a QTcpSocket and all
		// communication with the client is done over this QTcpSocket. QTcpSocket
		// works asynchronously, this means that all the communication is done
		// in the two slots readClient() and discardClient().
		QTcpSocket* s = new QTcpSocket( this );
		connect(s, SIGNAL( readyRead() ), this, SLOT( readClient() ) );
		connect(s, SIGNAL( disconnected() ), this, SLOT( discardClient() ) );
		s->setSocketDescriptor( socket );
	}

private slots:
	void readClient() {
		// This slot is called when the client sent data to the server. The
		// server looks if it was a get request and sends a very simple HTML
		// document back.
		QTcpSocket* connection = (QTcpSocket*) sender();

		handleConnection( connection );

		// Note: QLocalSocket, which is used in routingdaemon.h, is not
		// a subclass of QAbstractSocket, and has a slightly different
		// interface here.
		connection->close();
		if ( connection->state() == QTcpSocket::UnconnectedState ) {
		     delete connection;
		}
	}

	void discardClient() {
		QTcpSocket* connection = (QTcpSocket*) sender();
		connection->deleteLater();
	}
};

#endif // ROUTINGSERVER_H
