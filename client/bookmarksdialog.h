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

#ifndef BOOKMARKSDIALOG_H
#define BOOKMARKSDIALOG_H

#include "utils/coordinates.h"
#include <QDialog>
#include <QStandardItemModel>
#include <QItemSelection>

#ifdef Q_WS_MAEMO_5
	#include <QMaemo5ValueButton>
	#include <QMaemo5ListPickSelector>
#endif

namespace Ui {
	class BookmarksDialog;
}

class BookmarksDialog : public QDialog
{
	Q_OBJECT

public:
	explicit BookmarksDialog(QWidget *parent = 0);
	~BookmarksDialog();
	UnsignedCoordinate getCoordinate();

	static bool showBookmarks( UnsignedCoordinate* result, QWidget* p = NULL );

public slots:

	void deleteBookmark();
	void chooseBookmark();
	void addTargetBookmark();
	void addSourceBookmark();
	void currentItemChanged( QItemSelection current, QItemSelection previous );

protected:
	void connectSlots();

	QStandardItemModel m_names;
	QVector< UnsignedCoordinate > m_coordinates;
	int m_chosen;
	UnsignedCoordinate m_target;
	UnsignedCoordinate m_source;
#ifdef Q_WS_MAEMO_5
	QMaemo5ValueButton* m_valueButton;
	QMaemo5ListPickSelector* m_selector;
#endif

	Ui::BookmarksDialog *m_ui;
};

#endif // BOOKMARKSDIALOG_H
