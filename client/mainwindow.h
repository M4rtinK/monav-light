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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "utils/coordinates.h"

namespace Ui {
	class MainWindow;
}

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:

	explicit MainWindow( QWidget* parent = 0 );
	~MainWindow();

protected slots:

	void informationLoaded();
	void mapLoaded();
	void modulesLoaded();
	void modulesCancelled();

	void dataLoaded();

	void mouseClicked( ProjectedCoordinate clickPos );

	void addZoom();
	void subtractZoom();
	void setZoom( int zoom );

	void settingsGeneral();
	void settingsRenderer();
	void settingsRouter();
	void settingsGPSLookup();
	void settingsAddressLookup();
	void settingsGPS();

	void gotoSource();
	void gotoGpsCoordinate();
	void gotoGpsLocation();
	void gotoTarget();
	void gotoBookmark();
	void gotoAddress();

	void sourceByBookmark();
	void sourceByAddress();
	void sourceByGPS();

	void setWaypointID( int id );
	void waypointsChanged();

	void targetByBookmark();
	void targetByAddress();
	void subductRoutepoint();
	void addRoutepoint();

	void bookmarks();
	void computeRoundTrip();

	void setModeSourceSelection();
	void setModeTargetSelection();
	void setModeNoSelection();
	void toggleLocked();

	void toolsMenu();
	void gotoMenu();
	void settingsMenu();
	void sourceMenu();
	void targetMenu();

	void showInstructions();
	void instructionsChanged();

	void displayMapChooser();
	void displayModuleChooser();

protected:

	virtual void resizeEvent( QResizeEvent* event );
	void connectSlots();
	void setupMenu();
	void resizeIcons();

#ifdef Q_WS_MAEMO_5
	void grabZoomKeys( bool grab );
	void keyPressEvent( QKeyEvent* event );
#endif

	struct PrivateImplementation;
	PrivateImplementation* d;

	Ui::MainWindow* m_ui;
};

#endif // MAINWINDOW_H
