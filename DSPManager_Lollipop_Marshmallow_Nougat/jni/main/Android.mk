LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE    := fftw3
ifeq ($(TARGET_ARCH_ABI), armeabi-v7a)
LOCAL_SRC_FILES := libfftw3fNeon.a
else ifeq ($(TARGET_ARCH_ABI), armeabi)
LOCAL_SRC_FILES := libfftw3fplain.a
else ifeq ($(TARGET_ARCH_ABI), x86)
LOCAL_SRC_FILES := libfftw3fx86.a
endif
include $(PREBUILT_STATIC_LIBRARY)
include $(CLEAR_VARS)
LOCAL_MODULE    := fftw3thread
ifeq ($(TARGET_ARCH_ABI), armeabi-v7a)
LOCAL_SRC_FILES := libfftw3f_threadsNeon.a
else ifeq ($(TARGET_ARCH_ABI), armeabi)
LOCAL_SRC_FILES := libfftw3f_threadsPlain.a
else ifeq ($(TARGET_ARCH_ABI), x86)
LOCAL_SRC_FILES := libfftw3f_threadsx86.a
endif
include $(PREBUILT_STATIC_LIBRARY)
include $(CLEAR_VARS)
LOCAL_MODULE := jamesDSPImpulseToolbox
LOCAL_SRC_FILES := JdspImpResToolbox.c AutoConvolver.c
LOCAL_STATIC_LIBRARIES = libsndfile libsamplerate fftw3thread fftw3
LOCAL_CPPFLAGS += -ffunction-sections -fdata-sections -Ofast -ftree-vectorize -DNDEBUG
LOCAL_CFLAGS += -ffunction-sections -fdata-sections -Ofast -ftree-vectorize -DNDEBUG
LOCAL_LDFLAGS += -Wl,--gc-sections,--exclude-libs,ALL
include $(BUILD_SHARED_LIBRARY)