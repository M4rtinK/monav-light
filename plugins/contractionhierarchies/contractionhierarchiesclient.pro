TEMPLATE = lib
CONFIG += plugin static

INCLUDEPATH += ../..

DESTDIR = ../../bin/plugins_client
unix {
	QMAKE_CXXFLAGS_RELEASE -= -O2
	QMAKE_CXXFLAGS_RELEASE += -O3 \
		 -Wno-unused-function
	QMAKE_CXXFLAGS_DEBUG += -Wno-unused-function
}

nogui {
	DEFINES+=NOGUI
	QT -= gui
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
