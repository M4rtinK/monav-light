# -------------------------------------------------
# Project created by QtCreator 2010-06-08T10:48:36
# -------------------------------------------------
TEMPLATE = app
CONFIG += link_pkgconfig
CONFIG += console
QT -= gui

DEFINES+=_7ZIP_ST

INCLUDEPATH += ..

PKGCONFIG += libxml-2.0
PKGCONFIG += protobuf
SOURCES += ../utils/commandlineparser.cpp \
	 pluginmanager.cpp \
	 ../utils/log.cpp \
	 console-main.cpp \
	 ../utils/directorypacker.cpp \
	 ../utils/lzma/LzmaEnc.c \
	 ../utils/lzma/LzFind.c
HEADERS += ../interfaces/iimporter.h \
	 ../utils/coordinates.h \
	 ../utils/config.h \
	 ../interfaces/ipreprocessor.h \
	 ../interfaces/iguisettings.h \
	 ../interfaces/iconsolesettings.h \
	 ../utils/commandlineparser.h \
	 ../utils/formattedoutput.h \
	 pluginmanager.h \
	 ../utils/log.h \
	 ../utils/directorypacker.h \
	 ../utils/lzma/Types.h \
	 ../utils/lzma/LzmaEnc.h \
	 ../utils/lzma/LzFind.h \
	 ../utils/lzma/LzHash.h \
    ../encoding/Encoder.h
DESTDIR = ../bin
TARGET = monav-preprocessor

unix {
	QMAKE_CXXFLAGS_RELEASE -= -O2
	QMAKE_CXXFLAGS_RELEASE += -O3 \
		 -Wno-unused-function \
		 -fopenmp
	QMAKE_CXXFLAGS_DEBUG += -Wno-unused-function \
		 -fopenmp
}
LIBS += -L../bin/plugins_preprocessor -lmapnikrenderer -lcontractionhierarchies -lgpsgrid -losmrenderer -lqtilerenderer -lunicodetournamenttrie -losmimporter -ltestimporter
LIBS += -fopenmp -lmapnik -lbz2 -lz
