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

#include "addressdialog.h"
#include "ui_addressdialog.h"
#include "placechooser.h"
#include "streetchooser.h"
#include "mapdata.h"
#include "utils/qthelpers.h"

#include <QtDebug>

AddressDialog::AddressDialog(QWidget *parent) :
		QDialog(parent),
		m_ui(new Ui::AddressDialog)
{
	m_ui->setupUi(this);
	// Windows Mobile Window Flags
	setWindowFlags( windowFlags() & ( ~Qt::WindowOkButtonHint ) );
	setWindowFlags( windowFlags() | Qt::WindowCancelButtonHint );

	bool increaseFontSize = true;
#ifdef Q_WS_MAEMO_5
	setAttribute( Qt::WA_Maemo5StackedWindow );
	m_ui->characterList->hide();
	increaseFontSize = false;
#endif

	m_skipStreetPosition = false;
	m_ui->suggestionList->setAlternatingRowColors( true );
	m_ui->characterList->setAlternatingRowColors( true );
	if ( increaseFontSize ) {
		QFont font = m_ui->suggestionList->font();
		font.setPointSize( font.pointSize() * 1.5 );
		m_ui->suggestionList->setFont( font );
		m_ui->characterList->setFont( font );
	}
	resetCity();
	connectSlots();
}

AddressDialog::~AddressDialog()
{
	delete m_ui;
}

void AddressDialog::connectSlots()
{
	connect( m_ui->cityEdit, SIGNAL(textChanged(QString)), this, SLOT(cityTextChanged(QString)) );
	connect( m_ui->streetEdit, SIGNAL(textChanged(QString)), this, SLOT(streetTextChanged(QString)) );
	connect( m_ui->suggestionList, SIGNAL(itemClicked(QListWidgetItem*)), this, SLOT(suggestionClicked(QListWidgetItem*)) );
	connect( m_ui->characterList, SIGNAL(itemClicked(QListWidgetItem*)), this, SLOT(characterClicked(QListWidgetItem*)) );
	connect( m_ui->resetCity, SIGNAL(clicked()), this, SLOT(resetCity()) );
	connect( m_ui->resetStreet, SIGNAL(clicked()), this, SLOT(resetStreet()) );
}

void AddressDialog::characterClicked( QListWidgetItem * item )
{
	QString text = item->text();
	if ( m_mode == City ) {
		m_ui->cityEdit->setText( text );
		m_ui->cityEdit->setFocus( Qt::OtherFocusReason );
	} else if ( m_mode == Street ) {
		m_ui->streetEdit->setText( text );
		m_ui->streetEdit->setFocus( Qt::OtherFocusReason );
	}
}

void AddressDialog::suggestionClicked( QListWidgetItem * item )
{
	IAddressLookup* addressLookup = MapData::instance()->addressLookup();
	if ( addressLookup == NULL )
		return;

	QString text = item->text();
	if ( m_mode == City ) {
		QVector< int > placeIDs;
		QVector< UnsignedCoordinate > placeCoordinates;
		if ( !addressLookup->GetPlaceData( text, &placeIDs, &placeCoordinates ) )
			return;

		m_placeID = placeIDs.front();
		if ( placeIDs.size() > 1 )
		{
			int id = PlaceChooser::selectPlaces( placeCoordinates, this );
			if ( id >= 0 && id < placeIDs.size() )
				m_placeID = placeIDs[id];
			else
				return;
		}
		m_ui->cityEdit->setText( text );
		m_ui->cityEdit->setDisabled( true );
		m_ui->streetEdit->setEnabled( true );
		m_ui->streetEdit->setFocus();
		m_ui->resetStreet->setEnabled( true );
		m_mode = Street;
		streetTextChanged( m_ui->streetEdit->text() );
	} else {
		QVector< int > segmentLength;
		QVector< UnsignedCoordinate > coordinates;
		if ( !addressLookup->GetStreetData( m_placeID, text, &segmentLength, &coordinates ) )
			return;
		if ( coordinates.size() == 0 )
			return;
		if ( m_skipStreetPosition ) {
			m_result = coordinates.first();
			accept();
			return;
		}
		if( !StreetChooser::selectStreet( &m_result, segmentLength, coordinates, this ) )
			return;
		m_ui->streetEdit->setText( text );
		accept();
	}
}

void AddressDialog::cityTextChanged( QString text )
{
	IAddressLookup* addressLookup = MapData::instance()->addressLookup();
	if ( addressLookup == NULL )
		return;

	m_ui->suggestionList->clear();
	m_ui->characterList->clear();
	QStringList suggestions;
	QStringList characters;

	Timer time;
	bool found = addressLookup->GetPlaceSuggestions( text, 10, &suggestions, &characters );
	qDebug() << "City Lookup:" << time.elapsed() << "ms";

	if ( !found )
		return;

	m_ui->suggestionList->addItems( suggestions );
	m_ui->characterList->addItems( characters );
}

void AddressDialog::streetTextChanged( QString text)
{
	IAddressLookup* addressLookup = MapData::instance()->addressLookup();
	if ( addressLookup == NULL )
		return;

	if ( m_mode != Street )
		return;

	m_ui->suggestionList->clear();
	m_ui->characterList->clear();
	QStringList suggestions;
	QStringList characters;

	Timer time;
	bool found = addressLookup->GetStreetSuggestions( m_placeID, text, 10, &suggestions, &characters );
	qDebug() << "Street Lookup:" << time.elapsed() << "ms";

	if ( !found )
		return;

	m_ui->suggestionList->addItems( suggestions );
	m_ui->characterList->addItems( characters );
}

void AddressDialog::resetCity()
{
	m_ui->cityEdit->setEnabled( true );
	m_ui->resetCity->setEnabled( true );
	m_ui->streetEdit->setDisabled( true );
	m_ui->resetStreet->setDisabled( true );
	m_ui->streetEdit->setText( "" );
	m_ui->cityEdit->setText( "" );
	m_mode = City;
	cityTextChanged( "" );
	m_ui->cityEdit->setFocus( Qt::OtherFocusReason );
}

void AddressDialog::resetStreet()
{
	m_ui->streetEdit->setText( "" );
	streetTextChanged( "" );
	m_ui->streetEdit->setFocus( Qt::OtherFocusReason );
}

bool AddressDialog::getAddress( UnsignedCoordinate* result, QWidget* p, bool cityOnly )
{
	if ( result == NULL )
		return false;

	AddressDialog* window = new AddressDialog( p );
	window->m_skipStreetPosition = cityOnly;

	int value = window->exec();
	if ( value == Accepted )
		*result = window->m_result;

	delete window;
	return value == Accepted;
}

