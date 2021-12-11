INCLUDEPATH += $$PWD

FORMS += \
    $$PWD/AeqMeasurementItem.ui \
    $$PWD/AeqSelector.ui \
    $$PWD/FileDownloaderDialog.ui

HEADERS += \
    $$PWD/AeqListDelegates.h \
    $$PWD/AeqMeasurementItem.h \
    $$PWD/AeqMeasurementModel.h \
    $$PWD/AeqPackageManager.h \
    $$PWD/AeqPreviewPlot.h \
    $$PWD/AeqSelector.h \
    $$PWD/AeqStructs.h \
    $$PWD/GzipDownloader.h \
    $$PWD/GzipDownloaderDialog.h \
    $$PWD/HttpException.h \
    $$PWD/untar.h

SOURCES += \
    $$PWD/AeqMeasurementItem.cpp \
    $$PWD/AeqMeasurementModel.cpp \
    $$PWD/AeqPackageManager.cpp \
    $$PWD/AeqPreviewPlot.cpp \
    $$PWD/AeqSelector.cpp \
    $$PWD/GzipDownloader.cpp \
    $$PWD/GzipDownloaderDialog.cpp \
    $$PWD/HttpException.cpp
