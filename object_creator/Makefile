PKGCONFIG = pkg-config
QT5_MODULES = #Qt5Core Qt5Gui Qt5Widgets
QT5_LIBS = `$(PKGCONFIG) $(QT5_MODULES) --libs`
QT5_CFLAGS = `$(PKGCONFIG) $(QT5_MODULES) --cflags`

SOURCES = $(wildcard *.cpp) messages.cpp format.cpp
VPATH=../src:../tools/format
OBJS = $(sort $(SOURCES:%.cpp=$(ODIR)/%.o))

CATA_LIB=../$(BUILD_PREFIX)cataclysm.a

ODIR ?= obj

CXXFLAGS += $(QT5_CFLAGS) -I../src -I../tools/format -isystem ../src/third-party -fPIC
LDFLAGS += $(QT5_LIBS)
DEFINES += -DQT_NO_KEYWORDS
INCPATH = -I. -I/opt/mxe/usr/x86_64-w64-mingw32.static/qt5/include -I/opt/mxe/usr/x86_64-w64-mingw32.static/qt5/include/QtWidgets -I/opt/mxe/usr/x86_64-w64-mingw32.static/qt5/include/QtGui -I/opt/mxe/usr/x86_64-w64-mingw32.static/qt5/include/QtNetwork -I/opt/mxe/usr/x86_64-w64-mingw32.static/qt5/include/QtCore -Irelease -I/opt/mxe/usr/x86_64-w64-mingw32.static/qt5/mkspecs/win32-g++ -I../src -I../tools/format -isystem ../src/third-party -fPIC


# We use pkg-config to find out which libs are needed with MXE
LDFLAGS += $(shell $(PKG_CONFIG) --libs SDL2_image SDL2_ttf)
# We don't use SDL_main -- we have proper main()/WinMain()
LDFLAGS := $(filter-out -lSDL2main,$(LDFLAGS))

LDFLAGS += $(shell $(PKG_CONFIG) sdl2 --libs)
CXXFLAGS += $(shell $(PKG_CONFIG) --cflags sdl2 SDL2_image SDL2_ttf)

CXXFLAGS += $(shell $(PKG_CONFIG) --cflags SDL2_mixer)
LDFLAGS += $(shell $(PKG_CONFIG) --libs SDL2_mixer)
LDFLAGS += -lpthread


LINKER      =        /opt/mxe/usr/bin/x86_64-w64-mingw32.static-g++
LIBS        =        /opt/mxe/usr/x86_64-w64-mingw32.static/qt5/plugins/styles/libqwindowsvistastyle.a /opt/mxe/usr/x86_64-w64-mingw32.static/qt5/plugins/platforms/libqwindows.a -lwinspool -lwtsapi32 /opt/mxe/usr/x86_64-w64-mingw32.static/qt5/lib/libQt5EventDispatcherSupport.a /opt/mxe/usr/x86_64-w64-mingw32.static/qt5/lib/libQt5FontDatabaseSupport.a -lfontconfig -lexpat -ldwrite -ld2d1 /opt/mxe/usr/x86_64-w64-mingw32.static/qt5/lib/libQt5ThemeSupport.a /opt/mxe/usr/x86_64-w64-mingw32.static/qt5/lib/libQt5AccessibilitySupport.a /opt/mxe/usr/x86_64-w64-mingw32.static/qt5/lib/libQt5WindowsUIAutomationSupport.a /opt/mxe/usr/x86_64-w64-mingw32.static/qt5/plugins/imageformats/libqgif.a /opt/mxe/usr/x86_64-w64-mingw32.static/qt5/plugins/imageformats/libqicns.a /opt/mxe/usr/x86_64-w64-mingw32.static/qt5/plugins/imageformats/libqico.a /opt/mxe/usr/x86_64-w64-mingw32.static/qt5/plugins/imageformats/libqjp2.a -ljasper /opt/mxe/usr/x86_64-w64-mingw32.static/qt5/plugins/imageformats/libqjpeg.a /opt/mxe/usr/x86_64-w64-mingw32.static/qt5/plugins/imageformats/libqmng.a -lmng -llcms2 -lpthread /opt/mxe/usr/x86_64-w64-mingw32.static/qt5/plugins/imageformats/libqtga.a /opt/mxe/usr/x86_64-w64-mingw32.static/qt5/plugins/imageformats/libqtiff.a -ltiff -llzma -ljpeg /opt/mxe/usr/x86_64-w64-mingw32.static/qt5/plugins/imageformats/libqwbmp.a /opt/mxe/usr/x86_64-w64-mingw32.static/qt5/plugins/imageformats/libqwebp.a -lwebpmux -lwebpdemux -lwebp -lsharpyuv /opt/mxe/usr/x86_64-w64-mingw32.static/qt5/plugins/bearer/libqgenericbearer.a /opt/mxe/usr/x86_64-w64-mingw32.static/qt5/lib/libQt5Widgets.a -luxtheme -ldwmapi /opt/mxe/usr/x86_64-w64-mingw32.static/qt5/lib/libQt5Gui.a -ld3d11 -ldxgi -ldxguid -lharfbuzz -lfreetype -lbz2 -lpng16 -lharfbuzz_too -lfreetype_too -lglib-2.0 -lintl -liconv -lm -lpcre -lshlwapi -lcomdlg32 -loleaut32 -limm32 /opt/mxe/usr/x86_64-w64-mingw32.static/qt5/lib/libQt5Network.a -ldnsapi -liphlpapi -lssl -lcrypto -lgdi32 -lcrypt32 /opt/mxe/usr/x86_64-w64-mingw32.static/qt5/lib/libQt5Core.a -lmpr -luserenv -lversion -lz -lpcre2-16 -lzstd -lnetapi32 -lws2_32 -ladvapi32 -lkernel32 -lole32 -lshell32 -luuid -luser32 -lwinmm  -lmingw32 /opt/mxe/usr/x86_64-w64-mingw32.static/qt5/lib/libqtmain.a -lshell32 
LFLAGS        =        -Wl,-s -Wl,--gc-sections -Wl,-subsystem,windows -mthreads

QMAKE_TARGET  = object_creator
DESTDIR        = release/ #avoid trailing-slash linebreak
TARGET         = object_creator.exe
DESTDIR_TARGET = release/object_creator.exe


object_creator: $(OBJS) $(CATA_LIB)
	+$(CXX) -o $@ $(OBJS) $(CATA_LIB) $(CXXFLAGS) $(LDFLAGS)

object_creator.exe: /opt/mxe/usr/x86_64-w64-mingw32.static/qt5/lib/libQt5Widgets.a /opt/mxe/usr/x86_64-w64-mingw32.static/qt5/lib/libQt5Gui.a /opt/mxe/usr/x86_64-w64-mingw32.static/qt5/lib/libQt5Network.a /opt/mxe/usr/x86_64-w64-mingw32.static/qt5/lib/libQt5Core.a /opt/mxe/usr/x86_64-w64-mingw32.static/qt5/lib/libqtmain.a $(OBJS) $(CATA_LIB)
	$(LINKER) $(LFLAGS) -o $(DESTDIR_TARGET) $(OBJS) $(CATA_LIB) $(LIBS) $(INCPATH) $(LDFLAGS) $(CXXFLAGS)

clean:
	rm -f object_creator
	rm -rf $(ODIR)

$(shell mkdir -p $(ODIR))

$(ODIR)/%.o: %.cpp
	$(LINKER) $(LFLAGS) $(LDFLAGS) $(CXXFLAGS) $(DEFINES) -c ../object_creator/$< -o $@

# $(ODIR)/%.o: %.cpp
# 	$(CXX) $(CPPFLAGS) $(DEFINES) $(CXXFLAGS) -c ../object_creator/$< -o $@

.PHONY: clean object_creator

.SECONDARY: $(OBJS)

-include ${OBJS:.o=.d}
