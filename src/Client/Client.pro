TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

LIBS += -LD:\Biblioteki\SFML-2.4.2\lib

CONFIG(release, debug|release): LIBS += -lsfml-audio -lsfml-graphics -lsfml-main -lsfml-network -lsfml-window -lsfml-system
CONFIG(debug, debug|release): LIBS += -lsfml-audio-d -lsfml-graphics-d -lsfml-main-d -lsfml-network-d -lsfml-window-d -lsfml-system-d

INCLUDEPATH += "D:\Biblioteki\SFML-2.4.2\include"
DEPENDPATH += "D:\Biblioteki\SFML-2.4.2\include"

SOURCES += main.cpp \
    client.cpp

HEADERS += \
    client.h
