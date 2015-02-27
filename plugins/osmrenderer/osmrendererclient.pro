TEMPLATE = lib
CONFIG += plugin static
QT += widgets

INCLUDEPATH += ../..

DESTDIR = ../../bin/plugins_client
unix {
	QMAKE_CXXFLAGS_RELEASE -= -O2
	QMAKE_CXXFLAGS_RELEASE += -O3 \
		 -Wno-unused-function
	QMAKE_CXXFLAGS_DEBUG += -Wno-unused-function
}
HEADERS += osmrendererclient.h \
	 ../../interfaces/irenderer.h \
	 ../../utils/coordinates.h \
	 ../../utils/config.h \
	 rendererbase.h \
	 brsettingsdialog.h \
	 ../../utils/intersection.h \
    osmrsettingsdialog.h
SOURCES += osmrendererclient.cpp \
	 rendererbase.cpp \
	 brsettingsdialog.cpp \
    osmrsettingsdialog.cpp
QT += network

FORMS += \
	 brsettingsdialog.ui \
    osmrsettingsdialog.ui

include(../../vars.pri)

sailfish {
	DEFINES+=NOGUI SAILFISH
	QT -= widgets
	HEADERS -= brsettingsdialog.h \
		osmrsettingsdialog.h
	SOURCES -= brsettingsdialog.cpp \
		osmrsettingsdialog.cpp
	FORMS -= brsettingsdialog.ui \
		osmrsettingsdialog.ui
}
