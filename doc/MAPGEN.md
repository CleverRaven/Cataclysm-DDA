* 0 Intro
  * 0.0 How buildings and terrain are generated
* 1 Adding mapgen entries.
  * 1.0 Methods
  * 1.1 Placement
    * 1.1.0 Embedded
    * 1.1.1 Standalone
  * 1.2 Format and variables
    * 1.2.0 "method"
    * 1.2.1 "om_terrain"
    * 1.2.2 "weight"
  * 1.3 How "overmap_terrain" variables affect mapgen
  * 1.4 Limitations / TODO
* 2 Method: json
  * 2.0 "fill_ter":
  * 2.1 "rows":
    * 2.1.0 "terrain":
    * 2.1.1 "furniture":
  * 2.2 "set": [ ...
    * 2.2.0 "point" { ...
      * 2.2.0.0 "x" & "y": 123 | [ 12, 34 ]
      * 2.2.0.1 "id": "..."
      * 2.2.0.2 "chance": 123
      * 2.2.0.3 "repeat": [ 1, 23 ]
    * 2.2.1 "line" {}
      * 2.2.1.0 "x" & "y"
      * 2.2.1.1 "x2" & "y2"
      * 2.2.1.2 "id"
      * 2.2.1.3 "chance"
      * 2.2.1.4 "repeat"
    * 2.2.2 "square" {}
  * 2.3 "place_groups": []
    * 2.3.0 "monster":
      * 2.3.0.0 "x" & "y"
      * 2.3.0.1 "chance"
      * 2.3.0.2 "repeat"
  * 2.4 "place_monster": []
  * 2.5 "place_item": []
    * 2.5.0 "item":
      * 2.5.0.0 "x" / "y"
      * 2.5.0.1 "amount"
      * 2.5.0.2 "chance"
      * 2.5.0.3 "repeat"
  * 2.6 specials:
    * 2.6.0 "fields"
    * 2.6.1 "npcs"
    * 2.6.2 "signs"
    * 2.6.3 "vendingmachines"
    * 2.6.4 "toilets"
    * 2.6.5 "gaspumps"
    * 2.6.6 "items"
    * 2.6.7 "monsters"
    * 2.6.8 "vehicles"
    * 2.6.9 "item"
    * 2.6.10 "traps"
    * 2.6.11 "furniture"
    * 2.6.12 "terrain"
    * 2.6.13 "monster"
    * 2.6.14 "rubble"
    * 2.6.15 "place_liquids"
    * 2.6.16 "loot"
    * 2.6.17 "sealed_item"
    * 2.6.18 "graffiti"
    * 2.6.19 "translate_ter"
    * 2.6.20 "zones"
    * 2.6.21 "ter_furn_transforms"
  * 2.7 "rotation"
  * 2.8 "predecessor_mapgen"
* 3 update_mapgen
  * 3.1 overmap tile specification
    * 3.1.0 "assign_mission_target"
    * 3.1.1 "om_terrain"
  * 3.2 mission specials
    * 3.2.0 "target"

## 0.0 How buildings and terrain are generated
Cataclysm creates buildings and terrain on discovery via 'mapgen'; functions specific to an overmap terrain (the tiles you see in [m]ap are also determined by overmap terrain). Overmap terrains ("oter") are defined in overmap_terrain.json.

By default, an oter has a single built-in mapgen function which matches the '"id"' in it's json entry (examples: "house", "bank", etc). Multiple functions also possible. When a player moves into range of an area marked on the map as a house, the game chooses semi-randomly from a list of functions for "house", picks one, and runs it, laying down walls and adding items, monsters, rubber chickens and whatnot. This is all done in a fraction of a second (something to keep in mind for later).

All mapgen functions build in a 24x24 tile area - even for large buildings; obtuse but surprisingly effective methods are used to assemble giant 3x3 hotels, etc..

In order to make a world that's random and (somewhat) sensical, there are numerous rules and exceptions to them, which are clarified below.

# 1 Adding mapgen entries.
One doesn't need to create a new overmap_terrain for a new variation of a building. For a custom gas station, defining a mapgen entry and adding it to the "s_gas" mapgen list will add it to the random variations of gas station in the world.

If you use an existing overmap_terrain and it has a roof or other z-level linked to its file, the other levels will be generated with the ground floor. To avoid this, or add your own multiple z-levels, create an overmap_terrain with a similar name (s_gas_1).  

## 1.0 Methods
While adding mapgen as a c++ function is one of the fastest (and the most versatile) ways to generate procedural terrain on the fly, this requires recompiling the game.

Most of the existing c++ buildings have been moved to json and currently json mapping is the preferred method of adding both content and mods.

* JSON: A set of json arrays and objects for defining stuff and things. Pros: Fastest to apply, mostly complete. Cons: Not a programming language; no if statements or variables means instances of a particular json mapgen definition will all be similar. Third party map editors are currently out of date.

* JSON support includes the use of nested mapgen, smaller mapgen chunks which override a portion of the linked mapgen.  This allows for greater variety in furniture, terrain and spawns within a single mapgen file.  You can also link mapgen files for multiple z-level buildings and multi-tile buildings.  

## 1.1 Placement
Mapgen definitions can be added in 2 places:
### 1.1.0 Embedded
As "mapgen": { ... } objects inside an existing overmap_terrain object ( see "s_restaurant_fast" in overmap_terrain.json for full example ):
```C++
(snip)
    },{
        "type" : "overmap_terrain",
        "id" : "s_restaurant_fast",
(snip)
        "mapgen": [
            { "weight": 250,
                "method": "json", "object": {
                     (see below)
            }
        ]
(snip)

```
### 1.1.1 Standalone
As standalone { "type": "mapgen", ... } objects in a .json inside data/json. Below is the same fast food restaurant.
```C++
[
    {
        "type": "mapgen",
        "om_terrain": "s_restaurant_fast",
        "weight": 250,
        "method": "json",
        "object": {
            (see below)
        }
    }
]
```
Note how "om_terrain" matches the overmap "id". om_terrain is **required** for standalone mapgen entries.

## 1.2 Format and variables
The above example only illustrate the mapgen entries, not the actual format for building stuff. However, the following variables impact where and how often stuff gets applied:
### 1.2.0 "method":
**required**
> Values:
> * "json" - requires

```
"object": { (more json here) }
```

### 1.2.1 "om_terrain":
**required for standalone**
> Values:
> * "matching_overmap_terrain_id" - see overmap_terrain.json for a list

-or-

> * [ "list_of", "oter_ids" ]
This form creates duplicate overmap terrains by applying the same json mapgen to each of the listed overmap terrain ids.

##### Example: "om_terrain": [ "house", "house_base" ]

-or-

> * [ [ "oter_id", "oter_id", ... ], [ "oter_id", "oter_id", ... ], ... ]

This form allows for multiple overmap terrains to be defined using a single json object, with the "rows" property expanding in blocks of 24x24 characters to accommodate as many overmap terrains as are listed here. The terrain ids are specified using a nested array of strings which represent the rows and columns of overmap terrain ids (found in overmap_terrain.json) that are associated with the "rows" property described in section 2.1 of this document.

Characters mapped using the "terrain", "furniture", or any of the special mappings ("items", "monsters", etc) will be applied universally to all of the listed overmap terrains.

Placing things using x/y coordinates ("place_monsters", "place_loot", "place_item", etc) works using the full extended coordinates beyond 24x24. An important limitation is that ranged random coordinates (such as "x": [ 10, 18 ]) must not cross the 24x24 terrain boundaries. Ranges such as [ 0, 23 ] and [ 50, 70 ] are valid, but [ 0, 47 ] and [ 15, 35 ] are not because they extend beyond a single 24x24 block.

##### Example:

```
"om_terrain": [
  [ "apartments_mod_tower_NW", "apartments_mod_tower_NE" ],
  [ "apartments_mod_tower_SW", "apartments_mod_tower_SE" ]
]
```

In this example, the "rows" property should be 48x48, with each quadrant of 24x24 being associated with each of the four apartments_mod_tower overmap terrain ids specified.

### 1.2.2 "weight":
(optional) When the game randomly picks mapgen functions, each function's weight value determines how rare it is. 1000 is the default, so adding something with weight '500' will make it appear 1/3 times, unless more functions are added. (An insanely high value like 10000000 is useful for testing)
> Values:
> * number - *0 disables*

Default: 1000

## 1.3 How "overmap_terrain" variables affect mapgen
"id" is used to determine the required "om_terrain" id for standalone, -except- when the following variables are set in "overmap_terrain":
* "extras" - applies rare, random scenes after mapgen; helicopter crashes, etc
* "mondensity" - determines the default 'density' value for *"place_groups": [ { "monster": ...* (json), if this is not set then place_monsters will not work without its own explicitly set density argument.

## 1.4 Limitations / TODO
* JSON: adding specific monster spawns are still WIP.
* The old mapgen.cpp system involved *The Biggest "if / else if / else if / .." Statement Known to Man*(tm), and is only halfway converted to the "builtin" mapgen class. This means that while custom mapgen functions are allowed, the game will cheerfully forget the default if one is added.
* TODO: Add to this list.

# 2 Method: json
The json method is defined by a json object ( go figure ) with the following structure entries.
Note, either "fill_ter" or "rows" + "terrain" are required.

# 2.0 "fill_ter": "terrain_id"
*required if "rows" is unset*
> Value: ("string"): Valid terrain id from data/json/terrain.json

Example: "fill_ter": "t_grass"

# 2.1 "rows":
*required if "fill_ter" is unset*
> Value: ([array]): blocks of 24 rows of blocks of 24 character lines. Each character is defined by "terrain" and optionally "furniture" below

Other parts can be linked with this map, for example one can place things like a gaspump (with gasoline) or a toilet (with water) or items from an item group or fields at the square given by a character.

Example:

```
"rows":[
  ",_____ssssssssssss_____,",
  ",__,__#### ss ####__,__,",
  ",_,,,_#ssssssssss#__,__,",
  ",__,__#hthsssshth#__,__,",
  ",__,__#ssssssssss#_,,,_,",
  ",__,__|-555++555-|__,__,",
  ",_____|.hh....hh.%_____,",
  ",_____%.tt....tt.%_____,",
  ",_____%.tt....tt.%_____,",
  ",_____%.hh....hh.|_____,",
  ",_____|..........%_____,",
  ",,,,,,|..........+_____,",
  ",_____|ccccxcc|..%_____,",
  ",_____w.......+..|_____,",
  ",_____|e.ccl.c|+-|_____,",
  ",_____|O.....S|.S|_____,",
  ",_____||erle.x|T||_____,",
  ",_____#|----w-|-|#_____,",
  ",________,_____________,",
  ",________,_____________,",
  ",________,_____________,",
  " ,_______,____________, ",
  "  ,,,,,,,,,,,,,,,,,,,,  ",
  "   dd                   "
],

```

# 2.1.0 "terrain"
**required by "rows"** Defines terrain ids for "rows", each key is a single character with a terrain id string
> Value: {object}: { "a", "t_identifier", ... }

Example: "terrain":{ " ": "t_grass", "d": "t_floor", "5": "t_wall_glass_h", "%": "t_wall_glass_v", "O": "t_floor", ",": "t_pavement_y", "_": "t_pavement", "r": "t_floor", "6": "t_console", "x": "t_console_broken", "$": "t_shrub", "^": "t_floor", ".": "t_floor", "-": "t_wall_h", "|": "t_wall_v", "#": "t_shrub", "t": "t_floor", "+": "t_door_glass_c", "=": "t_door_locked_alarm", "D": "t_door_locked", "w": "t_window_domestic", "T": "t_floor", "S": "t_floor", "e": "t_floor", "h": "t_floor", "c": "t_floor", "l": "t_floor", "s": "t_sidewalk" },

# 2.1.1 "furniture"
**optional** Defines furniture ids for "rows" ( each character in rows is a terrain -or- terrain/furniture combo ). "f_null" means no furniture but the entry can be left out

Example: "furniture":{ "d": "f_dumpster", "5": "f_null", "%": "f_null", "O": "f_oven", "r": "f_rack", "^": "f_indoor_plant", "t": "f_table", "T": "f_toilet", "S": "f_sink", "e": "f_fridge", "h": "f_chair", "c": "f_counter", "l": "f_locker", },

## 2.2 "set"
**optional** Specific commands to set terrain, furniture, traps, radiation, etc. Array is processed in order.
> Value:  [array of {objects}]: [ { "point": .. }, { "line": .. }, { "square": .. }, ... ]

### 2.2.0 "point"
**required** Set things by point. Requires "x", "y", "id" (or "amount" for "radiation")
> Value: "terrain", "furniture", "trap", "radiation"

Example: { "point": "furniture", "id": "f_chair", "x:" 5, "y": 10 }

Example: { "point": "trap", "id": "tr_beartrap", "x:" [ 0, 23 ], "y": [ 5, 18 ], "chance": 10, "repeat": [ 2, 5 ] }

Example: { "point": "radiation", "id": "f_chair", "x:" 12, "y": 12, "amount": 20 }

#### 2.2.0.0 "x" / "y"
**both required** x/y coordinates. If it's an array, the result is a random number in that range
> Value: 0-23

-or-

> Value: [ 0-23, 0-23 ] - random point between [ a, b ]

Example: "x": 12, "y": [ 5, 15 ]

#### 2.2.0.1 "id"
**required except by "radiation"** See terrain.json, furniture.json, and trap.json for "id" strings
> Value: "ter_id", "furn_id", or "trap_id"

Example: "id": "tr_beartrap"

#### 2.2.0.2 "amount"
**required for "radiation"** Radiation amount
> Value: 0-100

#### 2.2.0.3 "chance"
**optional** one-in-??? chance to apply
> Value: *number*

#### 2.2.0.4 "repeat"
**optional** repeat this randomly between ??? and ??? times. Only makes sense if the coordinates are random
> Value: [ *number*, *number* ]

Example: [ 1, 3 ] - apply 1-3 times

### 2.2.1 "line"
**required** Set things in a line. Requires "x", "y", "x2", "y2", "id" (or "amount" for "radiation")
> Value: "terrain", "furniture", "trap", "radiation"

Example: { "point": "terrain", "id": "t_lava", "x:" 5, "y": 5, "x2": 20, "y2": 20 }

#### 2.2.1.0 "x" / "y"
**both required** start x/y coordinates. If it's an array, the result is a random number in that range
> Value: 0-23

-or-

> Value: [ 0-23, 0-23 ] - random point between [ a, b ]

Example: "x": 12, "y": [ 5, 15 ]

#### 2.2.1.1 "x2" / "y2"
**both required** end x/y coordinates. If it's an array, the result is a random number in that range
> Value: 0-23

-or-

> Value: [ 0-23, 0-23 ] - random point between [ a, b ]

Example: "x2": 22, "y2": [ 15, 20 ]

#### 2.2.1.2 "id"
**required except by "radiation"** See terrain.json, furniture.json, and trap.json for "id" strings
> Value: "ter_id", "furn_id", or "trap_id"

Example: "id": "f_counter"

#### 2.2.1.3 "amount"
**required for "radiation"** Radiation amount
> Value: 0-100

#### 2.2.1.4 "chance"
**optional** one-in-??? chance to apply
> Value: *number*

#### 2.2.1.5 "repeat"
**optional** repeat this randomly between ??? and ??? times. Only makes sense if the coordinates are random
> Value: [ *number*, *number* ]

Example: [ 1, 3 ] - apply 1-3 times

### 2.2.2 "square"
**required** Define a square of things. Requires "x", "y", "x2", "y2", "id" (or "amount" for "radiation")
> Value: "terrain", "furniture", "trap", "radiation"

Example: { "square": "radiation", "amount": 10, "x:" [ 0, 5 ], "y": [ 0, 5 ], "x2": [ 18, 23 ], "y2": [ 18, 23 ] }
The arguments are exactly the same as "line", but "x", "y" and "x2", "y2" define opposite corners

## 2.3 "place_groups"
**optional** Spawn items or monsters from item_groups.json and monster_groups.json
> Value: [ array of {objects} ]: [ { "monster": ... }, { "item": ... }, ... ]

### 2.3.0 "monster"
**required** The monster group id, which picks random critters from a list
> Value: "MONSTER_GROUP"

Example: { "monster": "GROUP_ZOMBIE", "x": [ 13, 15 ], "y": 15, "chance": 10 }

#### 2.3.0.0 "x" / "y"
**required** Spawn coordinates ( specific or area rectangle )
> Value: 0-23

-or-

> Value: [ 0-23, 0-23 ] - random point between [ a, b ]
When using a range, the minimum and maximum values will be used in creating rectangle coordinates to be used by map::place_spawns.
Each monster generated from the monster group will be placed in a different random location within the rectangle.

Example: "x": 12, "y": [ 5, 15 ]
These values will produce a rectangle for map::place_spawns from ( 12, 5 ) to ( 12, 15 ) inclusive.

#### 2.3.0.1 "density"
**optional** This is a multipier to the following "chance". If the result is bigger than 100% it gurantees one spawn point for every 100% and the rest is evaluated by chance (one added or not). Then the monsters are spawned according to their spawn-point cost "cost_multiplier" defined in the monster groups.
Additionally all overmap densities within a square of raduis 3 (7x7 around player) [exact value in mapgen.cpp/MON_RADIUS makro] are added to this.
The "pack_size" modifier in monstergroups is a random multiplier to the rolled spawn point amount.
> Value: *floating point number*

#### 2.3.0.2 "chance"
**optional** ???-in-100 chance to apply
> Value: *number*

### 2.3.1 "item"
**required** The item group id, which picks random stuff from a list
> Value: "ITEM_GROUP"

Example: { "item": "livingroom", "x": [ 13, 15 ], "y": 15, "chance": 50 }

#### 2.3.1.0 "x" / "y"
**required** Spawn coordinates ( specific or area rectangle )
> Value: 0-23

-or-

> Value: [ 0-23, 0-23 ] - a range between [ a, b ] inclusive
When using a range, the minimum and maximum values will be used in creating rectangle coordinates to be used by map::place_items.
Each item from the item group will be placed in a different random location within the rectangle.

Example: "x": 12, "y": [ 5, 15 ]
These values will produce a rectangle for map::place_items from ( 12, 5 ) to ( 12, 15 ) inclusive.

#### 2.3.1.1 "chance"
**required** unlike everything else, this is a percentage. Maybe
> Value: *number*

## 2.4 "place_monster"
**optional** Spawn single monster. Either specific monster or a random monster from a monster group. Is affected by spawn density game setting.
> Value: [ array of {objects} ]: [ { "monster": ... } ]

| Identifier | Description
|---         |---
| monster | ID of the monster to spawn.
| group | ID of the monster group from which the spawned monster is selected. `monster` and `group` should not be used together. `group` will act over `monster`.
| x, y  | Spawn coordinates ( specific or area rectangle ). Value: 0-23 or [ 0-23, 0-23 ] - random point between [ a, b ]. When using a range, the minimum and maximum values will be used in creating rectangle coordinates to be used by map::place_spawns. Each monster generated from the monster group will be placed in a different random location within the rectangle. Example: "x": 12, "y": [ 5, 15 ] These values will produce a rectangle for map::place_spawns from ( 12, 5 ) to ( 12, 15 ) inclusive.
| chance | Percentage chance to do spawning. If repeat is used each repeat has separate chance.
| repeat | The spawning is repeated this many times. Can be a number or a range.
| pack_size | How many monsters are spawned. Can be single number or range like [1-4]. Is affected by the chance and spawn density. Ignored when spawning from a group.
| one_or_none | Do not allow more than one to spawn due to high spawn density. If repeat is not defined or pack size is defined this defaults to true true, otherwise this defaults to false. Ignored when spawning from a group.
| friendly | Set true to make the monster friendly. Default false.
| name | Extra name to display on the monster.
|target | Set to true to make this into mission target. Only works when the monster is spawned from a mission.

Note that high spawn density game setting can cause extra monsters to spawn when `monster` is used. When `group` is used only one monster will spawn.

Example: `"place_monster": [ { "group": "GROUP_REFUGEE_BOSS_ZOMBIE", "name": "Sean McLaughlin", "x": 10, "y": 10, "target": true } ]`  
This places a single random monster from group "GROUP_REFUGEE_BOSS_ZOMBIE", sets the name to "Sean McLaughlin", spawns the monster at coordinate (10, 10) and also sets the monster as the target of this mission.

Example: `"place_monster": [ { "monster": "mon_secubot", "x": [ 7, 18 ], "y": [ 7, 18 ], "chance": 30, "repeat": [1, 3] } ]`  
This places "mon_secubot" at random coordinate (7-18, 7-18). The monster is placed with 30% probablity. The placement is repeated by random number of times [1-3].

## 2.5 "place_item"
**optional** A list of *specific* things to add. WIP: Monsters and vehicles will be here too
> Value: [ array of {objects} ]: [ { "item", ... }, ... ]

### 2.5.0 "item"
**required** A valid itype ID. see everything in data/json/items
> Value: "string"

Example: { "item": "weed", "x": 14, "y": 15, "amount": [ 10, 20 ], "repeat": [1, 3], "chance": 20 }

#### 2.5.0.0 "x" / "y"
**required** Spawn coordinates ( specific or random )
> Value: 0-23

-or-

> Value: [ 0-23, 0-23 ] - random point between [ a, b ]

Example: "x": 12, "y": [ 5, 15 ]

#### 2.5.0.1 "amount"
**required** Spawn this amount [ or, range ]
> Value: *number*

-or-

> Value: [ *number*, *number* ] - random point between [ a, b ]

Example: "amount": [ 5, 15 ]

#### 2.5.0.2 "chance"
**optional** one-in-??? chance to apply
> Value: *number*

#### 2.5.0.3 "repeat"
**optional** repeat this randomly between ??? and ??? times. Only makes sense if the coordinates are random
> Value: [ *number*, *number* ]

Example: [ 1, 3 ] - apply 1-3 times

# 2.5 specials
**optional** Special map features that do more than just placing furniture / terrain.

Specials can be defined either via a mapping like the terrain / furniture mapping using the "rows" entry above or through their exact location by its coordinates.

The mapping is defined with a json object like this:
```
"<type-of-special>" : {
    "A" : { <data-of-special> },
    "B" : { <data-of-special> },
    "C" : { <data-of-special> },
    ...
}
```
"\<type-of-special\>" is one of the types listed below. \<data-of-special\> is a json object with content specific to the special type. Some types require no data at all or all their data is optional, an empty object is enough for those specials. You can define as many mapping as you want.

Each mapping can be an array, for things that can appear several times on the tile (e.g. items, fields) each entry of the array is applied in order. For traps, furniture and terrain, one entry is randomly chosen (all entries have the same chances) and applied.
Example (places grass at 2/3 of all '.' square and dirt at 1/3 of them):
```
"terrain" : {
    ".": [ "t_grass", "t_grass", "t_dirt" ]
}
```
It is also possible to specify the number of instances (and consequently their chance) directly, which is particularly useful for rare occurrences (rather than repeating the common value many times):
```
"terrain" : {
    ".": [ [ "t_grass", 2 ], "t_dirt" ]
}
```


Example (places a blood and a bile field on each '.' square):
```
"fields" : {
    ".": [ { "field": "fd_blood" }, { "field": "fd_bile" } ]
}
```

Or define the mappings for one character at once:
```
"mapping" : {
    ".": {
        "traps": "tr_beartrap",
        "field": { "field": "fd_blood" },
        "item": { "item": "corpse" },
        "terrain": { "t_dirt" }
    }
}
```
This might be more useful if you want to put many different type of things on one place.

Defining specials through their specific location:
```
"place_<type-of-special>" : {
    { "x": <x>, "y": <y>, <data-of-special> },
    ...
}
```
\<x\> and \<y\> define where the special is placed (x is horizontal, y vertical). Valid value are in the range 0...23, min-max values are also supported: `"x": [ 0, 23 ], "y": [ 0, 23 ]` places the special anyway on the map.

Example with mapping (the characters 'O' and ';' should appear in the rows array where the specials should appear):
```
"gaspumps": {
    "O": { "amount": 1000 }
},
"toilets": {
    ";": { }
}
```
The amount of water to be placed in toilets is optional, an empty entry is therefor completely valid.

Example with coordinates:
```
"place_gaspumps": [
    { "x": 14, "y": 15, "amount": [ 1000, 2000 ] }
],
"place_toilets": [
    { "x": 19, "y": 22 }
]
```

Terrain, furniture and traps can specified as a single string, not a json object:
```
"traps" : {
    ".": "tr_beartrap"
}
```
Same as
```
"traps" : {
    ".": { "trap": "tr_beartrap" }
}
```

### 2.5.0 "fields"
Places a field (see fields.h). Values:
- "field": (required, string) the field type (e.g. "fd_blood")
- "density": (optional, integer) field density. Defaults to 1. Possible values are 1, 2, or 3.
- "age": (optional, integer) field age. Defaults to 0.

### 2.5.1 "npcs"
Places a new NPC. Values:
- "class": (required, string) the npc class id, see data/json/npcs/npc.json or define your own npc class.
- "target": (optional, bool) this NPC is a mission target.  Only valid for update_mapgen.
- "add_trait" (optional, string or string array) this NPC gets these traits, in addition to any from the class definition.

### 2.5.2 "signs"
Places a sign (furniture f_sign) with a message written on it. Either "signage" or "snippet" must be defined.  The message may include tags like \<full_name\>, \<given_name\>, and \<family_name\> that will insert a randomly generated name, or \<city\> that will insert the nearest city name.  Values:
- "signage": (optional, string) the message that should appear on the sign.
- "snippet": (optional, string) a category of snippets that can appear on the sign.

### 2.5.3 "vendingmachines"
Places a vending machine (furniture) and fills it with items. The machine can sometimes spawn as broken one. Values:
- "item_group": (optional, string) the item group that is used to create items inside the machine. It defaults to either "vending_food" or "vending_drink" (randomly chosen).

### 2.5.4 "toilets"
Places a toilet (furniture) and adds water to it. Values:
- "amount": (optional, integer or min/max array) the amount of water to be placed in the toilet.

### 2.5.5 "gaspumps"
Places a gas pump with gasoline (or sometimes diesel) in it. Values:
- "amount": (optional, integer or min/max array) the amount of fuel to be placed in the pump.
- "fuel": (optional, string: "gasoline" or "diesel") the type of fuel to be placed in the pump.

### 2.5.6 "items"
Places items from an item group. Values:
- "item": (required, string) the item group to use.
- "chance": (optional, integer or min/max array) x in 100 chance that a loop will continue to spawn items from the group (which itself may spawn multiple items or not depending on its type, see `ITEM_SPAWN.md`), unless the chance is 100, in which case it will trigger the item group spawn exactly 1 time (see `map::place_items`).
- "repeat": (optional, integer or min/max array) the number of times to repeat this placement, default is 1.

### 2.5.7 "monsters"
Places a monster spawn point, the actual monsters are spawned when the map is loaded. Values:
- "monster": (required, string) a monster group id, when the map is loaded, a random monsters from that group are spawned.
- "density": (optional, float) if defined, it overrides the default monster density at the location (monster density is bigger towards the city centers) (see `map::place_spawns`).
- "chance": (optional, integer or min/max array) one in x chance of spawn point being created (see `map::place_spawns`).

### 2.5.8 "vehicles"
Places a vehicle. Values:
- "vehicle": (required, string) type of the vehicle or id of a vehicle group.
- "chance": (optional, integer or min/max array) x in 100 chance of the vehicle spawning at all. The default is 1 (which means 1% probability that the vehicle spawns, you probably want something larger).
- "rotation": (optional, integer) the direction the vehicle faces.
- "fuel": (optional, integer) the fuel status. Default is -1 which makes the tanks 1-7% full. Positive values are interpreted as percentage of the vehicles tanks to fill (e.g. 100 means completely full).
- "status": (optional, integer) default is -1 (light damage), a value of 0 means perfect condition, 1 means heavily damaged.

### 2.5.9 "item"
Places a specific item. Values:
- "item": (required, string) the item type id of the new item.
- "chance": (optional, integer or min/max array) one in x chance that the item will spawn. Default is 1, meaning it will always spawn.
- "amount": (optional, integer or min/max array) the number of items to spawn, default is 1.
- "repeat": (optional, integer or min/max array) the number of times to repeat this placement, default is 1.

To use this type with explicit coordinates use the name "place_item" (this if for backwards compatibility) like this:
```
"item": {
    "x": { "item": "rock" }
},
"place_item": [
    { "x": 10, "y": 1, "item": "rock" }
]
```

### 2.5.10 "traps"
Places a trap. Values:
- "trap": (required, string) type id of the trap (e.g. tr_beartrap).

### 2.5.11 "furniture"
Places furniture. Values:
- "furn": (required, string) type id of the furniture (e.g. f_chair).

### 2.5.12 "terrain"
Places terrain. If the terrain has the value "roof" set and is in an enclosed space it's indoors. Values:
- "ter": (required, string) type id of the terrain (e.g. t_floor).

### 2.5.13 "monster"
Places a specific monster. Values:
- "monster": (required, string) type id of the monster (e.g. mon_zombie).
- "friendly": (optional, bool) whether the monster is friendly, default is false.
- "name": (optional, string) a name for that monster, optional, default is to create an unnamed monster.
- "target": (optional, bool) this monster is a mission target.  Only valid for update_mapgen.


### 2.5.14 "rubble"
Creates rubble and bashes existing terrain (this step is applied last, after other things like furniture/terrain have been set). Creating rubble invokes the bashing function that can destroy terrain and cause structures to collapse.
Values:
- "rubble_type": (optional, furniture id, default: f_rubble) the type of the created rubble.
- "items": (optional, bool, default: false) place items that result from bashing the structure.
- "floor_type": (optional, terrain id, default: t_dirt) only used if there is a non-bashable wall at the location or with overwrite = true.
- "overwrite": (optional, bool, default: false) if true it just writes on top of what currently exists.

To use this type with explicit coordinates use the name "place_rubble" (no plural) like this:
```JSON
"place_rubble": [
    { "x": 10, "y": 1 }
]
```

### 2.5.15 "place_liquids"
Creates a liquid item at the specified location. Liquids can't currently be picked up (except for gasoline in tanks or pumps), but can be used to add flavor to mapgen.
Values:
- "liquid": (required, item id) the item (a liquid)
- "amount": (optional, integer/min-max array) amount of liquid to place (a value of 0 defaults to the item's default charges)
- "chance": (optional, integer/min-max array) one in x chance of spawning a liquid, default value is 1 (100%)

Example for dropping a default amount of gasoline (200 units) on the ground (either by using a character in the rows array or explicit coordinates):
```JSON
"liquids": {
    "g": { "liquid": "gasoline" }
},
"place_liquids": [
    { "liquid": "gasoline", "x": 3, "y": 5 }
],
```

### 2.5.16 "loot"
Places item(s) from an item group, or an individual item. An important distinction between this and `place_item` and `item`/`items` is that `loot` can spawn a single item from a distribution group (without looping). It can also spawn a matching magazine and ammo for guns.
- Either `group` or `item` must be specified, but not both
  - "group": (string) the item group to use (see `ITEM_SPAWN.md` for notes on collection vs distribution groups)
  - "item": (string) the type id of the item to spawn
- "chance": (optional, integer) x in 100 chance of item(s) spawning. Defaults to 100.
- "ammo": (optional, integer) x in 100 chance of item(s) spawning with the default amount of ammo. Defaults to 0.
- "magazine": (optional, integer) x in 100 chance of item(s) spawning with the default magazine. Defaults to 0.

### 2.5.17 "sealed_item"
Places an item or item group inside furniture that has special handling for items.

This is intended for furniture such as `f_plant_harvest` with the `PLANT` flag, because placing items on such furniture via the other means will not work (since they have the `NOITEM` FLAG).

On such furniture, there is supposed to be a single (hidden) seed item which dictates the species of plant.  Using `sealed_item`, you can create such plants by specifying the furniture and a seed item.
- "furniture": (string) the id of the chosen furniture.
- Exactly one of:
  - "item": spawn an item as the "item" special.
  - "items": spawn an item group as the "items" special.

Example:
```
"sealed_item": {
  "p": {
    "items": { "item": "farming_seeds", "chance": 100 },
    "furniture": "f_plant_harvest"
  }
},
```

### 2.5.18 "graffiti"
Places a graffiti message at the location. Either "text" or "snippet" must be defined. The message may include tags like \<full_name\>, \<given_name\>, and \<family_name\> that will insert a randomly generated name, or \<city\> that will insert the nearest city name. Values:
- "text": (optional, string) the message that will be placed.
- "snippet": (optional, string) a category of snippets that the message will be pulled from.

### 2.5.19 "translate_ter"
Translates one type of terrain into another type of terrain.  There is no reason to do this with
normal mapgen, but it is useful for setting a baseline with update_mapgen.
- "from": (required, string) the terrain id of the terrain to be transformed
- "to": (required, string) the terrain id that the from terrain will transformed into

### 2.5.20 "zones"
Places a zone for an NPC faction.  NPCs in the faction will use the zone to influence the AI.
- "type": (required, string) must be one of NPC_RETREAT, NPC_NO_INVESTIGATE, or NPC_INVESTIGATE_ONLY.  NPCs will prefer to retreat towards NPC_RETREAT zones.  They will not move to the see the source of unseen sounds coming from NPC_NO_INVESTIGATE zones.  They will not move to the see the source of unseen sounds coming from outside NPC_INVESTIGATE_ONLY zones.
- "faction": (required, string) the faction id of the NPC faction that will use the zone.
- "name": (optional, string) the name of the zone.

### 2.5.21 "ter_furn_transforms"
Run a `ter_furn_transform` at the specified location.  This is mostly useful for applying transformations as part of
an update_mapgen, as normal mapgen can just specify the terrain directly.
- "transform": (required, string) the id of the `ter_furn_transform` to run.

# 2.6 "rotation"
Rotates the generated map after all the other mapgen stuff has been done. The value can be a single integer or a range (out of which a value will be randomly chosen). Example:
```JSON
"rotation": [ 0, 3 ],
```
Values are 90° steps.

# 2.7 "predecessor_mapgen"
Specifying an overmap terrain id here will run the entire mapgen for that overmap terrain type
first, before applying the rest of the mapgen defined here. The primary use case for this is when
our mapgen for a location takes place in a natural feature like a forest, swamp, or lake shore.
Many existing JSON mapgen attempt to emulate the mapgen of the type they're being placed on (e.g. a
cabin in the forest has placed the trees, grass and clutter of a forest to try to make the cabin
fit in) which leads to them being out of sync when the generation of that type changes. By
specifying the `predecessor_mapgen`, you can instead focus on the things that are added to the
existing location type. Example:
```json
"predecessor_mapgen": "forest"
```

# 3 update_mapgen
update_mapgen is a variant of normal JSON mapgen.  Instead of creating a new overmap tile, it
updates an existing overmap tile with a specific set of changes.  Currently, it only works within
the NPC mission interface, but it will be expanded to be a general purpose tool for modifying
existing maps.

update_mapgen generally uses the same fields as JSON mapgen, with a few exceptions.  update_mapgen has a few new fields to support missions, as well as ways to specify which overmap tile will be updated.

# 3.1 overmap tile specification
update_mapgen updates an existing overmap tile.  These fields provide a way to specify which tile to update.

### 3.1.0 "assign_mission_target"
assign_mission_target assigns an overmap tile as the target of a mission.  Any update_mapgen in the same scope will update that overmap tile.  The closet overmap terrain with the required terrain ID will be used, and if there is no matching terrain, an overmap special of om_special type will be created and then the om_terrain within that special will be used.
- "om_terrain" (required, string) the overmap terrain ID of the mission target
- "om_special" (required, string) the overmap special ID of the mission target

### 3.1.1 "om_terrain"
the closest overmap tile of type om_terrain in the closest overmap special of type om_special will be used.  The overmap tile will be updated but will not be set as the mission target.
- "om_terrain" (required, string) the overmap terrain ID of the mission target
- "om_special" (required, string) the overmap special ID of the mission target

# 3.2 mission specials
update_mapgen adds new optional keywords to a few mapgen JSON items.

### 3.2.0 "target"
place_npc, place_monster, and place_computer can take an optional target boolean. If they have `"target": true` and are invoked by update_mapgen with a valid mission, then the NPC, monster, or computer will be marked as the target of the mission.
