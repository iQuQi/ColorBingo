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
    ui/widgets/bingowidget.cpp \
    ui/widgets/colorcapturewidget.cpp \
    ui/widgets/multigamewidget.cpp \
    hardwareInterface/webcambutton.cpp \
    hardwareInterface/v4l2camera.cpp \

HEADERS  += mainwindow.h \
    ui/widgets/bingowidget.h \
    ui/widgets/colorcapturewidget.h \
    ui/widgets/multigamewidget.h \
    hardwareInterface/v4l2camera.h \
    hardwareInterface/webcambutton.h

FORMS += mainwindow.ui

LIBS += -lpthread
