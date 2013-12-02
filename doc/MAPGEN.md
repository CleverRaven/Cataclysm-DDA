* 0 Intro
	* 0.0 How buildings and terrain are generated
* 1 Adding mapgen entries.
        * 1.0 Methods
        * 1.1 Placement
		* 1.1.0 Embedded
		* 1.1.1 Standalone
        * 1.2 Format and variables
		* 1.2.0 "method":
		* 1.2.1 "om_terrain":
		* 1.2.1 "weight":
	* 1.3 How "overmap_terrain" variables affect mapgen
        * 1.4 Limitations / TODO
* 2 Method: json
	* 2.0 "fill_ter":
        * 2.1 "rows":
	* 2.1.0 "terrain":
	* 2.1.1 "furntiture":
        * 2.3 "set": [ ...
	        * 2.3.0 "point" { ...
		        * 2.3.0.0 "id": "..."
		        * 2.3.0.1 "x" & "y": 123 | [ 12, 34 ]
		        * 2.3.0.2 "chance": 123
		        * 2.3.0.3 "repeat": [ 1, 23 ]
	        * 2.3.1 "line" {}
		        * 2.3.1.0 "id"
		        * 2.3.1.1 "x" & "y"
		        * 2.3.1.2 "x2" & "y2"
		        * 2.3.1.3 "chance"
		        * 2.3.1.4 "repeat"
	        * 2.3.2 "square" {}
        * 2.4 "place_groups": []
	        * 2.4.0 "monster": 
		        * 2.4.0.0 "x" & "y"
		        * 2.4.0.1 "chance"
		        * 2.4.0.2 "repeat"
	        * 2.4.1 "item": 
		        * 2.4.1.0 "x" & "y"
                        * 2.4.1.1 "amount"
		        * 2.4.1.2 "chance"
		        * 2.4.1.3 "repeat"
        * 2.5 "add": []
                * 2.5.0 "item":
                        * 2.5.0.0 "x" / "y"
                        * 2.5.0.1 "amount"
                        * 2.5.0.2 "chance"
                        * 2.5.0.3 "repeat"
        * 2.6 "lua":

* 3 Method: lua
	* 3.0 Tested functions
        * 3.1 Example script inside overmap_terrain entry

# 0 Intro
Note: You may wish to read over JSON_INFO.md beforehand.

## 0.0 How buildings and terrain are generated
Cataclysm creates builings and terrain on discovery via 'mapgen'; functions specific to an overmap terrain (the tiles you see in [m]ap are also determined by overmap terrain). Overmap terrains ("oter") are defined in overmap_terrain.json.

By default, an oter has a single builtin mapgen function which matches the '"id"' in it's json entry (examples: "house", "bank", etc). Multiple functions also possible. When a player moves into range of an area marked on the map as a house, the game chooses semi-randomly from a list of functions for "house", picks one, and runs it, laying down walls and adding items, monsters, rubber chickens and whatnot. This is all done in a fraction of a second (something to keep in mind for later).

All mapgen functions build in a 24x24 tile area - even for large buildings; obtuse but surprisingly effective methods are used to assemble giant 3x3 hotels, etc. For the moment, mod support for big buildings is not fully supported, though technically possible (see below).

In order to make a world that's random and (somewhat) sensical, there are numerous rules and exceptions to them, which are clarified below.

# 1 Adding mapgen entries.
One doesn't (and shouldn't) need to create a new overmap_terrain for a new variation of a building. For a custom gas station, defining a mapgen entry and adding it to the "s_gas" mapgen list will add it to the random variations of gas station in the world.

## 1.0 Methods
While adding mapgen as a c++ function is one of the fastest (and the most versatile) ways to generate procedural terrain on the fly, this requires recompiling the game. For mods, one can instead define a mapgen function in:
* JSON: A set of json arrays and objects for defining stuff and things. Pros: Fastest to apply, mostly complete, supported by one third party map editor so far. Cons: Not a programming language; no if statements or variables means instances of a particular json mapgen definition will all be similar. Support was added for randomizing things, however.

* LUA: The same scripting language used in Garry's Mod, S.T.A.L.K.E.R, CIV5, etc. Pros: Powerful; as a full scripting language it allows for endless variety and options in theory. Cons: In theory; lua needs bindings for internal game functions, and this is a work in progress. Also, it will apply to the map slower than c++ or json.

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
            { "weight": 25000,
                "method": "json", "object": {
                     (see below)
            }
        ]
(snip)

```
### 1.1.1 Standalone
As standalone { "type": "mapgen", ... } objects in a .json inside data/json. Below is the same fast food restaurant, along with one written lua (as part of unhealthy_dining_expansion_mod.json)
```C++
[
    {
        "type": "mapgen",
        "om_terrain": "s_restaurant_fast",
        "weight": 25000,
        "method": "json",
        "object": {
            (see below)
        }
    },
    {
        "type": "mapgen",
        "om_terrain": "s_restaurant_fast",
        "weight": 50000,
        "method": "lua",
        "script": {
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
-or-
> * "lua" - requires 
```
"script": "-- string of lua code\n -- for newline use \n -- and escape \"quote\"s"
```
-or-
> "lua" - requires
```"script": [
   " -- array of lines ",
   " -- etc ",
]```

### 1.2.1 "om_terrain":
**required for standalone**
> Values:
> * "matching_overmap_terrain_id" - see overmap_terrain.json for a list
-or-
> * [ "list_of", "oter_ids" ]
##### Example: "om_terrain": [ "house", "house_base" ]

### 1.2.2 "weight":
(optional) When the game randomly picks mapgen functions, each function's weight value determines how rare it is. 100000 is the default, so adding something with weight '50000' will make it appear 1/3 times, unless more functions are added. (An insanely high value like 100000000 is useful for testing)
> Values:
> * number - *0 disables*
Default: 100000

## 1.3 How "overmap_terrain" variables affect mapgen
"id" is used to determine the required "om_terrain" id for standalone, -except- when the following variables are set in "overmap_terrain":
* "line_drawing": true - For roads etc, which use ID_straight, ID_curved, ID_tee, ID_four_way
The following variables also come into play 
* "rotates": true - The terrain will be rotated after generation (to face roads etc). For format mapping, north is always up unless rotated east, etc.
* "extras" - applies rare, random scenes after mapgen; helicoptor crashes, etc
* "mondensity" - determines the default 'density' value for *"place_groups": [ { "monster": ...* (json) or *map:place_monster(..)* (lua)
 
## 1.4 Limitations / TODO
* JSON: adding vehicles, gas pumps, field effects, and specific monster spawns are still WIP.
* lua: Just about *everything* is WIP; there are issues passing class pointers back and forth with the game that will be corrected eventually
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
> Value: ([array]): 24 rows of 24 character lines. Each character is defined by "terrain" and optionally "furniture" below

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

## 2.3 "set"
**optional** Specific commands to set terrain, furniture, traps, radiation, etc. Array is processed in order.
> Value:  [array of {objects}]: [ { "point": .. }, { "line": .. }, { "square": .. }, ... ]

### 2.3.0 "point"
**required** Set things by point. Requires "x", "y", "id" (or "amount" for "radiation")
> Value: "terrain", "furniture", "trap", "radiation"
Example: { "point": "furniture", "id": "f_chair", "x:" 5, "y": 10 }
Example: { "point": "trap", "id": "tr_beartrap", "x:" [ 0, 23 ], "y": [ 5, 18 ], "chance": 10, "repeat": [ 2, 5 ] }
Example: { "point": "radiation", "id": "f_chair", "x:" 12, "y": 12, "amount": 20 }

#### 2.3.0.0 "x" / "y"
**both required** x/y coordinates. If it's an array, the result is a random number in that range
> Value: 0-23
-or-
> Value: [ 0-23, 0-23 ] - random point between [ a, b ]
Example: "x": 12, "y": [ 5, 15 ]

#### 2.3.0.1 "id"
**required except by "radiation"** See terrain.json, furniture.json, and trap.json for "id" strings
> Value: "ter_id", "furn_id", or "trap_id"
Example: "id": "tr_beartrap"

#### 2.3.0.2 "amount"
**required for "radiation"** Radiation amount
> Value: 0-100

#### 2.3.0.3 "chance"
**optional** one-in-??? chance to apply
> Value: *number*

#### 2.3.0.4 "repeat"
**optional** repeat this randomly between ??? and ??? times. Only makes sense if the coordinates are random
> Value: [ *number*, *number* ]
Example: [ 1, 3 ] - apply 1-3 times

### 2.3.1 "line"
**required** Set things in a line. Requires "x", "y", "x2", "y2", "id" (or "amount" for "radiation")
> Value: "terrain", "furniture", "trap", "radiation"
Example: { "point": "terrain", "id": "t_lava", "x:" 5, "y": 5, "x2": 20, "y2": 20 }

#### 2.3.1.0 "x" / "y"
**both required** start x/y coordinates. If it's an array, the result is a random number in that range
> Value: 0-23
-or-
> Value: [ 0-23, 0-23 ] - random point between [ a, b ]
Example: "x": 12, "y": [ 5, 15 ]

#### 2.3.1.1 "x2" / "y2"
**both required** end x/y coordinates. If it's an array, the result is a random number in that range
> Value: 0-23
-or-
> Value: [ 0-23, 0-23 ] - random point between [ a, b ]
Example: "x": 22, "y": [ 15, 20 ]

#### 2.3.1.2 "id"
**required except by "radiation"** See terrain.json, furniture.json, and trap.json for "id" strings
> Value: "ter_id", "furn_id", or "trap_id"
Example: "id": "f_counter"

#### 2.3.1.3 "amount"
**required for "radiation"** Radiation amount
> Value: 0-100

#### 2.3.1.4 "chance"
**optional** one-in-??? chance to apply
> Value: *number*

#### 2.3.1.5 "repeat"
**optional** repeat this randomly between ??? and ??? times. Only makes sense if the coordinates are random
> Value: [ *number*, *number* ]
Example: [ 1, 3 ] - apply 1-3 times

### 2.3.2 "square"
**required** Define a square of things. Requires "x", "y", "x2", "y2", "id" (or "amount" for "radiation")
> Value: "terrain", "furniture", "trap", "radiation"
Example: { "square": "radiation", "amount": 10, "x:" [ 0, 5 ], "y": [ 0, 5 ], "x2": [ 18, 23 ], "y2": [ 18, 23 ] }
The arguments are exactly the same as "line", but "x", "y" and "x2", "y2" define opposite corners

## 2.4 "place_groups"
**optional** Spawn items or monsters from item_groups.json and monster_groups.json
> Value: [ array of {objects} ]: [ { "monster": ... }, { "item": ... }, ... ]

### 2.4.0 "monster"
**required** The monster group id, which picks random critters from a list
> Value: "MONSTER_GROUP"
Example: { "monster": "GROUP_ZOMBIE", "x": [ 13, 15 ], "y": 15, "chance": 10 }

#### 2.4.0.0 "x" / "y"
**required** Spawn coordinates ( specific or random )
> Value: 0-23
-or-
> Value: [ 0-23, 0-23 ] - random point between [ a, b ]
Example: "x": 12, "y": [ 5, 15 ]

#### 2.4.0.1 "density"
**optional** magic sauce spawn amount number. Someone else describe this better >.>
> Value: *floating point number*

#### 2.4.0.2 "chance"
**optional** one-in-??? chance to apply
> Value: *number*

### 2.4.1 "item"
**required** The item group id, which picks random stuff from a list
> Value: "ITEM_GROUP"
Example: { "item": "livingroom", "x": [ 13, 15 ], "y": 15, "chance": 50 }

#### 2.4.1.0 "x" / "y"
**required** Spawn coordinates ( specific or random )
> Value: 0-23
-or-
> Value: [ 0-23, 0-23 ] - random point between [ a, b ]
Example: "x": 12, "y": [ 5, 15 ]

#### 2.4.1.1 "chance"
**required** unlike everything else, this is a percentage. Maybe
> Value: *number*

## 2.5 "add"
**optional** A list of *specific* things to add. WIP: Monsters and vehicles will be here too
> Value: [ array of {objects} ]: [ { "item", ... }, ... ]

### 2.5.0 "item"
**required** A valid itype ID. see everything in data/json/items
> Value: "string"
Example: { "item": "weed", "x": 14, "y": 15, "amount": [ 10, 20 ], "repeat": [1, 3], "chance": 2 }

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

#### 2.6 "lua"
**optional** lua script to run after all of the above finishes. See below.
> Value: "string"
Example: "lua": "if game.one_in(5000) then\n map:square_ter(\"t_lava\", 3, 3, 20, 20)\n game.add_msg(\"Oh noes micro volcano ;.;\")\n end"

## 3 Method: lua
Lua is very WIP but supports the following map class functions:
# 3.0 Tested functions
```
ter_set(x, y, "ter_id")
furn_set(x, y, "furn_id")
line_ter("ter_id", x, y, x2, y2)
line_furn("furn_id", x, y, x2, y2)
fill_background("ter_id")
square_ter("ter_id", x, y, x2, y2)
square_furn("furn_id", x, y, x2, y2)
rough_circle("ter_id", x, y, radius)
place_items("itype", chance, x, y, x2, y2, bool ongrass, int chance)
```
## 3.1 Example script inside overmap_terrain entry
```
{ "method": "lua", "weight": 50000, "comment": "Inlined, easier to edit", "script": [
    "-- tertype, turn = ...",
    "function g_or_d_or_p()",
    "  if game.one_in(6) then",
    "      return \"t_water_sh\"",
    "  end",
    "  if game.rng(1,4) == 1 then",
    "    return \"t_grass\"",
    "  end",
    "  return \"t_dirt\"",
    "end",
    "x = 0",
    "while x < 24 do",
    "  y = 0",
    "  while y < 24 do",
    "    map:ter_set(x, y, g_or_d_or_p())",
    "    y = y + 1",
    "  end",
    "  x = x + 1",
    "end",
    "map:place_items(\"field\", 60, 0, 0, 23, 23, 1, turn)",
    "--map:square_furn(\"f_mutpoppy\", 10,10,13,13)",
    "--map:line_ter(\"t_paper\", 1, 1, 22, 22)",
    "game.add_msg(\"generated oter=\"..tertype..\" turn=\"..turn)"
  ]
},
```
Ideally there will also be a way to load a .lua file directly from the same directory as its mapgen definition

