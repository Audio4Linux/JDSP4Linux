TEMPLATE = lib
TARGET = webloader
DEPENDPATH += . src
INCLUDEPATH += . src

CONFIG += c++11

QT += network xml

HEADERS += src/HttpMultiPart_p.h \
	   src/NetworkQueue_p.h \
	   src/NetworkRequest.h \
	   src/NetworkRequestPrivate_p.h \
	   src/WebLoader_p.h \
	   src/WebRequest_p.h \
	   src/NetworkRequestLoader.h

SOURCES += src/HttpMultiPart_p.cpp \
	   src/WebLoader_p.cpp \
	   src/WebRequest_p.cpp \
	   src/NetworkQueue_p.cpp \
	   src/NetworkRequest.cpp
