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
    ui/widgets/bingowidget.cpp \
    ui/widgets/colorcapturewidget.cpp \
    ui/widgets/multigamewidget.cpp \
    hardwareInterface/webcambutton.cpp \
    hardwareInterface/v4l2camera.cpp \
    p2pnetwork.cpp \
    matchingwidget.cpp


HEADERS  += mainwindow.h \
    ui/widgets/bingowidget.h \
    ui/widgets/colorcapturewidget.h \
    ui/widgets/multigamewidget.h \
    hardwareInterface/v4l2camera.h \
    hardwareInterface/webcambutton.h \
    matchingwidget.h \
    p2pnetwork.h

FORMS += mainwindow.ui
>>>>>>> 1e975f51e5dd3bb753c94a1fbb9d18a130d01f93

LIBS += -lpthread
