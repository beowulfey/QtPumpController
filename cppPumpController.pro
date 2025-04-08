QT       += widgets core gui printsupport serialport

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    comsdialog.cpp \
    condmeter.cpp \
    main.cpp \
    plotwidget.cpp \
    pump.cpp \
    pumpcontroller.cpp \
    $$PWD/libs/qcustomplot/qcustomplot.cpp \
    tablemodel.cpp

HEADERS += \
    comsdialog.h \
    condmeter.h \
    plotwidget.h \
    pump.h \
    pumpcontroller.h \
    $$PWD/libs/qcustomplot/qcustomplot.h \
    tablemodel.h

FORMS += \
    pumpcontroller.ui \
    comsdialog.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
