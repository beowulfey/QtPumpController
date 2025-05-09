QT       += widgets core gui printsupport serialport

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    comsdialog.cpp \
    condinterface.cpp \
    condworker.cpp \
    main.cpp \
    plotwidget.cpp \
    protocol.cpp \
    pumpcommandworker.cpp \
    pumpcontroller.cpp \
    $$PWD/libs/qcustomplot/qcustomplot.cpp \
    pumpinterface.cpp \
    tablemodel.cpp \
    utils.cpp

HEADERS += \
    comsdialog.h \
    condinterface.h \
    condworker.h \
    plotwidget.h \
    protocol.h \
    pumpcommands.h \
    pumpcommandworker.h \
    pumpcontroller.h \
    $$PWD/libs/qcustomplot/qcustomplot.h \
    pumpinterface.h \
    tablemodel.h \
    theming.h \
    utils.h

FORMS += \
    comsdialog.ui \
    pumpcontroller-bak.ui \
    pumpcontroller.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

VERSION_MAJOR = 1
VERSION_MINOR = 0
VERSION_BUILD = 2

DEFINES += "VERSION_MAJOR=$$VERSION_MAJOR"\
       "VERSION_MINOR=$$VERSION_MINOR"\
       "VERSION_BUILD=$$VERSION_BUILD"

#Target version
VERSION = $${VERSION_MAJOR}.$${VERSION_MINOR}.$${VERSION_BUILD}
