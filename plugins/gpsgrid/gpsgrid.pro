#-------------------------------------------------
#
# Project created by QtCreator 2010-06-25T08:48:06
#
#-------------------------------------------------

TEMPLATE = lib static
CONFIG += plugin static
QT += widgets

INCLUDEPATH += ../..

DESTDIR = ../../bin/plugins_preprocessor

SOURCES += gpsgrid.cpp

HEADERS += ../../interfaces/ipreprocessor.h \
	 ../../interfaces/iimporter.h \
	 ../../utils/coordinates.h \
	 ../../utils/config.h \
	 gpsgrid.h \
	 cell.h \
	 table.h \
	 ../../utils/bithelpers.h \
	 ../../utils/intersection.h \
	 ../../utils/qthelpers.h \
	 ../../utils/edgeconnector.h

unix {
	QMAKE_CXXFLAGS_RELEASE -= -O2
	QMAKE_CXXFLAGS_RELEASE += -O3 \
		 -Wno-unused-function
	QMAKE_CXXFLAGS_DEBUG += -Wno-unused-function
}

!nogui {
	SOURCES += ggdialog.cpp
	HEADERS += ggdialog.h
	FORMS += ggdialog.ui
}
nogui {
	DEFINES += NOGUI
	QT -= gui
}
