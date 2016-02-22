LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)
WITH_DEXPREOPT := false
LOCAL_MODULE_TAGS := optional

LOCAL_PROGUARD_FLAG_FILES := proguard.flags

LOCAL_STATIC_JAVA_LIBRARIES := android-support-v13

LOCAL_SRC_FILES := $(call all-java-files-under, src)

LOCAL_PACKAGE_NAME := DSPManager8band

include $(BUILD_PACKAGE)
