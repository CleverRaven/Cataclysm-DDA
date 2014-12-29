# Platforms:
# Linux native
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
# Win32
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
# Install to system directories.
#  make install
# Enable lua debug support
#  make LUA=1
# Use user's home directory for save files.
#  make USE_HOME_DIR=1
# Use dynamic linking (requires system libraries).
#  make DYNAMIC_LINKING=1

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

VERSION = 0.B

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


# tiles object directories are because gcc gets confused
# when preprocessor defines change, but the source doesn't
ODIR = obj
ODIRTILES = obj/tiles
W32ODIR = objwin
W32ODIRTILES = objwin/tiles
DDIR = .deps

OS  = $(shell uname -s)
ifdef CCACHE
  CXX = ccache $(CROSS)g++
  LD  = ccache $(CROSS)g++
else
  CXX = $(CROSS)g++
  LD  = $(CROSS)g++
endif
RC  = $(CROSS)windres

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
endif

ifdef CLANG
  ifeq ($(NATIVE), osx)
    OTHERS += -stdlib=libc++
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

BINDIST_EXTRAS += README.md data
BINDIST    = cataclysmdda-$(VERSION).tar.gz
W32BINDIST = cataclysmdda-$(VERSION).zip
BINDIST_CMD    = tar --transform=s@^$(BINDIST_DIR)@cataclysmdda-$(VERSION)@ -czvf $(BINDIST) $(BINDIST_DIR)
W32BINDIST_CMD = cd $(BINDIST_DIR) && zip -r ../$(W32BINDIST) * && cd $(BUILD_DIR)


# Check if called without a special build target
ifeq ($(NATIVE),)
  ifeq ($(CROSS),)
    TARGETSYSTEM=LINUX
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
  WARNINGS = -Werror -Wall -Wextra -Wno-switch -Wno-sign-compare -Wno-missing-braces
  ifeq ($(LOCALIZE), 1)
    LDFLAGS += -lintl
    ifeq ($(MACPORTS), 1)
      LDFLAGS += -L$(shell ncursesw5-config --libdir)
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

ifdef SOUND
  ifndef TILES
    $(error "SOUND=1 only works with TILES=1")
  endif
  CXXFLAGS += $(shell pkg-config --cflags SDL2_mixer)
  CXXFLAGS += -DSDL_SOUND
  LDFLAGS += $(shell pkg-config --libs SDL2_mixer)
  LDFLAGS += -lvorbisfile -lvorbis -logg
endif

ifdef LUA
  ifeq ($(TARGETSYSTEM),WINDOWS)
    # Windows expects to have lua unpacked at a specific location
    LDFLAGS += -llua
  else
    # On unix-like systems, use pkg-config to find lua
    LDFLAGS += $(shell pkg-config --silence-errors --libs lua5.2)
    CXXFLAGS += $(shell pkg-config --silence-errors --cflags lua5.2)
    LDFLAGS += $(shell pkg-config --silence-errors --libs lua-5.2)
    CXXFLAGS += $(shell pkg-config --silence-errors --cflags lua-5.2)
    LDFLAGS += $(shell pkg-config --silence-errors --libs lua)
    CXXFLAGS += $(shell pkg-config --silence-errors --cflags lua)
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
      DEFINES += -DOSX_SDL_FW
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
      ifdef TILES
        LDFLAGS += -lSDL2_image
      endif
    endif
  else # not osx
    LDFLAGS += -lSDL2 -lSDL2_ttf
    ifdef TILES
      LDFLAGS += -lSDL2_image
    endif
  endif
  ifdef TILES
    DEFINES += -DSDLTILES
  endif
  DEFINES += -DTILES
  ifeq ($(TARGETSYSTEM),WINDOWS)
    ifndef DYNAMIC_LINKING
      # These differ depending on what SDL2 is configured to use.
      LDFLAGS += -lfreetype -lpng -lz -ljpeg -lbz2
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
  # Link to ncurses if we're using a non-tiles, Linux build
  ifeq ($(TARGETSYSTEM),LINUX)
    ifeq ($(LOCALIZE),1)
      ifeq ($(OS), Darwin)
        LDFLAGS += -lncurses
      else
        ifeq ($(NATIVE), osx)
            LDFLAGS += -lncurses
        else
            LDFLAGS += $(shell ncursesw5-config --libs)
            CXXFLAGS += $(shell ncursesw5-config --cflags)
        endif
      endif
      # Work around Cygwin not including gettext support in glibc
      ifeq ($(shell sh -c 'uname -o 2>/dev/null || echo not'),Cygwin)
        LDFLAGS += -lintl -liconv
      endif
    else
      LDFLAGS += -lncurses
    endif
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
    DEFINES += -DPREFIX="$(PREFIX)"
  endif
endif

ifeq ($(USE_HOME_DIR),1)
  DEFINES += -DUSE_HOME_DIR
endif

all: version $(TARGET) $(L10N)
	@

$(TARGET): $(ODIR) $(DDIR) $(OBJS)
	$(LD) $(W32FLAGS) -o $(TARGET) $(DEFINES) \
          $(OBJS) $(LDFLAGS)

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
	$(CXX) $(DEFINES) $(CXXFLAGS) -c $< -o $@

$(ODIR)/%.o: $(SRC_DIR)/%.rc
	$(RC) $(RFLAGS) $< -o $@

version.cpp: version

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
	rm -rf $(TARGET) $(TILESTARGET) $(W32TILESTARGET) $(W32TARGET)
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
DATA_PREFIX=$(PREFIX)/share/cataclysm-dda/
BIN_PREFIX=$(PREFIX)/bin
LOCALE_DIR=$(PREFIX)/share/locale
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
ifdef LUA
	mkdir -p $(DATA_PREFIX)/lua
	install --mode=644 lua/autoexec.lua $(DATA_PREFIX)/lua
	install --mode=644 lua/class_definitions.lua $(DATA_PREFIX)/lua
endif
	install --mode=644 data/changelog.txt data/cataicon.ico data/fontdata.json \
                   README.txt LICENSE.txt -t $(DATA_PREFIX)
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
	iconutil -c icns $@

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
	cp -R gfx $(APPRESOURCESDIR)/
ifdef FRAMEWORK
	cp -R /Library/Frameworks/SDL2.framework $(APPRESOURCESDIR)/
	cp -R /Library/Frameworks/SDL2_image.framework $(APPRESOURCESDIR)/
	cp -R /Library/Frameworks/SDL2_ttf.framework $(APPRESOURCESDIR)/
else # libsdl build
	cp $(SDLLIBSDIR)/libSDL2.dylib $(APPRESOURCESDIR)/
	cp $(SDLLIBSDIR)/libSDL2_image.dylib $(APPRESOURCESDIR)/
	cp $(SDLLIBSDIR)/libSDL2_ttf.dylib $(APPRESOURCESDIR)/
endif  # ifdef FRAMEWORK

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
	find data/json -name "*.json" -print0 | xargs -0 -L 50 etags --append

tests: $(ODIR) $(DDIR) $(OBJS)
	$(MAKE) -C tests

check: tests
	$(MAKE) -C tests check

clean-tests:
	$(MAKE) -C tests clean

.PHONY: tests check ctags etags clean-tests install

-include $(SOURCES:$(SRC_DIR)/%.cpp=$(DEPDIR)/%.P)
-include ${OBJS:.o=.d}
