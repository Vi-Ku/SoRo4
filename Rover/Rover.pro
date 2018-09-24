QT -= gui

CONFIG += c++11 console
CONFIG -= app_bundle

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

TEMPLATE = subdirs

SUBDIRS = \
    autonomous \
    slaveArm \
    driveSystem \
    roverMain \
    videoServer

CONFIG += ordered

roverMain.depends = autonomous slaveArm driveSystem
roverMain.file = roverMain/roverMain.pro
autonomous.file = autonomous/autonomous.pro
slaveArm.file = slaveArm/slaveArm.pro
driveSystem.file = driveSystem/driveSystem.pro

# Default rules for deployment.
#qnx: target.path = /tmp/$${TARGET}/bin
#else: unix:!android: target.path = /opt/$${TARGET}/bin
#!isEmpty(target.path): INSTALLS += target
