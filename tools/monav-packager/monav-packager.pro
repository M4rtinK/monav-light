#-------------------------------------------------
#
# Project created by QtCreator 2011-02-02T16:06:38
#
#-------------------------------------------------

QT       += core

QT       -= gui

INCLUDEPATH += ../..
DEFINES+=_7ZIP_ST


TARGET = monav-packager
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app


SOURCES += main.cpp \
	 ../../utils/directoryunpacker.cpp \
	 ../../utils/directorypacker.cpp \
	 ../../utils/lzma/LzmaEnc.c \
	 ../../utils/lzma/LzmaDec.c \
	 ../../utils/lzma/LzFind.c \
    ../../utils/log.cpp

HEADERS += \
	 ../../utils/qthelpers.h \
	 ../../utils/directoryunpacker.h \
	 ../../utils/directorypacker.h \
	 ../../utils/lzma/Types.h \
	 ../../utils/lzma/LzmaEnc.h \
	 ../../utils/lzma/LzmaDec.h \
	 ../../utils/lzma/LzHash.h \
	 ../../utils/lzma/LzFind.h \
    ../../utils/log.h
