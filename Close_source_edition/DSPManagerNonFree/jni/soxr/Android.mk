LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)
LOCAL_CFLAGS := -Wall -DSOXR_LIB -DPFFFT_SIMD_DISABLE
LOCAL_CPPFLAGS += -ffunction-sections -fdata-sections -Ofast -ftree-vectorize
LOCAL_CFLAGS += -ffunction-sections -fdata-sections -Ofast -ftree-vectorize
LOCAL_SRC_FILES := \
 src/data-io.c \
 src/dbesi0.c \
 src/fft4g32.c \
 src/fft4g64.c \
 src/filter.c \
 src/lsr.c \
 src/pffft.c \
 src/rate32.c \
 src/rate64.c \
 src/soxr.c \
 src/vr32.c

LOCAL_C_INCLUDES += \
 src \

LOCAL_MODULE := libsoxr
include $(BUILD_STATIC_LIBRARY)