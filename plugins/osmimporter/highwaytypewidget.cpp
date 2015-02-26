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

#include "highwaytypewidget.h"
#include "ui_highwaytypewidget.h"

HighwayTypeWidget::HighwayTypeWidget( QWidget* parent ) :
		QWidget( parent ),
		m_ui( new Ui::HighwayTypeWidget )
{
	m_ui->setupUi( this );
}

HighwayTypeWidget::~HighwayTypeWidget()
{
	delete m_ui;
}

MoNav::Highway HighwayTypeWidget::highway()
{
	MoNav::Highway highway;
	highway.priority = m_ui->priority->value();
	highway.value = m_ui->type->text();
	highway.maxSpeed = m_ui->maxSpeed->value();
	highway.defaultCitySpeed = m_ui->citySpeed->value();
	highway.averageSpeed = m_ui->averageSpeed->value();
	highway.pedestrian = m_ui->pedestrian->isChecked();
	highway.otherLeftPenalty = m_ui->otherCarsLeft->currentIndex() != 0;
	highway.otherLeftEqual = m_ui->otherCarsLeft->currentIndex() == 2;
	highway.otherRightPenalty = m_ui->otherCarsRight->currentIndex() != 0;
	highway.otherRightEqual = m_ui->otherCarsRight->currentIndex() == 2;
	highway.otherStraightPenalty = m_ui->otherCarsStraight->currentIndex() != 0;
	highway.otherStraightEqual = m_ui->otherCarsStraight->currentIndex() == 2;
	highway.leftPenalty = m_ui->leftPenalty->value();
	highway.rightPenalty = m_ui->rightPenalty->value();

	return highway;
}

void HighwayTypeWidget::setHighway( MoNav::Highway highway )
{
	m_ui->priority->setValue( highway.priority );
	m_ui->type->setText( highway.value );
	m_ui->maxSpeed->setValue( highway.maxSpeed );
	m_ui->citySpeed->setValue( highway.defaultCitySpeed );
	m_ui->averageSpeed->setValue( highway.averageSpeed );
	m_ui->pedestrian->setChecked( highway.pedestrian );
	m_ui->otherCarsLeft->setCurrentIndex( highway.otherLeftPenalty ? ( highway.otherLeftEqual ? 2 : 1 ) : 0 );
	m_ui->otherCarsRight->setCurrentIndex( highway.otherRightPenalty ? ( highway.otherRightEqual ? 2 : 1 ) : 0 );
	m_ui->otherCarsStraight->setCurrentIndex( highway.otherStraightPenalty ? ( highway.otherStraightEqual ? 2 : 1 ) : 0 );
	m_ui->leftPenalty->setValue( highway.leftPenalty );
	m_ui->rightPenalty->setValue( highway.rightPenalty );
}
