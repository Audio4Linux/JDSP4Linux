#-------------------------------------------------
#
# Project created by QtCreator 2019-08-30T23:36:04
#
#-------------------------------------------------

QT       += core xml network dbus

!HEADLESS {
    greaterThan(QT_MAJOR_VERSION, 5) {
        QT += svgwidgets
    }
    QT   += gui svg widgets
}
else {
    QT   -= gui svg widgets
}

TARGET = jamesdsp
TEMPLATE = app

UI_DEBUG: DEFINES += UI_DEBUG
DEBUG_FPE: DEFINES += DEBUG_FPE
DEBUG_ASAN: CONFIG += sanitizer sanitize_address
USE_PULSEAUDIO: DEFINES += USE_PULSEAUDIO
FLATPAK: DEFINES += USE_PORTALS
FLATPAK: DEFINES += IS_FLATPAK
HEADLESS: DEFINES += HEADLESS

USE_PULSEAUDIO {
    DEFINES += FLATPAK_APP_ID=\\\"me.timschneeberger.jdsp4linux.pulse\\\"
}
else {
    DEFINES += FLATPAK_APP_ID=\\\"me.timschneeberger.jdsp4linux\\\"
}

DEFINES += APP_VERSION=$$system(git describe --tags --long --always)
DEFINES += JDSP_VERSION=4.01

include(../3rdparty/3rdparty.pri)

include(audio/AudioDrivers.pri)

!HEADLESS {
    include(subprojects/AutoEqIntegration/AutoEqIntegration.pri)
    include(subprojects/FlatTabWidget/FlatTabWidget/FlatTabWidget.pri)
    include(subprojects/LiquidEqualizerWidget/LiquidEqualizerWidget.pri)
    include(subprojects/GraphicEQWidget/GraphicEQWidget/GraphicEQWidget.pri)
}

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

!HEADLESS {
    HEADERS += \
        data/PresetRuleTableDelegate.h \
        interface/AnimatedJdspIcon.h \
        interface/CListView.h \
        interface/CTableView.h \
        interface/FadingLabel.h \
        interface/FileSelectionWidget.h \
        interface/LiquidMultiEqualizerWidget.h \
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
        utils/DesktopServices.h \
        utils/OverlayMsgProxy.h \
        utils/StyleHelper.h

    SOURCES += \
        interface/AnimatedJdspIcon.cpp \
        interface/CListView.cpp \
        interface/CTableView.cpp \
        interface/FadingLabel.cpp \
        interface/FileSelectionWidget.cpp \
        interface/LiquidMultiEqualizerWidget.cpp \
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
        utils/AutoStartManager.cpp \
        utils/DesktopServices.cpp \
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
}


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
    data/model/AppItemModel.h \
    data/model/DeviceListModel.h \
    data/model/PresetListModel.h \
    data/model/PresetRuleTableModel.h \
    data/model/QJsonTableModel.h \
    data/model/RouteListModel.h \
    data/model/VdcDatabaseModel.h \
    utils/CliRemoteController.h \
    utils/DebuggerUtils.h \
    utils/Log.h \
    utils/QtCompat.h \
    utils/SingleInstanceMonitor.h \
    utils/VersionMacros.h \
    utils/dbus/ClientProxy.h \
    utils/dbus/IpcHandler.h \
    utils/dbus/ServerAdaptor.h \
    utils/FindBinary.h


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
    data/model/RouteListModel.cpp \
    data/model/VdcDatabaseModel.cpp \
    main.cpp \
    utils/CliRemoteController.cpp \
    utils/Log.cpp \
    utils/SingleInstanceMonitor.cpp \
    utils/dbus/ClientProxy.cpp \
    utils/dbus/IpcHandler.cpp \
    utils/dbus/ServerAdaptor.cpp

DISTFILES += utils/dbus/manifest.xml

RESOURCES += \
    ../resources/resources.qrc

TRANSLATIONS += ../resources/translations/jamesdsp_ar.ts \
                ../resources/translations/jamesdsp_ca.ts \
                ../resources/translations/jamesdsp_cs.ts \
                ../resources/translations/jamesdsp_da.ts \
                ../resources/translations/jamesdsp_el.ts \
                ../resources/translations/jamesdsp_fi.ts \
                ../resources/translations/jamesdsp_he.ts \
                ../resources/translations/jamesdsp_hi.ts \
                ../resources/translations/jamesdsp_hu.ts \
                ../resources/translations/jamesdsp_id.ts \
                ../resources/translations/jamesdsp_it.ts \
                ../resources/translations/jamesdsp_ja.ts \
                ../resources/translations/jamesdsp_ko.ts \
                ../resources/translations/jamesdsp_nl.ts \
                ../resources/translations/jamesdsp_no.ts \
                ../resources/translations/jamesdsp_de.ts \
                ../resources/translations/jamesdsp_en.ts \
                ../resources/translations/jamesdsp_fr.ts \
                ../resources/translations/jamesdsp_ru.ts \
                ../resources/translations/jamesdsp_uk.ts \
                ../resources/translations/jamesdsp_pl.ts \
                ../resources/translations/jamesdsp_es.ts \
                ../resources/translations/jamesdsp_zh_CN.ts \
                ../resources/translations/jamesdsp_pt_BR.ts \
                ../resources/translations/jamesdsp_af.ts \
                ../resources/translations/jamesdsp_pt.ts \
                ../resources/translations/jamesdsp_ro.ts \
                ../resources/translations/jamesdsp_sr.ts \
                ../resources/translations/jamesdsp_sv.ts \
                ../resources/translations/jamesdsp_th.ts \
                ../resources/translations/jamesdsp_tr.ts \
                ../resources/translations/jamesdsp_vi.ts \
                ../resources/translations/jamesdsp_zh_TW.ts

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

    FLATPAK {
        PKGCONFIG += libportal-qt6
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
