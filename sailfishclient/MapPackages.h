#ifndef MAPPACKAGES_H
#define MAPPACKAGES_H

#include "client/mapdata.h"

#include <QObject>
#include <QStringList>
#include <QVector>

class MapPackages : public QObject {
	Q_OBJECT
	Q_PROPERTY(QStringList modulenames READ getCurrentModuleNames)
public:
	MapPackages(QObject *parent = 0);
	~MapPackages();
public slots:
	void loadMapPackages();
	void changeMapPackage(int selected);
	void loadMapModules(int routingindex, int renderingindex, int addresslookupindex);
signals:
	void mapPackagesLoaded(QStringList list, int selected, QStringList routinglist, QStringList renderinglist, QStringList addresslookuplist);
private:
	QStringList getCurrentModuleNames();
	
	QVector< MapData::MapPackage > maps;
};

#endif
