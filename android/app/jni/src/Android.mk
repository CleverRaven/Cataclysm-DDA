LOCAL_PATH := $(call my-dir)/../../../../src

include $(CLEAR_VARS)

LOCAL_MODULE := main

LOCAL_CPP_FEATURES := exceptions rtti

# Add your application source files here...
CATA_SRCS := $(sort $(wildcard $(LOCAL_PATH)/*.cpp))
LOCAL_SRC_FILES := $(sort $(CATA_SRCS:$(LOCAL_PATH)/%=%))

LOCAL_STATIC_LIBRARIES := third-party imgui

LOCAL_SHARED_LIBRARIES := libhidapi SDL2 SDL2_mixer SDL2_image SDL2_ttf mpg123

LOCAL_LDLIBS := -lGLESv1_CM -lGLESv2 -llog -lz

LOCAL_CFLAGS += -DTILES=1 -DSDL_SOUND=1 -DBACKTRACE=1 -DLOCALIZE=1 -DIMGUI_INCLUDE='"imgui/imgui.h"' -DIMGUI_INTERNAL_INCLUDE='"imgui/imgui_internal.h"' -DCUSTOM_IMGUIFILEDIALOG_CONFIG='"CustomImGuiFileDialogConfig.h"' -DCUSTOM_IMWIDGETS_CONFIG='"CustomImWidgetsConfig.h"' -Wextra -Wall -fsigned-char

LOCAL_LDFLAGS += $(LOCAL_CFLAGS)

ifeq ($(OS),Windows_NT)
    # needed to bypass 8191 character limit on Windows command line
	LOCAL_SHORT_COMMANDS := true
endif

include $(BUILD_SHARED_LIBRARY)

include $(LOCAL_PATH)/../android/app/jni/src/third-party/Android.mk
include $(LOCAL_PATH)/../android/app/jni/src/third-party/imgui/Android.mk
