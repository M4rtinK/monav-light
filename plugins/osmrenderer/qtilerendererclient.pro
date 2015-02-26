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
HEADERS += qtilerendererclient.h \
    tile-write.h \
    types.h \
    img_writer.h \
	 ../../interfaces/irenderer.h \
	 ../../utils/coordinates.h \
	 ../../utils/config.h \
    rendererbase.h \
    brsettingsdialog.h \
    ../../utils/qthelpers.h
SOURCES += qtilerendererclient.cpp \
    rendererbase.cpp \
    brsettingsdialog.cpp \
    tile-write.cpp \
    agg2/agg_vcgen_stroke.cpp
QT += network


FORMS += \
    brsettingsdialog.ui
