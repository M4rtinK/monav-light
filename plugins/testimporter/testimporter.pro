TEMPLATE = lib
CONFIG += plugin static

INCLUDEPATH += ../..

HEADERS += \
    testimporter.h
SOURCES += \
    testimporter.cpp
DESTDIR = ../../bin/plugins_preprocessor
unix {
	QMAKE_CXXFLAGS_RELEASE -= -O2
	QMAKE_CXXFLAGS_RELEASE += -O3 -Wno-unused-function
	QMAKE_CXXFLAGS_DEBUG += -Wno-unused-function
}

!nogui {
}
nogui {
	DEFINES += NOGUI
	QT -= gui
}
