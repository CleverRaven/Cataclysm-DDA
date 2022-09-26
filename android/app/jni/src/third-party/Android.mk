LOCAL_PATH := $(call my-dir)/../../../../../src/third-party

include $(CLEAR_VARS)

LOCAL_C_INCLUDES := $(LOCAL_PATH) $(LOCAL_PATH)/..

LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)

LOCAL_MODULE := third-party

LOCAL_CPP_FEATURES := exceptions rtti

# Add your application source files here...
FLATBUFFERS_SRCS := $(sort $(wildcard $(LOCAL_PATH)/flatbuffers/*.cpp))
LOCAL_SRC_FILES := $(sort $(FLATBUFFERS_SRCS:$(LOCAL_PATH)/%=%))

LOCAL_CFLAGS += -DBACKTRACE=1 -DLOCALIZE=1 -Wextra -Wall -fsigned-char

ifeq ($(OS),Windows_NT)
    # needed to bypass 8191 character limit on Windows command line
	LOCAL_SHORT_COMMANDS := true
endif

include $(BUILD_STATIC_LIBRARY)
