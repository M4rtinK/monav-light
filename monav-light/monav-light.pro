TEMPLATE = app
DESTDIR = ../bin

CONFIG += link_pkgconfig

INCLUDEPATH += ..

DEFINES+=_7ZIP_ST

TARGET = monav-light
QT +=network

unix {
	QMAKE_CXXFLAGS_RELEASE -= -O2
	QMAKE_CXXFLAGS_RELEASE += -O3 \
		 -Wno-unused-function
	QMAKE_CXXFLAGS_DEBUG += -Wno-unused-function
}

LIBS += -L../bin/plugins_client -lcontractionhierarchiesclient -lgpsgridclient

SOURCES += \
	monav-light.cpp \
	../utils/lzma/LzmaDec.c \
	../utils/directoryunpacker.cpp

HEADERS += \
	signals.h \
	routingcommon.h \
	monav-light.h \
	../utils/lzma/LzmaDec.h \
	../utils/directoryunpacker.h \
