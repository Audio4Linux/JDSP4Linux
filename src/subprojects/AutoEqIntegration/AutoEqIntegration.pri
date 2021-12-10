INCLUDEPATH += $$PWD

FORMS += \
    $$PWD/AutoEqSelector.ui \
    $$PWD/FileDownloaderDialog.ui

HEADERS += \
    $$PWD/AeqPackageManager.h \
    $$PWD/AeqStructs.h \
    $$PWD/AutoEqSelector.h \
    $$PWD/GzipDownloader.h \
    $$PWD/GzipDownloaderDialog.h \
    $$PWD/HttpException.h \
    $$PWD/untar.h

SOURCES += \
    $$PWD/AeqPackageManager.cpp \
    $$PWD/AutoEqSelector.cpp \
    $$PWD/GzipDownloader.cpp \
    $$PWD/GzipDownloaderDialog.cpp \
    $$PWD/HttpException.cpp
