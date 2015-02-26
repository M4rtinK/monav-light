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

#ifndef MAPNIKRENDERER_H
#define MAPNIKRENDERER_H

#include "rendererbase.h"
#include <QNetworkAccessManager>
#include <QNetworkDiskCache>

class OSMRendererClient : public RendererBase
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "monav.IRenderer/1.2")
public:

	OSMRendererClient();
	virtual ~OSMRendererClient();
	virtual QString GetName();
	virtual bool IsCompatible( int fileFormatVersion );

public slots:
	void finished( QNetworkReply* reply );

protected:

	virtual void advancedSettingsChanged();
	virtual bool loadTile( int x, int y, int zoom, int magnification, QPixmap** tile );
	virtual bool load();
	virtual void unload();

	QString m_server;

	QNetworkAccessManager* network;
	QNetworkDiskCache* diskCache;
	int tileSize;
};

#endif // MAPNIKRENDERER_H
