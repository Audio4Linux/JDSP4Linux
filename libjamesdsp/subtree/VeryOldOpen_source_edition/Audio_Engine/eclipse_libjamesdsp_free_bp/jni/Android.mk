LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE := libjamesdsp
LOCAL_PRELINK_MODULE := false
LOCAL_SRC_FILES := \
	kissfft/kiss_fft.c \
	kissfft/kiss_fftr.c \
	jamesdsp.cpp \
	Effect.cpp \
	EffectDSPMain.cpp \
	JLimiter.c \
	reverb.c \
	compressor.c \
	AutoConvolver.c \
	mnspline.c \
	ArbFIRGen.c \
	vdc.c \
	bs2b.c \
	valve/12ax7amp/Tube.c \
	valve/12ax7amp/wdfcircuits_triode.c \
#	valve/wavechild670/amplifiers.c \
#	valve/wavechild670/wdfcircuits.c \
#	valve/wavechild670/wavechild670.c \
# terminator
#LOCAL_LDLIBS := -llog
ifeq ($(TARGET_ARCH_ABI), armeabi-v7a)
LOCAL_CPPFLAGS += -Wall -Wextra -ffunction-sections -fdata-sections -Ofast -march=armv7-a -mfpu=neon -ftree-vectorize -DNDEBUG# -DDEBUG
LOCAL_CFLAGS += -Wall -Wextra -ffunction-sections -fdata-sections -Ofast -march=armv7-a -mfpu=neon -ftree-vectorize -DNDEBUG# -DDEBUG
else ifeq ($(TARGET_ARCH_ABI), x86)
LOCAL_CPPFLAGS += -ffunction-sections -fdata-sections -Ofast -ftree-vectorize -DNDEBUG# -DDEBUG
LOCAL_CFLAGS += -ffunction-sections -fdata-sections -Ofast -ftree-vectorize -DNDEBUG# -DDEBUG
else ifeq ($(TARGET_ARCH_ABI), armeabi)
LOCAL_CPPFLAGS += -ffunction-sections -fdata-sections -Ofast -ftree-vectorize -DNDEBUG
LOCAL_CFLAGS += -ffunction-sections -fdata-sections -Ofast -ftree-vectorize -DNDEBUG
endif
LOCAL_LDFLAGS += -Wl,--gc-sections,--exclude-libs,ALL
include $(BUILD_SHARED_LIBRARY)