TEMPLATE = app

QT       += multimedia widgets

SOURCES  += \
            $$PWD/audiostreamengine.cpp \
            $$PWD/fftreal/fftreal_wrapper.cpp \
            $$PWD/frequencyspectrum.cpp \
            $$PWD/spectrograph.cpp \
            $$PWD/spectrumanalyser.cpp \
            $$PWD/utils.cpp

HEADERS  += \
            $$PWD/audiostreamengine.h \
            $$PWD/fftreal/Array.h \
            $$PWD/fftreal/Array.hpp \
            $$PWD/fftreal/DynArray.h \
            $$PWD/fftreal/DynArray.hpp \
            $$PWD/fftreal/FFTReal.h \
            $$PWD/fftreal/FFTReal.hpp \
            $$PWD/fftreal/FFTRealFixLen.h \
            $$PWD/fftreal/FFTRealFixLen.hpp \
            $$PWD/fftreal/FFTRealFixLenParam.h \
            $$PWD/fftreal/FFTRealPassDirect.h \
            $$PWD/fftreal/FFTRealPassDirect.hpp \
            $$PWD/fftreal/FFTRealPassInverse.h \
            $$PWD/fftreal/FFTRealPassInverse.hpp \
            $$PWD/fftreal/FFTRealSelect.h \
            $$PWD/fftreal/FFTRealSelect.hpp \
            $$PWD/fftreal/FFTRealUseTrigo.h \
            $$PWD/fftreal/FFTRealUseTrigo.hpp \
            $$PWD/fftreal/OscSinCos.h \
            $$PWD/fftreal/OscSinCos.hpp \
            $$PWD/fftreal/def.h \
            $$PWD/fftreal/fftreal_wrapper.h \
            $$PWD/frequencyspectrum.h \
            $$PWD/spectrograph.h \
            $$PWD/spectrum.h \
            $$PWD/spectrumanalyser.h \
            $$PWD/utils.h

fftreal_dir = $$PWD/fftreal

INCLUDEPATH += $${fftreal_dir}

#DEFINES += LOG_SPECTRUMANALYSER
#DEFINES += LOG_WAVEFORM
#DEFINES += LOG_ENGINE
#DEFINES += DUMP_SPECTRUMANALYSER
#DEFINES += DUMP_CAPTURED_AUDIO
DEFINES += DISABLE_LEVEL

win32{
    DEFINES += DISABLE_FFT
}

DEFINES += DISABLE_WAVEFORM
#DEFINES += SUPERIMPOSE_PROGRESS_ON_WAVEFORM
DEFINES += SPECTRUM_ANALYSER_SEPARATE_THREAD

CONFIG += install_ok  # Do not cargo-cult this!
