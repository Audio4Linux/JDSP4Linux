include(wrapper/GstPluginWrapper.pri)

SOURCES +=  \
    $$PWD/AudioProcessingThread.cpp \
    $$PWD/PipelineManager.cpp \
    $$PWD/PulseAudioService.cpp \
    $$PWD/PulseManager.cpp \
    $$PWD/RealtimeKit.cpp \
    $$PWD/pipeline/FilterElement.cpp \
    $$PWD/pipeline/GstElementProperties.cpp \
    $$PWD/pipeline/JamesDspElement.cpp \
    $$PWD/pipeline/sink/AutoSinkElement.cpp \
    $$PWD/pipeline/sink/PulseSinkElement.cpp \
    $$PWD/pipeline/source/FileSourceElement.cpp \
    $$PWD/pipeline/source/PulseSrcElement.cpp

HEADERS +=  \
    $$PWD/AudioProcessingThread.h \
    $$PWD/PipelineManager.h \
    $$PWD/PulseAudioService.h \
    $$PWD/PulseManager.h \
    $$PWD/RealtimeKit.h \
    $$PWD/Utils.h \
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
