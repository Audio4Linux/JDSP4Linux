LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE := libsndfile
C_FILE_LIST := $(call all-subdir-c-files) \
                $(wildcard $(LOCAL_PATH)/G72x/*.c) \
                $(wildcard $(LOCAL_PATH)/GSM610/*.c) \
                $(wildcard $(LOCAL_PATH)/*.c)
LOCAL_SRC_FILES := $(C_FILE_LIST:$(LOCAL_PATH)/%=%)
LOCAL_STATIC_LIBRARIES = libFLAC
LOCAL_CPPFLAGS += -ffunction-sections -fdata-sections -Ofast -ftree-vectorize -DNDEBUG
LOCAL_CFLAGS += -ffunction-sections -fdata-sections -Ofast -ftree-vectorize -DNDEBUG
include $(BUILD_STATIC_LIBRARY)