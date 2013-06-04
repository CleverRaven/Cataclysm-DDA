cmake_minimum_required(VERSION 2.8)

project(cataclysm)

set(SOURCES_RAW
action.cpp
addiction.cpp
artifact.cpp
basecamp.cpp
bionics.cpp
bodypart.cpp
calendar.cpp
catacurse.cpp
catajson.cpp
color.cpp
computer.cpp
construction.cpp
crafting.cpp
debug.cpp
defense.cpp
disease.cpp
event.cpp
faction.cpp
field.cpp
game.cpp
gamemode.cpp
graffiti.cpp
help.cpp
helper.cpp
iexamine.cpp
input.cpp
inventory.cpp
inventory_ui.cpp
item.cpp
item_factory.cpp
item_group.cpp
itypedef.cpp
iuse.cpp
keypress.cpp
lightmap.cpp
line.cpp
main.cpp
main_menu.cpp
mapbuffer.cpp
map.cpp
mapdata.cpp
mapgen.cpp
mapgenformat.cpp
mapitemsdef.cpp
melee.cpp
mission.cpp
missiondef.cpp
mission_end.cpp
mission_fail.cpp
mission_place.cpp
mission_start.cpp
monattack.cpp
mondeath.cpp
mongroupdef.cpp
monitemsdef.cpp
monmove.cpp
monster.cpp
mtypedef.cpp
mutation.cpp
mutation_data.cpp
name.cpp
newcharacter.cpp
npc.cpp
npcmove.cpp
npctalk.cpp
options.cpp
output.cpp
overmap.cpp
player.cpp
posix_time.cpp
profession.cpp
ranged.cpp
rng.cpp
settlement.cpp
setvector.cpp
skill.cpp
texthash.cpp
tileray.cpp
trapdef.cpp
trapfunc.cpp
tutorial.cpp
vehicle.cpp
veh_interact.cpp
veh_typedef.cpp
weather.cpp
wish.cpp
)

foreach(SRC ${SOURCES_RAW})
set(SOURCES ${SOURCES} ${CMAKE_CURRENT_LIST_DIR}/${SRC})
#message(STATUS ${SRC})
endforeach(SRC)

add_executable(cataclysm ${SOURCES})

target_link_libraries(cataclysm  
  ncurses
)
