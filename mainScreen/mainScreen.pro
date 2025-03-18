#-------------------------------------------------
#
# Project created by QtCreator 2025-03-11T16:05:14
#
#-------------------------------------------------

QT       += core gui network widgets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = mainScreen
TEMPLATE = app

# ALSA 직접 지정 (pkg-config 대신 사용)
# 아래 경로는 실제 ALSA 설치 경로로 수정하세요
# 아래 alsa 코드 필요한지 확인 필요
INCLUDEPATH += /home/user/work/alsa/install/include
LIBS += -L/home/user/work/alsa/install/lib -lasound
# pthread는 그대로 유지
LIBS += -lpthread
RESOURCES += resources/resources.qrc

SOURCES += main.cpp\
    mainwindow.cpp \
    ui/widgets/bingowidget.cpp \
    ui/widgets/bingopreparationwidget.cpp \
    ui/widgets/multigamewidget.cpp \
    hardwareInterface/webcambutton.cpp \
    hardwareInterface/v4l2camera.cpp \
    p2pnetwork.cpp \
    matchingwidget.cpp \
    hardwareInterface/SoundManager.cpp \
    utils/pixelartgenerator.cpp


HEADERS  += mainwindow.h \
    ui/widgets/bingowidget.h \
    ui/widgets/bingopreparationwidget.h \
    ui/widgets/multigamewidget.h \
    hardwareInterface/v4l2camera.h \
    hardwareInterface/webcambutton.h \
    matchingwidget.h \
    p2pnetwork.h \
    hardwareInterface/SoundManager.h \
    utils/pixelartgenerator.h

FORMS += mainwindow.ui

