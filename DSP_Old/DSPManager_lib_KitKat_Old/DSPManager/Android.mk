ifneq ($(TARGET_PROVIDES_AUDIO_EFFECTS),true)

LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional

LOCAL_PROGUARD_FLAG_FILES := proguard.flags

LOCAL_STATIC_JAVA_LIBRARIES := android-support-v13

LOCAL_SRC_FILES := $(call all-java-files-under, src)

LOCAL_PACKAGE_NAME := DSPManager
WITH_DEXPREOPT := false
include $(BUILD_PACKAGE)

endif
