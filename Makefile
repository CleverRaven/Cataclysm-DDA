# comment these to toggle them as one sees fit.
#WARNINGS = -Wall
DEBUG = -g
ODIR = obj


TARGET = cataclysm

CXX = g++
CFLAGS = $(WARNINGS) $(DEBUG)
LDFLAGS = $(CFLAGS) -lncurses

SOURCES = $(wildcard *.cpp)
_OBJS = $(SOURCES:.cpp=.o)
OBJS = $(patsubst %,$(ODIR)/%,$(_OBJS))

all: $(TARGET)
	@

$(TARGET): $(OBJS)
	$(CXX) $(LDFLAGS) $(OBJS) -o $(TARGET)

$(ODIR)/%.o: %.cpp
	$(CXX) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) $(ODIR)/*.o

Make.deps:
	gccmakedep -fMake.deps -- $(CFLAGS) -- $(SOURCES)

include Make.deps
