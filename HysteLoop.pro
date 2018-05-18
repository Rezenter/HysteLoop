#-------------------------------------------------
#
# Project created by QtCreator 2017-11-21T15:11:38
#
#-------------------------------------------------

QT       += core gui charts

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = HysteLoop
TEMPLATE = app

DEFINES += QT_DEPRECATED_WARNINGS

SOURCES += main.cpp\
        mainwindow.cpp \
    fileloader.cpp \
    extfsm.cpp \
    csvtable.cpp

HEADERS  += mainwindow.h \
    fileloader.h \
    extfsm.h \
    csvtable.h

FORMS    += mainwindow.ui
