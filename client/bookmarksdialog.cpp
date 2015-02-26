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

#include "bookmarksdialog.h"
#include "ui_bookmarksdialog.h"
#include "routinglogic.h"
#include <QSettings>
#include <QInputDialog>
#include <QtDebug>
#include <QVBoxLayout>

BookmarksDialog::BookmarksDialog(QWidget *parent) :
		QDialog( parent ),
		m_ui( new Ui::BookmarksDialog )
{
	m_ui->setupUi(this);

	// Windows Mobile Window Flags
	setWindowFlags( windowFlags() & ( ~Qt::WindowOkButtonHint ) );
	setWindowFlags( windowFlags() | Qt::WindowCancelButtonHint );

	QSettings settings( "MoNavClient" );
	settings.beginGroup( "Bookmarks" );
	QStringList names = settings.value( "names" ).toStringList();

	for ( int i = 0; i < names.size(); i++ ) {
		UnsignedCoordinate pos;
		pos.x = settings.value( QString( "%1.coordinates.x" ).arg( i ), 0 ).toUInt();
		pos.y = settings.value( QString( "%1.coordinates.y" ).arg( i ), 0 ).toUInt();
		m_coordinates.push_back( pos );
	}

	m_names.setColumnCount( 1 );
	for ( int i = 0; i < names.size(); i++ )
		m_names.appendRow( new QStandardItem( names[i] ) );

	m_chosen = -1;

	m_ui->bookmarkList->setModel( &m_names );

#ifdef Q_WS_MAEMO_5
	setAttribute( Qt::WA_Maemo5StackedWindow );
	QVBoxLayout* box = qobject_cast< QVBoxLayout* >( layout() );
	assert( box != NULL );
	m_ui->bookmarkList->hide();
	m_valueButton = new QMaemo5ValueButton( "Bookmark", this );
	m_selector = new QMaemo5ListPickSelector;
	m_selector->setModel( &m_names );
	m_selector->setCurrentIndex( 0 );
	m_valueButton->setPickSelector( m_selector );
	m_valueButton->setValueLayout( QMaemo5ValueButton::ValueBesideText );
	box->insertWidget( 0, m_valueButton );
	m_ui->deleteButton->setEnabled( m_names.rowCount() != 0 );
	m_ui->chooseButton->setEnabled( m_names.rowCount() != 0 );
#endif

	connectSlots();
}

void BookmarksDialog::connectSlots()
{
	connect( m_ui->chooseButton, SIGNAL(clicked()), this, SLOT(chooseBookmark()) );
	connect( m_ui->sourceButton, SIGNAL(clicked()), this, SLOT(addSourceBookmark()) );
	connect( m_ui->targetButton, SIGNAL(clicked()), this, SLOT(addTargetBookmark()) );
	connect( m_ui->deleteButton, SIGNAL(clicked()), this, SLOT(deleteBookmark()) );
#ifndef Q_WS_MAEMO_5
	connect( m_ui->bookmarkList->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)), this, SLOT(currentItemChanged(QItemSelection,QItemSelection)));
#endif
}

BookmarksDialog::~BookmarksDialog()
{
	QSettings settings( "MoNavClient" );
	settings.beginGroup( "Bookmarks" );

	QStringList names;
	for ( int i = 0; i < m_names.rowCount(); i++ )
		names.push_back( m_names.item( i )->text() );
	settings.setValue( "names", names );

	for ( int i = 0; i < names.size(); i++ ) {
		UnsignedCoordinate pos = m_coordinates[i];
		settings.setValue( QString( "%1.coordinates.x" ).arg( i ), pos.x );
		settings.setValue( QString( "%1.coordinates.y" ).arg( i ), pos.y );
	}
	delete m_ui;
}

void BookmarksDialog::deleteBookmark()
{
#ifdef Q_WS_MAEMO_5
	if ( m_selector->currentIndex() == -1 )
		return;
	int index = m_selector->currentIndex();
	m_ui->deleteButton->setEnabled( m_names.rowCount() != 1 );
	m_ui->chooseButton->setEnabled( m_names.rowCount() != 1 );
#else
	if ( m_ui->bookmarkList->selectionModel()->selectedRows().empty() )
		return;
	int index = m_ui->bookmarkList->selectionModel()->selectedRows().first().row();
#endif

	m_coordinates.remove( index );
	m_names.removeRow( index );

#ifdef Q_WS_MAEMO_5
	m_selector->setCurrentIndex( 0 );
#endif
}

void BookmarksDialog::chooseBookmark()
{
#ifdef Q_WS_MAEMO_5
	if ( m_selector->currentIndex() == -1 )
		return;
	int index = m_selector->currentIndex();
#else
	if ( m_ui->bookmarkList->selectionModel()->selectedRows().empty() )
		return;
	int index = m_ui->bookmarkList->selectionModel()->selectedRows().first().row();
#endif

	m_chosen = index;
	accept();
}

void BookmarksDialog::addTargetBookmark()
{
	if ( !m_target.IsValid() )
		return;

	bool ok = false;
	QString name = QInputDialog::getText( this, "Enter Bookmark Name", "Bookmark Name", QLineEdit::Normal, "New Bookmark", &ok );

	if ( !ok )
		return;

	m_names.appendRow( new QStandardItem( name ) );
	m_coordinates.push_back( m_target );
}

void BookmarksDialog::addSourceBookmark()
{
	if ( !m_source.IsValid() )
		return;

	bool ok = false;
	QString name = QInputDialog::getText( this, "Enter Bookmark Name", "Bookmark Name", QLineEdit::Normal, "New Bookmark", &ok );

	if ( !ok )
		return;

	m_names.appendRow( new QStandardItem( name ) );
	m_coordinates.push_back( m_source );
}

void BookmarksDialog::currentItemChanged( QItemSelection current, QItemSelection /*previous*/ )
{
	QModelIndexList list = current.indexes();
	m_ui->chooseButton->setDisabled( list.empty() );
	m_ui->deleteButton->setDisabled( list.empty() );
}

bool BookmarksDialog::showBookmarks( UnsignedCoordinate* result, QWidget* p )
{
	if ( result == NULL )
		return false;

	BookmarksDialog* window = new BookmarksDialog( p );

	window->m_target = RoutingLogic::instance()->target();
	window->m_ui->targetButton->setDisabled( !window->m_target.IsValid() );
	window->m_source = RoutingLogic::instance()->source();
	window->m_ui->sourceButton->setDisabled( !window->m_source.IsValid() );

	window->exec();

	int value = window->result();
	if ( window->m_chosen != -1 )
		*result = window->m_coordinates[window->m_chosen];
	delete window;

	return value == Accepted && result->IsValid();
}
