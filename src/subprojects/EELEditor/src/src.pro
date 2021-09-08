TARGET = eeleditor
QT       += core gui
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17
include(EELEditor.pri)

COMPILE_STANDALONE {
    # The following define makes your compiler emit warnings if you use
    # any Qt feature that has been marked deprecated (the exact warnings
    # depend on your compiler). Please consult the documentation of the
    # deprecated API in order to know how to port your code away from it.
    DEFINES += QT_DEPRECATED_WARNINGS

    SOURCES += \
        main.cpp

    # Default rules for deployment.
    qnx: target.path = /tmp/$${TARGET}/bin
    else: unix:!android: target.path = /opt/$${TARGET}/bin
    !isEmpty(target.path): INSTALLS += target
}
else {
    CONFIG += staticlib
    TEMPLATE = lib
}

include($$PWD/EELEditor-Linker.pri)
