# Lab JSON Quick Guide

Labs are heavily randomized but have very few actual overmap terrain tiles, meaning that variety in the maps gets almost entirely using JSON.  In particular, labs use place_nested extensively to randomly select submaps, such as new lab rooms.


## Quick Guide for new lab areas:

In lab_floorplans.json add a new mapgen json with om_terrain of lab_4side.

Keep the middle 2 spaces of each border clear, because that's where doors open out to.  Some or all of the border spaces will be turned into 1-width walls between lab sections, so make sure the map 'works' whether or not the outermost spaces are created or not (by keeping them clear, or having 'disposable' furniture like counters, lockers, broken consoles, etc).  Allow rotation.

To add a lab finale instead, use the om_terrain of lab_finale_1level and put your JSON in lab_floorplans_finale1level.json instead.


## Quick Guide for new lab dead-ends:

In lab_floorplans_1side.json add a new mapgen json with om_terrain of lab_1side.

Make the area facing north, keeping the 2 middle spaces of the northern border clear because that's where doors open out to.  If you want to have a locked area, set the floor to 't_strconc_floor' and stairs won't appear there (this floor is ',' in the lab_palette).  Use place_nested to place 'lab_1side_border_doors' at coords [0,0].  Do not allow rotation.


## Quick Guide for new lab rooms:

In lab_rooms.json add a new mapgen json with a nested_mapgen_id of lab_room_7x7 or lab_room_9x9 with a mapgensize of [7,7] or [9,9].

In some rare cases the first row or column of terrain may be replaced with a border wall, so prefer layouts that look intact even if that occurs.

If your layout needs to know where the doors of the room will open up into, instead use a nested_mapgen_id of lab_room_7x7_crossdoors and lab_room_9x9_crossdoors which will ensure that doors only appear in the exact middle of a wall.


# Lab JSON Full Guide

## How area mapgen json works

A lab is mostly made out of overmap tiles of type 'lab', 'lab_stairs', and typically one 'lab_finale'.

Lab terrain needs special treatment: stairs, border walls, dead-ends, and more, so JSON instead uses three 'fake' overmap tile types.  Create a JSON mapgen object with om_terrain set to one of these, and the code will convert it and place it correctly on the map.

* lab_1side - placed when exactly one lab is adjacent, maps should assume a north-facing entrance.  Do not allow rotation.
* lab_finale_1level - placed at the bottom of a lab on terrain types.  Rotation optional.
* lab_4side - all the other maps.  Rotation optional.


## Border walls

Labs have unusual borders: 1-width walls between lab areas, laid out on the south and east side of those tiles, potentially with metal doors in the middle of them, like so:

```
        "                       |",
        "                       |",
        "                       |",
        "                       |",
        "                       |",
        "                       |",
        "                       |",
        "                       |",
        "                       |",
        "                       |",
        "                       |",
        "                       M",
        "                       M",
        "                       |",
        "                       |",
        "                       |",
        "                       |",
        "                       |",
        "                       |",
        "                       |",
        "                       |",
        "                       |",
        "                       |",
        "-----------MM----------|"
```

If a north or west neighbor isn't a lab, the entire side gets needs to be overwritten with a wall, replacing whatever was normally there.  If the east or south neighbor isn't a lab, that door needs to be overwritten with a wall.

Some JSON maps can start from the above layout, disable rotation, and call place_nested with the 'lab_border_walls' chunk in JSON.  In this case, borders will be perfect and no additional code will fire.

But if borders have not been managed (determined by checking for the presence of an east-facing door/wall), then the code will create the lab border walls and doors on all four directions.

'lab_border_walls' does not work on rotated maps, so rotated maps need to rely on hardcoded border wall generation.  This is preferable because it creates more variety.  So if a map layout is amenable to not knowing if the final 1x1 border around it will be placed or not, it is preferable to allow rotation and not place 'lab_border_walls'.

One middle-ground: If just part of the map needs to cares about where the border wall is to look correct, put a wall on the east-side of the map and only allow rotation of [0,1].  That will ensure the wall gets placed on the east or south side of the map in the final rotation.  See the "electricity room" floorplan for an example of this.


## Other hardcoded map generation

Labs will have a small chance of randomly getting lights, the central & tower labs will always get them.  Stairs will be placed on any empty thconc_floor space if the overmap indicated stairs.  There's also a 10% chance of special effects like flooding, portals, radiation accidents, etc.  Ant-infested labs will get bashed in.

None of these require json changes to enact, but JSON-ideas for lab special effects in rooms can be added to the spawn tables in lab_maybe_effects_7x7 and lab_maybe_effects_9x9.  Currently this just adds spider-infestations.


## Room generation

The most common source of randomness *within* a map is to create a 7x7 or 9x9 room and give it random contents by using place_nested.  Don't place rooms directly, instead we use an intermediate map chunk called a 'spawn' which encodes more information and randomizes between all the kinds of rooms that would satisify those requirements.

* lab_spawn_7x7 - a 7x7 room with no guarantees on where the doors are.
* lab_spawn_7x7_crossdoors - a 7x7 room with doors only in the middle of each border wall.
* lab_spawn_9x9 - a 9x9 room with no guarantees on where the doors are.
* lab_spawn_9x9_crossdoors - a 9x9 room with doors only in the middle of each borders wall.

Selecting a spawn will randomize between: lab_room_[size] plus the _rare variants, and the _crossdoors variants if applicable.

If your room is also amenable to having two of its walls redefined, you can also use these spawns to add in more randomized rooms that might modify the walls by adding windows, replacing walls with chainlink, or creating multiple doors and interior walls.  These templates assume that there are no doors on the two walls which are not part of this submap.  Note these submaps are sized 8x8 and 10x10 because they include the walls to be modified.

* lab_spawn_7x7_wall_nw
* lab_spawn_7x7_wall_sw
* lab_spawn_9x9_wall_nw
* lab_spawn_9x9_wall_sw

Use the most specific spawn possible.


## Directory

* lab_central.json - hardcoded maps for the top of central lab.
* lab_common.json - terrain palette, loot palettes, common json objects.
* lab_escape.json - maps specifically for lab challenge escape, these are special placed on lvl 4 of a lab.
* lab_floorplan_cross.json - cross floorplans are unusual for having rock infill when bordering the edge of the lab, with rare vaults.
* lab_floorplans.json - the main source of lab layouts.
* lab_floorplans_1side.json - dead-end floor plans.
* lab_floorplans_finale1level.json - finale floorplans.
* lab_rooms.json - randomized rooms.
* lab_rooms_wall.json - randomized rooms which rewrite the wall borders.
* lab_trains.json - tiles for the lab science train which rarely happens on levels 2 & 4.