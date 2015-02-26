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

#ifndef SPEEDPROFILEDIALOG_H
#define SPEEDPROFILEDIALOG_H

#include "waymodificatorwidget.h"
#include "nodemodificatorwidget.h"
#include <QDialog>

namespace Ui {
	class SpeedProfileDialog;
}

class SpeedProfileDialog : public QDialog
{
	Q_OBJECT

public:
	explicit SpeedProfileDialog( QWidget* parent = 0, QString filename = "" );
	~SpeedProfileDialog();

protected slots:

	void addSpeed();
	void save();
	void load();
	void addWayModificator();
	void addNodeModificator();

protected:

	void connectSlots();

	bool load( const QString& filename );
	bool save( const QString& filename );

	Ui::SpeedProfileDialog* m_ui;
};

#endif // SPEEDPROFILEDIALOG_H
