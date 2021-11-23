#-------------------------------------------------
#
# Project created by QtCreator 2019-08-30T23:36:04
#
#-------------------------------------------------

QT       += core gui xml network dbus svg

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = jamesdsp
TEMPLATE = app
!msvc {
    QMAKE_CXXFLAGS += "-Wno-old-style-cast -Wno-double-promotion -Wno-unused-function"
}

USE_PULSEAUDIO: DEFINES += USE_PULSEAUDIO

DEFINES += APP_VERSION=$$system(git describe --tags --long --always)
DEFINES += JDSP_VERSION=4.1.0

include(../3rdparty/3rdparty.pri)

include(audio/AudioDrivers.pri)

#include(subprojects/Visualization/SpectrumAudioViewer.pri)
include(subprojects/FlatTabWidget/FlatTabWidget.pri)
include(subprojects/GraphicEQWidget/GraphicEQWidget.pri)

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

CONFIG += c++17

SOURCES += \
    config/AppConfig.cpp \
    config/ConfigContainer.cpp \
    config/ConfigIO.cpp \
    crash/airbag.c \
    data/EelParser.cpp \
    data/PresetManager.cpp \
    data/PresetProvider.cpp \
    data/model/AppItemModel.cpp \
    data/model/QJsonTableModel.cpp \
    data/model/VdcDatabaseModel.cpp \
    interface/AnimatedJdspIcon.cpp \
    interface/CListView.cpp \
    interface/FileSelectionWidget.cpp \
    interface/LiveprogSelectionWidget.cpp \
    interface/TrayIcon.cpp \
    interface/dialog/AutoEqSelector.cpp \
    interface/dialog/CrashReportDialog.cpp \
    interface/dialog/PaletteEditor.cpp \
    interface/LiquidEqualizerWidget.cpp \
    interface/fragment/AppManagerFragment.cpp \
    interface/fragment/FirstLaunchWizard.cpp \
    interface/fragment/PresetFragment.cpp \
    interface/fragment/SettingsFragment.cpp \
    interface/fragment/StatusFragment.cpp \
    interface/item/AppItem.cpp \
    interface/item/DetailListItem.cpp \
    interface/QAnimatedSlider.cpp \
    interface/QMenuEditor.cpp \
    interface/QMessageOverlay.cpp \
    interface/SlidingStackedWidget.cpp \
    interface/WidgetMarqueeLabel.cpp \
    MainWindow.cpp \
    main.cpp \
    utils/AutoEqClient.cpp \
    utils/AutoStartManager.cpp \
    utils/Log.cpp \
    utils/SingleInstanceMonitor.cpp \
    utils/dbus/ClientProxy.cpp \
    utils/dbus/ServerAdaptor.cpp \
    utils/OverlayMsgProxy.cpp \
    utils/StyleHelper.cpp

FORMS += \
    interface/FileSelectionWidget.ui \
    interface/LiveprogSelectionWidget.ui \
    interface/dialog/AutoEqSelector.ui \
    interface/dialog/CrashReportDialog.ui \
    interface/dialog/PaletteEditor.ui \
    interface/fragment/AppManagerFragment.ui \
    interface/fragment/FirstLaunchWizard.ui \
    interface/fragment/PresetFragment.ui \
    interface/fragment/SettingsFragment.ui \
    interface/fragment/StatusFragment.ui \
    interface/item/AppItem.ui \
    interface/item/configitem.ui \
    interface/menueditor.ui \
    MainWindow.ui

HEADERS += \
    config/AppConfig.h \
    config/ConfigContainer.h \
    config/ConfigIO.h \
    config/DspConfig.h \
    crash/airbag.h \
    crash/killer.h \
    crash/safecall.h \
    crash/stacktrace.h \
    data/EelParser.h \
    data/InitializableQMap.h \
    data/PresetManager.h \
    data/PresetProvider.h \
    data/VersionContainer.h \
    data/model/AppItemModel.h \
    data/model/QJsonTableModel.h \
    data/model/VdcDatabaseModel.h \
    interface/AnimatedJdspIcon.h \
    interface/CListView.h \
    interface/FileSelectionWidget.h \
    interface/LiveprogSelectionWidget.h \
    interface/QMessageOverlay.h \
    interface/TrayIcon.h \
    interface/dialog/AutoEqSelector.h \
    interface/dialog/CrashReportDialog.h \
    interface/dialog/PaletteEditor.h \
    interface/event/EventFilter.h \
    interface/event/ScrollFilter.h \
    interface/LiquidEqualizerWidget.h \
    interface/fragment/AppManagerFragment.h \
    interface/fragment/BaseFragment.h \
    interface/fragment/FirstLaunchWizard.h \
    interface/fragment/FragmentHost.h \
    interface/fragment/FragmentHostPrivate.h \
    interface/fragment/PresetFragment.h \
    interface/fragment/SettingsFragment.h \
    interface/fragment/StatusFragment.h \
    interface/item/AppItem.h \
    interface/item/AppItemStyleDelegate.h \
    interface/item/Delegates.h \
    interface/item/DetailListItem.h \
    interface/QAnimatedSlider.h \
    interface/QMenuEditor.h \
    interface/SlidingStackedWidget.h \
    interface/WidgetMarqueeLabel.h \
    MainWindow.h \
    utils/AutoEqClient.h \
    utils/AutoStartManager.h \
    utils/Log.h \
    utils/SingleInstanceMonitor.h \
    utils/dbus/ClientProxy.h \
    utils/dbus/ServerAdaptor.h \
    utils/FindBinary.h \
    utils/OverlayMsgProxy.h \
    utils/StyleHelper.h

DISTFILES += utils/dbus/manifest.xml

RESOURCES += \
    ../resources/resources.qrc

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /usr/bin/

!isEmpty(target.path): INSTALLS += target

unix {
    LIBS += -ldl
    QMAKE_LFLAGS += -ldl -lutil #-fsanitize=address
    QMAKE_CXXFLAGS += -g #-fno-omit-frame-pointer -fsanitize=address

    CONFIG += link_pkgconfig

    PKGCONFIG += glibmm-2.4 giomm-2.4

    USE_PULSEAUDIO {
        PKGCONFIG += gstreamer-1.0 gstreamer-audio-1.0
        PKGCONFIG += libpulse
    }
    else {
        PKGCONFIG += libpipewire-0.3 libspa-0.2
    }
}

# Link libjamesdsp
unix:!macx: LIBS += -L$$OUT_PWD/../libjamesdsp -llibjamesdsp
INCLUDEPATH += $$PWD/../libjamesdsp/subtree/Main/libjamesdsp/jni/jamesdsp/jdsp/ \
               $$PWD/../libjamesdsp
DEPENDPATH += $$PWD/../libjamesdsp
unix:!macx: PRE_TARGETDEPS += $$OUT_PWD/../libjamesdsp/liblibjamesdsp.a

# Link libEELEditor
unix:!macx: LIBS += -L$$OUT_PWD/subprojects/EELEditor/src -leeleditor
INCLUDEPATH += $$PWD/subprojects/EELEditor/src \
               $$PWD/subprojects/EELEditor/3rdparty/QCodeEditor/include
DEPENDPATH += $$PWD/subprojects/EELEditor/src
unix:!macx: PRE_TARGETDEPS += $$OUT_PWD/subprojects/EELEditor/src/libeeleditor.a

# Link libqtadvanceddocking (libEELEditor)
LIBS += -L$$OUT_PWD/subprojects/EELEditor/3rdparty/docking-system/lib
include($$PWD/subprojects/EELEditor/3rdparty/docking-system/ads.pri)
INCLUDEPATH += $$PWD/subprojects/EELEditor/3rdparty/docking-system/src
DEPENDPATH += $$PWD/subprojects/EELEditor/3rdparty/docking-system/src
