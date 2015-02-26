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

#include <QDataStream>
#include <QFile>
#include "rendererbase.h"

class MapnikRendererClient : public RendererBase
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "monav.IRenderer/1.2")
public:

	MapnikRendererClient();
	virtual ~MapnikRendererClient();
	virtual QString GetName();
	virtual bool IsCompatible( int fileFormatVersion );

protected:

	virtual bool loadTile( int x, int y, int zoom, int magnification, QPixmap** tile );
	virtual bool load();
	virtual void unload();

	struct Box
	{
		int minX;
		int maxX;
		int minY;
		int maxY;
	};

	QVector< Box > m_boxes;
	QFile* m_indexFile;
	QFile* m_tileFile;
	int m_fileZoom;
};

#endif // MAPNIKRENDERER_H
