# comment these to toggle them as one sees fit.
#WARNINGS = -Wall
DEBUG = -g

TARGET = cataclysm

CXX = g++
CFLAGS = $(WARNINGS) $(DEBUG)
LDFLAGS = $(CFLAGS) -lncurses

SOURCES = $(wildcard *.cpp)
OBJS = $(SOURCES:.cpp=.o)

all: $(TARGET)
	@

$(TARGET): $(OBJS)
	$(CXX) $(LDFLAGS) $(OBJS) -o $(TARGET)

%.o: %.cpp
	$(CXX) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) $(OBJS)

Make.deps:
	gccmakedep -fMake.deps -- $(CFLAGS) -- $(SOURCES)

include Make.deps
