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

#include "log.h"
#include "utils/qthelpers.h"
#include <QSet>
#include <QFile>
#include <QMetaMethod>
#include <cassert>

struct Log::PrivateImplementation{
	QSet< QtMsgType > disabled;
	QString filename;
	QFile file;
	bool console;
	bool time;
	bool type;
	bool enabled;
};

static QMetaMethod logPushMethod;

void messageBoxHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
	logPushMethod.invoke( Log::instance(), Q_ARG( int, type ), Q_ARG( QString, msg ) );
}

static Log* dummy = Log::instance();

Log::Log()
{
	d = new PrivateImplementation;
	d->console = true;
	d->time = true;
	d->type = true;
	d->enabled = true;

	int methodID = this->metaObject()->indexOfMethod( "pushItem(QtMsgType,QString)" );
	assert( methodID != -1 );
	logPushMethod = staticMetaObject.method( methodID );

	qInstallMessageHandler( messageBoxHandler );
}

Log::~Log()
{
	delete d;
}

// returns the only instance of the log
Log* Log::instance()
{
	static Log log;
	return &log;
}

// logs enabled log items to the console
bool Log::logToConsole()
{
	return d->console;
}
// logs log items to a file
// an empty string disables logging to a file
QString Log::logFile()
{
	return d->filename;
}
// enables / disables certain types of messages
bool Log::messageTypeEnabled( QtMsgType type )
{
	return !d->disabled.contains( type );
}
// adds the time to the message
bool Log::logTime()
{
	return d->time;
}
// adds the type to the message
bool Log::logType()
{
	return d->type;
}
bool Log::enabled()
{
	return d->enabled;
}

void Log::setLogToConsole( bool enabled )
{
	d->console = enabled;
}
void Log::setLogFile( QString filename )
{
	d->filename = filename;
	if ( filename.isEmpty() )
		return;
	if ( d->file.isOpen() )
		d->file.close();
	d->file.setFileName( filename );
	openQFile( &d->file, QIODevice::Append );
}

void Log::setMessageTypeEnabled( QtMsgType type, bool enabled )
{
	if ( enabled )
		d->disabled.remove( type );
	else
		d->disabled.insert( type );
}

void Log::setLogTime( bool enabled )
{
	d->time = enabled;
}

void Log::setLogType( bool enabled )
{
	d->type = enabled;
}

void Log::setEnabled( bool enabled )
{
	d->enabled = enabled;
}

// for internal use only
void Log::pushItem( QtMsgType type, QString text )
{
	if ( !d->enabled )
		return;

	QString message;
	if ( d->time )
		message += "[" + QTime::currentTime().toString() + "]";
	if ( d->type )
		message +="[" +  QString::number( type ) + "]";
	if ( !message.isEmpty() )
		message += " ";
	message += text;

	if ( d->disabled.contains( type ) )
		return;

	if ( d->console )
		printf( "%s\n", message.toUtf8().constData() );

	if ( type == QtCriticalMsg || type == QtFatalMsg )
		emit newError( text );

	emit newLogItem( message );
	emit newLogItem( type, message );
}
