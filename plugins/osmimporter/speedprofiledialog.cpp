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

#include "speedprofiledialog.h"
#include "ui_speedprofiledialog.h"
#include "waymodificatorwidget.h"
#include "nodemodificatorwidget.h"
#include "highwaytypewidget.h"
#include "types.h"
#include <QFileDialog>
#include <QSettings>
#include <QtDebug>

SpeedProfileDialog::SpeedProfileDialog( QWidget* parent, QString filename ) :
	QDialog( parent ),
	m_ui( new Ui::SpeedProfileDialog )
{
	m_ui->setupUi( this );

	QSettings settings( "MoNav" );
	settings.beginGroup( "SpeedProfile GUI" );
	restoreGeometry( settings.value( "geometry", saveGeometry() ).toByteArray() );

	MoNav::SpeedProfile profile;
	m_ui->accessList->addItems( profile.accessTypes() );

	if ( !filename.isEmpty() )
		load( filename );

	connectSlots();
}

SpeedProfileDialog::~SpeedProfileDialog()
{
	QSettings settings( "MoNav" );
	settings.beginGroup( "SpeedProfile GUI" );
	settings.setValue( "geometry", saveGeometry() );

	delete m_ui;
}

void SpeedProfileDialog::connectSlots()
{
	connect( m_ui->addWayType, SIGNAL(clicked()), this, SLOT(addSpeed()) );
	connect( m_ui->saveButton, SIGNAL(clicked()), this, SLOT(save()) );
	connect( m_ui->loadButton, SIGNAL(clicked()), this, SLOT(load()) );

	connect( m_ui->addWayModificator, SIGNAL(clicked()), this, SLOT(addWayModificator()) );
	connect( m_ui->addNodeModificator, SIGNAL(clicked()), this, SLOT(addNodeModificator()) );
}

void SpeedProfileDialog::addWayModificator()
{
	WayModificatorWidget* widget = new WayModificatorWidget( this );
	m_ui->wayModificators->widget()->layout()->addWidget( widget );
	if ( isVisible() )
		widget->show();
}

void SpeedProfileDialog::addNodeModificator()
{
	NodeModificatorWidget* widget = new NodeModificatorWidget( this );
	m_ui->nodeModificators->widget()->layout()->addWidget( widget );
	if ( isVisible() )
		widget->show();
}

void SpeedProfileDialog::addSpeed()
{
	HighwayTypeWidget* widget = new HighwayTypeWidget( this );
	m_ui->highwayTypes->widget()->layout()->addWidget( widget );
}

bool SpeedProfileDialog::save( const QString& filename )
{
	MoNav::SpeedProfile profile;

	profile.defaultCitySpeed = m_ui->setDefaultCitySpeed->isChecked();
	profile.ignoreOneway = m_ui->ignoreOneway->isChecked();
	profile.ignoreMaxspeed = m_ui->ignoreMaxspeed->isChecked();
	profile.acceleration = m_ui->acceleration->value();
	profile.decceleration = m_ui->decceleration->value();
	profile.tangentialAcceleration = m_ui->tangentialAcceleration->value();
	profile.pedestrian = m_ui->pedestrian->value();
	profile.otherCars = m_ui->otherCars->value();

	if ( m_ui->accessList->selectedItems().size() != 1 ) {
		qCritical() << "no access type selected";
		return false;
	}

	profile.setAccess( m_ui->accessList->selectedItems().front()->text() );

	QList< HighwayTypeWidget* > highwayWidgets = m_ui->highwayTypes->findChildren< HighwayTypeWidget* >();
	for ( int i = 0; i < highwayWidgets.size(); i++ ) {
		MoNav::Highway highway = highwayWidgets[i]->highway();
		profile.highways.push_back( highway );
	}

	QList< WayModificatorWidget* > wayModificators = m_ui->wayModificators->findChildren< WayModificatorWidget* >();
	for ( int i = 0; i < wayModificators.size(); i++ ) {
		MoNav::WayModificator mod = wayModificators[i]->modificator();
		profile.wayModificators.push_back( mod );
	}

	QList< NodeModificatorWidget* > nodeModificators = m_ui->nodeModificators->findChildren< NodeModificatorWidget* >();
	for ( int i = 0; i < nodeModificators.size(); i++ ) {
		MoNav::NodeModificator mod = nodeModificators[i]->modificator();
		profile.nodeModificators.push_back( mod );
	}

	if ( !profile.save( filename ) )
		return false;

	return true;
}

bool SpeedProfileDialog::load( const QString& filename )
{
	MoNav::SpeedProfile profile;

	if ( !profile.load( filename ) ) {
		qCritical() << "failed to load speed profile:" << filename;
		return false;
	}

	m_ui->setDefaultCitySpeed->setChecked( profile.defaultCitySpeed );
	m_ui->ignoreOneway->setChecked( profile.ignoreOneway );
	m_ui->ignoreMaxspeed->setChecked( profile.ignoreMaxspeed );
	m_ui->acceleration->setValue( profile.acceleration );
	m_ui->decceleration->setValue( profile.decceleration );
	m_ui->tangentialAcceleration->setValue( profile.tangentialAcceleration );
	m_ui->pedestrian->setValue( profile.pedestrian );
	m_ui->otherCars->setValue( profile.otherCars );

	foreach( QListWidgetItem* item, m_ui->accessList->selectedItems() )
		item->setSelected( false );

	QString accessType = profile.accessList.front();
	QList< QListWidgetItem* > items = m_ui->accessList->findItems( accessType, Qt::MatchFixedString );

	if ( items.size() < 1 ) {
		qCritical() << "invalid access type found:" << accessType;
		return false;
	}

	foreach( QListWidgetItem* item, m_ui->accessList->selectedItems() )
		item->setSelected( false );

	items.first()->setSelected( true );

	qDeleteAll( m_ui->highwayTypes->findChildren< HighwayTypeWidget* >() );

	int highwayCount = profile.highways.size();
	for ( int i = 0; i < highwayCount; i++ ) {
		const MoNav::Highway& highway = profile.highways[i];

		HighwayTypeWidget* widget = new HighwayTypeWidget( this );
		widget->setHighway( highway );
		m_ui->highwayTypes->widget()->layout()->addWidget( widget );
		if ( isVisible() )
			widget->show();
	}

	qDeleteAll( m_ui->wayModificators->findChildren< WayModificatorWidget* >() );

	int wayModificatorCount = profile.wayModificators.size();
	for ( int i = 0; i < wayModificatorCount; i++ ) {
		const MoNav::WayModificator& mod = profile.wayModificators[i];

		WayModificatorWidget* widget = new WayModificatorWidget( this );
		widget->setModificator( mod );
		m_ui->wayModificators->widget()->layout()->addWidget( widget );
		if ( isVisible() )
			widget->show();
	}

	qDeleteAll( m_ui->nodeModificators->findChildren< NodeModificatorWidget* >() );

	int nodeModificatorCount = profile.nodeModificators.size();
	for ( int i = 0; i < nodeModificatorCount; i++ ) {
		const MoNav::NodeModificator& mod = profile.nodeModificators[i];

		NodeModificatorWidget* widget = new NodeModificatorWidget( this );
		widget->setModificator( mod );
		m_ui->nodeModificators->widget()->layout()->addWidget( widget );
		if ( isVisible() )
			widget->show();
	}

	return true;
}

void SpeedProfileDialog::save()
{
	QString filename = QFileDialog::getSaveFileName( this, tr( "Enter Speed Profile Filename" ), "", "*.spp" );
	if ( filename.isEmpty() )
		return;

	if ( !filename.endsWith( ".spp" ) )
		filename += ".spp";

	save( filename );
}

void SpeedProfileDialog::load()
{
	QString filename = QFileDialog::getOpenFileName( this, tr( "Enter Speed Filename" ), "", "*.spp" );
	if ( filename.isEmpty() )
		return;

	load( filename );
}
