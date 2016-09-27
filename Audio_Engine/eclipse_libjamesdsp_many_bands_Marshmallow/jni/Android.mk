LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := libjamesdsp

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
	Biquad.cpp \
	Butterworth.cpp \
	Cascade.cpp \
	PoleFilter.cpp \
	RootFinder.cpp \
	State.cpp \
	FIR512.cpp \
# terminator

LOCAL_C_INCLUDES += \
	frameworks/base/include \
	hardware/libhardware/include \
	system/core/include \
	system/media/audio_effects/include \
# terminator

#LOCAL_LDLIBS := -llog
#TARGET_PLATFORM := android-23
ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
LOCAL_CPPFLAGS += -ffunction-sections -fdata-sections -O2 -ffast-math -mfloat-abi=softfp -march=armv7-a -mfpu=neon -ftree-vectorize
LOCAL_CFLAGS += -ffunction-sections -fdata-sections -O2 -ffast-math -mfloat-abi=softfp -march=armv7-a -mfpu=neon -ftree-vectorize
else ifeq ($(TARGET_ARCH_ABI), arm64-v8a)
LOCAL_CPPFLAGS += -ffunction-sections -fdata-sections -O2 -ffast-math -march=armv8-a -ftree-vectorize
LOCAL_CFLAGS += -ffunction-sections -fdata-sections -O2 -ffast-math -march=armv8-a -ftree-vectorize
else ifeq ($(TARGET_ARCH_ABI), x86)
LOCAL_CPPFLAGS += -ffunction-sections -fdata-sections -O2 -ffast-math -march=atom -msse2 -ftree-vectorize -mavx -maes
LOCAL_CFLAGS += -ffunction-sections -fdata-sections -O2 -ffast-math -march=atom -msse2 -ftree-vectorize -mavx -maes
else
LOCAL_CPPFLAGS += -ffunction-sections -fdata-sections
LOCAL_CFLAGS += -ffunction-sections -fdata-sections
endif
LOCAL_LDFLAGS += -Wl,--gc-sections -DNDEBUG

include $(BUILD_SHARED_LIBRARY)
