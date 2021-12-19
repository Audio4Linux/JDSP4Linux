include(wrapper/GstPluginWrapper.pri)

SOURCES +=  \
    $$PWD/PulseAppManager.cpp \
    $$PWD/PulseAudioProcessingThread.cpp \
    $$PWD/PulseAudioService.cpp \
    $$PWD/PulseDevice.cpp \
    $$PWD/PulseManager.cpp \
    $$PWD/PulsePipelineManager.cpp \
    $$PWD/RealtimeKit.cpp \
    $$PWD/pipeline/FilterElement.cpp \
    $$PWD/pipeline/GstElementProperties.cpp \
    $$PWD/pipeline/JamesDspElement.cpp \
    $$PWD/pipeline/sink/AutoSinkElement.cpp \
    $$PWD/pipeline/sink/PulseSinkElement.cpp \
    $$PWD/pipeline/source/FileSourceElement.cpp \
    $$PWD/pipeline/source/PulseSrcElement.cpp

HEADERS +=  \
    $$PWD/PulseAppManager.h \
    $$PWD/PulseAudioProcessingThread.h \
    $$PWD/PulseAudioService.h \
    $$PWD/PulseDataTypes.h \
    $$PWD/PulseDevice.h \
    $$PWD/PulseManager.h \
    $$PWD/PulsePipelineManager.h \
    $$PWD/RealtimeKit.h \
    $$PWD/pipeline/BaseElement.h \
    $$PWD/pipeline/FilterElement.h \
    $$PWD/pipeline/GstElementProperties.h \
    $$PWD/pipeline/JamesDspElement.h \
    $$PWD/pipeline/sink/AutoSinkElement.h \
    $$PWD/pipeline/sink/PulseSinkElement.h \
    $$PWD/pipeline/sink/SinkElement.h \
    $$PWD/pipeline/source/FileSourceElement.h \
    $$PWD/pipeline/source/PulseSrcElement.h \
    $$PWD/pipeline/source/SourceElement.h

INCLUDEPATH += $$PWD
