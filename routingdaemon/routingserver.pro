TEMPLATE = app
DESTDIR = ../bin

CONFIG += link_pkgconfig
PKGCONFIG += protobuf
QT += widgets

PROTOS = signals.proto
include(../utils/osm/protobuf.pri)
include(../utils/osm/protobuf_python.pri)

PRE_TARGETDEPS += signals.pb.h signals.pb.cc signals_pb2.py

INCLUDEPATH += ..

DEFINES+=_7ZIP_ST

TARGET = monav-server
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
	 routingserver.cpp \
	 ../utils/lzma/LzmaDec.c \
	 ../utils/directoryunpacker.cpp

HEADERS += \
	 signals.h \
	 routingcommon.h \
	 routingserver.h \
	 ../utils/lzma/LzmaDec.h \
	 ../utils/directoryunpacker.h
