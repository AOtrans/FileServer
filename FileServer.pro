UI_DIR  = obj/Gui
MOC_DIR = obj/Moc
OBJECTS_DIR = obj/Obj

QT += core network gui

CONFIG += c++11

TARGET = FileServer
CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app

SOURCES += main.cpp \
    http/http_parser.cpp \
    http/httpconnection.cpp \
    http/httprequest.cpp \
    http/httpresponse.cpp \
    http/httpserver.cpp

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

HEADERS += \
    http/http_parser.h \
    http/httpconnection.h \
    http/httprequest.h \
    http/httpresponse.h \
    http/httpserver.h \
    http/common.h \
    common.h
