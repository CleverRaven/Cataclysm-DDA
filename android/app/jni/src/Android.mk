LOCAL_PATH := $(call my-dir)/../../../../src

include $(CLEAR_VARS)

LOCAL_MODULE := main

SDL_PATH := ../SDL2

LOCAL_C_INCLUDES := $(SDL_PATH)/include

LOCAL_CPP_FEATURES := exceptions rtti

# Add your application source files here...
FILE_LIST := $(sort $(wildcard $(LOCAL_PATH)/*.cpp))
LOCAL_SRC_FILES := $(sort $(FILE_LIST:$(LOCAL_PATH)/%=%))

LOCAL_SHARED_LIBRARIES := libhidapi SDL2 SDL2_mixer SDL2_image SDL2_ttf libintl-lite mpg123

LOCAL_LDLIBS := -lGLESv1_CM -lGLESv2 -llog

LOCAL_CFLAGS += -DTILES=1 -DSDL_SOUND=1 -DCATA_NO_CPP11_STRING_CONVERSIONS=1 -DLOCALIZE=1 -Wextra -Wall -fsigned-char -ffast-math

LOCAL_LDFLAGS += $(LOCAL_CFLAGS)

ifeq ($(OS),Windows_NT)
    # needed to bypass 8191 character limit on Windows command line
	LOCAL_SHORT_COMMANDS := true
endif

include $(BUILD_SHARED_LIBRARY)
