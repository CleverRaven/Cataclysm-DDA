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


# comment these to toggle them as one sees fit.
# WARNINGS will spam hundreds of warnings, mostly safe, if turned on
# DEBUG is best turned on if you plan to debug in gdb -- please do!
# PROFILE is for use with gprof or a similar program -- don't bother generally
WARNINGS = -Wall -Wextra -Wno-switch -Wno-sign-compare -Wno-missing-braces -Wno-unused-parameter
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


VERSION = 0.5


TARGET = cataclysm
TILESTARGET = cataclysm-tiles
W32TILESTARGET = cataclysm-tiles.exe
W32TARGET = cataclysm.exe
BINDIST_DIR = bindist

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

# enable optimizations. slow to build
ifdef RELEASE
  OTHERS += -O3
  DEBUG =
endif

CXXFLAGS += $(WARNINGS) $(DEBUG) $(PROFILE) $(OTHERS) -MMD

BINDIST_EXTRAS = README data cataclysm-launcher
BINDIST    = cataclysmdda-$(VERSION).tar.gz
W32BINDIST = cataclysmdda-$(VERSION).zip
BINDIST_CMD    = tar -czvf $(BINDIST) $(BINDIST_DIR)
W32BINDIST_CMD = zip -r $(W32BINDIST) $(BINDIST_DIR)

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
  TARGETSYSTEM=LINUX
else
  # Linux 32-bit
  ifeq ($(NATIVE), linux32)
    CXXFLAGS += -m32
    TARGETSYSTEM=LINUX
  endif
endif

# OSX
ifeq ($(NATIVE), osx)
  CXXFLAGS += -mmacosx-version-min=10.6
  TARGETSYSTEM=LINUX
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
  TARGET = $(W32TARGET)
  BINDIST = $(W32BINDIST)
  BINDIST_CMD = $(W32BINDIST_CMD)
  ODIR = $(W32ODIR)
  LDFLAGS += -static -lgdi32 
  W32FLAGS += -Wl,-stack,12000000,-subsystem,windows
endif

ifdef TILES
  LDFLAGS += -lSDL -lSDL_ttf -lfreetype -lz
  DEFINES += -DTILES
  ifeq ($(TARGETSYSTEM),WINDOWS)
    LDFLAGS += -lgdi32 -ldxguid -lwinmm
    TARGET = $(W32TILESTARGET)
    ODIR = $(W32ODIRTILES)
  else
    TARGET = $(TILESTARGET)
    ODIR = $(ODIRTILES)
  endif
else
  # Link to ncurses if we're using a non-tiles, Linux build
  ifeq ($(TARGETSYSTEM),LINUX)
    LDFLAGS += -lncurses
  endif
endif


SOURCES = $(wildcard *.cpp)
HEADERS = $(wildcard *.h)
_OBJS = $(SOURCES:.cpp=.o)
OBJS = $(patsubst %,$(ODIR)/%,$(_OBJS))

all: version $(TARGET)
	@

$(TARGET): $(ODIR) $(DDIR) $(OBJS)
	$(LD) $(W32FLAGS) -o $(TARGET) $(DEFINES) $(CXXFLAGS) \
          $(OBJS) $(LDFLAGS)

.PHONY: version
version:
	@( VERSION_STRING=$(VERSION) ; \
            [ -e ".git" ] && GITVERSION=$$( git describe --tags --always --dirty --match "[0-9]*.[0-9]*" ) && VERSION_STRING=$$GITVERSION ; \
            [ -e "version.h" ] && OLDVERSION=$$(grep VERSION version.h|cut -d '"' -f2) ; \
            if [ "x$$VERSION_STRING" != "x$$OLDVERSION" ]; then echo "#define VERSION \"$$VERSION_STRING\"" | tee version.h ; fi \
         )

$(ODIR):
	mkdir -p $(ODIR)

$(DDIR):
	@mkdir $(DDIR)

$(ODIR)/%.o: %.cpp
	$(CXX) $(DEFINES) $(CXXFLAGS) -c $< -o $@

version.cpp: version

clean: clean-tests
	rm -rf $(TARGET) $(W32TARGET) $(ODIR) $(W32ODIR) $(W32BINDIST) \
	$(BINDIST)
	rm -rf $(BINDIST_DIR)
	rm -f version.h

bindist: $(BINDIST)

$(BINDIST): $(TARGET) $(BINDIST_EXTRAS)
	rm -rf $(BINDIST_DIR)
	mkdir -p $(BINDIST_DIR)
	cp -R $(TARGET) $(BINDIST_EXTRAS) $(BINDIST_DIR)
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

-include $(SOURCES:%.cpp=$(DEPDIR)/%.P)
-include ${OBJS:.o=.d}
