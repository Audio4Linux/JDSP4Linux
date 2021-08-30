#-------------------------------------------------
#
# Project created by QtCreator 2019-08-30T23:36:04
#
#-------------------------------------------------

QT       += core gui xml network dbus svg multimedia

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = jdsp-gui
TEMPLATE = app
!msvc {
    QMAKE_CXXFLAGS += "-Wno-old-style-cast -Wno-double-promotion -Wno-unused-function"
}

include(../3rdparty/3rdparty.pri)

include(audio/AudioCore.pri)

include(subprojects/Visualization/SpectrumAudioViewer.pri)
include(subprojects/FlatTabWidget/FlatTabWidget.pri)
include(subprojects/GraphicEQWidget/GraphicEQWidget.pri)
include(subprojects/EELEditor/EELEditor.pri)

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

CONFIG += c++17

SOURCES += \
    config/ConfigContainer.cpp \
    config/ConfigIO.cpp \
    crash/airbag.c \
    data/EelParser.cpp \
    data/PresetProvider.cpp \
    data/QJsonTableModel.cpp \
    interface/AnimatedJdspIcon.cpp \
    interface/TrayIcon.cpp \
    interface/dialog/AutoEqSelector.cpp \
    interface/dialog/PaletteEditor.cpp \
    interface/LiquidEqualizerWidget.cpp \
    interface/dialog/PresetDialog.cpp \
    interface/fragment/FirstLaunchWizard.cpp \
    interface/fragment/SettingsFragment.cpp \
    interface/fragment/StatusFragment.cpp \
    interface/model/DetailListItem.cpp \
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
    utils/dbus/ClientProxy.cpp \
    utils/dbus/ServerAdaptor.cpp \
    utils/OverlayMsgProxy.cpp \
    utils/StyleHelper.cpp

FORMS += \
    interface/dialog/AutoEqSelector.ui \
    interface/dialog/PaletteEditor.ui \
    interface/dialog/PresetDialog.ui \
    interface/fragment/FirstLaunchWizard.ui \
    interface/fragment/SettingsFragment.ui \
    interface/fragment/StatusFragment.ui \
    interface/model/configitem.ui \
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
    data/PresetProvider.h \
    data/QJsonTableModel.h \
    data/VersionContainer.h \
    interface/AnimatedJdspIcon.h \
    interface/QMessageOverlay.h \
    interface/TrayIcon.h \
    interface/dialog/AutoEqSelector.h \
    interface/dialog/PaletteEditor.h \
    interface/dialog/PresetDialog.h \
    interface/event/EventFilter.h \
    interface/event/ScrollFilter.h \
    interface/LiquidEqualizerWidget.h \
    interface/fragment/FirstLaunchWizard.h \
    interface/fragment/SettingsFragment.h \
    interface/fragment/StatusFragment.h \
    interface/model/Delegates.h \
    interface/model/DetailListItem.h \
    interface/QAnimatedSlider.h \
    interface/QMenuEditor.h \
    interface/SlidingStackedWidget.h \
    interface/WidgetMarqueeLabel.h \
    MainWindow.h \
    utils/AutoEqClient.h \
    utils/AutoStartManager.h \
    utils/Common.h \
    utils/JdspImpResToolbox.h \
    utils/Log.h \
    utils/dbus/ClientProxy.h \
    utils/dbus/ServerAdaptor.h \
    utils/FindBinary.h \
    utils/MathFunctions.h \
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
    QMAKE_LFLAGS += -ldl -lutil
    QMAKE_CXXFLAGS += -g

    CONFIG += link_pkgconfig
    PKGCONFIG += gstreamer-1.0 gstreamer-audio-1.0
    PKGCONFIG += libpulse glibmm-2.4 giomm-2.4
}

# Link libjamesdsp
unix:!macx: LIBS += -L$$OUT_PWD/../libjamesdsp/ -llibjamesdsp
INCLUDEPATH += $$PWD/../libjamesdsp/subtree/Main/libjamesdsp/jni/jamesdsp/jdsp/
DEPENDPATH += $$PWD/../libjamesdsp
unix:!macx: PRE_TARGETDEPS += $$OUT_PWD/../libjamesdsp/liblibjamesdsp.a
