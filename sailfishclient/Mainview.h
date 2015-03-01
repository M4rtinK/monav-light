#ifndef MAINVIEW_H
#define MAINVIEW_H

#include <QObject>
#include <QStringList>

#include "activationchangeeventfilter.h"

class Mainview : public QObject {
	Q_OBJECT
public:
	Mainview(QObject *parent = 0);
	~Mainview();
	template <typename T> T findObject(QString name);
	void init();
	void connectSlots();
public slots:
	void cleanup();
	void dataLoaded();
	void addZoom();
	void subtractZoom();
	void setNavigation(bool navigation);
	void instructionsChanged();
	void updateDisplayBlankingPause();
	void checkMapData();
	void pageActivating();
signals:
	void instructionsLoaded(QStringList label, QStringList icon);
	void mapDataLoadFailed();
private:
	void setZoom( int zoom );
	
	ActivationChangeEventFilter* activation_change_event_filter;
	
	bool navigation;
};

#endif
