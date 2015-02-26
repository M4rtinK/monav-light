TEMPLATE = lib
CONFIG += plugin static
QT += widgets

INCLUDEPATH += ../..

DESTDIR = ../../bin/plugins_preprocessor
unix {
	QMAKE_CXXFLAGS_RELEASE -= -O2
	QMAKE_CXXFLAGS_RELEASE += -O3 \
		 -Wno-unused-function \
		 -fopenmp
	QMAKE_CXXFLAGS_DEBUG += -Wno-unused-function \
		 -fopenmp
}
LIBS += -fopenmp
HEADERS += contractionhierarchies.h \
	 dynamicgraph.h \
	 contractioncleanup.h \
	 blockcache.h \
	 binaryheap.h \
	 contractor.h \
	 ../../interfaces/ipreprocessor.h \
	 ../../utils/coordinates.h \
	 ../../utils/config.h \
	 compressedgraph.h \
	 compressedgraphbuilder.h \
	 ../../utils/bithelpers.h \
	 ../../utils/qthelpers.h \
	 ../../interfaces/irouter.h
SOURCES += contractionhierarchies.cpp

!nogui {
	SOURCES += chsettingsdialog.cpp
	FORMS += chsettingsdialog.ui
	HEADERS += chsettingsdialog.h
}
nogui {
	DEFINES += NOGUI
	QT -= gui
}
