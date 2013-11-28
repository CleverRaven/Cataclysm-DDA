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
# Release (turn on optimizations)
#  make RELEASE=1
# Tiles (uses SDL rather than ncurses)
#  make TILES=1
# Disable gettext, on some platforms the dependencies are hard to wrangle.
#  make LOCALIZE=0
# Compile localization files for specified languages
#  make LANGUAGES="<lang_id_1>[ lang_id_2][ ...]"
#  (for example: make LANGUAGES="zh_CN zh_TW" for Chinese)
# Enable lua debug support
#  make LUA=1

# comment these to toggle them as one sees fit.
# DEBUG is best turned on if you plan to debug in gdb -- please do!
# PROFILE is for use with gprof or a similar program -- don't bother generally
WARNINGS = -Werror -Wall -Wextra -Wno-switch -Wno-sign-compare -Wno-missing-braces -Wno-unused-parameter -Wno-narrowing
# Uncomment below to disable warnings
#WARNINGS = -w
DEBUG = -g
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

VERSION = 0.9


TARGET = cataclysm
TILESTARGET = cataclysm-tiles
W32TILESTARGET = cataclysm-tiles.exe
W32TARGET = cataclysm.exe
CHKJSON_BIN = chkjson
BINDIST_DIR = bindist
BUILD_DIR = $(CURDIR)
SRC_DIR = src
LUA_DIR = lua
LOCALIZE = 1

# tiles object directories are because gcc gets confused
# when preprocessor defines change, but the source doesn't
ODIR = obj
ODIRTILES = obj/tiles
W32ODIR = objwin
W32ODIRTILES = objwin/tiles
DDIR = .deps

OS  = $(shell uname -o)
CXX = $(CROSS)g++
LD  = $(CROSS)g++
RC  = $(CROSS)windres

# enable optimizations. slow to build
ifdef RELEASE
  OTHERS += -O3
  DEBUG =
endif

ifdef CLANG
  CXX = $(CROSS)clang++
  LD  = $(CROSS)clang++
  OTHERS = --std=c++98 --analyze -fixit
  WARNINGS = -Wall -Wextra -Wno-switch -Wno-sign-compare -Wno-missing-braces -Wno-unused-parameter -Wno-type-limits -Wno-narrowing
endif

CXXFLAGS += $(WARNINGS) $(DEBUG) $(PROFILE) $(OTHERS) -MMD

BINDIST_EXTRAS = README.md data
BINDIST    = cataclysmdda-$(VERSION).tar.gz
W32BINDIST = cataclysmdda-$(VERSION).zip
BINDIST_CMD    = tar --transform=s@^$(BINDIST_DIR)@cataclysmdda-$(VERSION)@ -czvf $(BINDIST) $(BINDIST_DIR)
W32BINDIST_CMD = cd $(BINDIST_DIR) && zip -r ../$(W32BINDIST) * && cd $(BUILD_DIR)

# is this section even being used anymore?
# SOMEBODY PLEASE CHECK
#ifeq ($(OS), Msys)
#  LDFLAGS = -static -lpdcurses
#else
#  LDFLAGS += -lncurses
#endif

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
  WARNINGS = -Werror -Wall -Wextra -Wno-switch -Wno-sign-compare -Wno-missing-braces -Wno-unused-parameter
  ifeq ($(LOCALIZE), 1)
    LDFLAGS += -lintl
  endif
  TARGETSYSTEM=LINUX
  ifneq ($(OS), GNU/Linux)
    BINDIST_CMD = tar -s"@^$(BINDIST_DIR)@cataclysmdda-$(VERSION)@" -czvf $(BINDIST) $(BINDIST_DIR)
  endif
endif

# Win32 (mingw32?)
ifeq ($(NATIVE), win32)
  TARGETSYSTEM=WINDOWS
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
  LDFLAGS += -static -lgdi32 -lwinmm
  ifeq ($(LOCALIZE), 1)
    LDFLAGS += -lintl -liconv
  endif
  W32FLAGS += -Wl,-stack,12000000,-subsystem,windows
  RFLAGS = -J rc -O coff
endif

ifdef LUA
  ifeq ($(TARGETSYSTEM),WINDOWS)
    # Windows expects to have lua unpacked at a specific location
    LDFLAGS += -llua
  else
    # On unix-like systems, use pkg-config to find lua
    LDFLAGS += $(shell pkg-config --silence-errors --libs lua5.1)
    CXXFLAGS += $(shell pkg-config --silence-errors --cflags lua5.1)
    LDFLAGS += $(shell pkg-config --silence-errors --libs lua-5.1)
    CXXFLAGS += $(shell pkg-config --silence-errors --cflags lua-5.1)
    LDFLAGS += $(shell pkg-config --silence-errors --libs lua)
    CXXFLAGS += $(shell pkg-config --silence-errors --cflags lua)
  endif

  CXXFLAGS += -DLUA
  LUA_DEPENDENCIES = $(LUA_DIR)/catabindings.cpp
  BINDIST_EXTRAS  += $(LUA_DIR)/autoexec.lua
  BINDIST_EXTRAS  += $(LUA_DIR)/class_definitions.lua
endif

ifdef TILES
  SDL = 1
  BINDIST_EXTRAS += gfx
endif

ifdef SDL
  ifeq ($(NATIVE),osx)
    ifdef FRAMEWORK
      DEFINES += -DOSX_SDL_FW
      OSX_INC = -F/Library/Frameworks \
		-F$(HOME)/Library/Frameworks \
		-I/Library/Frameworks/SDL.framework/Headers \
		-I$(HOME)/Library/Frameworks/SDL.framework/Headers \
		-I/Library/Frameworks/SDL_image.framework/Headers \
		-I$(HOME)/Library/Frameworks/SDL_image.framework/Headers \
		-I/Library/Frameworks/SDL_ttf.framework/Headers \
		-I$(HOME)/Library/Frameworks/SDL_ttf.framework/Headers
      LDFLAGS += -F/Library/Frameworks \
		 -F$(HOME)/Library/Frameworks \
		 -framework SDL -framework SDL_image -framework SDL_ttf -framework Cocoa
      CXXFLAGS += $(OSX_INC)
    else # libsdl build
      DEFINES += -DOSX_SDL_LIBS
      # handle #include "SDL/SDL.h" and "SDL.h"
      CXXFLAGS += $(shell sdl-config --cflags) \
		  -I$(shell dirname $(shell sdl-config --cflags | sed 's/-I\(.[^ ]*\) .*/\1/'))
      LDFLAGS += $(shell sdl-config --libs) -lSDL_ttf
      ifdef TILES
	LDFLAGS += -lSDL_image
      endif
    endif
  else # not osx
    LDFLAGS += -lSDL -lSDL_ttf -lfreetype -lz
    ifdef TILES
      LDFLAGS += -lSDL_image
    endif
  endif
  ifdef TILES
    DEFINES += -DSDLTILES
  endif
  DEFINES += -DTILES
  ifeq ($(TARGETSYSTEM),WINDOWS)
    LDFLAGS += -lgdi32 -ldxguid -lwinmm -ljpeg -lpng
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
      ifeq ($(shell sh -c 'uname -o 2>/dev/null || echo not'),Darwin)
        LDFLAGS += -lncurses
      else
        ifeq ($(NATIVE), osx)
            LDFLAGS += -lncurses
        else
            LDFLAGS += -lncursesw
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

ifdef SDL
  ifeq ($(NATIVE),osx)
    OBJS += $(ODIR)/SDLMain.o
  endif
endif

ifdef LANGUAGES
  L10N = localization
  BINDIST_EXTRAS += lang/mo
endif

all: version $(TARGET) $(L10N)
	@

$(TARGET): $(ODIR) $(DDIR) $(OBJS)
	$(LD) $(W32FLAGS) -o $(TARGET) $(DEFINES) $(CXXFLAGS) \
          $(OBJS) $(LDFLAGS)

.PHONY: version
version:
	@( VERSION_STRING=$(VERSION) ; \
            [ -e ".git" ] && GITVERSION=$$( git describe --tags --always --dirty --match "[0-9]*.[0-9]*" ) && VERSION_STRING=$$GITVERSION ; \
            [ -e "$(SRC_DIR)/version.h" ] && OLDVERSION=$$(grep VERSION $(SRC_DIR)/version.h|cut -d '"' -f2) ; \
            if [ "x$$VERSION_STRING" != "x$$OLDVERSION" ]; then echo "#define VERSION \"$$VERSION_STRING\"" | tee $(SRC_DIR)/version.h ; fi \
         )

$(ODIR):
	mkdir -p $(ODIR)

$(DDIR):
	@mkdir $(DDIR)

$(ODIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) $(DEFINES) $(CXXFLAGS) -c $< -o $@

$(ODIR)/%.o: $(SRC_DIR)/%.rc
	$(RC) $(RFLAGS) $< -o $@

$(ODIR)/SDLMain.o: $(SRC_DIR)/SDLMain.m
	$(CC) -c $(OSX_INC) $< -o $@

version.cpp: version

$(LUA_DIR)/catabindings.cpp: $(LUA_DIR)/class_definitions.lua $(LUA_DIR)/generate_bindings.lua
	cd $(LUA_DIR) && lua generate_bindings.lua

$(SRC_DIR)/catalua.cpp: $(LUA_DEPENDENCIES)

localization:
	lang/compile_mo.sh $(LANGUAGES)

$(CHKJSON_BIN): src/chkjson/chkjson.cpp src/json.cpp
	$(CXX) -Isrc/chkjson -Isrc src/chkjson/chkjson.cpp src/json.cpp -o $(CHKJSON_BIN)

json-check: $(CHKJSON_BIN)
	./$(CHKJSON_BIN)

clean: clean-tests
	rm -rf $(TARGET) $(TILESTARGET) $(W32TILESTARGET) $(W32TARGET)
	rm -rf $(ODIR) $(W32ODIR) $(W32ODIRTILES)
	rm -rf $(BINDIST) $(W32BINDIST) $(BINDIST_DIR)
	rm -f $(SRC_DIR)/version.h $(LUA_DIR)/catabindings.cpp
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

$(BINDIST): distclean $(TARGET) $(L10N) $(BINDIST_EXTRAS)
	mkdir -p $(BINDIST_DIR)
	cp -R --parents $(TARGET) $(BINDIST_EXTRAS) $(BINDIST_DIR)
	$(BINDIST_CMD)

export ODIR _OBJS LDFLAGS CXX W32FLAGS DEFINES CXXFLAGS

ctags: $(SOURCES) $(HEADERS)
	ctags $(SOURCES) $(HEADERS)

etags: $(SOURCES) $(HEADERS)
	etags $(SOURCES) $(HEADERS)

tests: $(ODIR) $(DDIR) $(OBJS)
	$(MAKE) -C tests

check: tests
	$(MAKE) -C tests check

clean-tests:
	$(MAKE) -C tests clean

.PHONY: tests check ctags etags clean-tests

-include $(SOURCES:$(SRC_DIR)/%.cpp=$(DEPDIR)/%.P)
-include ${OBJS:.o=.d}
