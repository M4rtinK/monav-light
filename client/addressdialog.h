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

#ifndef ADDRESSDIALOG_H
#define ADDRESSDIALOG_H

#include <QDialog>
#include <QListWidget>
#include "interfaces/iaddresslookup.h"
#include "interfaces/irenderer.h"
#include "interfaces/igpslookup.h"

namespace Ui {
	class AddressDialog;
}

class AddressDialog : public QDialog
{
	Q_OBJECT

public:
	explicit AddressDialog(QWidget *parent = 0);
	~AddressDialog();

	static bool getAddress( UnsignedCoordinate* result, QWidget* p, bool cityOnly = false );

public slots:

	void characterClicked( QListWidgetItem* item );
	void suggestionClicked( QListWidgetItem* item );
	void cityTextChanged( QString text );
	void streetTextChanged( QString text );
	void resetCity();
	void resetStreet();

protected:
	void connectSlots();

	enum {
		City = 0, Street = 1
	} m_mode;
	int m_placeID;
	UnsignedCoordinate m_result;
	bool m_skipStreetPosition;

	Ui::AddressDialog* m_ui;
};

#endif // ADDRESSDIALOG_H
