CC = g++
DEBUG = -g
CFLAGS = -Wall -c -lncurses $(DEBUG)
LFLAGS = -Wall -lncurses $(DEBUG)
CXX = g++
CXXFLAGS = -lncurses
OBJS = main.o bionics.o bodypart.o crafting.o event.o faction.o field.o game.o help.o item.o itypedef.o iuse.o keypress.o line.o map.o mapgen.o mapitemsdef.o monattack.o mondeath.o mongroupdef.o monitemsdef.o monmove.o monster.o mtypedef.o newcharacter.o npc.o npcmove.o npctalk.o output.o overmap.o player.o rng.o settlement.o setvector.o skill.o trapdef.o trapfunc.o wish.o 
cataclysm: $(OBJS)
	$(CC) $(LFLAGS) $(OBJS) -o cataclysm
clean:
	rm -f cataclysm main.o bionics.o bodypart.o crafting.o event.o faction.o field.o game.o help.o item.o itypedef.o iuse.o keypress.o line.o map.o mapgen.o mapitemsdef.o monattack.o mondeath.o mongroupdef.o monitemsdef.o monmove.o monster.o mtypedef.o newcharacter.o npc.o npcmove.o npctalk.o output.o overmap.o player.o rng.o settlement.o setvector.o skill.o trapdef.o trapfunc.o wish.o 
