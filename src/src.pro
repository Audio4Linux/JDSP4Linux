#-------------------------------------------------
#
# Project created by QtCreator 2019-08-30T23:36:04
#
#-------------------------------------------------

QT       += core gui xml network dbus svg

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = jamesdsp
TEMPLATE = app

USE_PULSEAUDIO: DEFINES += USE_PULSEAUDIO
USE_PORTALS: DEFINES += USE_PORTALS
NO_CRASH_HANDLER: DEFINES += NO_CRASH_HANDLER
!unix {
    DEFINES += NO_CRASH_HANDLER
}

USE_PULSEAUDIO {
    DEFINES += FLATPAK_APP_ID=\\\"me.timschneeberger.jdsp4linux.pulse\\\"
}
else {
    DEFINES += FLATPAK_APP_ID=\\\"me.timschneeberger.jdsp4linux.pipewire\\\"
}

DEFINES += APP_VERSION=$$system(git describe --tags --long --always)
DEFINES += JDSP_VERSION=3.12

include(../3rdparty/3rdparty.pri)

include(audio/AudioDrivers.pri)

include(subprojects/AutoEqIntegration/AutoEqIntegration.pri)
include(subprojects/FlatTabWidget/FlatTabWidget/FlatTabWidget.pri)
include(subprojects/LiquidEqualizerWidget/LiquidEqualizerWidget.pri)
include(subprojects/GraphicEQWidget/GraphicEQWidget/GraphicEQWidget.pri)

DEFINES += HAS_JDSP_DRIVER
include(subprojects/EELEditor/src/EELEditor.pri)

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

QMAKE_CFLAGS += "-Wno-unused-variable -Wno-unused-function -Wno-unused-const-variable"
QMAKE_CXXFLAGS += "-Wno-deprecated-enum-enum-conversion -Wno-missing-field-initializers -Wno-unused-function -Wno-unused-parameter"

CONFIG += c++2a

SOURCES += \
    config/AppConfig.cpp \
    config/ConfigContainer.cpp \
    config/ConfigIO.cpp \
    data/AssetManager.cpp \
    data/EelParser.cpp \
    data/PresetManager.cpp \
    data/PresetProvider.cpp \
    data/model/AppItemModel.cpp \
    data/model/DeviceListModel.cpp \
    data/model/PresetListModel.cpp \
    data/model/PresetRuleTableModel.cpp \
    data/model/QJsonTableModel.cpp \
    data/model/VdcDatabaseModel.cpp \
    interface/AnimatedJdspIcon.cpp \
    interface/CListView.cpp \
    interface/CTableView.cpp \
    interface/FadingLabel.cpp \
    interface/FileSelectionWidget.cpp \
    interface/LiveprogSelectionWidget.cpp \
    interface/TrayIcon.cpp \
    interface/dialog/PaletteEditor.cpp \
    interface/dialog/PresetRuleDialog.cpp \
    interface/fragment/AppManagerFragment.cpp \
    interface/fragment/FirstLaunchWizard.cpp \
    interface/fragment/PresetAddRuleFragment.cpp \
    interface/fragment/PresetFragment.cpp \
    interface/fragment/SettingsFragment.cpp \
    interface/fragment/StatusFragment.cpp \
    interface/item/AppItem.cpp \
    interface/QAnimatedSlider.cpp \
    interface/QMenuEditor.cpp \
    interface/QMessageOverlay.cpp \
    interface/SlidingStackedWidget.cpp \
    interface/WidgetMarqueeLabel.cpp \
    MainWindow.cpp \
    main.cpp \
    utils/AutoStartManager.cpp \
    utils/CliRemoteController.cpp \
    utils/CrashReportSender.cpp \
    utils/DesktopServices.cpp \
    utils/Log.cpp \
    utils/SingleInstanceMonitor.cpp \
    utils/dbus/ClientProxy.cpp \
    utils/dbus/IpcHandler.cpp \
    utils/dbus/ServerAdaptor.cpp \
    utils/OverlayMsgProxy.cpp \
    utils/StyleHelper.cpp

FORMS += \
    interface/FileSelectionWidget.ui \
    interface/LiveprogSelectionWidget.ui \
    interface/dialog/PaletteEditor.ui \
    interface/dialog/PresetRuleDialog.ui \
    interface/fragment/AppManagerFragment.ui \
    interface/fragment/FirstLaunchWizard.ui \
    interface/fragment/PresetAddRuleFragment.ui \
    interface/fragment/PresetFragment.ui \
    interface/fragment/SettingsFragment.ui \
    interface/fragment/StatusFragment.ui \
    interface/item/AppItem.ui \
    interface/menueditor.ui \
    MainWindow.ui

HEADERS += \
    config/AppConfig.h \
    config/ConfigContainer.h \
    config/ConfigIO.h \
    config/DspConfig.h \
    data/AssetManager.h \
    data/EelParser.h \
    data/InitializableQMap.h \
    data/PresetManager.h \
    data/PresetProvider.h \
    data/PresetRule.h \
    data/PresetRuleTableDelegate.h \
    data/VersionContainer.h \
    data/model/AppItemModel.h \
    data/model/DeviceListModel.h \
    data/model/PresetListModel.h \
    data/model/PresetRuleTableModel.h \
    data/model/QJsonTableModel.h \
    data/model/VdcDatabaseModel.h \
    interface/AnimatedJdspIcon.h \
    interface/CListView.h \
    interface/CTableView.h \
    interface/FadingLabel.h \
    interface/FileSelectionWidget.h \
    interface/LiveprogSelectionWidget.h \
    interface/QMessageOverlay.h \
    interface/TrayIcon.h \
    interface/dialog/PaletteEditor.h \
    interface/dialog/PresetRuleDialog.h \
    interface/event/EventFilter.h \
    interface/event/ScrollFilter.h \
    interface/fragment/AppManagerFragment.h \
    interface/fragment/BaseFragment.h \
    interface/fragment/FirstLaunchWizard.h \
    interface/fragment/FragmentHost.h \
    interface/fragment/FragmentHostPrivate.h \
    interface/fragment/PresetAddRuleFragment.h \
    interface/fragment/PresetFragment.h \
    interface/fragment/SettingsFragment.h \
    interface/fragment/StatusFragment.h \
    interface/item/AppItem.h \
    interface/item/AppItemStyleDelegate.h \
    interface/QAnimatedSlider.h \
    interface/QMenuEditor.h \
    interface/SlidingStackedWidget.h \
    interface/WidgetMarqueeLabel.h \
    MainWindow.h \
    utils/AutoStartManager.h \
    utils/CliRemoteController.h \
    utils/CrashReportSender.h \
    utils/DebuggerUtils.h \
    utils/DesktopServices.h \
    utils/Log.h \
    utils/SingleInstanceMonitor.h \
    utils/VersionMacros.h \
    utils/WindowUtils.h \
    utils/dbus/ClientProxy.h \
    utils/dbus/IpcHandler.h \
    utils/dbus/ServerAdaptor.h \
    utils/FindBinary.h \
    utils/OverlayMsgProxy.h \
    utils/StyleHelper.h

!NO_CRASH_HANDLER {
    HEADERS += \
        crash/airbag.h \
        crash/killer.h \
        crash/safecall.h \
        crash/stacktrace.h \

    SOURCES += \
        crash/airbag.c \
}

DISTFILES += utils/dbus/manifest.xml

RESOURCES += \
    ../resources/resources.qrc

TRANSLATIONS += ../resources/translations/jamesdsp_de.ts \
                ../resources/translations/jamesdsp_en.ts \
                ../resources/translations/jamesdsp_ru.ts \
                ../resources/translations/jamesdsp_pt_BR.ts

# Default rules for deployment.
isEmpty(PREFIX){
    qnx: PREFIX = /tmp/$${TARGET}
    else: unix:!android: PREFIX = /usr
}

isEmpty(BINDIR) {
    BINDIR = bin
}

BINDIR = $$absolute_path($$BINDIR, $$PREFIX)
target.path = $$BINDIR
!isEmpty(target.path): INSTALLS += target

unix {
    LIBS += -ldl
    QMAKE_LFLAGS += -ldl -lutil #-fsanitize=address
    QMAKE_CXXFLAGS += -g #-fno-omit-frame-pointer -fsanitize=address

    CONFIG += link_pkgconfig

    PKGCONFIG += libarchive
    PKGCONFIG += glibmm-2.4 giomm-2.4

    USE_PORTALS {
        PKGCONFIG += libportal-qt5
    }

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
