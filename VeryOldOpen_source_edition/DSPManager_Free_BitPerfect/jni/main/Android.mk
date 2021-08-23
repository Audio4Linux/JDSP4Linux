LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE := jamesDSPImpulseToolbox
LOCAL_SRC_FILES := kissfft\kiss_fft.c kissfft\kiss_fftr.c JdspImpResToolbox.c AutoConvolver.c
LOCAL_STATIC_LIBRARIES = libsndfile libsamplerate
LOCAL_CPPFLAGS += -ffunction-sections -fdata-sections -Ofast -ftree-vectorize -DNDEBUG
LOCAL_CFLAGS += -ffunction-sections -fdata-sections -Ofast -ftree-vectorize -DNDEBUG
LOCAL_LDFLAGS += -Wl,--gc-sections,--exclude-libs,ALL
include $(BUILD_SHARED_LIBRARY)