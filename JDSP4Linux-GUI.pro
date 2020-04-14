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

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

include(visualization/SpectrumAudioViewer.pri)
include(phantom/phantom.pri)
include(dialog/FlatTabWidget/FlatTabWidget.pri)
include(dialog/GraphicEQWidget/GraphicEQWidget.pri)
include(dialog/EELEditor/EELEditor.pri)

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

CONFIG += c++11

SOURCES += main.cpp \
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
    config/appconfigwrapper.cpp \
    config/configconverter.cpp \
    config/container.cpp \
    config/dbusproxy.cpp \
    config/io.cpp \
    crashhandler/airbag.c \
    dbus/clientproxy.cpp \
    dbus/serveradaptor.cpp \
    dialog/androidimporterdlg.cpp \
    dialog/animatedjdspicon.cpp \
    dialog/autoeqselector.cpp \
    dialog/firstlaunchwizard.cpp \
    dialog/itemmodel/detaillistitem.cpp \
    dialog/liquidequalizerwidget.cpp \
    dialog/logdlg.cpp \
    dialog/palettedlg.cpp \
    dialog/presetdlg.cpp \
    dialog/pulseeffectscompatibility.cpp \
    dialog/qanimatedslider.cpp \
    dialog/qmenueditor.cpp \
    dialog/qmessageoverlay.cpp \
    dialog/settingsdlg.cpp \
    dialog/slidingstackedwidget.cpp \
    dialog/statusfragment.cpp \
    mainwindow.cpp \
    misc/autoeqclient.cpp \
    misc/autostartmanager.cpp \
    misc/biquad.cpp \
    misc/loghelper.cpp \
    misc/overlaymsgproxy.cpp \
    misc/presetprovider.cpp \
    misc/qjsontablemodel.cpp \
    misc/stylehelper.cpp

FORMS += \
    dialog/autoeqselector.ui \
    dialog/firstlaunchwizard.ui \
    dialog/importandroid.ui \
    dialog/itemmodel/configitem.ui \
    dialog/log.ui \
    dialog/menueditor.ui \
    dialog/palettedlg.ui \
    dialog/preset.ui \
    dialog/pulseeffectscompatibility.ui \
    dialog/settings.ui \
    dialog/statusfragment.ui \
    mainwindow.ui

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
    config/appconfigwrapper.h \
    config/configconverter.h \
    config/container.h \
    config/dbusproxy.h \
    config/io.h \
    crashhandler/airbag.h \
    crashhandler/killer.h \
    crashhandler/safecall.h \
    crashhandler/stacktrace.h \
    dbus/clientproxy.h \
    dbus/serveradaptor.h \
    dialog/androidimporterdlg.h \
    dialog/animatedjdspicon.h \
    dialog/autoeqselector.h \
    dialog/firstlaunchwizard.h \
    dialog/itemmodel/delegates.h \
    dialog/itemmodel/detaillistitem.h \
    dialog/liquidequalizerwidget.h \
    dialog/logdlg.h \
    dialog/palettedlg.h \
    dialog/presetdlg.h \
    dialog/pulseeffectscompatibility.h \
    dialog/qanimatedslider.h \
    dialog/qmenueditor.h \
    dialog/qmessageoverlay.h \
    dialog/settingsdlg.h \
    dialog/slidingstackedwidget.h \
    dialog/statusfragment.h \
    mainwindow.h \
    misc/autoeqclient.h \
    misc/autostartmanager.h \
    misc/biquad.h \
    misc/common.h \
    misc/eventfilter.h \
    misc/findbinary.h \
    misc/initializableqmap.h \
    misc/loghelper.h \
    misc/mathfunctions.h \
    misc/overlaymsgproxy.h \
    misc/presetprovider.h \
    misc/qjsontablemodel.h \
    misc/stylehelper.h \
    misc/versioncontainer.h

DISTFILES += \
    3rdparty/WAF/LICENSE \
    3rdparty/WAF/README.md \
    3rdparty/WebLoader/LICENSE \
    3rdparty/WebLoader/README.md \
    dbus/manifest.xml

RESOURCES += \
    resources.qrc \
    styles/styles.qrc

INCLUDEPATH += $$PWD/3rdparty/WAF \
               $$PWD/3rdparty/

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /usr/bin/

!isEmpty(target.path): INSTALLS += target

SUBDIRS += \
    3rdparty/WebLoader/WebLoader.pro

unix {
    QMAKE_LFLAGS += -ldl -lutil
    QMAKE_CXXFLAGS += -g
}

