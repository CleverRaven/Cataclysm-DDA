# vim: set expandtab tabstop=4 softtabstop=2 shiftwidth=2:
# Platforms:
# Linux/Cygwin native
#   (don't need to do anything)
# Linux 64-bit
#   make NATIVE=linux64
# Linux 32-bit
#   make NATIVE=linux32
# Linux cross-compile to Win32
#   make CROSS=i686-pc-mingw32-
#   or make CROSS=i586-mingw32msvc-
#   or whichever prefix your crosscompiler uses
#      as long as its name contains mingw32
# Linux cross-compile to OSX with osxcross
#   make CROSS=x86_64-apple-darwin15-
#        NATIVE=osx
#        CLANG=1
#        OSXCROSS=1
#        LIBSDIR=../libs
#        FRAMEWORKSDIR=../Frameworks
# Win32 (non-Cygwin)
#   Run: make NATIVE=win32
# OS X
#   Run: make NATIVE=osx

# Build types:
# Debug (no optimizations)
#  Default
# ccache (use compilation caches)
#  make CCACHE=1
# Release (turn on optimizations)
#  make RELEASE=1
# Tiles (uses SDL rather than ncurses)
#  make TILES=1
# Sound (requires SDL, so TILES must be enabled)
#  make TILES=1 SOUND=1
# Disable gettext, on some platforms the dependencies are hard to wrangle.
#  make LOCALIZE=0
# Disable backtrace support, not available on all platforms
#  make BACKTRACE=0
# Compile localization files for specified languages
#  make localization LANGUAGES="<lang_id_1>[ lang_id_2][ ...]"
#  (for example: make LANGUAGES="zh_CN zh_TW" for Chinese)
#  make localization LANGUAGES=all
#  (for every .po file in lang/po)
# Change mapsize (reality bubble size)
#  make MAPSIZE=<size>
# Adjust names of build artifacts (for example to allow easily toggling between build types).
#  make BUILD_PREFIX="release-"
# Generate a build artifact prefix from the other build flags.
#  make AUTO_BUILD_PREFIX=1
# Install to system directories.
#  make install
# Enable lua support. Required only for full-fledged mods.
#  make LUA=1
# Use user's XDG base directories for save files and configs.
#  make USE_XDG_DIR=1
# Use user's home directory for save files.
#  make USE_HOME_DIR=1
# Use dynamic linking (requires system libraries).
#  make DYNAMIC_LINKING=1
# Use MSYS2 as the build environment on Windows
#  make MSYS2=1
# Astyle the source files that aren't blacklisted. (maintain current level of styling)
#  make astyle
# Check if source files are styled properly (regression test, astyle_blacklist tracks un-styled files)
#  make astyle-check
# Astyle all source files using the current rules (don't PR this, it's too many changes at once).
#  make astyle-all
# Style the whitelisted json files (maintain the current level of styling).
#  make style-json
# Style all json files using the current rules (don't PR this, it's too many changes at once).
#  make style-all-json

# comment these to toggle them as one sees fit.
# DEBUG is best turned on if you plan to debug in gdb -- please do!
# PROFILE is for use with gprof or a similar program -- don't bother generally
# RELEASE is flags for release builds, this disables some debugging flags and
# enforces build failure when warnings are encountered.
# We want to error on everything to make sure we don't check in code with new warnings.
RELEASE_FLAGS = -Werror
WARNINGS = -Wall -Wextra
# Uncomment below to disable warnings
#WARNINGS = -w
DEBUGSYMS = -g
ifeq ($(shell sh -c 'uname -o 2>/dev/null || echo not'),Cygwin)
  DEBUG =
else
  DEBUG = -D_GLIBCXX_DEBUG
endif
#PROFILE = -pg
#OTHERS = -O3
#DEFINES = -DNDEBUG

# Disable debug. Comment this out to get logging.
#DEFINES = -DENABLE_LOGGING

# Limit debug to level. Comment out all for all levels.
#DEFINES += -DDEBUG_INFO
#DEFINES += -DDEBUG_WARNING
#DEFINES += -DDEBUG_ERROR
#DEFINES += -DDEBUG_PEDANTIC_INFO

# Limit debug to section. Comment out all for all levels.
#DEFINES += -DDEBUG_ENABLE_MAIN
#DEFINES += -DDEBUG_ENABLE_MAP
#DEFINES += -DDEBUG_ENABLE_MAP_GEN
#DEFINES += -DDEBUG_ENABLE_GAME

# Explicitly let 'char' to be 'signed char' to fix #18776
OTHERS += -fsigned-char

VERSION = 0.C

TARGET_NAME = cataclysm
TILES_TARGET_NAME = $(TARGET_NAME)-tiles

TARGET = $(BUILD_PREFIX)$(TARGET_NAME)
TILESTARGET = $(BUILD_PREFIX)$(TILES_TARGET_NAME)
ifdef TILES
APPTARGET = $(TILESTARGET)
else
APPTARGET = $(TARGET)
endif
W32TILESTARGET = $(BUILD_PREFIX)$(TILES_TARGET_NAME).exe
W32TARGET = $(BUILD_PREFIX)$(TARGET_NAME).exe
CHKJSON_BIN = $(BUILD_PREFIX)chkjson
BINDIST_DIR = $(BUILD_PREFIX)bindist
BUILD_DIR = $(CURDIR)
SRC_DIR = src
LUA_DIR = lua
LUASRC_DIR = $(SRC_DIR)/$(LUA_DIR)
# if you have LUAJIT installed, try make LUA_BINARY=luajit for extra speed
LUA_BINARY = lua
LOCALIZE = 1
ASTYLE_BINARY = astyle

# tiles object directories are because gcc gets confused # Appears that the default value of $LD is unsuitable on most systems

# when preprocessor defines change, but the source doesn't
ODIR = $(BUILD_PREFIX)obj
ODIRTILES = $(BUILD_PREFIX)obj/tiles
W32ODIR = $(BUILD_PREFIX)objwin
W32ODIRTILES = $(W32ODIR)/tiles

ifdef AUTO_BUILD_PREFIX
  BUILD_PREFIX = $(if $(RELEASE),release-)$(if $(TILES),tiles-)$(if $(SOUND),sound-)$(if $(LOCALIZE),local-)$(if $(BACKTRACE),back-)$(if $(MAPSIZE),map-$(MAPSIZE)-)$(if $(LUA),lua-)$(if $(USE_XDG_DIR),xdg-)$(if $(USE_HOME_DIR),home-)$(if $(DYNAMIC_LINKING),dynamic-)$(if $(MSYS2),msys2-)
  export BUILD_PREFIX
endif

OS  = $(shell uname -s)

# if $(OS) contains 'BSD'
ifneq ($(findstring BSD,$(OS)),)
  BSD = 1
endif

# This sets CXX and so must be up here
ifdef CLANG
  # Allow setting specific CLANG version
  ifeq ($(CLANG), 1)
    CLANGCMD = clang++
  else
    CLANGCMD = $(CLANG)
  endif
  ifeq ($(NATIVE), osx)
    USE_LIBCXX = 1
  endif
  ifdef USE_LIBCXX
    OTHERS += -stdlib=libc++
    LDFLAGS += -stdlib=libc++
  endif
  ifdef CCACHE
    CXX = CCACHE_CPP2=1 ccache $(CROSS)$(CLANGCMD)
    LD  = CCACHE_CPP2=1 ccache $(CROSS)$(CLANGCMD)
  else
    CXX = $(CROSS)$(CLANGCMD)
    LD  = $(CROSS)$(CLANGCMD)
  endif
else
  # Compiler version & target machine - used later for MXE ICE workaround
  ifdef CROSS
    CXXVERSION := $(shell $(CROSS)$(CXX) --version | grep -i gcc | sed 's/^.* //g')
    CXXMACHINE := $(shell $(CROSS)$(CXX) -dumpmachine)
  endif

  # Expand at reference time to avoid recursive reference
  OS_COMPILER := $(CXX)
  # Appears that the default value of $LD is unsuitable on most systems
  OS_LINKER := $(CXX)
  ifdef CCACHE
    CXX = ccache $(CROSS)$(OS_COMPILER)
    LD  = ccache $(CROSS)$(OS_LINKER)
  else
    CXX = $(CROSS)$(OS_COMPILER)
    LD  = $(CROSS)$(OS_LINKER)
  endif
endif

STRIP = $(CROSS)strip
RC  = $(CROSS)windres
AR  = $(CROSS)ar

# We don't need scientific precision for our math functions, this lets them run much faster.
CXXFLAGS += -ffast-math
LDFLAGS += $(PROFILE)

# enable optimizations. slow to build
ifdef RELEASE
  ifeq ($(NATIVE), osx)
    ifeq ($(shell $(CXX) -E -Os - < /dev/null > /dev/null 2>&1 && echo fos),fos)
      OPTLEVEL = -Os
    else
      OPTLEVEL = -O3
    endif
  else
    # MXE ICE Workaround
    # known bad on 4.9.3 and 4.9.4, if it gets fixed this could include a version test too
    ifeq ($(CXXMACHINE), x86_64-w64-mingw32.static)
      OPTLEVEL = -O3
    else
      OPTLEVEL = -Os
    endif
  endif
  ifdef LTO
    ifdef CLANG
      # LLVM's LTO will complain if the optimization level isn't between O0 and
      # O3 (inclusive)
      OPTLEVEL = -O3
    endif
  endif
  CXXFLAGS += $(OPTLEVEL)

  ifdef LTO
    LDFLAGS += -fuse-ld=gold
    ifdef CLANG
      LTOFLAGS += -flto
    else
      LTOFLAGS += -flto=jobserver -flto-odr-type-merging
    endif
  endif
  CXXFLAGS += $(LTOFLAGS)

  # OTHERS += -mmmx -m3dnow -msse -msse2 -msse3 -mfpmath=sse -mtune=native
  # Strip symbols, generates smaller executable.
  OTHERS += $(RELEASE_FLAGS)
  DEBUG =
  ifndef DEBUG_SYMBOLS
    DEBUGSYMS =
  endif
  DEFINES += -DRELEASE
  # Check for astyle or JSON regressions on release builds.
  CHECKS = astyle-check style-json
endif

ifndef RELEASE
  ifeq ($(shell $(CXX) -E -Og - < /dev/null > /dev/null 2>&1 && echo fog),fog)
    OPTLEVEL = -Og
  else
    OPTLEVEL = -O0
  endif
  CXXFLAGS += $(OPTLEVEL)
endif

ifeq ($(shell sh -c 'uname -o 2>/dev/null || echo not'),Cygwin)
    OTHERS += -std=gnu++11
  else
    OTHERS += -std=c++11
endif

CXXFLAGS += $(WARNINGS) $(DEBUG) $(DEBUGSYMS) $(PROFILE) $(OTHERS) -MMD -MP

BINDIST_EXTRAS += README.md data doc
BINDIST    = $(BUILD_PREFIX)cataclysmdda-$(VERSION).tar.gz
W32BINDIST = $(BUILD_PREFIX)cataclysmdda-$(VERSION).zip
BINDIST_CMD    = tar --transform=s@^$(BINDIST_DIR)@cataclysmdda-$(VERSION)@ -czvf $(BINDIST) $(BINDIST_DIR)
W32BINDIST_CMD = cd $(BINDIST_DIR) && zip -r ../$(W32BINDIST) * && cd $(BUILD_DIR)


# Check if called without a special build target
ifeq ($(NATIVE),)
  ifeq ($(CROSS),)
    ifeq ($(shell sh -c 'uname -o 2>/dev/null || echo not'),Cygwin)
      DEFINES += -DCATA_NO_CPP11_STRING_CONVERSIONS
      TARGETSYSTEM=CYGWIN
    else
      TARGETSYSTEM=LINUX
    endif
  endif
endif

# Linux 64-bit
ifeq ($(NATIVE), linux64)
  CXXFLAGS += -m64
  LDFLAGS += -m64
  TARGETSYSTEM=LINUX
  ifdef GOLD
    CXXFLAGS += -fuse-ld=gold
    LDFLAGS += -fuse-ld=gold
  endif
else
  # Linux 32-bit
  ifeq ($(NATIVE), linux32)
    CXXFLAGS += -m32
    LDFLAGS += -m32
    TARGETSYSTEM=LINUX
    ifdef GOLD
      CXXFLAGS += -fuse-ld=gold
      LDFLAGS += -fuse-ld=gold
    endif
  endif
endif

# OSX
ifeq ($(NATIVE), osx)
  ifdef CLANG
    OSX_MIN = 10.7
  else
    OSX_MIN = 10.5
  endif
  DEFINES += -DMACOSX
  CXXFLAGS += -mmacosx-version-min=$(OSX_MIN)
  LDFLAGS += -mmacosx-version-min=$(OSX_MIN) -framework CoreFoundation
  ifdef FRAMEWORK
    ifeq ($(FRAMEWORKSDIR),)
      FRAMEWORKSDIR := $(strip $(if $(shell [ -d $(HOME)/Library/Frameworks ] && echo 1), \
                             $(if $(shell find $(HOME)/Library/Frameworks -name 'SDL2.*'), \
                               $(HOME)/Library/Frameworks,),))
    endif
    ifeq ($(FRAMEWORKSDIR),)
      FRAMEWORKSDIR := $(strip $(if $(shell find /Library/Frameworks -name 'SDL2.*'), \
                                 /Library/Frameworks,))
    endif
    ifeq ($(FRAMEWORKSDIR),)
      $(error "SDL2 framework not found")
    endif
  endif
  ifeq ($(LOCALIZE), 1)
    LDFLAGS += -lintl
    ifdef OSXCROSS
      LDFLAGS += -L$(LIBSDIR)/gettext/lib
      CXXFLAGS += -I$(LIBSDIR)/gettext/include
    endif
    ifeq ($(MACPORTS), 1)
      ifneq ($(TILES), 1)
        CXXFLAGS += -I$(shell ncursesw6-config --includedir)
        LDFLAGS += -L$(shell ncursesw6-config --libdir)
      endif
    endif
  endif
  TARGETSYSTEM=LINUX
  ifneq ($(OS), Linux)
    BINDIST_CMD = tar -s"@^$(BINDIST_DIR)@cataclysmdda-$(VERSION)@" -czvf $(BINDIST) $(BINDIST_DIR)
  endif
endif

# Win32 (MinGW32 or MinGW-w64(32bit)?)
ifeq ($(NATIVE), win32)
# Any reason not to use -m32 on MinGW32?
  TARGETSYSTEM=WINDOWS
else
  # Win64 (MinGW-w64? 64bit isn't currently working.)
  ifeq ($(NATIVE), win64)
    CXXFLAGS += -m64
    LDFLAGS += -m64
    TARGETSYSTEM=WINDOWS
  endif
endif

# Cygwin
ifeq ($(NATIVE), cygwin)
  TARGETSYSTEM=CYGWIN
endif

# MXE cross-compile to win32
ifneq (,$(findstring mingw32,$(CROSS)))
  DEFINES += -DCROSS_LINUX
  TARGETSYSTEM=WINDOWS
endif

# Global settings for Windows targets
ifeq ($(TARGETSYSTEM),WINDOWS)
  BACKTRACE = 0
  CHKJSON_BIN = chkjson.exe
  TARGET = $(W32TARGET)
  BINDIST = $(W32BINDIST)
  BINDIST_CMD = $(W32BINDIST_CMD)
  ODIR = $(W32ODIR)
  ifdef DYNAMIC_LINKING
    # Windows isn't sold with programming support, these are static to remove MinGW dependency.
    LDFLAGS += -static-libgcc -static-libstdc++
  else
    LDFLAGS += -static
  endif
  ifeq ($(LOCALIZE), 1)
    LDFLAGS += -lintl -liconv
  endif
  W32FLAGS += -Wl,-stack,12000000,-subsystem,windows
  RFLAGS = -J rc -O coff
  ifeq ($(NATIVE), win64)
    RFLAGS += -F pe-x86-64
  endif
endif

ifdef MAPSIZE
    CXXFLAGS += -DMAPSIZE=$(MAPSIZE)
endif

ifeq ($(shell git rev-parse --is-inside-work-tree),true)
  # We have a git repository, use git version
  DEFINES += -DGIT_VERSION
endif

PKG_CONFIG = $(CROSS)pkg-config
SDL2_CONFIG = $(CROSS)sdl2-config

ifdef SOUND
  ifndef TILES
    $(error "SOUND=1 only works with TILES=1")
  endif
  ifeq ($(NATIVE),osx)
    ifdef FRAMEWORK
      CXXFLAGS += -I$(FRAMEWORKSDIR)/SDL2_mixer.framework/Headers
      LDFLAGS += -F$(FRAMEWORKSDIR)/SDL2_mixer.framework/Frameworks \
		 -framework SDL2_mixer -framework Vorbis -framework Ogg
    else # libsdl build
      ifeq ($(MACPORTS), 1)
        LDFLAGS += -lSDL2_mixer -lvorbisfile -lvorbis -logg
      else # homebrew
        CXXFLAGS += $(shell $(PKG_CONFIG) --cflags SDL2_mixer)
        LDFLAGS += $(shell $(PKG_CONFIG) --libs SDL2_mixer)
        LDFLAGS += -lvorbisfile -lvorbis -logg
      endif
    endif
  else # not osx
    CXXFLAGS += $(shell $(PKG_CONFIG) --cflags SDL2_mixer)
    LDFLAGS += $(shell $(PKG_CONFIG) --libs SDL2_mixer)
    LDFLAGS += -lpthread
  endif

  ifdef MSYS2
    LDFLAGS += -lmad
  endif

  CXXFLAGS += -DSDL_SOUND
endif

ifdef LUA
  ifeq ($(TARGETSYSTEM),WINDOWS)
    ifdef MSYS2
      LUA_USE_PKGCONFIG := 1
    else
      # Windows expects to have lua unpacked at a specific location
      LUA_LIBS := -llua
    endif
  else
    LUA_USE_PKGCONFIG := 1
  endif

  ifdef OSXCROSS
    LUA_LIBS = -L$(LIBSDIR)/lua/lib -llua -lm
    LUA_CFLAGS = -I$(LIBSDIR)/lua/include
  else
    ifdef LUA_USE_PKGCONFIG
      # On unix-like systems, use pkg-config to find lua
      LUA_CANDIDATES = lua5.3 lua5.2 lua-5.3 lua-5.2 lua5.1 lua-5.1 lua $(LUA_BINARY)
      LUA_FOUND = $(firstword $(foreach lua,$(LUA_CANDIDATES),\
          $(shell if $(PKG_CONFIG) --silence-errors --exists $(lua); then echo $(lua);fi)))
      LUA_PKG = $(if $(LUA_FOUND),$(LUA_FOUND),$(error "Lua not found by $(PKG_CONFIG), install it or make without 'LUA=1'"))
      LUA_LIBS := $(shell $(PKG_CONFIG) --silence-errors --libs $(LUA_PKG))
      LUA_CFLAGS := $(shell $(PKG_CONFIG) --silence-errors --cflags $(LUA_PKG))
    endif
  endif

  LDFLAGS += $(LUA_LIBS)
  CXXFLAGS += $(LUA_CFLAGS)

  CXXFLAGS += -DLUA
  LUA_DEPENDENCIES = $(LUASRC_DIR)/catabindings.cpp
  BINDIST_EXTRAS  += $(LUA_DIR)
endif

ifdef SDL
  TILES = 1
endif

ifdef TILES
  SDL = 1
  BINDIST_EXTRAS += gfx
  ifeq ($(NATIVE),osx)
    ifdef FRAMEWORK
      OSX_INC = -F$(FRAMEWORKSDIR) \
		-I$(FRAMEWORKSDIR)/SDL2.framework/Headers \
		-I$(FRAMEWORKSDIR)/SDL2_image.framework/Headers \
		-I$(FRAMEWORKSDIR)/SDL2_ttf.framework/Headers
      LDFLAGS += -F$(FRAMEWORKSDIR) \
		 -framework SDL2 -framework SDL2_image -framework SDL2_ttf -framework Cocoa
      CXXFLAGS += $(OSX_INC)
    else # libsdl build
      DEFINES += -DOSX_SDL2_LIBS
      # handle #include "SDL2/SDL.h" and "SDL.h"
      CXXFLAGS += $(shell sdl2-config --cflags) \
		  -I$(shell dirname $(shell sdl2-config --cflags | sed 's/-I\(.[^ ]*\) .*/\1/'))
      LDFLAGS += -framework Cocoa $(shell sdl2-config --libs) -lSDL2_ttf
      LDFLAGS += -lSDL2_image
    endif
  else # not osx
    CXXFLAGS += $(shell $(SDL2_CONFIG) --cflags)
    CXXFLAGS += $(shell $(PKG_CONFIG) SDL2_image --cflags)
    CXXFLAGS += $(shell $(PKG_CONFIG) SDL2_ttf --cflags)

    ifdef STATIC
      LDFLAGS += $(shell $(SDL2_CONFIG) --static-libs)
    else
      LDFLAGS += $(shell $(SDL2_CONFIG) --libs)
    endif

    LDFLAGS += -lSDL2_ttf -lSDL2_image

    # We don't use SDL_main -- we have proper main()/WinMain()
    CXXFLAGS := $(filter-out -Dmain=SDL_main,$(CXXFLAGS))
    LDFLAGS := $(filter-out -lSDL2main,$(LDFLAGS))
  endif

  DEFINES += -DTILES

  ifeq ($(TARGETSYSTEM),WINDOWS)
    ifndef DYNAMIC_LINKING
      # These differ depending on what SDL2 is configured to use.
      ifneq (,$(findstring mingw32,$(CROSS)))
        # We use pkg-config to find out which libs are needed with MXE
        LDFLAGS += $(shell $(PKG_CONFIG) SDL2_image --libs)
        LDFLAGS += $(shell $(PKG_CONFIG) SDL2_ttf --libs)
      else
        ifdef MSYS2
          LDFLAGS += -lfreetype -lpng -lz -ltiff -lbz2 -lharfbuzz -lglib-2.0 -llzma -lws2_32 -lintl -liconv -lwebp -ljpeg -luuid
        else
          LDFLAGS += -lfreetype -lpng -lz -ljpeg -lbz2
        endif
      endif
    else
      # Currently none needed by the game itself (only used by SDL2 layer).
      # Placeholder for future use (savegame compression, etc).
      LDFLAGS +=
    endif
    TARGET = $(W32TILESTARGET)
    ODIR = $(W32ODIRTILES)
  else
    TARGET = $(TILESTARGET)
    ODIR = $(ODIRTILES)
  endif
else
  # ONLY when not cross-compiling, check for pkg-config or ncurses5-config
  # When doing a cross-compile, we can't rely on the host machine's -configs
  ifeq ($(CROSS),)
    ifneq ($(shell pkg-config --libs ncurses 2>/dev/null),)
      HAVE_PKGCONFIG = 1
    endif
    ifneq ($(shell which ncurses5-config 2>/dev/null),)
      HAVE_NCURSES5CONFIG = 1
    endif
  endif

  # Link to ncurses if we're using a non-tiles, Linux build
  ifeq ($(HAVE_PKGCONFIG),1)
    ifeq ($(LOCALIZE),1)
      CXXFLAGS += $(shell pkg-config --cflags ncursesw)
      LDFLAGS += $(shell pkg-config --libs ncursesw)
    else
      CXXFLAGS += $(shell pkg-config --cflags ncurses)
      LDFLAGS += $(shell pkg-config --libs ncurses)
    endif
  else
    ifeq ($(HAVE_NCURSES5CONFIG),1)
      ifeq ($(LOCALIZE),1)
        CXXFLAGS += $(shell ncursesw5-config --cflags)
        LDFLAGS += $(shell ncursesw5-config --libs)
      else
        CXXFLAGS += $(shell ncurses5-config --cflags)
        LDFLAGS += $(shell ncurses5-config --libs)
      endif
    else
      ifneq ($(TARGETSYSTEM),WINDOWS)
        LDFLAGS += -lncurses
      endif

      ifdef OSXCROSS
        LDFLAGS += -L$(LIBSDIR)/ncurses/lib
        CXXFLAGS += -I$(LIBSDIR)/ncurses/include
      endif # OSXCROSS
    endif # HAVE_NCURSES5CONFIG
  endif # HAVE_PKGCONFIG
endif # TILES

ifeq ($(TARGETSYSTEM),CYGWIN)
  BACKTRACE = 0
  ifeq ($(LOCALIZE),1)
    # Work around Cygwin not including gettext support in glibc
    LDFLAGS += -lintl -liconv
  endif
endif

ifeq ($(BSD), 1)
  # BSDs have backtrace() and friends in a separate library
  ifeq ($(BACKTRACE), 1)
    LDFLAGS += -lexecinfo
    # ...which requires the frame pointer
    CXXFLAGS += -fno-omit-frame-pointer
  endif

 # And similarly, their libcs don't have gettext built in
  ifeq ($(LOCALIZE),1)
    LDFLAGS += -lintl -liconv
  endif
endif

# Global settings for Windows targets (at end)
ifeq ($(TARGETSYSTEM),WINDOWS)
    LDFLAGS += -lgdi32 -lwinmm -limm32 -lole32 -loleaut32 -lversion
endif

ifeq ($(BACKTRACE),1)
  DEFINES += -DBACKTRACE
endif

ifeq ($(LOCALIZE),1)
  DEFINES += -DLOCALIZE
endif

ifeq ($(TARGETSYSTEM),LINUX)
  BINDIST_EXTRAS += cataclysm-launcher
endif

ifeq ($(TARGETSYSTEM),CYGWIN)
  BINDIST_EXTRAS += cataclysm-launcher
  DEFINES += -D_GLIBCXX_USE_C99_MATH_TR1
endif

ifdef MSYS2
  DEFINES += -D_GLIBCXX_USE_C99_MATH_TR1
endif

# Enumerations of all the source files and headers.
SOURCES = $(wildcard $(SRC_DIR)/*.cpp)
HEADERS = $(wildcard $(SRC_DIR)/*.h)
TESTSRC = $(wildcard tests/*.cpp)
TESTHDR = $(wildcard tests/*.h)
TOOLSRC = $(wildcard tools/json_tools/format/*.[ch]*)

_OBJS = $(SOURCES:$(SRC_DIR)/%.cpp=%.o)
ifeq ($(TARGETSYSTEM),WINDOWS)
  RSRC = $(wildcard $(SRC_DIR)/*.rc)
  _OBJS += $(RSRC:$(SRC_DIR)/%.rc=%.o)
endif
OBJS = $(sort $(patsubst %,$(ODIR)/%,$(_OBJS)))

ifdef LANGUAGES
  L10N = localization
endif

ifeq ($(TARGETSYSTEM), LINUX)
  ifneq ($(PREFIX),)
    DEFINES += -DPREFIX="$(PREFIX)" -DDATA_DIR_PREFIX
  endif
endif

ifeq ($(TARGETSYSTEM), CYGWIN)
  ifneq ($(PREFIX),)
    DEFINES += -DPREFIX="$(PREFIX)" -DDATA_DIR_PREFIX
  endif
endif

ifeq ($(USE_HOME_DIR),1)
  ifeq ($(USE_XDG_DIR),1)
    $(error "USE_HOME_DIR=1 does not work with USE_XDG_DIR=1")
  endif
  DEFINES += -DUSE_HOME_DIR
endif

ifeq ($(USE_XDG_DIR),1)
  ifeq ($(USE_HOME_DIR),1)
    $(error "USE_HOME_DIR=1 does not work with USE_XDG_DIR=1")
  endif
  DEFINES += -DUSE_XDG_DIR
endif

ifdef LTO
  # Depending on the compiler version, LTO usually requires all the
  # optimization flags to be specified on the link line, and requires them to
  # match the original invocations.
  LDFLAGS += $(CXXFLAGS)
endif

all: version $(CHECKS) $(TARGET) $(L10N) tests
	@

$(TARGET): $(ODIR) $(OBJS)
	+$(LD) $(W32FLAGS) -o $(TARGET) $(OBJS) $(LDFLAGS)
ifdef RELEASE
  ifndef DEBUG_SYMBOLS
	$(STRIP) $(TARGET)
  endif
endif

$(BUILD_PREFIX)$(TARGET_NAME).a: $(ODIR) $(OBJS)
	$(AR) rcs $(BUILD_PREFIX)$(TARGET_NAME).a $(filter-out $(ODIR)/main.o $(ODIR)/messages.o,$(OBJS))

.PHONY: version json-verify
version:
	@( VERSION_STRING=$(VERSION) ; \
            [ -e ".git" ] && GITVERSION=$$( git describe --tags --always --dirty --match "[0-9A-Z]*.[0-9A-Z]*" ) && VERSION_STRING=$$GITVERSION ; \
            [ -e "$(SRC_DIR)/version.h" ] && OLDVERSION=$$(grep VERSION $(SRC_DIR)/version.h|cut -d '"' -f2) ; \
            if [ "x$$VERSION_STRING" != "x$$OLDVERSION" ]; then echo "#define VERSION \"$$VERSION_STRING\"" | tee $(SRC_DIR)/version.h ; fi \
         )
json-verify:
	$(LUA_BINARY) lua/json_verifier.lua

$(ODIR):
	mkdir -p $(ODIR)

$(ODIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) $(CPPFLAGS) $(DEFINES) $(CXXFLAGS) -c $< -o $@

$(ODIR)/%.o: $(SRC_DIR)/%.rc
	$(RC) $(RFLAGS) $< -o $@

src/version.h: version

src/version.cpp: src/version.h

$(LUASRC_DIR)/catabindings.cpp: $(LUA_DIR)/class_definitions.lua $(LUASRC_DIR)/generate_bindings.lua
	cd $(LUASRC_DIR) && $(LUA_BINARY) generate_bindings.lua

$(SRC_DIR)/catalua.cpp: $(LUA_DEPENDENCIES)

localization:
	lang/compile_mo.sh $(LANGUAGES)

$(CHKJSON_BIN): src/chkjson/chkjson.cpp src/json.cpp
	$(CXX) $(CXXFLAGS) -Isrc/chkjson -Isrc src/chkjson/chkjson.cpp src/json.cpp -o $(CHKJSON_BIN)

json-check: $(CHKJSON_BIN)
	./$(CHKJSON_BIN)

clean: clean-tests
	rm -rf *$(TARGET_NAME) *$(TILES_TARGET_NAME)
	rm -rf *$(TILES_TARGET_NAME).exe *$(TARGET_NAME).exe *$(TARGET_NAME).a
	rm -rf *obj *objwin
	rm -rf *$(BINDIST_DIR) *cataclysmdda-*.tar.gz *cataclysmdda-*.zip
	rm -f $(SRC_DIR)/version.h $(LUASRC_DIR)/catabindings.cpp
	rm -f $(CHKJSON_BIN)

distclean:
	rm -rf *$(BINDIST_DIR)
	rm -rf save
	rm -rf lang/mo
	rm -f data/options.txt
	rm -f data/keymap.txt
	rm -f data/auto_pickup.txt
	rm -f data/fontlist.txt

bindist: $(BINDIST)

ifeq ($(TARGETSYSTEM), LINUX)
DATA_PREFIX=$(DESTDIR)$(PREFIX)/share/cataclysm-dda/
BIN_PREFIX=$(DESTDIR)$(PREFIX)/bin
LOCALE_DIR=$(DESTDIR)$(PREFIX)/share/locale
install: version $(TARGET)
	mkdir -p $(DATA_PREFIX)
	mkdir -p $(BIN_PREFIX)
	install --mode=755 $(TARGET) $(BIN_PREFIX)
	cp -R --no-preserve=ownership data/core $(DATA_PREFIX)
	cp -R --no-preserve=ownership data/font $(DATA_PREFIX)
	cp -R --no-preserve=ownership data/json $(DATA_PREFIX)
	cp -R --no-preserve=ownership data/mods $(DATA_PREFIX)
	cp -R --no-preserve=ownership data/names $(DATA_PREFIX)
	cp -R --no-preserve=ownership data/raw $(DATA_PREFIX)
	cp -R --no-preserve=ownership data/motd $(DATA_PREFIX)
	cp -R --no-preserve=ownership data/credits $(DATA_PREFIX)
	cp -R --no-preserve=ownership data/title $(DATA_PREFIX)
ifdef TILES
	cp -R --no-preserve=ownership gfx $(DATA_PREFIX)
endif
ifdef SOUND
	cp -R --no-preserve=ownership data/sound $(DATA_PREFIX)
endif
ifdef LUA
	mkdir -p $(DATA_PREFIX)/lua
	install --mode=644 lua/autoexec.lua $(DATA_PREFIX)/lua
	install --mode=644 lua/class_definitions.lua $(DATA_PREFIX)/lua
endif
	install --mode=644 data/changelog.txt data/cataicon.ico data/fontdata.json \
                   LICENSE.txt -t $(DATA_PREFIX)
	mkdir -p $(LOCALE_DIR)
ifdef LANGUAGES
	LOCALE_DIR=$(LOCALE_DIR) lang/compile_mo.sh $(LANGUAGES)
endif
endif

ifeq ($(TARGETSYSTEM), CYGWIN)
DATA_PREFIX=$(DESTDIR)$(PREFIX)/share/cataclysm-dda/
BIN_PREFIX=$(DESTDIR)$(PREFIX)/bin
LOCALE_DIR=$(DESTDIR)$(PREFIX)/share/locale
install: version $(TARGET)
	mkdir -p $(DATA_PREFIX)
	mkdir -p $(BIN_PREFIX)
	install --mode=755 $(TARGET) $(BIN_PREFIX)
	cp -R --no-preserve=ownership data/core $(DATA_PREFIX)
	cp -R --no-preserve=ownership data/font $(DATA_PREFIX)
	cp -R --no-preserve=ownership data/json $(DATA_PREFIX)
	cp -R --no-preserve=ownership data/mods $(DATA_PREFIX)
	cp -R --no-preserve=ownership data/names $(DATA_PREFIX)
	cp -R --no-preserve=ownership data/raw $(DATA_PREFIX)
	cp -R --no-preserve=ownership data/motd $(DATA_PREFIX)
	cp -R --no-preserve=ownership data/credits $(DATA_PREFIX)
	cp -R --no-preserve=ownership data/title $(DATA_PREFIX)
ifdef TILES
	cp -R --no-preserve=ownership gfx $(DATA_PREFIX)
endif
ifdef SOUND
	cp -R --no-preserve=ownership data/sound $(DATA_PREFIX)
endif
ifdef LUA
	mkdir -p $(DATA_PREFIX)/lua
	install --mode=644 lua/autoexec.lua $(DATA_PREFIX)/lua
	install --mode=644 lua/class_definitions.lua $(DATA_PREFIX)/lua
endif
	install --mode=644 data/changelog.txt data/cataicon.ico data/fontdata.json \
                   LICENSE.txt -t $(DATA_PREFIX)
	mkdir -p $(LOCALE_DIR)
ifdef LANGUAGES
	LOCALE_DIR=$(LOCALE_DIR) lang/compile_mo.sh $(LANGUAGES)
endif
endif


ifeq ($(NATIVE), osx)
APPTARGETDIR=Cataclysm.app
APPRESOURCESDIR=$(APPTARGETDIR)/Contents/Resources
APPDATADIR=$(APPRESOURCESDIR)/data
ifndef FRAMEWORK
SDLLIBSDIR=$(shell sdl2-config --libs | sed -n 's/.*-L\([^ ]*\) .*/\1/p')
endif  # ifndef FRAMEWORK

appclean:
	rm -rf $(APPTARGETDIR)
	rm -f data/options.txt
	rm -f data/keymap.txt
	rm -f data/auto_pickup.txt
	rm -f data/fontlist.txt

data/osx/AppIcon.icns: data/osx/AppIcon.iconset
	iconutil -c icns $<

ifdef OSXCROSS
app: appclean version $(APPTARGET)
else
app: appclean version data/osx/AppIcon.icns $(APPTARGET)
endif
	mkdir -p $(APPTARGETDIR)/Contents
	cp data/osx/Info.plist $(APPTARGETDIR)/Contents/
	mkdir -p $(APPTARGETDIR)/Contents/MacOS
	cp data/osx/Cataclysm.sh $(APPTARGETDIR)/Contents/MacOS/
	mkdir -p $(APPRESOURCESDIR)
	cp $(APPTARGET) $(APPRESOURCESDIR)/
	cp data/osx/AppIcon.icns $(APPRESOURCESDIR)/
	mkdir -p $(APPDATADIR)
	cp data/fontdata.json $(APPDATADIR)
	cp -R data/core $(APPDATADIR)
	cp -R data/font $(APPDATADIR)
	cp -R data/json $(APPDATADIR)
	cp -R data/mods $(APPDATADIR)
	cp -R data/names $(APPDATADIR)
	cp -R data/raw $(APPDATADIR)
	cp -R data/motd $(APPDATADIR)
	cp -R data/credits $(APPDATADIR)
	cp -R data/title $(APPDATADIR)
ifdef LANGUAGES
	lang/compile_mo.sh $(LANGUAGES)
	mkdir -p $(APPRESOURCESDIR)/lang/mo/
	cp -pR lang/mo/* $(APPRESOURCESDIR)/lang/mo/
endif
ifeq ($(LOCALIZE), 1)
	LIBINTL=$$($(CROSS)otool -L $(APPTARGET) | grep libintl | sed -n 's/\(.*\.dylib\).*/\1/p') && if [ -f $$LIBINTL ]; then cp $$LIBINTL $(APPRESOURCESDIR)/; fi; \
		if [ ! -z "$$OSXCROSS" ]; then LIBINTL=$$(basename $$LIBINTL) && if [ ! -z "$$LIBINTL" ]; then cp $(LIBSDIR)/gettext/lib/$$LIBINTL $(APPRESOURCESDIR)/; fi; fi
endif
ifdef LUA
	cp -R lua $(APPRESOURCESDIR)/
	LIBLUA=$$($(CROSS)otool -L $(APPTARGET) | grep liblua | sed -n 's/\(.*\.dylib\).*/\1/p') && if [ -f $$LIBLUA ]; then cp $$LIBLUA $(APPRESOURCESDIR)/; fi; \
		if [ ! -z "$$OSXCROSS" ]; then LIBLUA=$$(basename $$LIBLUA) && if [ ! -z "$$LIBLUA" ]; then cp $(LIBSDIR)/lua/lib/$$LIBLUA $(APPRESOURCESDIR)/; fi; fi
endif # ifdef LUA
ifdef TILES
ifdef SOUND
	cp -R data/sound $(APPDATADIR)
endif  # ifdef SOUND
	cp -R gfx $(APPRESOURCESDIR)/
ifdef FRAMEWORK
	cp -R $(FRAMEWORKSDIR)/SDL2.framework $(APPRESOURCESDIR)/
	cp -R $(FRAMEWORKSDIR)/SDL2_image.framework $(APPRESOURCESDIR)/
	cp -R $(FRAMEWORKSDIR)/SDL2_ttf.framework $(APPRESOURCESDIR)/
ifdef SOUND
	cp -R $(FRAMEWORKSDIR)/SDL2_mixer.framework $(APPRESOURCESDIR)/
	cd $(APPRESOURCESDIR)/ && ln -s SDL2_mixer.framework/Frameworks/Vorbis.framework Vorbis.framework
	cd $(APPRESOURCESDIR)/ && ln -s SDL2_mixer.framework/Frameworks/Ogg.framework Ogg.framework
	cd $(APPRESOURCESDIR)/SDL2_mixer.framework/Frameworks && find . -maxdepth 1 -type d -not -name '*Vorbis.framework' -not -name '*Ogg.framework' -not -name '.' | xargs rm -rf
endif  # ifdef SOUND
else # libsdl build
	cp $(SDLLIBSDIR)/libSDL2.dylib $(APPRESOURCESDIR)/
	cp $(SDLLIBSDIR)/libSDL2_image.dylib $(APPRESOURCESDIR)/
	cp $(SDLLIBSDIR)/libSDL2_ttf.dylib $(APPRESOURCESDIR)/
endif  # ifdef FRAMEWORK

endif  # ifdef TILES

dmgdistclean:
	rm -rf Cataclysm
	rm -f Cataclysm.dmg
	rm -rf lang/mo

dmgdist: dmgdistclean $(L10N) app
ifdef OSXCROSS
	mkdir Cataclysm
	cp -a $(APPTARGETDIR) Cataclysm/$(APPTARGETDIR)
	cp data/osx/DS_Store Cataclysm/.DS_Store
	cp data/osx/dmgback.png Cataclysm/.background.png
	ln -s /Applications Cataclysm/Applications
	genisoimage -quiet -D -V "Cataclysm DDA" -no-pad -r -apple -o Cataclysm-uncompressed.dmg Cataclysm/
	dmg dmg Cataclysm-uncompressed.dmg Cataclysm.dmg
	rm Cataclysm-uncompressed.dmg
else
	dmgbuild -s data/osx/dmgsettings.py "Cataclysm DDA" Cataclysm.dmg
endif

endif  # ifeq ($(NATIVE), osx)

$(BINDIST): distclean version $(TARGET) $(L10N) $(BINDIST_EXTRAS) $(BINDIST_LOCALE)
	mkdir -p $(BINDIST_DIR)
	cp -R $(TARGET) $(BINDIST_EXTRAS) $(BINDIST_DIR)
ifdef LANGUAGES
	cp -R --parents lang/mo $(BINDIST_DIR)
endif
	$(BINDIST_CMD)

export ODIR _OBJS LDFLAGS CXX W32FLAGS DEFINES CXXFLAGS

ctags: $(SOURCES) $(HEADERS) $(TESTSRC) $(TESTHDR)
	ctags $(SOURCES) $(HEADERS) $(TESTSRC) $(TESTHDR)

etags: $(SOURCES) $(HEADERS) $(TESTSRC) $(TESTHDR)
	etags $(SOURCES) $(HEADERS) $(TESTSRC) $(TESTHDR)
	find data -name "*.json" -print0 | xargs -0 -L 50 etags --append

# Generate a list of files to check based on the difference between the blacklist and the existing source files.
ASTYLED_WHITELIST = $(filter-out $(shell cat astyle_blacklist), $(SOURCES) $(HEADERS) $(TESTSRC) $(TESTHDR) $(TOOLSRC) )

astyle: $(ASTYLED_WHITELIST)
	$(ASTYLE_BINARY) --options=.astylerc -n $(ASTYLED_WHITELIST)

astyle-all: $(SOURCES) $(HEADERS) $(TESTSRC) $(TESTHDR) $(TOOLSRC)
	$(ASTYLE_BINARY) --options=.astylerc -n $(SOURCES) $(HEADERS)
	$(ASTYLE_BINARY) --options=.astylerc -n $(TESTSRC) $(TESTHDR)

# Test whether the system has a version of astyle that supports --dry-run
ifeq ($(shell if $(ASTYLE_BINARY) -Q -X --dry-run src/game.h > /dev/null; then echo foo; fi),foo)
ASTYLE_CHECK=$(shell LC_ALL=C $(ASTYLE_BINARY) --options=.astylerc --dry-run -X -Q $(ASTYLED_WHITELIST))
endif

astyle-check:
ifdef ASTYLE_CHECK
	@if [ "$(findstring Formatted,$(ASTYLE_CHECK))" = "" ]; then echo "no astyle regressions";\
        else printf "astyle regressions found.\n$(ASTYLE_CHECK)\n" && false; fi
else
	@echo Cannot run an astyle check, your system either does not have astyle, or it is too old.
endif

JSON_FILES = $(shell find data -name *.json | sed "s|^\./||")
JSON_WHITELIST = $(filter-out $(shell cat json_blacklist), $(JSON_FILES))

style-json: $(JSON_WHITELIST)

$(JSON_WHITELIST): json_blacklist json_formatter
ifndef CROSS
	@tools/format/json_formatter.cgi $@
else
	@echo Cannot run json formatter in cross compiles.
endif

style-all-json: json_formatter
	find data -name "*.json" -print0 | xargs -0 -L 1 tools/format/json_formatter.cgi

json_formatter: tools/format/format.cpp src/json.cpp
	$(CXX) $(CXXFLAGS) -Itools/format -Isrc tools/format/format.cpp src/json.cpp -o tools/format/json_formatter.cgi

tests: version $(BUILD_PREFIX)cataclysm.a
	$(MAKE) -C tests

check: version $(BUILD_PREFIX)cataclysm.a
	$(MAKE) -C tests check

clean-tests:
	$(MAKE) -C tests clean

.PHONY: tests check ctags etags clean-tests install lint

-include $(SOURCES:$(SRC_DIR)/%.cpp=$(DEPDIR)/%.P)
-include ${OBJS:.o=.d}
