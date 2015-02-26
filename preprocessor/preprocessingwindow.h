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

#ifndef PREPROCESSINGWINDOW_H
#define PREPROCESSINGWINDOW_H

#include <QMainWindow>
#include <QPluginLoader>
#include "interfaces/iguisettings.h"

namespace Ui {
	class PreprocessingWindow;
}

class PreprocessingWindow : public QMainWindow {

	Q_OBJECT

public:

	PreprocessingWindow( QWidget* parent = 0, QString configFile = "" );
	~PreprocessingWindow();

public slots:

	void preprocessAll();
	void preprocessDaemon();

protected slots:

	void inputBrowse();
	void outputBrowse();
	void inputChanged( QString text );
	void outputChanged( QString text );
	void nameChanged( QString text );
	void threadsChanged( int threads );
	void importerPreprocessing();
	void rendererPreprocessing();
	void routerPreprocessing();
	void addressLookupPreprocessing();
	void writeConfig();
	void deleteTemporary();
	void saveSettingsToFile();
	void loadSettingsFromFile();

	void nextTask();
	void taskFinished( bool result );

protected:

	enum TaskType {
		TaskImporting, TaskRouting, TaskRendering, TaskAddressLookup, TaskConfig, TaskDeleteTemporary
	};

	void connectSlots();
	void loadPlugins();
	bool testPlugin( QObject* plugin );
	void unloadPlugins();
	bool saveSettings( QString configFile = "" );
	bool loadSettings( QString configFile = "");
	bool readSettingsWidgets();
	bool fillSettingsWidgets();

	virtual void closeEvent( QCloseEvent* event );

	Ui::PreprocessingWindow* m_ui;
	QList< TaskType > m_tasks;
	QVector< QWidget* > m_settings;
	QVector< IGUISettings* > m_settingPlugins;
};

#endif // PREPROCESSINGWINDOW_H
