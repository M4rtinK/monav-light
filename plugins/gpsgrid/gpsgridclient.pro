#-------------------------------------------------
#
# Project created by QtCreator 2010-06-25T08:48:06
#
#-------------------------------------------------

TEMPLATE = lib
CONFIG += plugin static

INCLUDEPATH += ../..

DESTDIR = ../../bin/plugins_client

nogui {
	DEFINES+=NOGUI
	QT -= gui
}

HEADERS += \
	 ../../utils/coordinates.h \
	 ../../utils/config.h \
	 cell.h \
	 ../../interfaces/igpslookup.h \
	 gpsgridclient.h \
	 table.h \
	 ../../utils/bithelpers.h \
	 ../../utils/qthelpers.h

unix {
	QMAKE_CXXFLAGS_RELEASE -= -O2
	QMAKE_CXXFLAGS_RELEASE += -O3 \
		 -Wno-unused-function
	QMAKE_CXXFLAGS_DEBUG += -Wno-unused-function
}

SOURCES += \
	 gpsgridclient.cpp
