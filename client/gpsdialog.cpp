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

#include "gpsdialog.h"
#include "ui_gpsdialog.h"
#include "routinglogic.h"

GPSDialog::GPSDialog( QWidget *parent ) :
		QDialog( parent ),
		m_ui( new Ui::GPSDialog )
{
	m_ui->setupUi(this);
	// Windows Mobile Window Flags
	setWindowFlags( windowFlags() & ( ~Qt::WindowOkButtonHint ) );
#ifdef Q_WS_MAEMO_5
	setAttribute( Qt::WA_Maemo5StackedWindow );
#endif
	gpsInfoUpdated();
	connect( RoutingLogic::instance(), SIGNAL(gpsInfoChanged()), this, SLOT(gpsInfoUpdated()) );
}

void GPSDialog::gpsInfoUpdated()
{
	const RoutingLogic::GPSInfo& gpsInfo = RoutingLogic::instance()->gpsInfo();
	if ( gpsInfo.position.IsValid() ) {
		GPSCoordinate gps = gpsInfo.position.ToGPSCoordinate();
		m_ui->latitude->setText( QString::number( gps.latitude ) + QString::fromUtf8( "\302\260" ) + " +- " + QString::number( gpsInfo.horizontalAccuracy ) + "m" );
		m_ui->longitude->setText( QString::number( gps.longitude ) + QString::fromUtf8( "\302\260" ) + " +- " + QString::number( gpsInfo.horizontalAccuracy ) + "m" );
	} else {
		m_ui->latitude->setText( tr( "N/A" ) );
		m_ui->longitude->setText( tr( "N/A" ) );
	}
	if ( gpsInfo.heading >= 0 )
		m_ui->heading->setText( QString::number( gpsInfo.heading ) + QString::fromUtf8( "\302\260" ) );
	else
		m_ui->heading->setText( tr( "N/A" ) );
	if ( gpsInfo.altitude >= 0 )
		m_ui->height->setText( QString::number( gpsInfo.altitude ) + " +- " + QString::number( gpsInfo.verticalAccuracy ) + "m" );
	else
		m_ui->height->setText( tr( "N/A" ) );
	if ( gpsInfo.groundSpeed >= 0 )
		m_ui->speed->setText( QString::number( gpsInfo.groundSpeed ) + "m/s" );
	else
		m_ui->speed->setText( tr( "N/A" ) );
	if ( gpsInfo.verticalSpeed >= 0 )
		m_ui->verticalSpeed->setText( QString::number( gpsInfo.verticalSpeed ) + "m/s" );
	else
		m_ui->verticalSpeed->setText( tr( "N/A" ) );
	if ( gpsInfo.timestamp.isValid() )
		m_ui->time->setText( gpsInfo.timestamp.toString() );
	else
		m_ui->time->setText( tr( "N/A" ) );
}

GPSDialog::~GPSDialog()
{
	delete m_ui;
}
