# -------------------------------------------------
# Project created by QtCreator 2010-06-08T10:48:36
# -------------------------------------------------
TEMPLATE = app
CONFIG += link_pkgconfig

INCLUDEPATH += ..

DEFINES+=_7ZIP_ST

PKGCONFIG += libxml-2.0
PKGCONFIG += protobuf
SOURCES += main.cpp \
	 preprocessingwindow.cpp \
	 ../utils/commandlineparser.cpp \
	 pluginmanager.cpp \
	 ../utils/log.cpp \
	 ../utils/logwindow.cpp \
	 ../utils/lzma/LzmaEnc.c \
	 ../utils/lzma/LzFind.c \
    ../utils/directorypacker.cpp
HEADERS += preprocessingwindow.h \
	 ../interfaces/iimporter.h \
	 ../utils/coordinates.h \
	 ../utils/config.h \
	 ../interfaces/ipreprocessor.h \
	 ../interfaces/ipreprocessor.h \
	 ../interfaces/iimporter.h \
	 ../interfaces/iguisettings.h \
	 ../interfaces/iconsolesettings.h \
	 ../utils/commandlineparser.h \
	 ../utils/formattedoutput.h \
	 pluginmanager.h \
	 ../utils/log.h \
	 ../utils/logwindow.h \
	 ../utils/lzma/Types.h \
	 ../utils/lzma/LzmaEnc.h \
	 ../utils/lzma/LzHash.h \
	 ../utils/lzma/LzFind.h \
    ../utils/directorypacker.h
DESTDIR = ../bin
TARGET = monav-preprocessor-gui
FORMS += preprocessingwindow.ui \
	 ../utils/logwindow.ui
RESOURCES += images.qrc
unix {
	QMAKE_CXXFLAGS_RELEASE -= -O2
	QMAKE_CXXFLAGS_RELEASE += -O3 \
		 -Wno-unused-function \
		 -fopenmp
	QMAKE_CXXFLAGS_DEBUG += -Wno-unused-function \
		 -fopenmp
}
LIBS += -L../bin/plugins_preprocessor -lmapnikrenderer -lcontractionhierarchies -lgpsgrid -losmrenderer -lqtilerenderer -lunicodetournamenttrie -losmimporter
LIBS += -fopenmp -lmapnik -lbz2 -lz
