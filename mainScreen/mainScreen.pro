#-------------------------------------------------
#
# Project created by QtCreator 2025-03-11T16:05:14
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = mainScreen
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
        bingowidget.cpp \
        v4l2camera.cpp \
        colorcapturewidget.cpp

HEADERS  += mainwindow.h \
    bingowidget.h \
    v4l2camera.h \
    colorcapturewidget.h

FORMS    += mainwindow.ui

LIBS += -lpthread