TEMPLATE = app
TARGET = harbour-monav

CONFIG += sailfishapp

DEFINES += SAILFISH

QT = core gui quick positioning svg dbus

OTHER_FILES += \
    qml/Main.qml \
    qml/MainviewPage.qml \
    qml/TapMenuPage.qml \
    qml/MapPackagesPage.qml \
    qml/BookmarksPage.qml

SOURCES += \
    main.cpp \
    Mainview.cpp \
    TapMenu.cpp \
    MapPackages.cpp \
    Bookmarks.cpp \
    activationchangeeventfilter.cpp \
    ../client/paintwidget.cpp \
    ../client/mapdata.cpp \
    ../client/routinglogic.cpp \
    ../client/logger.cpp \
    ../client/globalsettings.cpp \
    ../client/gpsdpositioninfosource.cpp \
    ../client/json.cpp \
    ../client/worldmapchooser.cpp \
    ../utils/directoryunpacker.cpp \
    ../utils/lzma/LzmaDec.c
    
HEADERS += \
    Mainview.h \
    TapMenu.h \
    MapPackages.h \
    Bookmarks.h \
	activationchangeeventfilter.h \
	../client/paintwidget.h \
	../client/mapdata.h \
	../client/routinglogic.h \
	../client/logger.h \
	../client/globalsettings.h \
	../client/gpsdpositioninfosource.h \
	../client/json.h \
	../client/worldmapchooser.h \
	../interfaces/irenderer.h \
	../interfaces/irouter.h \
	../utils/directoryunpacker.h \
	../utils/lzma/LzmaDec.h

DESTDIR = ../

LIBS += -L../bin/plugins_client -lmapnikrendererclient -lcontractionhierarchiesclient -lgpsgridclient -losmrendererclient -lunicodetournamenttrieclient -lqtilerendererclient

INCLUDEPATH += ..

RESOURCES += ../client/images.qrc
