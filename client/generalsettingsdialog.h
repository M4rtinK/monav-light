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

#ifndef GENERALSETTINGSDIALOG_H
#define GENERALSETTINGSDIALOG_H

#include <QDialog>
#include <QFileDialog>

namespace Ui {
	class GeneralSettingsDialog;
}

class GeneralSettingsDialog : public QDialog
{
	Q_OBJECT

public:

	explicit GeneralSettingsDialog( QWidget* parent = 0 );
	~GeneralSettingsDialog();

	// not necessary to call after exec, as exec does this itself
	void fillSettings() const;

public slots:

	int exec();
	void setDefaultIconSize();
	void selectPathLogging();

protected slots:

	void confirmClearTracklog();

private:

	Ui::GeneralSettingsDialog* m_ui;
};

#endif // GENERALSETTINGSDIALOG_H
