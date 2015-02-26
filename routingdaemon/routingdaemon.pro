TEMPLATE = app
DESTDIR = ../bin

CONFIG += link_pkgconfig
PKGCONFIG += protobuf

PROTOS = signals.proto
include(../utils/osm/protobuf.pri)
include(../utils/osm/protobuf_python.pri)

PRE_TARGETDEPS += signals.pb.h signals.pb.cc signals_pb2.py

INCLUDEPATH += ..

DEFINES+=_7ZIP_ST

TARGET = monav-daemon
QT -= gui
QT +=network

unix {
	QMAKE_CXXFLAGS_RELEASE -= -O2
	QMAKE_CXXFLAGS_RELEASE += -O3 \
		 -Wno-unused-function
	QMAKE_CXXFLAGS_DEBUG += -Wno-unused-function
}

LIBS += -L../bin/plugins_client -lcontractionhierarchiesclient -lgpsgridclient

SOURCES += \
	 routingdaemon.cpp \
	 ../utils/lzma/LzmaDec.c \
	 ../utils/directoryunpacker.cpp

HEADERS += \
	 signals.h \
	 routingcommon.h \
	 routingdaemon.h \
	 ../utils/lzma/LzmaDec.h \
	 ../utils/directoryunpacker.h

include(qtservice-2.6_1-opensource/src/qtservice.pri)
