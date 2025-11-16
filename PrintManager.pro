QT += core widgets
QT -= gui

CONFIG += c++17

TARGET = PrintManagerGUI
TEMPLATE = app

SOURCES += \
    main_gui.cpp \
    mainwindow.cpp

HEADERS += \
    mainwindow.h \
    printmanager.h

