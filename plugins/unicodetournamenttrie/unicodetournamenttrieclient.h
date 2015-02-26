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

#ifndef UNICODETOURNAMENTTRIECLIENT_H
#define UNICODETOURNAMENTTRIECLIENT_H

#include <QObject>
#include <QtPlugin>
#include <QFile>
#include "interfaces/iaddresslookup.h"
#include "trie.h"

class UnicodeTournamentTrieClient : public QObject, public IAddressLookup
{
	Q_OBJECT
	Q_INTERFACES( IAddressLookup )
public:

	 explicit UnicodeTournamentTrieClient();
	 ~UnicodeTournamentTrieClient();

	 virtual QString GetName();
	 virtual void SetInputDirectory( const QString& dir );
	 virtual void ShowSettings();
	 virtual bool IsCompatible( int fileFormatVersion );
	 virtual bool LoadData();
	 virtual bool UnloadData();
	 virtual bool GetPlaceSuggestions( const QString& input, int amount, QStringList* suggestions, QStringList* inputSuggestions );
	 virtual bool GetStreetSuggestions( int placeID, const QString& input, int amount, QStringList* suggestions, QStringList* inputSuggestions );
	 virtual bool GetPlaceData( QString input, QVector< int >* placeIDs, QVector< UnsignedCoordinate >* placeCoordinates );
	 virtual bool GetStreetData( int placeID, QString input, QVector< int >* segmentLength, QVector< UnsignedCoordinate >* coordinates );

signals:

public slots:

protected:

	 struct Suggestion {
		unsigned importance;
		unsigned index;
		QString prefix;

		bool operator<( const Suggestion& right ) const {
			return importance > right.importance;
		}
	};

	bool find( const char* trie, unsigned* resultNode, QString* missingPrefix, QString prefix );
	int getSuggestion( const char* trie, QStringList* resultNames, unsigned node, int count, const QString prefix );

	QString directory;
	QFile* trieFile;
	QFile* subTrieFile;
	QFile* dataFile;
	const char* trieData;
	const char* subTrieData;

};

#endif // UNICODETOURNAMENTTRIECLIENT_H
