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

#ifndef UNICODETOURNAMENTTRIE_H
#define UNICODETOURNAMENTTRIE_H

#include "interfaces/ipreprocessor.h"
#include "interfaces/iguisettings.h"
#include "trie.h"
#include <QFile>
#include <vector>

class UnicodeTournamentTrie :
		public QObject,
#ifndef NOGUI
		public IGUISettings,
#endif
		public IPreprocessor
{
	Q_OBJECT
	Q_INTERFACES( IPreprocessor )
#ifndef NOGUI
	Q_INTERFACES( IGUISettings )
#endif

public:

	UnicodeTournamentTrie();
	virtual ~UnicodeTournamentTrie();

	// IPreprocessor
	virtual QString GetName();
	virtual int GetFileFormatVersion();
	virtual Type GetType();
	virtual bool LoadSettings( QSettings* settings );
	virtual bool SaveSettings( QSettings* settings );
	virtual bool Preprocess( IImporter* importer, QString dir );

#ifndef NOGUI
	// IGUISettings
	virtual bool GetSettingsWindow( QWidget** window );
	virtual bool FillSettingsWindow( QWidget* window );
	virtual bool ReadSettingsWindow( QWidget* window );
#endif

protected:

	void insert( std::vector< utt::Node >* trie, unsigned importance, const QString& name, utt::Data data );
	void writeTrie( std::vector< utt::Node >* trie, QFile& file );

	struct PlaceImportance {
		unsigned id;
		int population;
		unsigned type;
		QString name;
		bool operator<( const PlaceImportance& right ) const {
			if ( population != right.population )
				return population < right.population;
			if ( type != right.type )
				return type < right.type;
			return name < right.name;
		}
	};
};

#endif // UNICODETOURNAMENTTRIE_H
