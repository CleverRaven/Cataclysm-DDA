PKGCONFIG = pkg-config
QT5_MODULES = Qt5Core Qt5Gui Qt5Widgets
QT5_LIBS = `$(PKGCONFIG) $(QT5_MODULES) --libs`
QT5_CFLAGS = `$(PKGCONFIG) $(QT5_MODULES) --cflags`

SOURCES = $(wildcard *.cpp) messages.cpp format.cpp
VPATH=../src:../tools/format
OBJS = $(sort $(SOURCES:%.cpp=$(ODIR)/%.o))

CATA_LIB=../$(BUILD_PREFIX)cataclysm.a

ODIR ?= obj

# MXE cross-compile to win32
ifneq (,$(findstring mingw32,$(CROSS)))
  TARGETSYSTEM=WINDOWS
endif

DEFINES += -DQT_NO_KEYWORDS

# Settings for either WINDOWS or Linux
ifeq ($(TARGETSYSTEM),WINDOWS)
    CXXFLAGS += -I../src -I../tools/format -isystem ../src/third-party -fPIC
    INCPATH = -I. -I/opt/mxe/usr/x86_64-w64-mingw32.static/qt5/include 
    LINKER = /opt/mxe/usr/bin/x86_64-w64-mingw32.static-g++

    QT5DIR = /opt/mxe/usr/x86_64-w64-mingw32.static/qt5
    LIBS = $(QT5DIR)/plugins/platforms/libqwindows.a \
           $(QT5DIR)/lib/libQt5EventDispatcherSupport.a \
           $(QT5DIR)/lib/libQt5FontDatabaseSupport.a \
           $(QT5DIR)/lib/libQt5ThemeSupport.a \
           $(QT5DIR)/lib/libQt5WindowsUIAutomationSupport.a \
           $(QT5DIR)/lib/libQt5Widgets.a \
           $(QT5DIR)/lib/libQt5Gui.a \
           $(QT5DIR)/lib/libQt5Core.a \
           -lwtsapi32 -luserenv -lpcre2-16 -lzstd -lnetapi32 -ldwmapi
    
    DESTDIR_TARGET = object_creator.exe
    # Without this the resulting file size is about 8x larger
    LFLAGS = -Wl,-s -Wl,-subsystem,windows -mthreads

  else
    CXXFLAGS += $(QT5_CFLAGS) -I../src -I../tools/format -isystem ../src/third-party -fPIC
    LDFLAGS += $(QT5_LIBS)
endif



object_creator: $(OBJS) $(CATA_LIB)
	+$(CXX) -o $@ $(OBJS) $(CATA_LIB) $(CXXFLAGS) $(LDFLAGS)

object_creator.exe: $(OBJS) $(CATA_LIB)
	$(LINKER) $(LFLAGS) -o $(DESTDIR_TARGET) $(OBJS) $(CATA_LIB) $(LIBS) $(INCPATH) $(LDFLAGS) $(CXXFLAGS)

clean:
	rm -f object_creator object_creator.exe
	rm -rf *obj *objwin

$(shell mkdir -p $(ODIR))



# Build the windows version if the terget system is windows
# Otherwise, build the linux version
ifeq ($(TARGETSYSTEM),WINDOWS)
$(ODIR)/%.o: %.cpp
	$(LINKER) $(LFLAGS) $(LDFLAGS) $(CXXFLAGS) $(DEFINES) $(INCPATH) -c ../object_creator/$< -o $@
else
$(ODIR)/%.o: %.cpp
	$(CXX) $(CPPFLAGS) $(DEFINES) $(CXXFLAGS) -c ../object_creator/$< -o $@
endif

.PHONY: clean object_creator

.SECONDARY: $(OBJS)

-include ${OBJS:.o=.d}
