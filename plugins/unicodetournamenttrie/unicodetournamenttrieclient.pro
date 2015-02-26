#-------------------------------------------------
#
# Project created by QtCreator 2010-06-22T13:51:45
#
#-------------------------------------------------

DESTDIR = ../../bin/plugins_client
TEMPLATE = lib
CONFIG += plugin static
QT += widgets

INCLUDEPATH += ../..

HEADERS += ../../utils/coordinates.h \
	 ../../utils/config.h \
	 ../../interfaces/iaddresslookup.h \
	 trie.h \
	 unicodetournamenttrieclient.h \
	 ../../utils/qthelpers.h

unix {
	QMAKE_CXXFLAGS_RELEASE -= -O2
	QMAKE_CXXFLAGS_RELEASE += -O3 -Wno-unused-function
	QMAKE_CXXFLAGS_DEBUG += -Wno-unused-function
}

SOURCES += \
	 unicodetournamenttrieclient.cpp
