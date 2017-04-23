LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE := libjamesdsp
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
	gverb.cpp \
# terminator

LOCAL_C_INCLUDES += \
	frameworks/base/include \
	hardware/libhardware/include \
	system/core/include \
	system/media/audio_effects/include \
# terminator

#LOCAL_LDLIBS := -llog
ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
LOCAL_CPPFLAGS += -DNDEBUG -ffunction-sections -fdata-sections -Ofast -march=armv7-a -mfpu=neon -ftree-vectorize
LOCAL_CFLAGS += -DNDEBUG -ffunction-sections -fdata-sections -Ofast -march=armv7-a -mfpu=neon -ftree-vectorize
else ifeq ($(TARGET_ARCH_ABI), x86)
LOCAL_CPPFLAGS += -DNDEBUG -ffunction-sections -fdata-sections -Ofast -ftree-vectorize
LOCAL_CFLAGS += -DNDEBUG -ffunction-sections -fdata-sections -Ofast -ftree-vectorize
else
LOCAL_CPPFLAGS += -DNDEBUG -ffunction-sections -Ofast -fdata-sections
LOCAL_CFLAGS += -DNDEBUG -ffunction-sections -Ofast -fdata-sections
endif
LOCAL_LDFLAGS += -Wl,--gc-sections -DNDEBUG
include $(BUILD_SHARED_LIBRARY)