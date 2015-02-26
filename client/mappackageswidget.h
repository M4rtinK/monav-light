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

#ifndef MAPPACKAGESWIDGET_H
#define MAPPACKAGESWIDGET_H

#include <QWidget>
#include <QUrl>

#include "serverlogic.h"

namespace Ui {
	class MapPackagesWidget;
}

class MapPackagesWidget : public QWidget
{
	Q_OBJECT

public:

	explicit MapPackagesWidget( QWidget* parent = 0 );
	~MapPackagesWidget();

public slots:
	void serverIndexChanged( int newIndex );
	void mapSelectionChanged();
	void updateSelectionChanged();
	void downloadSelectionChanged();

signals:

	void mapChanged();
	void closed();

protected slots:

	void load();
	void directory();
	void changeTab(int tabIndex = 0);
	void downloadList();
	void downloadPackages();
	void check();
	void update();
	void editServerList();
	void populateServerPackageList();
	void handleProgress( QString text );
	void selected( int id );
	void cleanUp( ServerLogic::ERROR_TYPE, QString message = "" );

protected:

	virtual void resizeEvent( QResizeEvent* event );
	virtual void showEvent( QShowEvent* event );
	void setupNetworkAccess();

	struct PrivateImplementation;
	PrivateImplementation* d;
	Ui::MapPackagesWidget* m_ui;
};

#endif // MAPPACKAGESWIDGET_H
