/****************************************************************************
 **
 ** Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies).
 ** All rights reserved.
 ** Contact: Nokia Corporation (qt-info@nokia.com)
 **
 ** This file is part of the examples of the Qt Toolkit.
 **
 ** $QT_BEGIN_LICENSE:LGPL$
 ** No Commercial Usage
 ** This file contains pre-release code and may not be distributed.
 ** You may use this file in accordance with the terms and conditions
 ** contained in the Technology Preview License Agreement accompanying
 ** this package.
 **
 ** GNU Lesser General Public License Usage
 ** Alternatively, this file may be used under the terms of the GNU Lesser
 ** General Public License version 2.1 as published by the Free Software
 ** Foundation and appearing in the file LICENSE.LGPL included in the
 ** packaging of this file.  Please review the following information to
 ** ensure the GNU Lesser General Public License version 2.1 requirements
 ** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
 **
 ** In addition, as a special exception, Nokia gives you certain additional
 ** rights.  These rights are described in the Nokia Qt LGPL Exception
 ** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
 **
 ** If you have questions regarding the use of this file, please contact
 ** Nokia at qt-info@nokia.com.
 **
 **
 **
 **
 **
 **
 **
 **
 ** $QT_END_LICENSE$
 **
 ****************************************************************************/

 #ifndef FULLSCREENEXITBUTTON_H
 #define FULLSCREENEXITBUTTON_H

 #include <QtGui/qtoolbutton.h>
 #include <QtGui/qevent.h>

 class FullScreenExitButton : public QToolButton
 {
	  Q_OBJECT
 public:
	  inline explicit FullScreenExitButton(QWidget *parent);

 protected:
	  inline bool eventFilter(QObject *obj, QEvent *ev);
 };

 FullScreenExitButton::FullScreenExitButton(QWidget *parent)
			: QToolButton(parent)
 {
	  Q_ASSERT(parent);

	  // set the fullsize icon from Maemo's theme
	  setIcon(QIcon::fromTheme(QLatin1String("general_fullsize")));

	  // ensure that our size is fixed to our ideal size
	  setFixedSize(sizeHint());

	  // set the background to 0.5 alpha
	  QPalette pal = palette();
	  QColor backgroundColor = pal.color(backgroundRole());
	  backgroundColor.setAlpha(128);
	  pal.setColor(backgroundRole(), backgroundColor);
	  setPalette(pal);

	  // ensure that we're painting our background
	  setAutoFillBackground(true);

	  // when we're clicked, tell the parent to exit fullscreen
	  connect(this, SIGNAL(clicked()), parent, SLOT(showNormal()));

	  // install an event filter to listen for the parent's events
	  parent->installEventFilter(this);
 }

 bool FullScreenExitButton::eventFilter(QObject *obj, QEvent *ev)
 {
	  if (obj != parent())
			return QToolButton::eventFilter(obj, ev);

	  QWidget *parent = parentWidget();
	  bool isFullScreen = parent->windowState() & Qt::WindowFullScreen;

	  switch (ev->type()) {
	  case QEvent::WindowStateChange:
			disconnect( parent );
			if ( isFullScreen )
				connect(this, SIGNAL(clicked()), parent, SLOT(showNormal()));
			else
				connect(this, SIGNAL(clicked()), parent, SLOT(showFullScreen()));
			//setVisible(isFullScreen);
			if (isFullScreen)
				 raise();
			// fall through
	  case QEvent::Resize:
			if (isVisible())
				 move( parent->width() - width(), parent->height() - height() );
			break;
	  default:
			break;
	  }

	  return QToolButton::eventFilter(obj, ev);
 }

 #endif
