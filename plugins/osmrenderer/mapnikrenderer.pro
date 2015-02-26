TEMPLATE = lib
CONFIG += plugin static

INCLUDEPATH += ../..

CONFIG += link_pkgconfig
PKGCONFIG += freetype2
HEADERS += mapnikrenderer.h \
	 ../../interfaces/ipreprocessor.h \
	 ../../interfaces/iimporter.h \
	 ../../utils/coordinates.h \
	 ../../utils/config.h \
	 ../../utils/qthelpers.h
SOURCES += mapnikrenderer.cpp
DESTDIR = ../../bin/plugins_preprocessor
unix {
	QMAKE_CXXFLAGS_RELEASE -= -O2
	QMAKE_CXXFLAGS_RELEASE += -O3 \
		 -Wno-unused-function \
		 -fopenmp
	QMAKE_CXXFLAGS_DEBUG += -Wno-unused-function \
		 -fopenmp
}
LIBS += -fopenmp \
	 -lmapnik

!nogui {
	FORMS += mrsettingsdialog.ui
	SOURCES += mrsettingsdialog.cpp
	HEADERS += mrsettingsdialog.h
}
nogui {
	DEFINES += NOGUI
	QT -= gui
}
