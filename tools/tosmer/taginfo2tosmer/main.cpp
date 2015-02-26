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

#include <QtSql/QSqlDatabase>
#include <QStringList>
#include <QtCore/QCoreApplication>
#include <QtSql/QSqlError>
#include <QtDebug>
#include <iostream>
#include <QtSql/QSqlQuery>
#include <QVariant>
#include <QVector>
#include <QtAlgorithms>
#include <QMap>
#include <algorithm>

struct Implication
{
	QString key;
	QString value;
	QString impliedKey;
	QString impliedValue;

	bool operator<( const Implication& right ) const {
		if ( key != right.key )
			return key < right.key;
		if ( value != right.value )
			return value < right.value;
		return impliedValue < right.impliedValue;
	}

	bool operator==( const Implication& right ) const {
		if ( key != right.key )
			return false;
		if ( value != right.value )
			return false;
		if ( impliedKey != right.impliedKey )
			return false;
		return impliedValue == right.impliedValue;
	}

	bool conflicts( const Implication& right )
	{
		if ( key != right.key )
			return false;
		if ( value != right.value )
			return false;
		if ( impliedKey != right.impliedKey )
			return false;
		return impliedValue != right.impliedValue;
	}
};

QDebug operator<<( QDebug stream, const Implication& implication )
{
	if ( implication.value.isEmpty() )
		stream << right << implication.key << "=>" << implication.impliedKey << "=" << implication.impliedValue;
	else
		stream << right << implication.key << "=" << implication.value << "=>" << implication.impliedKey << "=" << implication.impliedValue;
	return stream;
}

int main(int argc, char *argv[])
{
	QCoreApplication a( argc, argv );

	QStringList args = a.arguments();
	if ( args.size() != 3 || !args[1].endsWith( "taginfo-wiki.db" ) )
	{
		qDebug() << "Converts taginfo databases into tosmer rule files";
		qDebug() << "Usage:";
		qDebug() << "taginfo2tosmer *taginfo-wiki.db language";
		a.quit();
		return -1;
	}

	QSqlDatabase db = QSqlDatabase::addDatabase( "QSQLITE" );
	db.setDatabaseName( args[1] );
	if ( !db.open() )
	{
		qCritical() << "Error openeing database:" << db.lastError();
		a.quit();
		return 1;
	}

	QVector< Implication > dataVector;

	// read elements from the database
	QSqlQuery query( "SELECT key, value, tags_implies from wikipages WHERE lang = '" + args[2] + "'" );
	while ( query.next() )
	{
		Implication data;
		data.key = query.value( 0 ).toString();
		data.value = query.value( 1 ).toString();
		QString implied = query.value( 2 ).toString();

		if ( implied.isEmpty() )
			continue;

		QStringList impliedList = implied.split( ',' );
		impliedList.sort();

		foreach( QString implication, impliedList )
		{
			QStringList splitImplication = implication.split( '=' );
			if ( splitImplication.size() != 2 || splitImplication[1].isEmpty() )
			{
				qWarning() << "%%Skipped invalied implication:" << data.key << "=" << data.value << "=>" << implication;
				continue;
			}

			data.impliedKey = splitImplication[0];
			data.impliedValue = splitImplication[1];
			dataVector.push_back( data );
		}
	}

	// remove conflicting elements
	qSort( dataVector );
	dataVector.resize( std::unique( dataVector.begin(), dataVector.end() ) - dataVector.begin() );
	QVector< bool > unique( dataVector.size(), true );
	for ( int element = 1; element < dataVector.size(); element++ )
	{
		if ( dataVector[element].conflicts( dataVector[element - 1] ) )
		{
			unique[element] = false;
			unique[element - 1] = false;
		}
	}

	for ( int element = 0; element < dataVector.size(); element++ )
	{
		if ( !unique[element] )
		{
			qDebug() << "%%Skipped conflicting implied tag:" << (dataVector[element]);
			continue;
		}
		qDebug() << (dataVector[element]);
	}

	a.quit();
	return 0;
}
