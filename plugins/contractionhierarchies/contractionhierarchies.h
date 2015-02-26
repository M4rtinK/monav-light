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

#ifndef ContractionHierarchies_H
#define ContractionHierarchies_H

#include <QObject>
#include "interfaces/ipreprocessor.h"
#include "interfaces/iguisettings.h"
#include "interfaces/iconsolesettings.h"

class ContractionHierarchies :
		public QObject,
#ifndef NOGUI
		public IGUISettings,
#endif
		public IConsoleSettings,
		public IPreprocessor
{
	Q_OBJECT
	Q_INTERFACES( IPreprocessor )
	Q_INTERFACES( IConsoleSettings )
#ifndef NOGUI
	Q_INTERFACES( IGUISettings )
#endif

public:

	struct Settings
	{
		int blockSize;
	};

	ContractionHierarchies();

	// IPreprocessor
	virtual QString GetName();
	virtual int GetFileFormatVersion();
	virtual Type GetType();
	virtual bool LoadSettings( QSettings* settings );
	virtual bool SaveSettings( QSettings* settings );
	virtual bool Preprocess( IImporter* importer, QString dir );
	virtual ~ContractionHierarchies();

#ifndef NOGUI
	// IGUISettings
	virtual bool GetSettingsWindow( QWidget** window );
	virtual bool FillSettingsWindow( QWidget* window );
	virtual bool ReadSettingsWindow( QWidget* window );
#endif

	// IConsoleSettings
	virtual QString GetModuleName();
	virtual bool GetSettingsList( QVector< Setting >* settings );
	virtual bool SetSetting( int id, QVariant data );

protected:
	Settings m_settings;
};

#endif // ContractionHierarchies_H
