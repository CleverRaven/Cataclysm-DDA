# Reference: https://developer.android.com/ndk/guides/application_mk.html

# See CPLUSPLUS-SUPPORT.html in the NDK documentation for more information
APP_STL := c++_shared
APP_CPPFLAGS += -std=c++14
ifneq ($(OS),Windows_NT)
    APP_LDFLAGS += -fuse-ld=gold
endif

# Do not specify APP_OPTIM here, it is done through ndk-build NDK_DEBUG=0/1 setting instead
# See https://developer.android.com/ndk/guides/ndk-build.html#dvr
#APP_OPTIM := debug
#APP_OPTIM := release

# Min SDK level
APP_PLATFORM=android-16

