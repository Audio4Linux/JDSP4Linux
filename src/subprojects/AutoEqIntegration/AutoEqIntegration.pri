INCLUDEPATH += $$PWD

FORMS += \
    $$PWD/AutoEqSelector.ui \
    $$PWD/FileDownloaderDialog.ui

HEADERS += \
    $$PWD/AeqPackageManager.h \
    $$PWD/AeqStructs.h \
    $$PWD/AutoEqClient.h \
    $$PWD/AutoEqSelector.h \
    $$PWD/FileDownloader.h \
    $$PWD/FileDownloaderDialog.h \
    $$PWD/HttpException.h

SOURCES += \
    $$PWD/AeqPackageManager.cpp \
    $$PWD/AutoEqClient.cpp \
    $$PWD/AutoEqSelector.cpp \
    $$PWD/FileDownloader.cpp \
    $$PWD/FileDownloaderDialog.cpp \
    $$PWD/HttpException.cpp
