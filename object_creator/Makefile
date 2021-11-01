PKGCONFIG = pkg-config
QT5_MODULES = Qt5Core Qt5Gui Qt5Widgets
QT5_LIBS = `$(PKGCONFIG) $(QT5_MODULES) --libs`
QT5_CFLAGS = `$(PKGCONFIG) $(QT5_MODULES) --cflags`

SOURCES = $(wildcard *.cpp) messages.cpp format.cpp
VPATH=../src:../tools/format
OBJS = $(sort $(SOURCES:%.cpp=$(ODIR)/%.o))

CATA_LIB=../$(BUILD_PREFIX)cataclysm.a

ODIR ?= obj

CXXFLAGS += $(QT5_CFLAGS) -I../src -I../tools/format -fPIC
LDFLAGS += $(QT5_LIBS)
DEFINES += -DQT_NO_KEYWORDS

object_creator: $(OBJS) $(CATA_LIB)
	+$(CXX) -o $@ $(OBJS) $(CATA_LIB) $(CXXFLAGS) $(LDFLAGS)

clean:
	rm -f object_creator
	rm -rf $(ODIR)

$(shell mkdir -p $(ODIR))

$(ODIR)/%.o: %.cpp
	$(CXX) $(CPPFLAGS) $(DEFINES) $(CXXFLAGS) -c ../object_creator/$< -o $@

.PHONY: clean object_creator

.SECONDARY: $(OBJS)

-include ${OBJS:.o=.d}
