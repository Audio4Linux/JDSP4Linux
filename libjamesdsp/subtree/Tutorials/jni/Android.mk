LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE := libjamesdsp
LOCAL_PRELINK_MODULE := false
LOCAL_SRC_FILES := \
	../src/jamesdsp.c \
# terminator
LOCAL_LDLIBS := -llog
ifeq ($(TARGET_ARCH_ABI), armeabi-v7a)
LOCAL_CPPFLAGS += -Wall -Wextra -ffunction-sections -fdata-sections -Ofast -march=armv7-a -mfpu=neon -ftree-vectorize -fvisibility=hidden# -DDEBUG# -DNDEBUG
LOCAL_CFLAGS += -Wall -Wextra -ffunction-sections -fdata-sections -Ofast -march=armv7-a -mfpu=neon -ftree-vectorize -fvisibility=hidden# -DDEBUG# -DNDEBUG
else ifeq ($(TARGET_ARCH_ABI), arm64-v8a)
LOCAL_CPPFLAGS += -Wall -Wextra -ffunction-sections -fdata-sections -Ofast -march=armv8-a -mfpu=neon -ftree-vectorize -fvisibility=hidden# -DDEBUG# -DNDEBUG
LOCAL_CFLAGS += -Wall -Wextra -ffunction-sections -fdata-sections -Ofast -march=armv8-a -mfpu=neon -ftree-vectorize -fvisibility=hidden# -DDEBUG# -DNDEBUG
else ifeq ($(TARGET_ARCH_ABI), x86)
LOCAL_CPPFLAGS += -ffunction-sections -fdata-sections -Ofast -ftree-vectorize -DNDEBUG -fvisibility=hidden# -DDEBUG
LOCAL_CFLAGS += -ffunction-sections -fdata-sections -Ofast -ftree-vectorize -DNDEBUG -fvisibility=hidden# -DDEBUG
else ifeq ($(TARGET_ARCH_ABI), armeabi)
LOCAL_CPPFLAGS += -ffunction-sections -fdata-sections -Ofast -ftree-vectorize -DNDEBUG -fvisibility=hidden
LOCAL_CFLAGS += -ffunction-sections -fdata-sections -Ofast -ftree-vectorize -DNDEBUG -fvisibility=hidden
endif
LOCAL_LDFLAGS += -nodefaultlibs -Wl,--gc-sections,--exclude-libs,ALL
include $(BUILD_SHARED_LIBRARY)