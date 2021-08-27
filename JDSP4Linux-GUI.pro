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

include(src/subprojects/Visualization/SpectrumAudioViewer.pri)
include(src/subprojects/FlatTabWidget/FlatTabWidget.pri)
include(src/subprojects/GraphicEQWidget/GraphicEQWidget.pri)
include(src/subprojects/EELEditor/EELEditor.pri)

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

CONFIG += c++17

SOURCES += \
    3rdparty/WAF/Animation/Animation.cpp \
    3rdparty/WAF/Animation/CircleFill/CircleFillAnimator.cpp \
    3rdparty/WAF/Animation/CircleFill/CircleFillDecorator.cpp \
    3rdparty/WAF/Animation/Expand/ExpandAnimator.cpp \
    3rdparty/WAF/Animation/Expand/ExpandDecorator.cpp \
    3rdparty/WAF/Animation/SideSlide/SideSlideAnimator.cpp \
    3rdparty/WAF/Animation/SideSlide/SideSlideDecorator.cpp \
    3rdparty/WAF/Animation/Slide/SlideAnimator.cpp \
    3rdparty/WAF/Animation/Slide/SlideForegroundDecorator.cpp \
    3rdparty/WebLoader/src/HttpMultiPart_p.cpp \
    3rdparty/WebLoader/src/NetworkQueue_p.cpp \
    3rdparty/WebLoader/src/NetworkRequest.cpp \
    3rdparty/WebLoader/src/WebLoader_p.cpp \
    3rdparty/WebLoader/src/WebRequest_p.cpp \
    3rdparty/WebLoader/src/customcookiejar.cpp \
    src/config/ConfigContainer.cpp \
    src/config/ConfigIO.cpp \
    src/crash/airbag.c \
    src/data/EelParser.cpp \
    src/data/PresetProvider.cpp \
    src/data/QJsonTableModel.cpp \
    src/interface/AnimatedJdspIcon.cpp \
    src/interface/TrayIcon.cpp \
    src/interface/dialog/AutoEqSelector.cpp \
    src/interface/dialog/PaletteEditor.cpp \
    src/interface/LiquidEqualizerWidget.cpp \
    src/interface/dialog/PresetDialog.cpp \
    src/interface/fragment/FirstLaunchWizard.cpp \
    src/interface/fragment/SettingsFragment.cpp \
    src/interface/fragment/StatusFragment.cpp \
    src/interface/model/DetailListItem.cpp \
    src/interface/QAnimatedSlider.cpp \
    src/interface/QMenuEditor.cpp \
    src/interface/QMessageOverlay.cpp \
    src/interface/SlidingStackedWidget.cpp \
    src/interface/WidgetMarqueeLabel.cpp \
    src/MainWindow.cpp \
    src/main.cpp \
    src/utils/AutoEqClient.cpp \
    src/utils/AutoStartManager.cpp \
    src/utils/Log.cpp \
    src/utils/dbus/ClientProxy.cpp \
    src/utils/dbus/ServerAdaptor.cpp \
    src/utils/OverlayMsgProxy.cpp \
    src/utils/StyleHelper.cpp

FORMS += \
    src/interface/dialog/AutoEqSelector.ui \
    src/interface/dialog/PaletteEditor.ui \
    src/interface/dialog/Preset.ui \
    src/interface/fragment/FirstLaunchWizard.ui \
    src/interface/fragment/SettingsFragment.ui \
    src/interface/fragment/StatusFragment.ui \
    src/interface/model/configitem.ui \
    src/interface/menueditor.ui \
    src/MainWindow.ui

HEADERS += \
    3rdparty/WAF/AbstractAnimator.h \
    3rdparty/WAF/Animation/Animation.h \
    3rdparty/WAF/Animation/AnimationPrivate.h \
    3rdparty/WAF/Animation/CircleFill/CircleFillAnimator.h \
    3rdparty/WAF/Animation/CircleFill/CircleFillDecorator.h \
    3rdparty/WAF/Animation/Expand/ExpandAnimator.h \
    3rdparty/WAF/Animation/Expand/ExpandDecorator.h \
    3rdparty/WAF/Animation/SideSlide/SideSlideAnimator.h \
    3rdparty/WAF/Animation/SideSlide/SideSlideDecorator.h \
    3rdparty/WAF/Animation/Slide/SlideAnimator.h \
    3rdparty/WAF/Animation/Slide/SlideForegroundDecorator.h \
    3rdparty/WAF/WAF.h \
    3rdparty/WebLoader/src/HttpMultiPart_p.h \
    3rdparty/WebLoader/src/NetworkQueue_p.h \
    3rdparty/WebLoader/src/NetworkRequest.h \
    3rdparty/WebLoader/src/NetworkRequestLoader.h \
    3rdparty/WebLoader/src/NetworkRequestPrivate_p.h \
    3rdparty/WebLoader/src/WebLoader_p.h \
    3rdparty/WebLoader/src/WebRequest_p.h \
    3rdparty/WebLoader/src/customcookiejar.h \
    src/config/AppConfig.h \
    src/config/ConfigContainer.h \
    src/config/ConfigIO.h \
    src/config/DspConfig.h \
    src/crash/airbag.h \
    src/crash/killer.h \
    src/crash/safecall.h \
    src/crash/stacktrace.h \
    src/data/EelParser.h \
    src/data/InitializableQMap.h \
    src/data/PresetProvider.h \
    src/data/QJsonTableModel.h \
    src/data/VersionContainer.h \
    src/interface/AnimatedJdspIcon.h \
    src/interface/QMessageOverlay.h \
    src/interface/TrayIcon.h \
    src/interface/dialog/AutoEqSelector.h \
    src/interface/dialog/PaletteEditor.h \
    src/interface/dialog/PresetDialog.h \
    src/interface/event/EventFilter.h \
    src/interface/event/ScrollFilter.h \
    src/interface/LiquidEqualizerWidget.h \
    src/interface/fragment/FirstLaunchWizard.h \
    src/interface/fragment/SettingsFragment.h \
    src/interface/fragment/StatusFragment.h \
    src/interface/model/Delegates.h \
    src/interface/model/DetailListItem.h \
    src/interface/QAnimatedSlider.h \
    src/interface/QMenuEditor.h \
    src/interface/SlidingStackedWidget.h \
    src/interface/WidgetMarqueeLabel.h \
    src/MainWindow.h \
    src/utils/AutoEqClient.h \
    src/utils/AutoStartManager.h \
    src/utils/Common.h \
    src/utils/Log.h \
    src/utils/dbus/ClientProxy.h \
    src/utils/dbus/ServerAdaptor.h \
    src/utils/FindBinary.h \
    src/utils/MathFunctions.h \
    src/utils/OverlayMsgProxy.h \
    src/utils/StyleHelper.h

DISTFILES += src/utils/dbus/manifest.xml

RESOURCES += \
    resources/resources.qrc

INCLUDEPATH += $$PWD/3rdparty/WAF \
               $$PWD/3rdparty/ \
               $$PWD/src

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /usr/bin/

!isEmpty(target.path): INSTALLS += target

unix {
    LIBS += -ldl
    QMAKE_LFLAGS += -ldl -lutil
    QMAKE_CXXFLAGS += -g
}

