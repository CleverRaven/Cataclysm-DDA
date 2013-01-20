# Platforms:
# Linux native
#   (don't need to do anything)
# Linux 64-bit
#   make NATIVE=linux64
# Linux 32-bit
#   make NATIVE=linux32
# Linux cross-compile to Win32
#   make CROSS=i686-pc-mingw32-
# Win32
#   Run: make NATIVE=win32

# Build types: 
# Debug (no optimizations)
#  Default
# Release (turn on optimizations)
#  make RELEASE=1


# comment these to toggle them as one sees fit.
# WARNINGS will spam hundreds of warnings, mostly safe, if turned on
# DEBUG is best turned on if you plan to debug in gdb -- please do!
# PROFILE is for use with gprof or a similar program -- don't bother generally
#WARNINGS = -Wall -Wextra -Wno-switch -Wno-sign-compare -Wno-missing-braces -Wno-unused-parameter -Wno-char-subscripts
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

ODIR = obj
W32ODIR = objwin
DDIR = .deps

TARGET = cataclysm
W32TARGET = cataclysm.exe

OS  = $(shell uname -o)
CXX = $(CROSS)g++

# enable optimizations. slow to build
ifdef ($(RELEASE))
  OTHERS += -O3
  DEBUG =
endif

CXXFLAGS = $(WARNINGS) $(DEBUG) $(PROFILE) $(OTHERS)

# is this mingw check even being used anymore?
ifeq ($(OS), Msys)
  LDFLAGS = -static -lpdcurses
else 
  LDFLAGS = -lncurses
endif

# Win32 (mingw32?)
ifeq ($(NATIVE), win32)
  TARGET = $(W32TARGET)
  W32LDFLAGS = -Wl,-stack,12000000,-subsystem,windows
  LDFLAGS = -static -lgdi32 
  ODIR = $(W32ODIR)
endif
# Linux 64-bit
ifeq ($(NATIVE), linux64)
  CXXFLAGS += -m64
else
  # Linux 32-bit
  ifeq ($(NATIVE), linux32)
    CXXFLAGS += -m32
  endif
endif
# MXE cross-compile to win32
ifeq ($(CROSS), i686-pc-mingw32-)
  TARGET = $(W32TARGET)
  LDFLAGS = -lgdi32
  ODIR = $(W32ODIR)
endif

SOURCES = $(wildcard *.cpp)
_OBJS = $(SOURCES:.cpp=.o)
OBJS = $(patsubst %,$(ODIR)/%,$(_OBJS))

all: $(TARGET)
	@

$(TARGET): $(ODIR) $(DDIR) $(OBJS)
	$(CXX) $(W32FLAGS) -o $(TARGET) $(DEFINES) $(CXXFLAGS) \
          $(OBJS) $(LDFLAGS) 

$(ODIR):
	mkdir $(ODIR)

$(DDIR):
	@mkdir $(DDIR)

$(ODIR)/%.o: %.cpp
	$(CXX) $(DEFINES) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) $(W32TARGET) $(ODIR)/*.o $(W32ODIR)/*.o

-include $(SOURCES:%.cpp=$(DEPDIR)/%.P)
