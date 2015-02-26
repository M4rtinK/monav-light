# -------------------------------------------------
# Project created by QtCreator 2010-06-15T15:30:10
# -------------------------------------------------

TARGET = MoNavClient
TEMPLATE = app

QT += svg xml

INCLUDEPATH += ..

SOURCES += main.cpp \
	paintwidget.cpp \
	addressdialog.cpp \
	bookmarksdialog.cpp \
	routedescriptiondialog.cpp \
	mapdata.cpp \
	routinglogic.cpp \
	overlaywidget.cpp \
	scrollarea.cpp \
	gpsdialog.cpp \
	generalsettingsdialog.cpp \
	logger.cpp \
    ../utils/directoryunpacker.cpp \
    ../utils/lzma/LzmaDec.c \
    mappackageswidget.cpp \
    mainwindow.cpp \
    mapmoduleswidget.cpp \
    placechooser.cpp \
    globalsettings.cpp \
    streetchooser.cpp \
         gpsdpositioninfosource.cpp \
	 json.cpp \
    serverinputdialog.cpp \
    serverlogic.cpp \
    worldmapchooser.cpp

HEADERS += \
	paintwidget.h \
	../utils/coordinates.h \
	../utils/config.h \
	../interfaces/irenderer.h \
	../interfaces/iaddresslookup.h \
	addressdialog.h \
	../interfaces/igpslookup.h \
	../interfaces/irouter.h \
	bookmarksdialog.h \
	routedescriptiondialog.h \
	descriptiongenerator.h \
	mapdata.h \
	routinglogic.h \
	fullscreenexitbutton.h \
	overlaywidget.h \
	scrollarea.h \
	gpsdialog.h \
	generalsettingsdialog.h \
	logger.h \
    ../utils/directoryunpacker.h \
    ../utils/lzma/LzmaDec.h \
    mappackageswidget.h \
    mainwindow.h \
    mapmoduleswidget.h \
    placechooser.h \
    globalsettings.h \
    streetchooser.h \
	 json.h \
	 gpsdpositioninfosource.h \
    serverinputdialog.h \
    serverlogic.h \
    worldmapchooser.h

FORMS += \
	paintwidget.ui \
	addressdialog.ui \
	bookmarksdialog.ui \
	routedescriptiondialog.ui \
	gpsdialog.ui \
	generalsettingsdialog.ui \
    mappackageswidget.ui \
    mainwindow.ui \
    mapmoduleswidget.ui \
    placechooser.ui \
    streetchooser.ui \
    serverinputdialog.ui

DESTDIR = ../bin

TARGET = monav
unix {
	QMAKE_CXXFLAGS_RELEASE -= -O2
	QMAKE_CXXFLAGS_RELEASE += -O3 \
		-Wno-unused-function
	QMAKE_CXXFLAGS_DEBUG += -Wno-unused-function
}
maemo5 {
	QT += maemo5
}

RESOURCES += images.qrc

win32 {
    RC_FILE = ../images/WindowsResources.rc
}

macx {
    ICON = ../images/AppIcons.icns
}

LIBS += -L../bin/plugins_client -lmapnikrendererclient -lcontractionhierarchiesclient -lgpsgridclient -losmrendererclient -lunicodetournamenttrieclient -lqtilerendererclient

# Required by osmrendererclient
QT += network
CONFIG += mobility
MOBILITY += location
# Required to get a non-debug build (at least on Windows)
# CONFIG += release

unix:!symbian {
    maemo5 {
        target.path = /opt/usr/bin
    } else {
        target.path = /usr/local/bin
    }
    INSTALLS += target
}
