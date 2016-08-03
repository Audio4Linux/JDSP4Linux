LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := libjamesdspnlib

LOCAL_MODULE_TAGS := optional

LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/soundfx

LOCAL_PRELINK_MODULE := false

LOCAL_SRC_FILES := \
	jamesdsp.cpp \
	Biquad4proc.cpp \
	Delay.cpp \
	Effect.cpp \
	EffectBassBoost.cpp \
	EffectCompression.cpp \
	EffectEqualizer.cpp \
	EffectVirtualizer.cpp \
	EffectStereoWide.cpp \
	FIR16.cpp \
	Bessel.cpp \
	Biquad.cpp \
	Butterworth.cpp \
	Cascade.cpp \
	ChebyshevI.cpp \
	ChebyshevII.cpp \
	Custom.cpp \
	Elliptic.cpp \
	Legendre.cpp \
	PoleFilter.cpp \
	RBJ.cpp \
	RootFinder.cpp \
	State.cpp\
# terminator

LOCAL_C_INCLUDES += \
	frameworks/base/include \
	hardware/libhardware/include \
	system/core/include \
	system/media/audio_effects/include \
# terminator

LOCAL_LDLIBS := -llog

#LOCAL_CPPFLAGS += -fexceptions
#LOCAL_LDFLAGS += -Wl,--gc-sections

include $(BUILD_SHARED_LIBRARY)
