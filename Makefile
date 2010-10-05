# comment these to toggle them as one sees fit.
WARNINGS = -Wall
DEBUG = -g

TARGET = cataclysm

CXX = g++
CFLAGS = $(WARNINGS) $(DEBUG)
LDFLAGS = $(CFLAGS) -lncurses

SOURCES = main.cpp bionics.cpp bodypart.cpp crafting.cpp event.cpp faction.cpp field.cpp game.cpp help.cpp item.cpp itypedef.cpp iuse.cpp keypress.cpp line.cpp map.cpp mapgen.cpp mapitemsdef.cpp monattack.cpp mondeath.cpp mongroupdef.cpp monitemsdef.cpp monmove.cpp monster.cpp mtypedef.cpp newcharacter.cpp npc.cpp npcmove.cpp npctalk.cpp output.cpp overmap.cpp player.cpp rng.cpp settlement.cpp setvector.cpp skill.cpp trapdef.cpp trapfunc.cpp wish.cpp 
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
