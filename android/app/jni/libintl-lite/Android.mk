LOCAL_PATH := $(call my-dir)

###########################
#
# libintl-lite shared library
#
###########################

include $(CLEAR_VARS)

LOCAL_MODULE := libintl-lite

LOCAL_C_INCLUDES := $(LOCAL_PATH)

LOCAL_EXPORT_C_INCLUDES := $(LOCAL_C_INCLUDES)

LOCAL_SRC_FILES := libintl.cpp

LOCAL_CPP_FEATURES := exceptions rtti

LOCAL_LDLIBS := -llog

include $(BUILD_SHARED_LIBRARY)
