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

#include "routedescriptiondialog.h"
#include "ui_routedescriptiondialog.h"
#include "routinglogic.h"

#include <cassert>
#include <QtDebug>

RouteDescriptionWidget::RouteDescriptionWidget( QWidget *parent ) :
		QWidget( parent ),
		m_ui(new Ui::RouteDescriptionDialog)
{
	m_ui->setupUi(this);
	connect( m_ui->back, SIGNAL(clicked()), this, SIGNAL(closed()) );
	instructionsChanged();
}

RouteDescriptionWidget::~RouteDescriptionWidget()
{
	delete m_ui;
}

void RouteDescriptionWidget::instructionsChanged()
{
	m_ui->descriptionList->clear();
	QStringList labels;
	QStringList icons;
	RoutingLogic::instance()->instructions( &labels, &icons );
	assert( icons.size() == labels.size() );
	for ( int entry = 0; entry < icons.size(); entry++ ) {
		new QListWidgetItem( QIcon( icons[entry] ), labels[entry], m_ui->descriptionList );
		qDebug() << "Route Description:" << labels[entry];
	}
}
