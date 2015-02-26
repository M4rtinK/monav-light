TEMPLATE = lib
TARGET = QtLocation
QT = core

DEFINES += QT_BUILD_LOCATION_LIB QT_MAKEDLL

INCLUDEPATH += .
DEPENDPATH += .

PUBLIC_HEADERS += qgeocoordinate.h \
                  qgeopositioninfo.h \
                  qgeosatelliteinfo.h \
                  qgeosatelliteinfosource.h \
                  qgeopositioninfosource.h \
                  qgeoareamonitor.h \
                  qnmeapositioninfosource.h

PRIVATE_HEADERS += qlocationutils_p.h \
                   qnmeapositioninfosource_p.h \
                   qgeopositioninfosource_android.h

HEADERS += $$PUBLIC_HEADERS $$PRIVATE_HEADERS

SOURCES += qlocationutils.cpp \
           qgeocoordinate.cpp \
           qgeopositioninfo.cpp \
           qgeosatelliteinfo.cpp \
           qgeosatelliteinfosource.cpp \
           qgeopositioninfosource.cpp \
           qgeoareamonitor.cpp \
           qnmeapositioninfosource.cpp \
           qgeopositioninfosource_android.cpp
