TARGET = libjamesdsp
TEMPLATE = lib
CONFIG += staticlib
DEFINES += LIBJAMESDSP_PLUGIN

CONFIG += c++17

QMAKE_CFLAGS += -std=gnu11 -O2
#QMAKE_CFLAGS += -std=gnu11 -g3 -Og -gdwarf-2 -finline-functions

CONFIG += warn_off # Disable warnings for library

DEBUG_ASAN: CONFIG += sanitizer sanitize_address

# Enable liveprog logging redirection
DEFINES += CUSTOM_CMD

# Redirect printf & android logging to our custom handlers
DEFINES += printf=redirected_printf ANDROID_LOG_INFO=0 DEBUG

BASEPATH = $$PWD/subtree/Main/libjamesdsp/jni/jamesdsp/jdsp/

INCLUDEPATH += $$BASEPATH \
               $$PWD/subtree/Main/libjamesdsp/jni/jamesdsp # required due to includes in library code

HEADERS += \
    $$BASEPATH/Effects/eel2/cpthread.h \
    $$BASEPATH/Effects/eel2/dirent.h \
    $$BASEPATH/Effects/eel2/dr_flac.h \
    $$BASEPATH/Effects/eel2/dr_mp3.h \
    $$BASEPATH/Effects/eel2/dr_wav.h \
    $$BASEPATH/Effects/eel2/eelCommon.h \
    $$BASEPATH/Effects/eel2/eel_matrix.h \
    $$BASEPATH/Effects/eel2/fft.h \
    $$BASEPATH/Effects/eel2/glue_port.h \
    $$BASEPATH/Effects/eel2/ns-eel-int.h \
    $$BASEPATH/Effects/eel2/ns-eel.h \
    $$BASEPATH/Effects/eel2/numericSys/FFTConvolver.h \
    $$BASEPATH/Effects/eel2/numericSys/FilterDesign/fdesign.h \
    $$BASEPATH/Effects/eel2/numericSys/FilterDesign/polyphaseASRC.h \
    $$BASEPATH/Effects/eel2/numericSys/FilterDesign/polyphaseFilterbank.h \
    $$BASEPATH/Effects/eel2/numericSys/SolveLinearSystem/qr_fact.h \
    $$BASEPATH/Effects/eel2/numericSys/codelet.h \
    $$BASEPATH/Effects/eel2/numericSys/cpoly.h \
    $$BASEPATH/Effects/eel2/numericSys/libsamplerate/common.h \
    $$BASEPATH/Effects/eel2/numericSys/libsamplerate/samplerate.h \
    $$BASEPATH/Effects/eel2/numericSys/libsamplerate/src_config.h \
    $$BASEPATH/Effects/eel2/numericSys/quadprog.h \
    $$BASEPATH/Effects/eel2/numericSys/solvopt.h \
    $$BASEPATH/Effects/eel2/s_str.h \
    $$BASEPATH/Effects/eel2/stb_sprintf.h \
    $$BASEPATH/generalDSP/ArbFIRGen.h \
    $$BASEPATH/generalDSP/TwoStageFFTConvolver.h \
    $$BASEPATH/generalDSP/digitalFilters.h \
    $$BASEPATH/generalDSP/interpolation.h \
    $$BASEPATH/generalDSP/spectralInterpolatorFloat.h \
    $$BASEPATH/jdsp_header.h \
    EELStdOutExtension.h \
    PrintfStdOutExtension.h \
    JdspImpResToolbox.h \
    subtree/Main/libjamesdsp/jni/jamesdsp/jdsp/Effects/eel2/numericSys/HPFloat/cxpre.h \
    subtree/Main/libjamesdsp/jni/jamesdsp/jdsp/Effects/eel2/numericSys/HPFloat/cxxsrc/xcomplex.h \
    subtree/Main/libjamesdsp/jni/jamesdsp/jdsp/Effects/eel2/numericSys/HPFloat/cxxsrc/xreal.h \
    subtree/Main/libjamesdsp/jni/jamesdsp/jdsp/Effects/eel2/numericSys/HPFloat/doc/xcomplex.h \
    subtree/Main/libjamesdsp/jni/jamesdsp/jdsp/Effects/eel2/numericSys/HPFloat/doc/xreal.h \
    subtree/Main/libjamesdsp/jni/jamesdsp/jdsp/Effects/eel2/numericSys/HPFloat/hpaconf.h \
    subtree/Main/libjamesdsp/jni/jamesdsp/jdsp/Effects/eel2/numericSys/HPFloat/mp/const07.h \
    subtree/Main/libjamesdsp/jni/jamesdsp/jdsp/Effects/eel2/numericSys/HPFloat/mp/const11.h \
    subtree/Main/libjamesdsp/jni/jamesdsp/jdsp/Effects/eel2/numericSys/HPFloat/mp/const15.h \
    subtree/Main/libjamesdsp/jni/jamesdsp/jdsp/Effects/eel2/numericSys/HPFloat/mp/const19.h \
    subtree/Main/libjamesdsp/jni/jamesdsp/jdsp/Effects/eel2/numericSys/HPFloat/mp/const23.h \
    subtree/Main/libjamesdsp/jni/jamesdsp/jdsp/Effects/eel2/numericSys/HPFloat/mp/const27.h \
    subtree/Main/libjamesdsp/jni/jamesdsp/jdsp/Effects/eel2/numericSys/HPFloat/mp/const31.h \
    subtree/Main/libjamesdsp/jni/jamesdsp/jdsp/Effects/eel2/numericSys/HPFloat/xpre.h

SOURCES += \
    $$BASEPATH/Effects/arbEqConv.c \
    $$BASEPATH/Effects/bs2b.c \
    $$BASEPATH/Effects/convolver1D.c \
    $$BASEPATH/Effects/crossfeed.c \
    $$BASEPATH/Effects/dbb.c \
    $$BASEPATH/Effects/dynamic.c \
    $$BASEPATH/Effects/eel2/cpthread.c \
    $$BASEPATH/Effects/eel2/fft.c \
    $$BASEPATH/Effects/eel2/nseel-compiler.c \
    $$BASEPATH/Effects/eel2/nseel-ram.c \
    $$BASEPATH/Effects/eel2/numericSys/FFTConvolver.c \
    $$BASEPATH/Effects/eel2/numericSys/FilterDesign/cos_fib_paraunitary.c \
    $$BASEPATH/Effects/eel2/numericSys/FilterDesign/eqnerror.c \
    $$BASEPATH/Effects/eel2/numericSys/FilterDesign/firls.c \
    $$BASEPATH/Effects/eel2/numericSys/FilterDesign/generalFdesign.c \
    $$BASEPATH/Effects/eel2/numericSys/FilterDesign/polyphaseASRC.c \
    $$BASEPATH/Effects/eel2/numericSys/FilterDesign/polyphaseFilterbank.c \
    $$BASEPATH/Effects/eel2/numericSys/MersenneTwister.c \
    $$BASEPATH/Effects/eel2/numericSys/SolveLinearSystem/inv.c \
    $$BASEPATH/Effects/eel2/numericSys/SolveLinearSystem/mldivide.c \
    $$BASEPATH/Effects/eel2/numericSys/SolveLinearSystem/mrdivide.c \
    $$BASEPATH/Effects/eel2/numericSys/SolveLinearSystem/pinv.c \
    $$BASEPATH/Effects/eel2/numericSys/SolveLinearSystem/qr_fact.c \
    $$BASEPATH/Effects/eel2/numericSys/codelet.c \
    $$BASEPATH/Effects/eel2/numericSys/cpoly.c \
    $$BASEPATH/Effects/eel2/numericSys/libsamplerate/samplerate.c \
    $$BASEPATH/Effects/eel2/numericSys/libsamplerate/src_sinc.c \
    $$BASEPATH/Effects/eel2/numericSys/quadprog.c \
    $$BASEPATH/Effects/eel2/numericSys/solvopt.c \
    $$BASEPATH/Effects/eel2/s_str.c \
    $$BASEPATH/Effects/eel2/y.tab.c \
    $$BASEPATH/Effects/liveprogWrapper.c \
    $$BASEPATH/Effects/reverb.c \
    $$BASEPATH/Effects/stereoEnhancement.c \
    $$BASEPATH/Effects/vacuumTube.c \
    $$BASEPATH/Effects/vdc.c \
    $$BASEPATH/binaryBlobs.c \
    $$BASEPATH/generalDSP/ArbFIRGen.c \
    $$BASEPATH/generalDSP/TwoStageFFTConvolver.c \
    $$BASEPATH/generalDSP/digitalFilters.c \
    $$BASEPATH/generalDSP/generalProg.c \
    $$BASEPATH/generalDSP/interpolation.c \
    $$BASEPATH/generalDSP/spectralInterpolatorFloat.c \
    $$BASEPATH/jdspController.c \
    EELStdOutExtension.c \
    PrintfStdOutExtension.c \
    JdspImpResToolbox.c \
    subtree/Main/libjamesdsp/jni/jamesdsp/jdsp/Effects/eel2/loose_eel.c \
    subtree/Main/libjamesdsp/jni/jamesdsp/jdsp/Effects/eel2/numericSys/HPFloat/atox.c \
    subtree/Main/libjamesdsp/jni/jamesdsp/jdsp/Effects/eel2/numericSys/HPFloat/constant.c \
    subtree/Main/libjamesdsp/jni/jamesdsp/jdsp/Effects/eel2/numericSys/HPFloat/cxaop.c \
    subtree/Main/libjamesdsp/jni/jamesdsp/jdsp/Effects/eel2/numericSys/HPFloat/cxbasic.c \
    subtree/Main/libjamesdsp/jni/jamesdsp/jdsp/Effects/eel2/numericSys/HPFloat/cxconstant.c \
    subtree/Main/libjamesdsp/jni/jamesdsp/jdsp/Effects/eel2/numericSys/HPFloat/cxconvf.c \
    subtree/Main/libjamesdsp/jni/jamesdsp/jdsp/Effects/eel2/numericSys/HPFloat/cxexp.c \
    subtree/Main/libjamesdsp/jni/jamesdsp/jdsp/Effects/eel2/numericSys/HPFloat/cxhypb.c \
    subtree/Main/libjamesdsp/jni/jamesdsp/jdsp/Effects/eel2/numericSys/HPFloat/cxidiv.c \
    subtree/Main/libjamesdsp/jni/jamesdsp/jdsp/Effects/eel2/numericSys/HPFloat/cxpow.c \
    subtree/Main/libjamesdsp/jni/jamesdsp/jdsp/Effects/eel2/numericSys/HPFloat/cxprcmp.c \
    subtree/Main/libjamesdsp/jni/jamesdsp/jdsp/Effects/eel2/numericSys/HPFloat/cxtrig.c \
    subtree/Main/libjamesdsp/jni/jamesdsp/jdsp/Effects/eel2/numericSys/HPFloat/hpaconf.c \
    subtree/Main/libjamesdsp/jni/jamesdsp/jdsp/Effects/eel2/numericSys/HPFloat/prcxpr.c \
    subtree/Main/libjamesdsp/jni/jamesdsp/jdsp/Effects/eel2/numericSys/HPFloat/print.c \
    subtree/Main/libjamesdsp/jni/jamesdsp/jdsp/Effects/eel2/numericSys/HPFloat/prxpr.c \
    subtree/Main/libjamesdsp/jni/jamesdsp/jdsp/Effects/eel2/numericSys/HPFloat/sfmod.c \
    subtree/Main/libjamesdsp/jni/jamesdsp/jdsp/Effects/eel2/numericSys/HPFloat/shift.c \
    subtree/Main/libjamesdsp/jni/jamesdsp/jdsp/Effects/eel2/numericSys/HPFloat/xadd.c \
    subtree/Main/libjamesdsp/jni/jamesdsp/jdsp/Effects/eel2/numericSys/HPFloat/xchcof.c \
    subtree/Main/libjamesdsp/jni/jamesdsp/jdsp/Effects/eel2/numericSys/HPFloat/xdiv.c \
    subtree/Main/libjamesdsp/jni/jamesdsp/jdsp/Effects/eel2/numericSys/HPFloat/xevtch.c \
    subtree/Main/libjamesdsp/jni/jamesdsp/jdsp/Effects/eel2/numericSys/HPFloat/xexp.c \
    subtree/Main/libjamesdsp/jni/jamesdsp/jdsp/Effects/eel2/numericSys/HPFloat/xfmod.c \
    subtree/Main/libjamesdsp/jni/jamesdsp/jdsp/Effects/eel2/numericSys/HPFloat/xfrac.c \
    subtree/Main/libjamesdsp/jni/jamesdsp/jdsp/Effects/eel2/numericSys/HPFloat/xhypb.c \
    subtree/Main/libjamesdsp/jni/jamesdsp/jdsp/Effects/eel2/numericSys/HPFloat/xivhypb.c \
    subtree/Main/libjamesdsp/jni/jamesdsp/jdsp/Effects/eel2/numericSys/HPFloat/xivtrg.c \
    subtree/Main/libjamesdsp/jni/jamesdsp/jdsp/Effects/eel2/numericSys/HPFloat/xlog.c \
    subtree/Main/libjamesdsp/jni/jamesdsp/jdsp/Effects/eel2/numericSys/HPFloat/xmul.c \
    subtree/Main/libjamesdsp/jni/jamesdsp/jdsp/Effects/eel2/numericSys/HPFloat/xneg.c \
    subtree/Main/libjamesdsp/jni/jamesdsp/jdsp/Effects/eel2/numericSys/HPFloat/xprcmp.c \
    subtree/Main/libjamesdsp/jni/jamesdsp/jdsp/Effects/eel2/numericSys/HPFloat/xpwr.c \
    subtree/Main/libjamesdsp/jni/jamesdsp/jdsp/Effects/eel2/numericSys/HPFloat/xsigerr.c \
    subtree/Main/libjamesdsp/jni/jamesdsp/jdsp/Effects/eel2/numericSys/HPFloat/xsqrt.c \
    subtree/Main/libjamesdsp/jni/jamesdsp/jdsp/Effects/eel2/numericSys/HPFloat/xtodbl.c \
    subtree/Main/libjamesdsp/jni/jamesdsp/jdsp/Effects/eel2/numericSys/HPFloat/xtoflt.c \
    subtree/Main/libjamesdsp/jni/jamesdsp/jdsp/Effects/eel2/numericSys/HPFloat/xtrig.c \
    subtree/Main/libjamesdsp/jni/jamesdsp/jdsp/Effects/multimodalEQ.c

unix {
    isEmpty(LIBDIR) {
        LIBDIR = lib
    }

    LIBDIR = $$absolute_path($$LIBDIR, $$PREFIX)
    target.path = $$LIBDIR
}
else: error("Static linking only available on Linux systems")

!isEmpty(target.path): INSTALLS += target

DISTFILES += \
    subtree/Main/libjamesdsp/jni/jamesdsp/jdsp/Effects/eel2/numericSys/HPFloat/doc/hpa.txt
