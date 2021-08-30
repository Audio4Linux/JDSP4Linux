LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE := jamesDSPImpulseToolbox
LOCAL_SRC_FILES := cpthread.c interpolation.c JdspImpResToolbox.c
LOCAL_STATIC_LIBRARIES = libsamplerate
LOCAL_CPPFLAGS += -ffunction-sections -fdata-sections -Ofast -ftree-vectorize -DNDEBUG
LOCAL_CFLAGS += -ffunction-sections -fdata-sections -Ofast -ftree-vectorize -DNDEBUG
LOCAL_LDFLAGS += -Wl,--gc-sections,--exclude-libs,ALL
include $(BUILD_SHARED_LIBRARY)