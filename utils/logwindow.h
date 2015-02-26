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

#ifndef LOGWINDOW_H
#define LOGWINDOW_H

#include <QMainWindow>

namespace Ui {
	class LogWindow;
}

class LogWindow : public QMainWindow
{
	Q_OBJECT

public:
	explicit LogWindow( QWidget* parent = 0 );
	~LogWindow();

public slots:

	void addItem( QString text );
	void clear();

private:
	Ui::LogWindow* m_ui;
};

#endif // LOGWINDOW_H
