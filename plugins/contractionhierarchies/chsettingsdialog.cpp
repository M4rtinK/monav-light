#include "chsettingsdialog.h"
#include "ui_chsettingsdialog.h"

CHSettingsDialog::CHSettingsDialog(QWidget *parent) :
	 QWidget(parent),
	 m_ui(new Ui::CHSettingsDialog)
{
	 m_ui->setupUi(this);
}

CHSettingsDialog::~CHSettingsDialog()
{
	 delete m_ui;
}

bool CHSettingsDialog::readSettings( const ContractionHierarchies::Settings& settings )
{
	m_ui->blockSize->setValue( settings.blockSize );
	return true;
}

bool CHSettingsDialog::fillSettings( ContractionHierarchies::Settings* settings ) const
{
	if ( settings == NULL )
		return false;
	settings->blockSize = m_ui->blockSize->value();
	return true;
}
