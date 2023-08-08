LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE := libjamesdsp
LOCAL_PRELINK_MODULE := false
LOCAL_SRC_FILES := \
	cpthread.c \
	jdsp/generalDSP/spectralInterpolatorFloat.c \
	jdsp/generalDSP/ArbFIRGen.c \
	jdsp/Effects/eel2/numericSys/codelet.c \
	jdsp/generalDSP/digitalFilters.c \
	jdsp/Effects/eel2/numericSys/FFTConvolver.c \
	jdsp/generalDSP/TwoStageFFTConvolver.c \
	jdsp/generalDSP/interpolation.c \
	jdsp/generalDSP/generalProg.c \
	jdsp/Effects/vdc.c \
	jdsp/Effects/vacuumTube.c \
	jdsp/Effects/stereoEnhancement.c \
	jdsp/Effects/reverb.c \
	jdsp/Effects/liveprogWrapper.c \
	jdsp/Effects/multimodalEQ.c \
	jdsp/Effects/dynamic.c \
	jdsp/Effects/dbb.c \
	jdsp/Effects/convolver1D.c \
	jdsp/Effects/crossfeed.c \
	jdsp/Effects/bs2b.c \
	jdsp/Effects/arbEqConv.c \
	jdsp/Effects/eel2/numericSys/libsamplerate/samplerate.c \
	jdsp/Effects/eel2/numericSys/libsamplerate/src_sinc.c \
	jdsp/Effects/eel2/numericSys/libsamplerate/src_linear.c \
	jdsp/Effects/eel2/numericSys/FilterDesign/generalFdesign.c \
	jdsp/Effects/eel2/numericSys/FilterDesign/cos_fib_paraunitary.c \
	jdsp/Effects/eel2/numericSys/FilterDesign/polyphaseFilterbank.c \
	jdsp/Effects/eel2/numericSys/FilterDesign/polyphaseASRC.c \
	jdsp/Effects/eel2/numericSys/FilterDesign/eqnerror.c \
	jdsp/Effects/eel2/numericSys/FilterDesign/firls.c \
	jdsp/Effects/eel2/numericSys/SolveLinearSystem/inv.c \
	jdsp/Effects/eel2/numericSys/SolveLinearSystem/pinv.c \
	jdsp/Effects/eel2/numericSys/SolveLinearSystem/mldivide.c \
	jdsp/Effects/eel2/numericSys/SolveLinearSystem/mrdivide.c \
	jdsp/Effects/eel2/numericSys/SolveLinearSystem/qr_fact.c \
	jdsp/Effects/eel2/numericSys/solvopt.c \
	jdsp/Effects/eel2/numericSys/cpoly.c \
	jdsp/Effects/eel2/numericSys/MersenneTwister.c \
	jdsp/Effects/eel2/numericSys/quadprog.c \
	jdsp/Effects/eel2/numericSys/HPFloat/atox.c \
	jdsp/Effects/eel2/numericSys/HPFloat/constant.c \
	jdsp/Effects/eel2/numericSys/HPFloat/cxaop.c \
	jdsp/Effects/eel2/numericSys/HPFloat/cxbasic.c \
	jdsp/Effects/eel2/numericSys/HPFloat/cxconstant.c \
	jdsp/Effects/eel2/numericSys/HPFloat/cxconvf.c \
	jdsp/Effects/eel2/numericSys/HPFloat/cxexp.c \
	jdsp/Effects/eel2/numericSys/HPFloat/cxhypb.c \
	jdsp/Effects/eel2/numericSys/HPFloat/cxidiv.c \
	jdsp/Effects/eel2/numericSys/HPFloat/cxpow.c \
	jdsp/Effects/eel2/numericSys/HPFloat/cxprcmp.c \
	jdsp/Effects/eel2/numericSys/HPFloat/cxtrig.c \
	jdsp/Effects/eel2/numericSys/HPFloat/hpaconf.c \
	jdsp/Effects/eel2/numericSys/HPFloat/prcxpr.c \
	jdsp/Effects/eel2/numericSys/HPFloat/print.c \
	jdsp/Effects/eel2/numericSys/HPFloat/prxpr.c \
	jdsp/Effects/eel2/numericSys/HPFloat/sfmod.c \
	jdsp/Effects/eel2/numericSys/HPFloat/shift.c \
	jdsp/Effects/eel2/numericSys/HPFloat/xadd.c \
	jdsp/Effects/eel2/numericSys/HPFloat/xchcof.c \
	jdsp/Effects/eel2/numericSys/HPFloat/xdiv.c \
	jdsp/Effects/eel2/numericSys/HPFloat/xevtch.c \
	jdsp/Effects/eel2/numericSys/HPFloat/xexp.c \
	jdsp/Effects/eel2/numericSys/HPFloat/xfmod.c \
	jdsp/Effects/eel2/numericSys/HPFloat/xfrac.c \
	jdsp/Effects/eel2/numericSys/HPFloat/xhypb.c \
	jdsp/Effects/eel2/numericSys/HPFloat/xivhypb.c \
	jdsp/Effects/eel2/numericSys/HPFloat/xivtrg.c \
	jdsp/Effects/eel2/numericSys/HPFloat/xlog.c \
	jdsp/Effects/eel2/numericSys/HPFloat/xmul.c \
	jdsp/Effects/eel2/numericSys/HPFloat/xneg.c \
	jdsp/Effects/eel2/numericSys/HPFloat/xprcmp.c \
	jdsp/Effects/eel2/numericSys/HPFloat/xpwr.c \
	jdsp/Effects/eel2/numericSys/HPFloat/xsigerr.c \
	jdsp/Effects/eel2/numericSys/HPFloat/xsqrt.c \
	jdsp/Effects/eel2/numericSys/HPFloat/xtodbl.c \
	jdsp/Effects/eel2/numericSys/HPFloat/xtoflt.c \
	jdsp/Effects/eel2/numericSys/HPFloat/xtrig.c \
	jdsp/Effects/eel2/s_str.c \
	jdsp/Effects/eel2/fft.c \
	jdsp/Effects/eel2/nseel-compiler.c \
	jdsp/Effects/eel2/nseel-ram.c \
	jdsp/Effects/eel2/y.tab.c \
	jdsp/binaryBlobs.c \
	jdsp/jdspController.c \
	jamesdsp.c \
# terminator
LOCAL_LDLIBS := -llog
ifeq ($(TARGET_ARCH_ABI), armeabi-v7a)
LOCAL_CPPFLAGS += -Wall -Wextra -ffunction-sections -fdata-sections -Ofast -march=armv7-a -mfpu=neon -ftree-vectorize -fvisibility=hidden -DJAMESDSP_REFERENCE_IMPL #-DDEBUG
LOCAL_CFLAGS += -Wall -Wextra -ffunction-sections -fdata-sections -Ofast -march=armv7-a -mfpu=neon -ftree-vectorize -fvisibility=hidden -DJAMESDSP_REFERENCE_IMPL #-DDEBUG
else ifeq ($(TARGET_ARCH_ABI), arm64-v8a)
LOCAL_CPPFLAGS += -Wall -Wextra -ffunction-sections -fdata-sections -Ofast -march=armv8-a -mfpu=neon -ftree-vectorize -fvisibility=hidden -DJAMESDSP_REFERENCE_IMPL #-DDEBUG
LOCAL_CFLAGS += -Wall -Wextra -ffunction-sections -fdata-sections -Ofast -march=armv8-a -mfpu=neon -ftree-vectorize -fvisibility=hidden -DJAMESDSP_REFERENCE_IMPL #-DDEBUG
else ifeq ($(TARGET_ARCH_ABI), x86)
LOCAL_CPPFLAGS += -ffunction-sections -fdata-sections -Ofast -ftree-vectorize -fvisibility=hidden -DJAMESDSP_REFERENCE_IMPL #-DDEBUG
LOCAL_CFLAGS += -ffunction-sections -fdata-sections -Ofast -ftree-vectorize -fvisibility=hidden -DJAMESDSP_REFERENCE_IMPL #-DDEBUG
else ifeq ($(TARGET_ARCH_ABI), armeabi)
LOCAL_CPPFLAGS += -ffunction-sections -fdata-sections -Ofast -ftree-vectorize -fvisibility=hidden -DJAMESDSP_REFERENCE_IMPL #-DDEBUG
LOCAL_CFLAGS += -ffunction-sections -fdata-sections -Ofast -ftree-vectorize -fvisibility=hidden -DJAMESDSP_REFERENCE_IMPL #-DDEBUG
endif
LOCAL_LDFLAGS += -Wl,--gc-sections,--exclude-libs,ALL
include $(BUILD_SHARED_LIBRARY)