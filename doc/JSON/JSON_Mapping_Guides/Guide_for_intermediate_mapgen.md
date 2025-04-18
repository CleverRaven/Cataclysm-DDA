## Guide For Intermediate Mapgen

This guide assumes you are comfortable with basic mapgen elements and adding regular mapgen.  It is meant as a supplement to the mapgen.md and overmap.md documents.

**This guide will cover:**

* When to use nested mapgen vs. regular mapgen variants.
* How to make and spawn a nested map.
* Some nested map possibilities.
* NPC spawning.
* Leveraging existing nested maps for modders.
* Basic update_mapgen using traps.
* Merged maps for large buildings.


**Files you’ll use:**
* a new or existing file to hold your nested map entries (there is a sub folder in mapgen for nested map files).
* the mapgen file you want your nests to spawn in
* palette (optional)
* the files used in the beginner's tutorial (a mapgen file, overmap_terrain, and specials.json or multitile_city_buildings.json/regional_map_settings.json.)

**Nested maps vs. variant maps:**

Variant maps are mapgen files that completely replace another map variant by using the same `om_terrain name` and applying a `weight` entry to each variant for spawning in relation to each other.

Nested maps are a new layer of map applied to an existing mapgen file.  They can overwrite existing terrain, furniture, add spawns for loot, monsters, vehicles, etc.  They can contain anything that a regular mapgen contains within the object entry.  Nested maps are spawned by adding an entry into the main mapgen’s object entry.  They can be any size from a single tile to the entire 24x24 overmap terrain.

Both approaches offer advantages and disadvantages to adding variety to maps.

Variants are good for large structural changes or complete overhauls.

* For example, I may do a portion of a farm with a barn and another with a set of farm plots.
* If I want clearly different variants for an entire map, like the same house as empty, abandoned, and furnished.
* Before we added json roofs (and other linked z levels) to our mapgen, most buildings utilized variants for spawning, most have now been renamed but some variants still exist, especially in older specials.

Nested maps are good for adding more targeted variety and randomness to maps.  In addition to what the variant offers, nested maps can let you do things like:

* rearrange the furniture in a room, or in every room on a case by case basis.
* add smaller targeted thematic content like set pieces, additional monsters, hidden rooms, rare spawns.
* Have different sets of loot and room themes that randomly spawn across multiple buildings.


**Update_mapgen:**

Update mapgen is triggered during game play instead of being initialized during worldgen.  I’ll cover some of the update_mapgen uses in this document but it deserves its own guide.

 * Traps can trigger mapgen changes.
 * Allows missions to trigger mapgen changes.
 * Used by the faction camp building system for blueprints along with nested maps.
 * Used by map_extras.

update mapgen maps are similar to nested maps but are applied to an existing map, not to the mapgen file.  Like nested maps, they can overwrite existing terrain, furniture, add spawns for loot, monsters, vehicles, etc.  They can contain anything that a regular mapgen contains within the object entry.  They can be any size from a single tile to the entire 24x24 overmap terrain, and can even included nested mapgen objects.

**Merged Maps**

Merged maps are when you combine the mapgen entries for several OMTs into a single mapgen entry.  The `rows` are combined for a group the maps.  This is usually used for improved readability for the json and a more compact file size.  They are generally handled the same as a single OMT mapgen, with a few exceptions included in this document.  Like any mapgen option, there are tradeoffs, a notable limitation is the single fill_ter entry for multiple OMTs.

**Creating Nested Maps:**

You’ll want to make some choices before adding nested maps.

* Their purpose, where are they spawning, multiple locations?
* If it is within a larger building, will you include doors/walls?
* What size map do you need to make.

A nested map gives you the ability to overwrite or fill in a portion of an existing mapgen.  The contents of the nested map entry can contain any entry within mapgen objects (excepting fill_ter).  This includes adding nested maps inside your nested map for extra variability.

Example json entry for the nested map:

```
  {
    "type": "mapgen",
    "method": "json",
    "nested_mapgen_id": "room_9x9_recroom_E",
    "//": "an entertainment area for various recreations",
    "object": {
      "mapgensize": [ 9, 9 ],
      "rotation": [ 0, 3 ],
      "rows": [
        "|||||||||",
        "|HHH...=|",
        "|Hl....x|",
        "|%.....=|",
        "|.A.A..&|",
        "|JJ5JJ  +",
        "|       |",
        "|mVJ14 T|",
        "|||||||||"
      ],
      "palettes": [ "standard_domestic_palette" ],
      "terrain": {
        ".": "t_carpet_green",
        "H": "t_carpet_green",
        "l": "t_carpet_green",
        "A": "t_carpet_green",
        "=": "t_carpet_green",
        "x": "t_carpet_green",
        "%": "t_carpet_green",
        "&": "t_carpet_green"
      },
      "flags": [ "ERASE_ALL_BEFORE_PLACING_TERRAIN" ],
      "furniture": { "%": "f_arcade_machine", "=": "f_speaker_cabinet", "&": "f_pinball_machine" },
      "place_loot": [ { "item": "stereo", "x": 7, "y": 2, "chance": 100 }, { "item": "laptop", "x": 5, "y": 5, "chance": 60 } ]
    }
  }
  ```

This should feel pretty familiar since it looks like the `object` entry in mapgen and shares the same `type`.
Note the ID is now `nested_mapgen_id` and the object uses a new entry `mapgensize`

* `nested_mapgen_id`:  Your ID should provide useful information about the nest.  Multiple nests can share the same ID.

* `weight`:  This value is new and most nested maps don't have it yet.  It allows you to weight the spawns of nests that share the same `nested_mapgen_id` (aka variants of nests).

* `mapgensize`:  Nested mapgen can be any size from 1x1 to 24x24 but it must be square.  You don't have to use every row or column of the `rows` entry.  Any unused portions will fall back to the main mapgen.

* `flags`:  More information about mapgen flags can be found in [doc/JSON/MAPGEN.md](https://github.com/CleverRaven/Cataclysm-DDA/blob/master/doc/JSON/MAPGEN.md#clearing-flags-for-layered-mapgens)

* `terrain` & `furniture`: If you don't want to overwrite existing terrain and furniture from the main mapgen you can use a combination of `t_null` and `f_null`. ` ` is `t_null` and `f_null` by default. For example in the above example, there's a green carpet in 1/2 the map and the rest picks up the floor of the mapgen (indoor concrete). `f_clear` can also be used to remove existing furniture without replacing it.

_Tips:_
If you're doing interior spaces, pay attention to door placement and access pathways.

* For walled nests, I generally keep the doors in the center of the walls.
* For open floor plan nests, I try to preserve the corner spaces for doors in the regular mapgen (if I can design the mapgen with the nests).

example: note the corner tiles are all empty.
```
      "rows": [
        " CR ",
        "O   ",
        " EE ",
        " EE "
      ],
```

example room outline with possible door placements.  The `+` denotes any valid door placement for the above nest.  This approach gives you the maximum number of possibilities for fitting this nest into enclosed rooms without blocking the doorway.  I toss some indoor plants or other small furniture into the unused corners in the main mapgen as well, to fill out the rooms.

```
      "rows": [
        "|+||+|",
        "+ CR +",
        "|O   |",
        "| EE |",
        "+ EE +",
        "|+||+|"
      ],
```

Fitting nests into existing foundations/building outlines can be a bit of a puzzle, especially as you add variations that share the same ID or are used over many maps.  So having your doors in predictable places in all your nests will aid in their re-use for future maps.

We use a naming convention that includes the nested map size and some indication of its orientation or we add a comment with extra context in the json entry.

Some ID's:
* room_9x9_recroom_N
* room_9x9_recroom_S
* 7x7_band_practice_open


by using room and a compass direction, it quickly identifies the nest as having walls/door and the door's orientation.  The band practice one is an open space, no walls included.  Including the map size will make searching, debug spawning and re-using nests easier as well.


**Debug testing:**

So many nests, so hard to find ones to use!

We've recently gotten the ability to spawn nested maps via debug in game and this is a huge help for making sure nests fit, don't conflict with elements in the existing maps and are oriented well without adjusting spawn weights to force the nest to spawn naturally.

This is also a good way to "shop around" for existing nests to re-use in your maps.

To debug spawn a nested map in game:
* Open debug menu (you will need to add a key bind)
* Choose `map` [m]
* Choose `spawn nested map` [n]
* Search list by using [/]
* Use your selector to place the nested map.  The indicator is 0,0 coordinate of the nest.

You'll quickly see why it's good to use a coherent name format.

**Mini nests:**

Nests can be as small as one tile which is very useful if you want special or rare spawns for items, monsters, vehicles, NPCs or other elements.

An example of a spawn for a particular loot group:
These nests were used in a larger nest of a basement study.  I didn't want the study to offer all the magiclysm class books at once.  I made nests for each spell class item_group:

3 1x1 nested maps that only include loot placement.  They feed into a larger nest.

```
  {
    "type": "mapgen",
    "method": "json",
    "nested_mapgen_id": "animist_loot_spawn",
    "object": { "mapgensize": [ 1, 1 ], "place_loot": [ { "group": "animist_items", "x": 0, "y": 0, "chance": 70 } ] }
  },
  {
    "type": "mapgen",
    "method": "json",
    "nested_mapgen_id": "magus_loot_spawn",
    "object": { "mapgensize": [ 1, 1 ], "place_loot": [ { "group": "magus_items", "x": 0, "y": 0, "chance": 70 } ] }
  },
  {
    "type": "mapgen",
    "method": "json",
    "nested_mapgen_id": "stormshaper_loot_spawn",
    "object": { "mapgensize": [ 1, 1 ], "place_loot": [ { "group": "stormshaper_items", "x": 0, "y": 0, "chance": 70 } ] }
  }
  ```

**NPC spawning:**

If you use `place_npc` on the main mapgen, the NPC will spawn 100% of the time.  NPC's like the refugee center ones get placed like this.

However, many NPCs should be closer to random encounters.  The below example creates a nest that spawns an NPC (and only the NPC):

```
  {
    "type": "mapgen",
    "method": "json",
    "nested_mapgen_id": "SEER_Brigitte_LaCroix_spawn",
    "object": { "mapgensize": [ 1, 1 ], "place_npcs": [ { "class": "SEER_Brigitte_LaCroix", "x": 0, "y": 0 } ] }
  }
```

You can also customize the surroundings in which the NPC spawns.  For the Chef NPC that spawns in one restaurant, they get their survivor themed setting included:
Note the use of `t_null` in the majority of the map.  A lot of the map is unused and relies on the main mapgen.  It rearranges furniture to blockade the entrance and adds a little survivor's den flavor.
```
  {
    "type": "mapgen",
    "method": "json",
    "nested_mapgen_id": "chef_s_restaurant",
    "object": {
      "mapgensize": [ 13, 13 ],
      "rows": [
        "mt         #t",
        "            r",
        "          ffr",
        "           W ",
        "             ",
        "             ",
        "             ",
        "             ",
        "             ",
        "             ",
        "             ",
        "             ",
        "             "
      ],
      "terrain": { "-": "t_wall", "W": "t_window_boarded" },
      "furniture": { "r": "f_rack", "f": "f_fridge", "t": "f_table", "#": "f_counter", "m": "f_makeshift_bed" },
      "place_npcs": [ { "class": "survivor_chef", "x": 5, "y": 1 } ],
      "place_loot": [
        { "group": "produce", "x": [ 10, 11 ], "y": 2, "chance": 70, "repeat": [ 2, 3 ] },
        { "group": "groce_meat", "x": [ 10, 11 ], "y": 2, "chance": 70, "repeat": [ 2, 3 ] },
        { "group": "groce_dairyegg", "x": [ 10, 11 ], "y": 2, "chance": 70, "repeat": [ 2, 3 ] },
        { "group": "bar_food", "x": [ 10, 11 ], "y": 2, "chance": 70, "repeat": [ 2, 3 ] },
        { "group": "bar_fridge", "x": [ 10, 11 ], "y": 2, "chance": 70, "repeat": [ 2, 3 ] },
        { "group": "jackets", "x": 0, "y": 0, "chance": 70, "repeat": [ 2, 3 ] },
        { "group": "alcohol_bottled_canned", "x": 1, "y": 0, "chance": 80 },
        { "group": "baked_goods", "x": [ 2, 4 ], "y": 2, "chance": 50, "repeat": [ 2, 3 ] },
        { "group": "groce_bread", "x": [ 2, 4 ], "y": 2, "chance": 50, "repeat": [ 2, 3 ] },
        { "group": "cannedfood", "x": [ 5, 6 ], "y": 0, "chance": 50, "repeat": [ 2, 3 ] },
        { "group": "cannedfood", "x": [ 9, 11 ], "y": 0, "chance": 50, "repeat": [ 2, 3 ] }
      ]
    }
  }
```

  **Spawning the nested maps**

  You can spawn nests in the usual two methods, using explicit symbol placement or x,y coordinates.  I've encountered rare instances where one style works and the other doesn't but haven't pinned down the cause yet.


  **x,y coordinate placement:**

This adds some nice variability if you want the nest's spawn location to shift a bit around the map.  I used this extensively on roofs since they are open spaces and I wanted to decrease how static they would feel otherwise.


  In the main mapgen's `object` you enter the following entry:
```
  "place_nested": [
        {
          "chunks": [ [ "null", 30 ], [ "roof_6x6_utility", 30 ], [ "roof_4x4_utility_1", 40 ] ],
          "x": [ 5, 14 ],
          "y": [ 14, 16 ]
        }
      ]
```

A NPC example:

```
"place_nested": [ { "chunks": [ [ "SEER_Brigitte_LaCroix_spawn", 20 ], [ "null", 80 ] ], "x": 18, "y": 6 } ]
```

The `chunks` are the `nested_mapgen_id` of the nests and each includes their spawn weight in relation to each other.  Adding a `null` entry allows for the nest to have a chance to not spawn anything.  `Null` also ensures your NPC related nest spawns as a chance instead of being guaranteed.

**Explicit symbol placement:**

In the main mapgen's `object` you add the following entry:
Wherever on the main mapgen's `rows` I place a `1` the first set of chunks will spawn.  The other set will spawn wherever I place a `2`.  The symbol should be placed wherever you want the 0,0 coordinate of your nested map placed.

```
"nested": {
        "1": {
          "chunks": [ [ "null", 60 ], [ "room_10x10_woodworker_E", 30 ], [ "room_10x10_guns_E", 10 ], [ "room_10x10_guns_N", 10 ] ]
        },
        "2": {
          "chunks": [
            [ "null", 50 ],
            [ "room_6x6_brewer_N", 10 ],
            [ "room_6x6_junk", 30 ],
            [ "room_6x6_office_N", 30 ],
            [ "6x6_sewing_open", 20 ],
            [ "6x6_electronics_open", 10 ],
            [ "room_6x6_brewer_W", 10 ],
            [ "room_6x6_junk_W", 30 ],
            [ "room_6x6_office_W", 30 ]
          ]
        }
      }
```

The main mapgen `rows` with symbols:
```
      "rows": [
        "                        ",
        "                        ",
        "                        ",
        "                        ",
        "        ||||||||||||||  ",
        "        |.g...JJJWZ.U|  ",
        "        |F..........<|  ",
        "        |............|  ",
        "        |............|  ",
        "        |......2.....|  ",
        "  |||||||............|  ",
        "  |1.................|  ",
        "  |..................|  ",
        "  |..................|  ",
        "  |..................|  ",
        "  |..........|||||||||  ",
        "  |..........|          ",
        "  |..........|          ",
        "  |..........|          ",
        "  |..........|          ",
        "  |..........|          ",
        "  ||||||||||||          ",
        "                        ",
        "                        "
      ],
```

**Nested maps and z levels:**

Currently, nested maps do not support z level linking, so any nested map you make will rely on the main mapgen's roof or attempt to generate the c++ roofs.  This works with varying degrees of success.  Mostly I find it annoying when I can't put a glass roof on greenhouses.

**Leveraging existing nested maps for modders.**

As the nested maps used in vanilla increase, modders can make use of these existing entries to incorporate their mod maps into existing buildings.  This should greatly expand the mod's ability to add its content into vanilla maps.  By using the same `nested mapgen id` and assigning a `weight` to both your new nest and existing nests (as needed).

I recommend the modder take a look through existing maps and see if there is one that fits the same overall size, orientation, and spawning rarity that they would like their modded nest to have.
You can search for the nested mapgen ids in the github to make sure its representation meets your needs.


**Update_mapgen:**

As mentioned before, Update_mapgen is applied to existing maps due to events that occur during game play.

Update mapgen will be covered more in advanced guides that address its uses in faction bases and NPC missions.

For this guide, the most likely use will be for trap triggered update_mapgen.  This is a very new feature and currently is used in the microlab and the magiclysm mod's magic basement.

Since it is still a new feature, it hasn't been expanded upon much.  Currently the following limitation applies:
*  Traps can only be triggered by a Player or NPC moving on to the trap tile.

We are going to use the simpler implementation found in the Magiclysm basement over the micro_lab but I'd recommend looking at the micro lab's multi-trap system as well.

You will need a trap entry (or use an existing one)

Trap example:
```
  {
    "type": "trap",
    "id": "tr_magic_door",
    "name": "magic door",
    "color": "brown",
    "symbol": "+",
    "visibility": 99,
    "avoidance": 99,
    "difficulty": 99,
    "action": "map_regen",
    "map_regen": "magic_door_appear",
    "benign": false
  }
  ```

  All the trap specific entries can be learned about in other documentation.  What concerns us here are the `action` and `map_regen` entries.  We want this trap to trigger a `map_regen` action and the `map_regen` references our `update_mapgen_id`.

  update_magen similarities to nested maps:

  * it can target a smaller map chunk.
  * it uses the `object` data.

  differences from nested maps:

  * Coordinates used refer to the main mapgen it is updating.

  update_mapgen sample:

  ```
    {
    "type": "mapgen",
    "update_mapgen_id": "magic_door_appear",
    "method": "json",
    "object": {
      "place_terrain": [ { "ter": "t_carpet_green", "x": 12, "y": 6 } ],
      "place_furniture": [ { "furn": "f_beaded_door", "x": 12, "y": 6 } ]
    }
  }
  ```
In this example, instead of making rows and assigning symbols, I used a condensed alternative with `place_terrain` and `place_furniture`.  The x,y coordinates refer to the main mapgen this is altering.  This could have used the typical rows coding we normally use in mapgen as well.

The main mapgen and spawning your trap:

```
  {
    "type": "mapgen",
    "method": "json",
    "om_terrain": [ "magic_basement" ],
    "weight": 100,
    "object": {
      "fill_ter": "t_carpet_green",
      "rows": [
        "                        ",
        "        ||||||||||||||| ",
        "        |gUU|yRRRRRRET| ",
        "   ||||||~~~%........E| ",
        "  ||????|||||........L| ",
        " ||.......E!|...E....|| ",
        " |&.........|.yrrr...<| ",
        " ||...$...Py||||||/|||| ",
        "  ||TIII|||||RRR......| ",
        "   ||||||88S|.....H..x| ",
        "        |~~~/.....H..x| ",
        "        |B~~|.....s..x| ",
        "        |BYt|RRRR.....| ",
        "        ||||||||||||||| ",
        "                        ",
        "                        ",
        "                        ",
        "                        ",
        "                        ",
        "                        ",
        "                        ",
        "                        ",
        "                        ",
        "                        "
      ],
      "palettes": [ "standard_domestic_palette" ],
      "place_traps": [ { "trap": "tr_magic_door", "x": 21, "y": 4 } ]
    }
  }
  ```

  I've placed the trap under a coat rack to dissuade casual triggering.  Note: when triggered, the trap will update the wall across the room with a beaded door entrance.


**Merged maps**

A merged map is json that has grouped several OMTs together within one mapgen entry.  There is no size limit but you should do your best to keep the json readable, so break it up into manageable segments.  3 OMTs together left to right is 72 tiles, and fits easily inside CDDA's preference for no more than 100 columns per line, but some people do as many as 5 OMTs together.  More than 5 OMTs together left to right can be hard to read on smaller screens and should be avoided.  The same logic applies for vertical grouping: 2-3 OMTs fit easily on most screens, but more than that can be hard to read.

You can insert existing OMT's into your merged map including forest, field, swamp and water tiles.  Generic tiles like forests don't need to be added in your mapgen file, they will be called in the specials.json or multitile_city_buildings entry.

For the most part, merged maps use the exact same rules and entries as regular mapgen files with a few notable exceptions:

om_terrain value:
  This example is for a map that is 4 OMTs wide and 3 OMTs long (4x3) on the overmap.  Each row of OMT's are grouped into an array.

```
"om_terrain": [
      [ "farm_stills_4", "farm_stills_3", "farm_stills_2", "farm_stills_1" ],
      [ "farm_stills_8", "farm_stills_7", "farm_stills_6", "farm_stills_5" ],
      [ "farm_stills_12", "farm_stills_11", "farm_stills_10", "farm_stills_9" ]
    ]
```

Each OMT's coordinates will continue the row and column numbers, they do not reset to 0,0 at each map boundary.

For our farm the coordinates will be:
* x sets:
  * first  OMT: 0, 23
  * second OMT: 24, 47
  * third  OMT: 48, 71
  * fourth OMT: 72, 95

* y sets:
  * first  row: 0, 23
  * second row: 24, 47
  * third  row: 48, 71


The object entries rows reflect all the OMTs together and all the other object entries are shared across all the OMTs:

<!-- {% raw %} -->
```
      "rows": [
        " ɅɅɅɅɅɅ ,,,  ,,, ɅɅɅɅɅɅ                p ,, ,,   ɅɅɅɅɅɅɅɅɅɅɅɅɅɅɅɅɅɅɅɅɅɅɅɅɅɅɅɅɅɅɅɅɅɅɅɅɅɅɅɅɅɅɅɅɅɅ ",
        " Ʌ      ,,,  ,,,      ? [ ????  ???? [   ,, ,, [ ?……………………………………………………………………………………………………………………Ʌ ",
        " Ʌ [    ,,,  ,,,      ?   #oo#  #oo#     ,, ,,   ?…                                          …Ʌ ",
        " Ʌ      ,,,  ,,,      ? ###..####..###   ,, ,,   ?… $  $  $  $  $  $  $  $  $  $  $  $  $  $ …Ʌ ",
        " Ʌ      ,,,  ,,,      ? #y...TxxT...y#   ,, ,,   ?… $  $  $  $  $  $  $  $  $  $  $  $  $  $ …Ʌ ",
        " Ʌ      ,,,  ,,,      ? #............o   ,, ,,   ?… $  $  $  $  $  $  $  $  $  $  $  $  $  $ …Ʌ ",
        " Ʌ      ,,,……,,, [    ? oH..E.##;;;..#,  ,, ,,   ?… $  $  $  $  $  $  $  $  $  $  $  $  $  $ …Ʌ ",
        " Ʌ   ………,,,,,,,,      ? oH.l..k#;;;..*,,,,, ,,…………… $  $  $  $  $  $  $  $  $  $  $  $  $  $ …Ʌ ",
        " Ʌ  ……,,,,,,,,,,      ? oH..E.##;;;.L#,  ,, ,,   ?… $  $  $  $  $  $  $  $  $  $  $  $  $  $ …Ʌ ",
        " Ʌ  …,,,,,,,,,,,    [ ? #............o           ?… $  $  $  $  $  $  $  $  $  $  $  $  $  $ …Ʌ ",
        " Ʌ  …,,,,,,,,,,,      ? #yRR......RRy#   #oo# ^  ?… $  $  $  $  $  $  $  $  $  $  $  $  $  $ …Ʌ ",
        " Ʌ ……,,,,,,,,,,,  X   ? #|||......|||#####HH#### ?… $  $  $  $  $  $  $  $  $  $  $  $  $  $ …Ʌ ",
        " Ʌ …,,,,,,,,,,,,      ? #i..........yxxxT|...rr# ?… $  $  $  $  $  $  $  $  $  $  $  $  $  $ …Ʌ ",
        " Ʌ …,,,,,,,,,,,,    j ? oi...............|...Ero ?… $  $  $  $  $  $  $  $  $  $  $  $  $  $ …Ʌ ",
        " Ʌ …,,,,,,,,,,,,    j ? #y...............|PP..T# ?… $  $  $  $  $  $  $  $  $  $  $  $  $  $ …Ʌ ",
        " Ʌ …,,,,,,,,,,,,    j ? #||+|||..;;hffh;;||||+|# ?… $  $  $  $  $  $  $  $  $  $  $  $  $  $ …Ʌ ",
        " Ʌ …,,,,,,,,,,,,      ? #J````1..;;hffh;;.....<# ?… $  $  $  $  $  $  $  $  $  $  $  $  $  $ …Ʌ ",
        " Ʌ …,,,,,,,,,,,,     [? #5````3A.;;hffh;;.E||+|# ?… $  $  $  $  $  $  $  $  $  $  $  $  $  $ …Ʌ ",
        " Ʌ …,,,,,,,,,,,,      ? oO````JA.;;hffh;;.y|t``# ?… $  $  $  $  $  $  $  $  $  $  $  $  $  $ …Ʌ ",
        " Ʌ …,,,,,,,,,,,,  [   ? #J````2A.;;hffh;;.E|```# ?… $  $  $  $  $  $  $  $  $  $  $  $  $  $ …Ʌ ",
        " Ʌ …,,,,,,,,,,,,      ? #66``47...........T|8S8# ?… $  $  $  $  $  $  $  $  $  $  $  $  $  $ …Ʌ ",
        " Ʌ …,,,,,,,,,,,,…     ? ###*####oo####oo#####o## ?… $  $  $  $  $  $  $  $  $  $  $  $  $  $ …Ʌ ",
        " Ʌ …,,,,,,,,,,,,……    ?    ,#♣#                  ?… $  $  $  $  $  $  $  $  $  $  $  $  $  $ …Ʌ ",
        " Ʌ ……,,,,,,,,,,,,……   ?????,,,????????????????????… $  $  $  $  $  $  $  $  $  $  $  $  $  $ …Ʌ ",
        " Ʌ  …,,,,,,,,,,,,,……………………………………………………………………………………… $  $  $  $  $  $  $  $  $  $  $  $  $  $ …Ʌ ",
        " Ʌ  ……,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,… $  $  $  $  $  $  $  $  $  $  $  $  $  $ …Ʌ ",
        " Ʌ   ………,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,… $  $  $  $  $  $  $  $  $  $  $  $  $  $ …Ʌ ",
        " Ʌ     …………………,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,… $  $  $  $  $  $  $  $  $  $  $  $  $  $ …Ʌ ",
        " Ʌ       …………………………………………………………………………………,,,,,}…………… $  $  $  $  $  $  $  $  $  $  $  $  $  $ …Ʌ ",
        " Ʌ???????………………???????? #==#==#   ,   ##{{{{{##  ?… $  $  $  $  $  $  $  $  $  $  $  $  $  $ …Ʌ ",
        " Ʌ……………………………………………………? #‡_±__#   ,   #(_____}#  ?… $  $  $  $  $  $  $  $  $  $  $  $  $  $ …Ʌ ",
        " Ʌ…                  …? #‡_±_0#   ,   #(______#  ?… $  $  $  $  $  $  $  $  $  $  $  $  $  $ …Ʌ ",
        " Ʌ… $$$$$$$$$$$$$$$$ …? #‡___0#   ,  &#(______]  ?… $  $  $  $  $  $  $  $  $  $  $  $  $  $ …Ʌ ",
        " Ʌ…                  …? #‡_Ŧ__#   ,,,,*_______#  ?… $  $  $  $  $  $  $  $  $  $  $  $  $  $ …Ʌ ",
        " Ʌ…                  …? #######   ,   #-______#  ?… $  $  $  $  $  $  $  $  $  $  $  $  $  $ …Ʌ ",
        " Ʌ… $$$$$$$$$$$$$$$$ …?           ,   #-______]  ?… $  $  $  $  $  $  $  $  $  $  $  $  $  $ …Ʌ ",
        " Ʌ…                  …?           ,   #-______#  ?… $  $  $  $  $  $  $  $  $  $  $  $  $  $ …Ʌ ",
        " Ʌ…                  …?           ,   #_______#  ?… $  $  $  $  $  $  $  $  $  $  $  $  $  $ …Ʌ ",
        " Ʌ… $$$$$$$$$$$$$$$$ …?           ,   #########  ?… $  $  $  $  $  $  $  $  $  $  $  $  $  $ …Ʌ ",
        " Ʌ…                  …?           ,              ?… $  $  $  $  $  $  $  $  $  $  $  $  $  $ …Ʌ ",
        " Ʌ…                  …????????????,???????????????… $  $  $  $  $  $  $  $  $  $  $  $  $  $ …Ʌ ",
        " Ʌ… $$$$$$$$$$$$$$$$ …?           ,              ?… $  $  $  $  $  $  $  $  $  $  $  $  $  $ …Ʌ ",
        " Ʌ…                  …?  [   [   [,   [   [   [  ?… $  $  $  $  $  $  $  $  $  $  $  $  $  $ …Ʌ ",
        " Ʌ…                  …?           ,              ?… $  $  $  $  $  $  $  $  $  $  $  $  $  $ …Ʌ ",
        " Ʌ… $$$$$$$$$$$$$$$$ …?  ,,,,,,,,,,,,,,,,,,,,,,  ?… $  $  $  $  $  $  $  $  $  $  $  $  $  $ …Ʌ ",
        " Ʌ…                  …?           ,              ?… $  $  $  $  $  $  $  $  $  $  $  $  $  $ …Ʌ ",
        " Ʌ…                  …?  [   [   [,   [   [   [  ?… $  $  $  $  $  $  $  $  $  $  $  $  $  $ …Ʌ ",
        " Ʌ… $$$$$$$$$$$$$$$$ …?           ,              ?… $  $  $  $  $  $  $  $  $  $  $  $  $  $ …Ʌ ",
        " Ʌ…                  …?  ,,,,,,,,,,,,,,,,,,,,,,  ?… $  $  $  $  $  $  $  $  $  $  $  $  $  $ …Ʌ ",
        " Ʌ…                  …?           ,              ?… $  $  $  $  $  $  $  $  $  $  $  $  $  $ …Ʌ ",
        " Ʌ… $$$$$$$$$$$$$$$$ …?  [   [   [,   [   [   [  ?… $  $  $  $  $  $  $  $  $  $  $  $  $  $ …Ʌ ",
        " Ʌ…                  …?           ,              ?… $  $  $  $  $  $  $  $  $  $  $  $  $  $ …Ʌ ",
        " Ʌ…                  …?  ,,,,,,,,,,,,,,,,,,,,,,  ?… $  $  $  $  $  $  $  $  $  $  $  $  $  $ …Ʌ ",
        " Ʌ… $$$$$$$$$$$$$$$$ …?           ,              ?… $  $  $  $  $  $  $  $  $  $  $  $  $  $ …Ʌ ",
        " Ʌ…                  …?  [   [   [,   [   [   [  ?… $  $  $  $  $  $  $  $  $  $  $  $  $  $ …Ʌ ",
        " Ʌ…                  …?           ,              ?… $  $  $  $  $  $  $  $  $  $  $  $  $  $ …Ʌ ",
        " Ʌ… $$$$$$$$$$$$$$$$ …?  ,,,,,,,,,,,,,,,,,,,,,,  ?… $  $  $  $  $  $  $  $  $  $  $  $  $  $ …Ʌ ",
        " Ʌ…                  …?           ,              ?… $  $  $  $  $  $  $  $  $  $  $  $  $  $ …Ʌ ",
        " Ʌ…                  …?  [   [   [,   [   [   [  ?… $  $  $  $  $  $  $  $  $  $  $  $  $  $ …Ʌ ",
        " Ʌ… $$$$$$$$$$$$$$$$ …?           ,              ?… $  $  $  $  $  $  $  $  $  $  $  $  $  $ …Ʌ ",
        " Ʌ…                  …?  ,,,,,,,,,,,,,,,,,,,,,,  ?… $  $  $  $  $  $  $  $  $  $  $  $  $  $ …Ʌ ",
        " Ʌ…                  …?           ,              ?… $  $  $  $  $  $  $  $  $  $  $  $  $  $ …Ʌ ",
        " Ʌ… $$$$$$$$$$$$$$$$ …?  [   [   [,   [   [   [  ?… $  $  $  $  $  $  $  $  $  $  $  $  $  $ …Ʌ ",
        " Ʌ…                  …?           ,              ?… $  $  $  $  $  $  $  $  $  $  $  $  $  $ …Ʌ ",
        " Ʌ…                  …?  ,,,,,,,,,,,,,,,,,,,,,,  ?… $  $  $  $  $  $  $  $  $  $  $  $  $  $ …Ʌ ",
        " Ʌ… $$$$$$$$$$$$$$$$ …?           ,              ?… $  $  $  $  $  $  $  $  $  $  $  $  $  $ …Ʌ ",
        " Ʌ…                  …?  [   [   [,   [   [   [  ?… $  $  $  $  $  $  $  $  $  $  $  $  $  $ …Ʌ ",
        " Ʌ…                  …?           ,              ?… $  $  $  $  $  $  $  $  $  $  $  $  $  $ …Ʌ ",
        " Ʌ… $$$$$$$$$$$$$$$$ …?  ,,,,,,,,,,,,,,,,,,,,,,  ?… $  $  $  $  $  $  $  $  $  $  $  $  $  $ …Ʌ ",
        " Ʌ…                  …?                          ?…                                          …Ʌ ",
        " Ʌ……………………………………………………?  [   [   [    [   [   [  ?……………………………………………………………………………………………………………………Ʌ ",
        " ɅɅɅɅɅɅɅɅɅɅɅɅɅɅɅɅɅɅɅɅɅɅɅɅɅɅɅɅɅɅɅɅɅɅɅɅɅɅɅɅɅɅɅɅɅɅɅɅɅɅɅɅɅɅɅɅɅɅɅɅɅɅɅɅɅɅɅɅɅɅɅɅɅɅɅɅɅɅɅɅɅɅɅɅɅɅɅɅɅɅɅɅɅɅ "
      ],
```
<!-- {% endraw %} -->

Important note about spawning items, vehicles and monsters:

While these maps are merged for the benefit of readability, this isn't automatically recognized by our mapgen processes.  When you add spawns using x,y coordinates you can't cross OMT boundaries.  Make multiple entries if you want those spawns to occur across more than one OMT.  This applies for both x and y coordinates.

An example:  each x coordinate encompasses one OMT from a segment of the mall.
```
"place_monster": [
        { "group": "GROUP_MALL", "x": [ 2, 23 ], "y": [ 2, 23 ], "repeat": [ 2, 4 ] },
        { "group": "GROUP_MALL", "x": [ 26, 47 ], "y": [ 2, 23 ], "repeat": [ 4, 8 ] },
        { "group": "GROUP_MALL", "x": [ 49, 71 ], "y": [ 2, 10 ], "repeat": [ 4, 6 ] },
        { "group": "GROUP_MALL", "x": [ 56, 68 ], "y": [ 17, 21 ], "repeat": [ 2, 4 ] },
        { "group": "GROUP_MALL", "x": [ 73, 95 ], "y": [ 2, 10 ], "repeat": [ 4, 6 ] },
        { "group": "GROUP_MALL", "x": [ 73, 95 ], "y": [ 17, 19 ], "repeat": [ 2, 4 ] },
        { "group": "GROUP_MALL", "x": [ 98, 119 ], "y": [ 0, 11 ], "repeat": [ 4, 6 ] },
        { "group": "GROUP_MALL", "x": [ 96, 105 ], "y": [ 16, 21 ], "repeat": [ 2, 4 ] },
        { "group": "GROUP_MALL", "x": [ 170, 191 ], "y": [ 2, 23 ], "repeat": [ 2, 4 ] },
        { "group": "GROUP_MALL", "x": [ 194, 215 ], "y": [ 2, 23 ], "repeat": [ 1, 3 ] }
      ]
```

* You can mostly get around this by using explicit symbol placement instead, which will apply to all the OMT's within your `rows`.  This can get a little messy for monster and vehicle spawns, so I usually keep those to x,y coordinates.

* Vehicles that spawn across 2 OMT's won't spawn at all.  So if you can't get your vehicle to spawn, adjust its placement.  Vehicle's 0,0 coordinate can vary depending on the vehicle's own json entry so this usually will take some trial and error to get them spawning nicely.

* Nested maps of all sorts can be used in a merged map, but they can't cross boundary lines (the nested map will be cut off at the boundary).

* More information about monster spawning can be found in [doc/JSON/MAPGEN.md](https://github.com/CleverRaven/Cataclysm-DDA/blob/master/doc/JSON/MAPGEN.md#spawn-item-or-monster-groups-with-place_groups)

Note: set point type entries (see the example below) don't work well with merged maps and the issue has been reported.  If you use this entry, the points will be repeated in every OMT in your merged map.

sample:
```
"set": [
        { "point": "trap", "id": "tr_funnel", "x": [ 2, 9 ], "y": 19, "repeat": [ 1, 2 ] },
        { "point": "trap", "id": "tr_cot", "x": [ 2, 9 ], "y": [ 15, 17 ], "repeat": [ 1, 2 ] }
      ]
```
I wanted to place this cot and funnel in the first OMT but it is repeated every time it encounters a new OMT boundary.
