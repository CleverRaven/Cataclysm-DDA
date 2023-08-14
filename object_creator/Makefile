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

    LIBS = /opt/mxe/usr/x86_64-w64-mingw32.static/qt5/plugins/platforms/libqwindows.a -lwinspool -lwtsapi32 \
           /opt/mxe/usr/x86_64-w64-mingw32.static/qt5/lib/libQt5EventDispatcherSupport.a \
           /opt/mxe/usr/x86_64-w64-mingw32.static/qt5/lib/libQt5FontDatabaseSupport.a -lfontconfig -lexpat -ldwrite -ld2d1 \
           /opt/mxe/usr/x86_64-w64-mingw32.static/qt5/lib/libQt5ThemeSupport.a \
           /opt/mxe/usr/x86_64-w64-mingw32.static/qt5/lib/libQt5WindowsUIAutomationSupport.a \
           /opt/mxe/usr/x86_64-w64-mingw32.static/qt5/lib/libQt5Widgets.a -luxtheme -ldwmapi \
           /opt/mxe/usr/x86_64-w64-mingw32.static/qt5/lib/libQt5Gui.a -ld3d11 -ldxgi -ldxguid -lharfbuzz -lfreetype -lbz2 -lpng16 -lharfbuzz_too -lfreetype_too -lglib-2.0 -lintl -liconv -lm -lpcre -lshlwapi -lcomdlg32 -loleaut32 -limm32 \
           /opt/mxe/usr/x86_64-w64-mingw32.static/qt5/lib/libQt5Core.a -lmpr -luserenv -lversion -lz -lpcre2-16 -lzstd -lnetapi32 -lws2_32 -ladvapi32 -lkernel32 -lole32 -lshell32 -luuid -luser32 -lwinmm  -lmingw32 
    
    DESTDIR_TARGET = object_creator.exe
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



# Global settings for Windows targets
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
