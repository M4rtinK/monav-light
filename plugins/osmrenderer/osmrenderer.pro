TEMPLATE = lib
CONFIG += plugin static
QT += widgets

INCLUDEPATH += ../..

HEADERS += osmrenderer.h \
	 ../../interfaces/ipreprocessor.h \
	 ../../interfaces/iimporter.h \
	 ../../utils/coordinates.h \
	 ../../utils/config.h
SOURCES += osmrenderer.cpp
DESTDIR = ../../bin/plugins_preprocessor
unix {
	QMAKE_CXXFLAGS_RELEASE -= -O2
	QMAKE_CXXFLAGS_RELEASE += -O3 \
		 -Wno-unused-function
	QMAKE_CXXFLAGS_DEBUG += -Wno-unused-function
}

!nogui {
	SOURCES += orsettingsdialog.cpp
	HEADERS += orsettingsdialog.h
	FORMS += orsettingsdialog.ui
}
nogui {
	DEFINES += NOGUI
	QT -= gui
}
