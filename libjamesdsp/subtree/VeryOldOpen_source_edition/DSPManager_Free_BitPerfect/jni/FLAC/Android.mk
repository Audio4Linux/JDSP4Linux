LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE := libFLAC
LOCAL_SRC_FILES:= \
	libFLAC/bitmath.c \
	libFLAC/bitreader.c \
	libFLAC/bitwriter.c \
	libFLAC/cpu.c \
	libFLAC/crc.c \
	libFLAC/fixed.c \
	libFLAC/float.c \
	libFLAC/format.c \
	libFLAC/lpc.c \
	libFLAC/memory.c \
	libFLAC/md5.c \
	libFLAC/metadata_object.c \
	libFLAC/stream_decoder.c \
	libFLAC/stream_encoder.c \
	libFLAC/stream_encoder_framing.c \
	libFLAC/window.c

LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/libFLAC/include/ \
	$(LOCAL_PATH)/include

LOCAL_CFLAGS += -DHAVE_CONFIG_H -DFLAC__NO_MD5 -DFLAC__INTEGER_ONLY_LIBRARY
LOCAL_CFLAGS += -D_REENTRANT -DPIC -DU_COMMON_IMPLEMENTATION -fPIC
LOCAL_CFLAGS += -O3 -funroll-loops -finline-functions
LOCAL_ARM_MODE := arm
include $(BUILD_STATIC_LIBRARY)