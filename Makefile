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
# Compile localization files for specified languages
#  make LANGUAGES="<lang_id_1>[ lang_id_2][ ...]"
#  (for example: make LANGUAGES="zh_CN zh_TW" for Chinese)
# Change mapsize (reality bubble size)
#  make MAPSIZE=<size>
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

# comment these to toggle them as one sees fit.
# DEBUG is best turned on if you plan to debug in gdb -- please do!
# PROFILE is for use with gprof or a similar program -- don't bother generally
# RELEASE is flags for release builds, we want to error on everything to make sure
# we don't check in code with new warnings, but we also have to disable some classes of warnings
# for now as we get rid of them.  In non-release builds we want to show all the warnings,
# even the ones we're allowing in release builds so they're visible to developers.
RELEASE_FLAGS = -Werror
WARNINGS = -Wall -Wextra
# Uncomment below to disable warnings
#WARNINGS = -w
ifeq ($(shell sh -c 'uname -o 2>/dev/null || echo not'),Cygwin)
  DEBUG = -g
else
  DEBUG = -g -D_GLIBCXX_DEBUG
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

VERSION = 0.C

TARGET = cataclysm
TILESTARGET = cataclysm-tiles
W32TILESTARGET = cataclysm-tiles.exe
W32TARGET = cataclysm.exe
CHKJSON_BIN = chkjson
BINDIST_DIR = bindist
BUILD_DIR = $(CURDIR)
SRC_DIR = src
LUA_DIR = lua
LUASRC_DIR = src/lua
# if you have LUAJIT installed, try make LUA_BINARY=luajit for extra speed
LUA_BINARY = lua
LOCALIZE = 1


# tiles object directories are because gcc gets confused # Appears that the default value of $LD is unsuitable on most systems

# when preprocessor defines change, but the source doesn't
ODIR = obj
ODIRTILES = obj/tiles
W32ODIR = objwin
W32ODIRTILES = objwin/tiles
DDIR = .deps

OS  = $(shell uname -s)

# if $(OS) contains 'BSD'
ifneq ($(findstring BSD,$(OS)),)
  BSD = 1
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
RC  = $(CROSS)windres

# We don't need scientific precision for our math functions, this lets them run much faster.
CXXFLAGS += -ffast-math
LDFLAGS += $(PROFILE)

# enable optimizations. slow to build
ifdef RELEASE
  ifeq ($(NATIVE), osx)
    CXXFLAGS += -O3
  else
    CXXFLAGS += -Os
    LDFLAGS += -s
  endif
  # OTHERS += -mmmx -m3dnow -msse -msse2 -msse3 -mfpmath=sse -mtune=native
  # Strip symbols, generates smaller executable.
  OTHERS += $(RELEASE_FLAGS)
  DEBUG =
  DEFINES += -DRELEASE
endif

ifdef CLANG
  ifeq ($(NATIVE), osx)
    OTHERS += -stdlib=libc++
    LDFLAGS += -stdlib=libc++
  endif
  ifdef CCACHE
    CXX = CCACHE_CPP2=1 ccache $(CROSS)clang++
    LD  = CCACHE_CPP2=1 ccache $(CROSS)clang++
  else
    CXX = $(CROSS)clang++
    LD  = $(CROSS)clang++
  endif
  WARNINGS = -Wall -Wextra -Wno-switch -Wno-sign-compare -Wno-missing-braces -Wno-type-limits -Wno-narrowing
endif

OTHERS += --std=c++11

CXXFLAGS += $(WARNINGS) $(DEBUG) $(PROFILE) $(OTHERS) -MMD

BINDIST_EXTRAS += README.md data doc
BINDIST    = cataclysmdda-$(VERSION).tar.gz
W32BINDIST = cataclysmdda-$(VERSION).zip
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
else
  # Linux 32-bit
  ifeq ($(NATIVE), linux32)
    CXXFLAGS += -m32
    LDFLAGS += -m32
    TARGETSYSTEM=LINUX
  endif
endif

# OSX
ifeq ($(NATIVE), osx)
  OSX_MIN = 10.5
  DEFINES += -DMACOSX
  CXXFLAGS += -mmacosx-version-min=$(OSX_MIN)
  LDFLAGS += -mmacosx-version-min=$(OSX_MIN)
  WARNINGS = -Werror -Wall -Wextra -Wno-switch -Wno-sign-compare -Wno-missing-braces
  ifeq ($(LOCALIZE), 1)
    LDFLAGS += -lintl
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
      CXXFLAGS += -I/Library/Frameworks/SDL2_mixer.framework/Headers \
		-I$(HOME)/Library/Frameworks/SDL2_mixer.framework/Headers
      LDFLAGS += -F/Library/Frameworks/SDL2_mixer.framework/Frameworks \
		 -F$(HOME)/Library/Frameworks/SDL2_mixer.framework/Frameworks \
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
    # Windows expects to have lua unpacked at a specific location
    LDFLAGS += -llua
  else
    LUA_CANDIDATES = lua5.2 lua-5.2 lua5.1 lua-5.1 lua
    LUA_FOUND = $(firstword $(foreach lua,$(LUA_CANDIDATES),\
        $(shell if $(PKG_CONFIG) --silence-errors --exists $(lua); then echo $(lua);fi)))
    LUA_PKG += $(if $(LUA_FOUND),$(LUA_FOUND),$(error "Lua not found by $(PKG_CONFIG), install it or make without 'LUA=1'"))
    # On unix-like systems, use pkg-config to find lua
    LDFLAGS += $(shell $(PKG_CONFIG) --silence-errors --libs $(LUA_PKG))
    CXXFLAGS += $(shell $(PKG_CONFIG) --silence-errors --cflags $(LUA_PKG))
  endif

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
      OSX_INC = -F/Library/Frameworks \
		-F$(HOME)/Library/Frameworks \
		-I/Library/Frameworks/SDL2.framework/Headers \
		-I$(HOME)/Library/Frameworks/SDL2.framework/Headers \
		-I/Library/Frameworks/SDL2_image.framework/Headers \
		-I$(HOME)/Library/Frameworks/SDL2_image.framework/Headers \
		-I/Library/Frameworks/SDL2_ttf.framework/Headers \
		-I$(HOME)/Library/Frameworks/SDL2_ttf.framework/Headers
      LDFLAGS += -F/Library/Frameworks \
		 -F$(HOME)/Library/Frameworks \
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
  ifeq ($(CROSS),)
    ifneq ($(shell which ncursesw5-config 2>/dev/null),)
      HAVE_NCURSESW5CONFIG = 1
    endif
  endif

  # Link to ncurses if we're using a non-tiles, Linux build
  ifeq ($(HAVE_NCURSESW5CONFIG),1)
    CXXFLAGS += $(shell ncursesw5-config --cflags)
    LDFLAGS += $(shell ncursesw5-config --libs)
  else
    LDFLAGS += -lncurses
  endif
endif

ifeq ($(TARGETSYSTEM),CYGWIN)
  ifeq ($(LOCALIZE),1)
    # Work around Cygwin not including gettext support in glibc
    LDFLAGS += -lintl -liconv
  endif
endif

# BSDs have backtrace() and friends in a separate library
ifeq ($(BSD), 1)
  LDFLAGS += -lexecinfo

 # And similarly, their libcs don't have gettext built in
  ifeq ($(LOCALIZE),1)
    LDFLAGS += -lintl -liconv
  endif
endif

# Global settings for Windows targets (at end)
ifeq ($(TARGETSYSTEM),WINDOWS)
    LDFLAGS += -lgdi32 -lwinmm -limm32 -lole32 -loleaut32 -lversion
endif

ifeq ($(LOCALIZE),1)
  DEFINES += -DLOCALIZE
endif

ifeq ($(TARGETSYSTEM),LINUX)
  BINDIST_EXTRAS += cataclysm-launcher
endif

ifeq ($(TARGETSYSTEM),CYGWIN)
  BINDIST_EXTRAS += cataclysm-launcher
endif

SOURCES = $(wildcard $(SRC_DIR)/*.cpp)
HEADERS = $(wildcard $(SRC_DIR)/*.h)
_OBJS = $(SOURCES:$(SRC_DIR)/%.cpp=%.o)
ifeq ($(TARGETSYSTEM),WINDOWS)
  RSRC = $(wildcard $(SRC_DIR)/*.rc)
  _OBJS += $(RSRC:$(SRC_DIR)/%.rc=%.o)
endif
OBJS = $(patsubst %,$(ODIR)/%,$(_OBJS))

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

all: version $(TARGET) $(L10N) tests
	@

$(TARGET): $(ODIR) $(DDIR) $(OBJS)
	$(LD) $(W32FLAGS) -o $(TARGET) $(OBJS) $(LDFLAGS)

cataclysm.a: $(ODIR) $(DDIR) $(OBJS)
	ar rcs cataclysm.a $(filter-out $(ODIR)/main.o $(ODIR)/messages.o,$(OBJS))

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

$(DDIR):
	@mkdir $(DDIR)

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
	rm -rf $(TARGET) $(TILESTARGET) $(W32TILESTARGET) $(W32TARGET) cataclysm.a
	rm -rf $(ODIR) $(W32ODIR) $(W32ODIRTILES)
	rm -rf $(BINDIST) $(W32BINDIST) $(BINDIST_DIR)
	rm -f $(SRC_DIR)/version.h $(LUASRC_DIR)/catabindings.cpp
	rm -f $(CHKJSON_BIN)

distclean:
	rm -rf $(BINDIST_DIR)
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
	cp -R --no-preserve=ownership data/font $(DATA_PREFIX)
	cp -R --no-preserve=ownership data/json $(DATA_PREFIX)
	cp -R --no-preserve=ownership data/mods $(DATA_PREFIX)
	cp -R --no-preserve=ownership data/names $(DATA_PREFIX)
	cp -R --no-preserve=ownership data/raw $(DATA_PREFIX)
	cp -R --no-preserve=ownership data/recycling $(DATA_PREFIX)
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
	LOCALE_DIR=$(LOCALE_DIR) lang/compile_mo.sh
endif

ifeq ($(TARGETSYSTEM), CYGWIN)
DATA_PREFIX=$(DESTDIR)$(PREFIX)/share/cataclysm-dda/
BIN_PREFIX=$(DESTDIR)$(PREFIX)/bin
LOCALE_DIR=$(DESTDIR)$(PREFIX)/share/locale
install: version $(TARGET)
	mkdir -p $(DATA_PREFIX)
	mkdir -p $(BIN_PREFIX)
	install --mode=755 $(TARGET) $(BIN_PREFIX)
	cp -R --no-preserve=ownership data/font $(DATA_PREFIX)
	cp -R --no-preserve=ownership data/json $(DATA_PREFIX)
	cp -R --no-preserve=ownership data/mods $(DATA_PREFIX)
	cp -R --no-preserve=ownership data/names $(DATA_PREFIX)
	cp -R --no-preserve=ownership data/raw $(DATA_PREFIX)
	cp -R --no-preserve=ownership data/recycling $(DATA_PREFIX)
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
	LOCALE_DIR=$(LOCALE_DIR) lang/compile_mo.sh
endif

ifdef TILES
ifeq ($(NATIVE), osx)
APPTARGETDIR=Cataclysm.app
APPRESOURCESDIR=$(APPTARGETDIR)/Contents/Resources
APPDATADIR=$(APPRESOURCESDIR)/data
ifndef FRAMEWORK
SDLLIBSDIR=$(shell sdl2-config --libs | sed -n 's/.*-L\([^ ]*\) .*/\1/p')
endif  # ifndef FRAMEWORK

appclean:
	rm -rf $(APPTARGETDIR)

data/osx/AppIcon.icns: data/osx/AppIcon.iconset
	iconutil -c icns $<

app: appclean version data/osx/AppIcon.icns $(TILESTARGET)
	mkdir -p $(APPTARGETDIR)/Contents
	cp data/osx/Info.plist $(APPTARGETDIR)/Contents/
	mkdir -p $(APPTARGETDIR)/Contents/MacOS
	cp data/osx/Cataclysm.sh $(APPTARGETDIR)/Contents/MacOS/
	mkdir -p $(APPRESOURCESDIR)
	cp $(TILESTARGET) $(APPRESOURCESDIR)/
	cp data/osx/AppIcon.icns $(APPRESOURCESDIR)/
	mkdir -p $(APPDATADIR)
	cp data/fontdata.json $(APPDATADIR)
	cp -R data/font $(APPDATADIR)
	cp -R data/json $(APPDATADIR)
	cp -R data/mods $(APPDATADIR)
	cp -R data/names $(APPDATADIR)
	cp -R data/raw $(APPDATADIR)
	cp -R data/recycling $(APPDATADIR)
	cp -R data/motd $(APPDATADIR)
	cp -R data/credits $(APPDATADIR)
	cp -R data/title $(APPDATADIR)
	# bundle libc++ to fix bad buggy version on osx 10.7
	LIBCPP=$$(otool -L $(TILESTARGET) | grep libc++ | sed -n 's/\(.*\.dylib\).*/\1/p') && cp $$LIBCPP $(APPRESOURCESDIR)/ && cp $$(otool -L $$LIBCPP | grep libc++abi | sed -n 's/\(.*\.dylib\).*/\1/p') $(APPRESOURCESDIR)/
ifdef SOUND
	cp -R data/sound $(APPDATADIR)
endif  # ifdef SOUND
ifdef LUA
	cp -R lua $(APPRESOURCESDIR)/
	LIBLUA=$$(otool -L $(TILESTARGET) | grep liblua | sed -n 's/\(.*\.dylib\).*/\1/p') && cp $$LIBLUA $(APPRESOURCESDIR)/
endif # ifdef LUA
	cp -R gfx $(APPRESOURCESDIR)/
ifdef FRAMEWORK
	cp -R /Library/Frameworks/SDL2.framework $(APPRESOURCESDIR)/
	cp -R /Library/Frameworks/SDL2_image.framework $(APPRESOURCESDIR)/
	cp -R /Library/Frameworks/SDL2_ttf.framework $(APPRESOURCESDIR)/
ifdef SOUND
	cp -R /Library/Frameworks/SDL2_mixer.framework $(APPRESOURCESDIR)/
	cd $(APPRESOURCESDIR)/ && ln -s SDL2_mixer.framework/Frameworks/Vorbis.framework Vorbis.framework
	cd $(APPRESOURCESDIR)/ && ln -s SDL2_mixer.framework/Frameworks/Ogg.framework Ogg.framework
	cd $(APPRESOURCESDIR)/SDL2_mixer.framework/Frameworks && find . -type d -maxdepth 1 -not -name '*Vorbis.framework' -not -name '*Ogg.framework' -not -name '.' | xargs rm -rf
endif  # ifdef SOUND
else # libsdl build
	cp $(SDLLIBSDIR)/libSDL2.dylib $(APPRESOURCESDIR)/
	cp $(SDLLIBSDIR)/libSDL2_image.dylib $(APPRESOURCESDIR)/
	cp $(SDLLIBSDIR)/libSDL2_ttf.dylib $(APPRESOURCESDIR)/
endif  # ifdef FRAMEWORK

dmgdistclean:
	rm -f Cataclysm.dmg

dmgdist: app dmgdistclean
	dmgbuild -s data/osx/dmgsettings.py "Cataclysm DDA" Cataclysm.dmg

endif  # ifeq ($(NATIVE), osx)
endif  # ifdef TILES

$(BINDIST): distclean version $(TARGET) $(L10N) $(BINDIST_EXTRAS) $(BINDIST_LOCALE)
	mkdir -p $(BINDIST_DIR)
	cp -R $(TARGET) $(BINDIST_EXTRAS) $(BINDIST_DIR)
ifdef LANGUAGES
	cp -R --parents lang/mo $(BINDIST_DIR)
endif
	$(BINDIST_CMD)

export ODIR _OBJS LDFLAGS CXX W32FLAGS DEFINES CXXFLAGS

ctags: $(SOURCES) $(HEADERS)
	ctags $(SOURCES) $(HEADERS)

etags: $(SOURCES) $(HEADERS)
	etags $(SOURCES) $(HEADERS)
	find data -name "*.json" -print0 | xargs -0 -L 50 etags --append

tests: version cataclysm.a
	$(MAKE) -C tests

check: version cataclysm.a
	$(MAKE) -C tests check

clean-tests:
	$(MAKE) -C tests clean

.PHONY: tests check ctags etags clean-tests install

-include $(SOURCES:$(SRC_DIR)/%.cpp=$(DEPDIR)/%.P)
-include ${OBJS:.o=.d}
