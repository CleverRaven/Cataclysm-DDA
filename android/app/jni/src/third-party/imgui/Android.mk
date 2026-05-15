include $(CLEAR_VARS)

LOCAL_C_INCLUDES := $(LOCAL_PATH) $(LOCAL_PATH)/third-party $(LOCAL_PATH)/../android/app/jni/SDL2_ttf/external/freetype-2.4.12/include

LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)

LOCAL_MODULE := imgui

LOCAL_CPP_FEATURES := exceptions rtti

LOCAL_SHARED_LIBRARIES := SDL2

# Add your application source files here...
IMGUI_SRCS := $(sort $(wildcard $(LOCAL_PATH)/third-party/imgui/*.cpp))
IMGUI_SRCS := $(filter-out %/imgui_impl_sdl3.cpp %/imgui_impl_sdlrenderer3.cpp,$(IMGUI_SRCS))
LOCAL_SRC_FILES := $(sort $(IMGUI_SRCS:$(LOCAL_PATH)/%=%))

LOCAL_CFLAGS += -DBACKTRACE=1 -DLOCALIZE=1 -fsigned-char

ifeq ($(OS),Windows_NT)
    # needed to bypass 8191 character limit on Windows command line
	LOCAL_SHORT_COMMANDS = true
endif

include $(BUILD_STATIC_LIBRARY)
