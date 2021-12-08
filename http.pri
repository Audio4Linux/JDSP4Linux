QT *= network

INCLUDEPATH += $$PWD/src
DEPENDPATH += $$PWD/src
DEFINES += HTTP

HEADERS += \
    $$PWD/src/cachedhttp.h \
    $$PWD/src/http.h \
    $$PWD/src/httpreply.h \
    $$PWD/src/httprequest.h \
    $$PWD/src/localcache.h \
    $$PWD/src/networkhttpreply.h \
    $$PWD/src/throttledhttp.h

SOURCES += \
    $$PWD/src/cachedhttp.cpp \
    $$PWD/src/http.cpp \
    $$PWD/src/httpreply.cpp \
    $$PWD/src/localcache.cpp \
    $$PWD/src/networkhttpreply.cpp \
    $$PWD/src/throttledhttp.cpp
