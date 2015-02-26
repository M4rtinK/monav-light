#include <QSettings>
#include <QDebug>

#include "serverinputdialog.h"
#include "ui_serverinputdialog.h"

ServerInputDialog::ServerInputDialog( const QVector< ServerLogic::Server > &servers, QWidget *parent ) :
	QDialog( parent ),
	m_ui( new Ui::ServerInputDialog )
{
	m_ui->setupUi( this );

	connect( m_ui->buttonSave, SIGNAL( accepted() ), this, SLOT( accept() ) );
	connect( m_ui->buttonCancel, SIGNAL( rejected() ), this, SLOT( reject() ) );
	connect( m_ui->add, SIGNAL( clicked() ), this, SLOT( addServer() ) );
	connect( m_ui->remove, SIGNAL( clicked() ), this, SLOT( removeServer() ) );

	QStringList labels;
	labels << "name" << "url";
	m_ui->serverList->setHorizontalHeaderLabels( labels );
	m_ui->serverList->horizontalHeader()->setResizeMode( QHeaderView::ResizeToContents );
	m_ui->serverList->horizontalHeader()->setStretchLastSection( true );

	for ( int i = 0; i < servers.size(); i++ ) {
		m_ui->serverList->insertRow(i);
		m_ui->serverList->setItem( i, 0, new QTableWidgetItem( servers.at(i).name ) );
				m_ui->serverList->setItem( i, 1, new QTableWidgetItem( servers.at(i).url.toString( ) ) );
	}

}

ServerInputDialog::~ServerInputDialog()
{
	delete m_ui;
}

void ServerInputDialog::writeServerSettings( QVector< ServerLogic::Server > *servers )
{
	int entries = m_ui->serverList->rowCount();
	servers->resize( entries );
	for ( int i = 0; i < entries; i++ )
	{
			(*servers)[i].name = m_ui->serverList->item( i, 0 )->text();

			QString path = m_ui->serverList->item( i, 1 )->text();

			if( !path.endsWith( "packageList.xml" ) )
			{
				if ( !path.endsWith( '/' ) && !path.endsWith( '\\') )
					path.append( '/' );

				path.append( "packageList.xml" );
			}

			(*servers)[i].url = QUrl( path );
	}
}

void ServerInputDialog::addServer()
{
		int index = std::max( m_ui->serverList->currentRow(), 0 );

		m_ui->serverList->insertRow( index );
		m_ui->serverList->setItem( index, 0, new QTableWidgetItem( "" ) );
		m_ui->serverList->setItem( index, 1, new QTableWidgetItem( "" ) );
}

void ServerInputDialog::removeServer()
{
	m_ui->serverList->removeRow( m_ui->serverList->currentRow() );
}
