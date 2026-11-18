QT = core testlib
CONFIG += console testcase c++17
TEMPLATE = app
TARGET = tst_liveprogwatcher
SOURCES += tst_liveprogwatcher.cpp ../../src/utils/LiveprogFileWatcher.cpp
HEADERS += ../../src/utils/LiveprogFileWatcher.h
INCLUDEPATH += ../../src/utils
