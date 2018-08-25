LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE    := fftw3
ifeq ($(TARGET_ARCH_ABI), armeabi-v7a)
LOCAL_SRC_FILES := fftw/DoubleARM/libfftw3.a
else ifeq ($(TARGET_ARCH_ABI), arm64-v8a)
LOCAL_SRC_FILES := fftw/DoubleARM64/libfftw3.a
else ifeq ($(TARGET_ARCH_ABI), x86)
LOCAL_SRC_FILES := fftw/Doublex86/libfftw3.a
endif
include $(PREBUILT_STATIC_LIBRARY)
include $(CLEAR_VARS)
LOCAL_MODULE    := fftw3thread
ifeq ($(TARGET_ARCH_ABI), armeabi-v7a)
LOCAL_SRC_FILES := fftw/DoubleARM/libfftw3_threads.a
else ifeq ($(TARGET_ARCH_ABI), arm64-v8a)
LOCAL_SRC_FILES := fftw/DoubleARM64/libfftw3_threads.a
else ifeq ($(TARGET_ARCH_ABI), x86)
LOCAL_SRC_FILES := fftw/Doublex86/libfftw3_threads.a
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