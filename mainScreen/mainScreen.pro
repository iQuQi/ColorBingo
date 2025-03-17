#-------------------------------------------------
#
# Project created by QtCreator 2025-03-11T16:05:14
#
#-------------------------------------------------

QT       += core gui network widgets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = mainScreen
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
        bingowidget.cpp \
        v4l2camera.cpp \
        colorcapturewidget.cpp \
        webcambutton.cpp \
    	multigamewidget.cpp \
        p2pnetwork.cpp \
        matchingwidget.cpp

HEADERS  += mainwindow.h \
    bingowidget.h \
    v4l2camera.h \
    colorcapturewidget.h \
    webcambutton.h \
    multigamewidget.h \
    matchingwidget.h \
    p2pnetwork.h

FORMS    += mainwindow.ui \

LIBS += -lpthread
