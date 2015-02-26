#include <QFile>
#include <QDir>
#include <QSettings>
#include <QNetworkReply>
#include <QDebug>

#include "serverlogic.h"
#include "utils/directoryunpacker.h"

ServerLogic::ServerLogic()
{
	m_localDir = "";
	m_currentOp = INACTIVE;
	m_errorStatus = NO_ERROR;
	m_network = NULL;
	m_unpacker = NULL;

	connect( this, SIGNAL( error( ServerLogic::ERROR_TYPE, QString ) ), this, SLOT( cleanUp( ServerLogic::ERROR_TYPE, QString ) ) );
}

ServerLogic::~ServerLogic()
{
	if ( m_network != NULL )
		delete m_network;

	if( m_unpacker != NULL )
		delete m_unpacker;
}

void ServerLogic::setOp( ServerLogic::OPERATION_TYPE operation )
{
	m_currentOp = operation;
}

void ServerLogic::setStatus( ServerLogic::ERROR_TYPE error )
{
	m_errorStatus = error;
}

const ServerLogic::OPERATION_TYPE& ServerLogic::getOp() const
{
	return m_currentOp;
}

const ServerLogic::ERROR_TYPE& ServerLogic::getStatus() const
{
	return m_errorStatus;
}

void ServerLogic::clearPackagesToLoad()
{
	m_packagesToLoad.clear();
}

void ServerLogic::addPackagesToLoad( const QList< ServerLogic::PackageInfo > &packageLocations )
{
	m_packagesToLoad.append( packageLocations );
}

void ServerLogic::removePackagesToLoad( const QList< ServerLogic::PackageInfo > &packageLocations )
{
	foreach ( const ServerLogic::PackageInfo &package, packageLocations )
		m_packagesToLoad.removeOne( package );
}

const QList< ServerLogic::PackageInfo >& ServerLogic::packagesToLoad() const
{
	return m_packagesToLoad;
}

void ServerLogic::clearUpdatablePackages()
{
	m_updatablePackages.clear();
}

void ServerLogic::addUpdatablePackages( const QList< ServerLogic::PackageInfo >& packageLocations )
{
	m_updatablePackages.append( packageLocations );
}

const QList< ServerLogic::PackageInfo >& ServerLogic::updatablePackages() const
{
	return m_updatablePackages;
}

const QDomDocument& ServerLogic::packageList() const
{
	return m_packageList;
}

void ServerLogic::connectNetworkManager()
{
	if(m_network == NULL)
	{
		m_network = new QNetworkAccessManager( this );
		connect( m_network, SIGNAL(finished(QNetworkReply*)), this, SLOT(finished(QNetworkReply*)) );
	}
}

void ServerLogic::loadPackageList( const QUrl &url )
{
	m_packageList.clear();

	qDebug() << "loading package list from:" << url << ", isvalid =" << url.isValid();

	m_network->get( QNetworkRequest( url ) );
}

bool ServerLogic::loadPackage()
{
	if( m_packagesToLoad.isEmpty() )
		return false;

	ServerLogic::PackageInfo package = m_packagesToLoad.first();
    QUrl url( package.server + package.path );

	m_localDir = package.dir;

	qDebug() << "loading package from:" << url;

	QNetworkRequest request( url );

	QNetworkReply * reply = m_network->get( request );
	reply->setReadBufferSize( 2 * 16 * 1024 );

	if( package.path.endsWith(".mmm") )
	{
		QString filename = package.path;
		filename.remove( 0, filename.lastIndexOf( "/" ) + 1 ).replace( ".mmm", "" );

		m_unpacker = new DirectoryUnpacker( m_localDir.append( filename + '/' ), reply );
		connect( m_unpacker, SIGNAL( error() ), this, SIGNAL( error() ) );
	}

	return true;
}

bool ServerLogic::checkPackage()
{
	if( m_packagesToLoad.isEmpty() )
		return true;

	PackageInfo package = m_packagesToLoad.takeFirst();

	QString type = package.dir.endsWith( "MoNav.ini" ) ? "map" : "module";
	QString map = package.path;
	QString name = ( type == "map" ) ? map : package.dir.right( package.dir.size() - package.dir.lastIndexOf( '_') - 1);
	QString id = ( type == "map" ) ? "map" : package.dir.right( package.dir.size() - package.dir.lastIndexOf( '/') - 1);
	QString server = package.server;

	QString packageProgressInfo = package.path + ' ' + id + " : ";

	if( m_packageList.documentElement().isNull() )
	{
		packageProgressInfo.append( "error downloading list" );

		if( m_packagesToLoad.isEmpty() || m_packagesToLoad.first().server == server  )
			emit checkedPackage( packageProgressInfo );
		else
			loadPackageList( m_packagesToLoad.first().server + "packageList.xml" );

		return true;
	}

	if( server != m_packageList.documentElement().attribute( "path" ) )
	{
		QString listPath = server + "packageList.xml";
		loadPackageList( listPath );
		return false;
	}

	QDomElement packageElement = findElement( type, name, map);

	if( packageElement.isNull() )
		packageProgressInfo.append( "not found in list" );

	else if( packageElement.attribute( "timestamp" ).toUInt() > package.timestamp )
	{
		packageProgressInfo.append( "update found" );


		if( type == "map")
			package.path = packageElement.attribute( "path" );
		else
			package.path = packageElement.text();
		package.dir.truncate( package.dir.lastIndexOf('/') + 1 );
		package.size = packageElement.attribute( "size" ).toLongLong();
		package.timestamp = packageElement.attribute( "timestamp" ).toUInt();

		m_updatablePackages.append( package );
	}

	else
		packageProgressInfo.append( "up to date" );

	emit checkedPackage( packageProgressInfo );

	return true;
}

QDomElement ServerLogic::findElement( QString packageType, QString packageName, QString mapName )
{
	QDomNodeList packageNodeList = m_packageList.elementsByTagName( packageType );

	for( int i=0; i < packageNodeList.size(); i++ )
	{
		QDomElement package = packageNodeList.at( i ).toElement();

		if( packageType == "map" && package.attribute( "name" ) == packageName )
				return package;

		if( packageType == "module" && package.attribute( "name" ) == packageName )
		{
			if( package.parentNode().parentNode().toElement().attribute( "name" ) == mapName )
				return package;
		}
	}

	return QDomElement();
}

void ServerLogic::cleanUp( ERROR_TYPE error, QString message )
{
	m_errorStatus = error;

	if( error == LIST_DL_ERROR )
		return;

	m_packagesToLoad.clear();

	if( m_unpacker != NULL )
		m_unpacker->deleteLater();
	m_unpacker = NULL;
}

void ServerLogic::finished( QNetworkReply* reply )
{
	if( reply->error() != QNetworkReply::NoError )
	{
		qDebug( "Error loading " + reply->url().toString().toUtf8() + " : " + reply->errorString().toUtf8() );

		if( reply->url().path().endsWith( "packageList.xml" ) )
		{
			emit error( ServerLogic::LIST_DL_ERROR, reply->url().toString().toUtf8() + ":\n" + reply->errorString().toUtf8() + '\n' );
			emit loadedList();
		}
		else
			emit error( ServerLogic::PACKAGE_DL_ERROR, reply->url().toString().toUtf8() + ":\n" + reply->errorString().toUtf8() + '\n' );
	}

	else if( reply->url().path().endsWith( ".mmm" ) )
	{
		while ( reply->bytesAvailable() > 0 )
			m_unpacker->processNetworkData();

		m_unpacker->deleteLater();
		m_unpacker = NULL;

		QSettings serverConfig( m_localDir + "Server.ini", QSettings::IniFormat );
		serverConfig.setValue( "server", m_packagesToLoad.first().server );
		serverConfig.setValue( "timestamp", m_packagesToLoad.first().timestamp );

		QString packageProgressInfo = m_packagesToLoad.first().path + " ok";

		if( !m_updatablePackages.isEmpty() )
			m_updatablePackages.removeOne( m_packagesToLoad.first() );

		m_packagesToLoad.removeFirst();

		emit loadedPackage( packageProgressInfo );
	}

	else if( reply->url().path().endsWith( "packageList.xml" ) )
	{
		if ( !m_packageList.setContent( reply ) )
			qCritical( "error parsing package list\n" );
		emit loadedList();
	}

	else if( reply->url().path().endsWith( "MoNav.ini" ) )
	{
		QDir dir( m_localDir );
		if( !dir.exists() && !dir.mkpath( m_localDir ) )
		{
            emit error( ServerLogic::FILE_ERROR, "Unable to create target directory:\n" + dir.path() + '\n');
			return;
		}

		QFile file( m_localDir + "MoNav.ini" );
		if ( !file.open( QIODevice::WriteOnly ) )
		{
			emit error( ServerLogic::FILE_ERROR, "Unable to open target file:\n" + file.fileName() + '\n');
			return;
		}

		file.write( reply->readAll() );
		file.close();

		QSettings serverConfig( m_localDir + "Server.ini", QSettings::IniFormat );
		serverConfig.setValue( "server", m_packagesToLoad.first().server );
		serverConfig.setValue( "timestamp", m_packagesToLoad.first().timestamp );

		QString packageProgressInfo = m_packagesToLoad.first().path + " ok";

		if( !m_updatablePackages.isEmpty() )
			m_updatablePackages.removeOne( m_packagesToLoad.first() );

		m_packagesToLoad.removeFirst();

		emit loadedPackage( packageProgressInfo );
	}

	reply->deleteLater();
}
