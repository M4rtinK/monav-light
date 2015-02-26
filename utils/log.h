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

#ifndef LOG_H
#define LOG_H

#include <QObject>

// this data structure is not thread safe
// it only recieves qDebug messages in a thread safe manner
// it is safe to connect to newLogItem from several threads at once
class Log : public QObject
{
	Q_OBJECT

public:

	// returns the only instance of the log
	static Log* instance();

	// logs enabled log items to the console
	bool logToConsole();
	// logs log items to a file
	// an empty string disables logging to a file
	QString logFile();
	// enables / disables certain types of messages
	bool messageTypeEnabled( QtMsgType type );
	// adds the time to the message
	bool logTime();
	// adds the type to the message
	bool logType();
	// enables / disables all logging
	bool enabled();

signals:

	// a new log items was added
	void newLogItem( QtMsgType type, QString text );
	void newLogItem( QString text );
	void newError( QString text );

public slots:

	void setLogToConsole( bool enabled );
	void setLogFile( QString filename );
	void setMessageTypeEnabled( QtMsgType type, bool enabled );
	void setLogTime( bool enabled );
	void setLogType( bool enabled );
	void setEnabled( bool enabled );

private slots:

	// for internal use only
	void pushItem( QtMsgType type, QString text );

private:

	struct PrivateImplementation;
	PrivateImplementation* d;

	Log();
	~Log();

};

#endif // LOG_H
