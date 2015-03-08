TEMPLATE = lib
CONFIG += plugin static

INCLUDEPATH += ../..

DEFINES += NOGUI
QT -= gui

DESTDIR = ../../bin/plugins_client
unix {
	QMAKE_CXXFLAGS_RELEASE -= -O2
	QMAKE_CXXFLAGS_RELEASE += -O3 \
		 -Wno-unused-function -std=c++0x
	QMAKE_CXXFLAGS_DEBUG += -Wno-unused-function -std=c++0x
}

HEADERS += \
	 ../../utils/coordinates.h \
	 ../../utils/config.h \
	 blockcache.h \
	 binaryheap.h \
	 ../../interfaces/irouter.h \
	 contractionhierarchiesclient.h \
	 compressedgraph.h \
	 ../../interfaces/igpslookup.h \
	 ../../utils/bithelpers.h \
	 ../../utils/qthelpers.h

SOURCES += \
	 contractionhierarchiesclient.cpp

include(../../vars.pri)
