# MAPGEN

* [How buildings and terrain are generated](#how-buildings-and-terrain-are-generated)
* [Adding mapgen entries](#adding-mapgen-entries)
  * [Methods](#methods)
  * [Mapgen definition Placement](#mapgen-definition-placement)
    * [Embedded mapgen](#embedded-mapgen)
    * [Standalone mapgen](#standalone-mapgen)
  * [Format and variables](#format-and-variables)
    * [Define mapgen "method"](#define-mapgen-method)
    * [Define overmap terrain with "om_terrain" value, array, or nested array](#define-overmap-terrain-with-om_terrain-value-array-or-nested-array)
    * [Define mapgen "weight"](#define-mapgen-weight)
  * [How "overmap_terrain" variables affect mapgen](#how-overmap_terrain-variables-affect-mapgen)
  * [Limitations / TODO](#limitations--todo)
* [JSON object definition](#json-object-definition)
  * [Fill terrain using "fill_ter"](#fill-terrain-using-fill_ter)
  * [ASCII map using "rows" array](#ascii-map-using-rows-array)
    * [Row terrains in "terrain"](#row-terrains-in-terrain)
    * [Furniture symbols in "furniture" array](#furniture-symbols-in-furniture-array)
  * [Set terrain, furniture, or traps with a "set" array](#set-terrain-furniture-or-traps-with-a-set-array)
    * [Set things at a "point"](#set-things-at-a-point)
    * [Set things in a "line"](#set-things-in-a-line)
    * [Set things in a "square"](#set-things-in-a-square)
  * [Spawn item or monster groups with "place_groups"](#spawn-item-or-monster-groups-with-place_groups)
    * [Spawn monsters from a group with "monster"](#spawn-monsters-from-a-group-with-monster)
    * [Spawn items from a group with "item"](#spawn-items-from-a-group-with-item)
  * [Spawn a single monster with "place_monster"](#spawn-a-single-monster-with-place_monster)
  * [Spawn specific items with a "place_item" array](#spawn-specific-items-with-a-place_item-array)
  * [Extra map features with specials](#extra-map-features-with-specials)
    * [Place smoke, gas, or blood with "fields"](#place-smoke-gas-or-blood-with-fields)
    * [Place NPCs with "npcs"](#place-npcs-with-npcs)
    * [Place signs with "signs"](#place-signs-with-signs)
    * [Place a vending machine and items with "vendingmachines"](#place-a-vending-machine-and-items-with-vendingmachines)
    * [Place a toilet with some amount of water with "toilets"](#place-a-toilet-with-some-amount-of-water-with-toilets)
    * [Place a gas or diesel pump with some fuel with "gaspumps"](#place-a-gas-or-diesel-pump-with-some-fuel-with-gaspumps)
    * [Place items from an item group with "items"](#place-items-from-an-item-group-with-items)
    * [Place monsters from a monster group with "monsters"](#place-monsters-from-a-monster-group-with-monsters)
    * [Place a vehicle by type or group with "vehicles"](#place-a-vehicle-by-type-or-group-with-vehicles)
    * [Place a specific item with "item"](#place-a-specific-item-with-item)
    * [Place a specific monster with "monster"](#place-a-specific-monster-with-monster)
    * [Place a trap with "traps"](#place-a-trap-with-traps)
    * [Place furniture with "furniture"](#place-furniture-with-furniture)
    * [Place terrain with "terrain"](#place-terrain-with-terrain)
    * [Place rubble and smash existing terrain with "rubble"](#place-rubble-and-smash-existing-terrain-with-rubble)
    * [Place spilled liquids with "place_liquids"](#place-spilled-liquids-with-place_liquids)
    * [Place a specific item or an item from a group with "loot"](#place-a-specific-item-or-an-item-from-a-group-with-loot)
    * [Plant seeds in a planter with "sealed_item"](#plant-seeds-in-a-planter-with-sealed_item)
    * [Place messages with "graffiti"](#place-messages-with-graffiti)
    * [Place a zone for an NPC faction with "zones"](#place-a-zone-for-an-npc-faction-with-zones)
    * [Translate terrain type with "translate_ter"](#translate-terrain-type-with-translate_ter)
    * [Apply mapgen transformation with "ter_furn_transforms"](#apply-mapgen-transformation-with-ter_furn_transforms)
  * [Rotate the map with "rotation"](#rotate-the-map-with-rotation)
  * [Pre-load a base mapgen with "predecessor_mapgen"](#pre-load-a-base-mapgen-with-predecessor_mapgen)
* [Using update_mapgen](#using-update_mapgen)
  * [Overmap tile specification](#overmap-tile-specification)
    * ["assign_mission_target"](#assign_mission_target)
    * ["om_terrain"](#om_terrain)
* [Mission specials](#mission-specials)
    * ["target"](#target)

# How buildings and terrain are generated

Cataclysm creates buildings and terrain on discovery via 'mapgen'; functions specific to an overmap terrain (the tiles
you see in `[m]`ap are also determined by overmap terrain). Overmap terrains ("oter") are defined in
`overmap_terrain.json`.

By default, an oter has a single built-in mapgen function which matches the '"id"' in it's json entry (examples:
"house", "bank", etc). Multiple functions also possible. When a player moves into range of an area marked on the map as
a house, the game chooses semi-randomly from a list of functions for "house", picks one, and runs it, laying down walls
and adding items, monsters, rubber chickens and whatnot. This is all done in a fraction of a second (something to keep
in mind for later).

All mapgen functions build in a 24x24 tile area - even for large buildings; obtuse but surprisingly effective methods
are used to assemble giant 3x3 hotels, etc..

In order to make a world that's random and (somewhat) sensical, there are numerous rules and exceptions to them, which
are clarified below.

There are three methods:

- Adding mapgen entries
- JSON object definition
- Using update_mapgen


# Adding mapgen entries

One doesn't need to create a new `overmap_terrain` for a new variation of a building. For a custom gas station, defining a
mapgen entry and adding it to the "s_gas" mapgen list will add it to the random variations of gas station in the world.

If you use an existing `overmap_terrain` and it has a roof or other z-level linked to its file, the other levels will be
generated with the ground floor. To avoid this, or add your own multiple z-levels, create an `overmap_terrain` with a
similar name (`s_gas_1`).


## Methods

While adding mapgen as a c++ function is one of the fastest (and the most versatile) ways to generate procedural terrain
on the fly, this requires recompiling the game.

Most of the existing c++ buildings have been moved to json and currently json mapping is the preferred method of adding
both content and mods.

* JSON: A set of json arrays and objects for defining stuff and things. Pros: Fastest to apply, mostly complete. Cons:
    Not a programming language; no if statements or variables means instances of a particular json mapgen definition
    will all be similar. Third party map editors are currently out of date.

* JSON support includes the use of nested mapgen, smaller mapgen chunks which override a portion of the linked mapgen.
    This allows for greater variety in furniture, terrain and spawns within a single mapgen file.  You can also link
    mapgen files for multiple z-level buildings and multi-tile buildings.


## Mapgen definition Placement

Mapgen definitions can be added in 2 places:

### Embedded mapgen

As `"mapgen": { ... }` only used in combination with the 'builtin' method:

```json
"mapgen": [ { "method": "builtin", "name": "parking_lot" } ]
```

Do not use this, use standalone instead.


### Standalone mapgen

As standalone `{ "type": "mapgen", ... }` objects in a .json inside data/json. Below is the fast food restaurant.

```json
[
    {
        "type": "mapgen",
        "om_terrain": "s_restaurant_fast",
        "weight": 250,
        "method": "json",
        "object": {
            "//": "(see below)"
        }
    }
]
```

Note how "om_terrain" matches the overmap "id". om_terrain is **required** for standalone mapgen entries.


## Format and variables

The above example only illustrate the mapgen entries, not the actual format for building stuff. However, the following
variables impact where and how often stuff gets applied:

- method
- om_terrain
- weight


### Define mapgen "method"

**required**
Values: *json* - required

```
"object": { (more json here) }
```

### Define overmap terrain with "om_terrain" value, array, or nested array

**required for standalone**

The `om_terrain` value may be declared in one of three forms: with a single overmap terrain ID, with a list of IDs, or
with a nested list (of lists) of IDs.

With the first form, simply give the ID of an overmap terrain from `overmap_terrain.json`:

```
"om_terrain": "oter_id"
```

In the second form, provide a list of IDs:

```
"om_terrain": [ "house", "house_base" ]
```

This creates duplicate overmap terrains by applying the same json mapgen to each of the listed overmap terrain IDs.

The third option is a nested list:

```
"om_terrain": [ [ "oter_id_1a", "oter_id_1b", ... ], [ "oter_id_2a", "oter_id_2b", ... ], ... ]
```

This form allows for multiple overmap terrains to be defined using a single json object, with the "rows" property
expanding in blocks of 24x24 characters to accommodate as many overmap terrains as are listed here. The terrain ids are
specified using a nested array of strings which represent the rows and columns of overmap terrain ids (found in
`overmap_terrain.json`) that are associated with the "rows" property described in section 2.1 of this document.

Characters mapped using the "terrain", "furniture", or any of the special mappings ("items", "monsters", etc) will be
applied universally to all of the listed overmap terrains.

Placing things using x/y coordinates ("place_monsters", "place_loot", "place_item", etc) works using the full extended
coordinates beyond 24x24. An important limitation is that ranged random coordinates (such as "x": `[ 10, 18 ]`) must not
cross the 24x24 terrain boundaries. Ranges such as `[ 0, 23 ]` and `[ 50, 70 ]` are valid, but `[ 0, 47 ]` and
`[ 15, 35 ]` are not because they extend beyond a single 24x24 block.

Example:

```json
"om_terrain": [
  [ "apartments_mod_tower_NW", "apartments_mod_tower_NE" ],
  [ "apartments_mod_tower_SW", "apartments_mod_tower_SE" ]
]
```

In this example, the "rows" property should be 48x48, with each quadrant of 24x24 being associated with each of the four
apartments_mod_tower overmap terrain ids specified.


### Define mapgen "weight"

(optional) When the game randomly picks mapgen functions, each function's weight value determines how rare it is. 1000
is the default, so adding something with weight '500' will make it appear 1/3 times, unless more functions are added.
(An insanely high value like 10000000 is useful for testing)

Values: number - *0 disables*

Default: 1000


## How "overmap_terrain" variables affect mapgen

"id" is used to determine the required "om_terrain" id for standalone, *except* when the following variables are set in
"overmap_terrain":

* "extras" - applies rare, random scenes after mapgen; helicopter crashes, etc
* "mondensity" - determines the default 'density' value for `"place_groups": [ { "monster": ...` (json). If this is not
    set then place_monsters will not work without its own explicitly set density argument.

## Limitations / TODO

* JSON: adding specific monster spawns are still WIP.
* The old mapgen.cpp system involved *The Biggest "if / else if / else if / .." Statement Known to Man*(tm), and is only
    halfway converted to the "builtin" mapgen class. This means that while custom mapgen functions are allowed, the game
    will cheerfully forget the default if one is added.
* TODO: Add to this list.


# JSON object definition

The JSON object for a mapgen entry must include either `"fill_ter"`, or `"rows"` and `"terrain"`. All other fields are
optional.


## Fill terrain using "fill_ter"
*required if "rows" is unset* Fill with the given terrain.

Value: `"string"`: Valid terrain id from data/json/terrain.json

Example: `"fill_ter": "t_grass"`


## ASCII map using "rows" array
*required if "fill_ter" is unset*

Nested array of 24 (or 48) strings, each 24 (or 48) characters long, where each character is defined by "terrain" and
optionally "furniture" or other entries below.

Usage:

```
"rows": [ "row1...", "row2...", ..., "row24..." ]
```

Other parts can be linked with this map, for example one can place things like a gaspump (with gasoline) or a toilet
(with water) or items from an item group or fields at the square given by a character.

Any character used here must have some definition elsewhere to indicate its purpose.  Failing to do so is an error which
will be caught by running the tests.  The tests will run automatically when you make a pull request for adding new maps
to the game.  If you have defined `fill_ter` or you are writing nested mapgen, then there are a couple of exceptions.
The space and period characters (` ` and `.`) are permitted to have no definition and be used for 'background' in the
`rows`.

As keys, you can use any Unicode characters which are not double-width.  This includes for example most European
alphabets but not Chinese characters.  If you intend to take advantage of this, ensure that your editor is saving the
file with a UTF-8 encoding.  Accents are acceptable, even when using [combining
characters](https://en.wikipedia.org/wiki/Combining_character).  No normalization is performed; comparison is done at
the raw bytes (code unit) level.  Therefore, there are literally an infinite number of mapgen key characters available.
Please don't abuse this by using distinct characters that are visually indistinguishable, or which are so rare as to be
unlikely to render correctly for other developers.

Example:

```json
"rows": [
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

### Row terrains in "terrain"
**required by "rows"**

Defines terrain ids for "rows", each key is a single character with a terrain id string

Value: `{object}: { "a", "t_identifier", ... }`

Example:

```json
"terrain": {
  " ": "t_grass",
  "d": "t_floor",
  "5": "t_wall_glass_h",
  "%": "t_wall_glass_v",
  "O": "t_floor",
  ",": "t_pavement_y",
  "_": "t_pavement",
  "r": "t_floor",
  "6": "t_console",
  "x": "t_console_broken",
  "$": "t_shrub",
  "^": "t_floor",
  ".": "t_floor",
  "-": "t_wall_h",
  "|": "t_wall_v",
  "#": "t_shrub",
  "t": "t_floor",
  "+": "t_door_glass_c",
  "=": "t_door_locked_alarm",
  "D": "t_door_locked",
  "w": "t_window_domestic",
  "T": "t_floor",
  "S": "t_floor",
  "e": "t_floor",
  "h": "t_floor",
  "c": "t_floor",
  "l": "t_floor",
  "s": "t_sidewalk"
},
```

### Furniture symbols in "furniture" array
**optional**

Defines furniture ids for "rows" ( each character in rows is a terrain -or- terrain/furniture combo ). "f_null" means no
furniture but the entry can be left out

Example:

```json
"furniture": {
  "d": "f_dumpster",
  "5": "f_null",
  "%": "f_null",
  "O": "f_oven",
  "r": "f_rack",
  "^": "f_indoor_plant",
  "t": "f_table",
  "T": "f_toilet",
  "S": "f_sink",
  "e": "f_fridge",
  "h": "f_chair",
  "c": "f_counter",
  "l": "f_locker"
},
```

## Set terrain, furniture, or traps with a "set" array
**optional** Specific commands to set terrain, furniture, traps, radiation, etc. Array is processed in order.

Value: `[ array of {objects} ]: [ { "point": .. }, { "line": .. }, { "square": .. }, ... ]`

Example:

```json
"set": [
  { "point": "furniture", "id": "f_chair", "x": 5, "y": 10 },
  { "point": "radiation", "id": "f_chair", "x": 12, "y": 12, "amount": 20 },
  { "point": "trap", "id": "tr_beartrap", "x": [ 0, 23 ], "y": [ 5, 18 ], "chance": 10, "repeat": [ 2, 5 ] }
]
```

All X and Y values may be either a single integer between `0` and `23`, or an array of two integers `[ n1, n2 ]` (each
between `0` and `23`). If X or Y are set to an array, the result is a random number in that range (inclusive). In the above
examples, the furniture `"f_chair"` is always at coordinates `"x": 5, "y": 10`, but the trap `"tr_beartrap"` is
randomly repeated in the area `"x": [ 0, 23 ], "y": [ 5, 18 ]"`.

See terrain.json, furniture.json, and trap.json for "id" strings.


### Set things at a "point"

- Requires "point" type, and coordinates "x" and "y"
- For "point" type "radiation", requires "amount"
- For other types, requires "id" of terrain, furniture, or trap

| Field  | Description
| ---    | ---
| point  | Allowed values: `"terrain"`, `"furniture"`, `"trap"`, `"radiation"`
| id     | Terrain, furniture, or trap ID. Examples: `"id": "f_counter"`, `"id": "tr_beartrap"`. Omit for "radiation".
| x, y   | X, Y coordinates. Value from `0-23`, or range `[ 0-23, 0-23 ]` for a random value in that range. Example: `"x": 12, "y": [ 5, 15 ]`
| amount | Radiation amount. Value from `0-100`.
| chance | (optional) One-in-N chance to apply
| repeat | (optional) Value: `[ n1, n2 ]`. Spawn item randomly between `n1` and `n2` times. Only makes sense if the coordinates are random. Example: `[ 1, 3 ]` - repeat 1-3 times.


### Set things in a "line"

- Requires "line" type, and endpoints "x", "y" and "x2", "y2"
- For "line" type "radiation", requires "amount"
- For other types, requires "id" of terrain, furniture, or trap

Example:
```json
{ "line": "terrain", "id": "t_lava", "x": 5, "y": 5, "x2": 20, "y2": 20 }
```

| Field  | Description
| ---    | ---
| line   | Allowed values: `"terrain"`, `"furniture"`, `"trap"`, `"radiation"`
| id     | Terrain, furniture, or trap ID. Examples: `"id": "f_counter"`, `"id": "tr_beartrap"`. Omit for "radiation".
| x, y   | Start X, Y coordinates. Value from `0-23`, or range `[ 0-23, 0-23 ]` for a random value in that range. Example: `"x": 12, "y": [ 5, 15 ]`
| x2, y2 | End X, Y coordinates. Value from `0-23`, or range `[ 0-23, 0-23 ]` for a random value in that range. Example: `"x": 22, "y": [ 15, 20 ]`
| amount | Radiation amount. Value from `0-100`.
| chance | (optional) One-in-N chance to apply
| repeat | (optional) Value: `[ n1, n2 ]`. Spawn item randomly between `n1` and `n2` times. Only makes sense if the coordinates are random. Example: `[ 1, 3 ]` - repeat 1-3 times.


### Set things in a "square"

- Requires "square" type, and opposite corners at "x", "y" and "x2", "y2"
- For "square" type "radiation", requires "amount"
- For other types, requires "id" of terrain, furniture, or trap

The "square" arguments are the same as for "line", but "x", "y" and "x2", "y2" define opposite corners.

Example:
```json
{ "square": "radiation", "amount": 10, "x": [ 0, 5 ], "y": [ 0, 5 ], "x2": [ 18, 23 ], "y2": [ 18, 23 ] }
```

| Field  | Description
| ---    | ---
| square | Allowed values: `"terrain"`, `"furniture"`, `"trap"`, `"radiation"`
| id     | Terrain, furniture, or trap ID. Examples: `"id": "f_counter"`, `"id": "tr_beartrap"`. Omit for "radiation".
| x, y   | Top-left corner of square.
| x2, y2 | Bottom-right corner of square.


## Spawn item or monster groups with "place_groups"
**optional** Spawn items or monsters from item_groups.json and monster_groups.json

Value: `[ array of {objects} ]: [ { "monster": ... }, { "item": ... }, ... ]`

### Spawn monsters from a group with "monster"

| Field   | Description
| ---     | ---
| monster | (required) Value: `"MONSTER_GROUP"`. The monster group id, which picks random critters from a list
| x, y    | (required) Spawn coordinates. Value from `0-23`, or range `[ 0-23, 0-23 ]` for a random value in that range.
| density | (optional) Floating-point multiplier to "chance" (see below).
| chance  | (optional) One-in-N chance to spawn

Example:

```json
{ "monster": "GROUP_ZOMBIE", "x": [ 13, 15 ], "y": 15, "chance": 10 }
```

When using a range for `"x"` or `"y"`, the minimum and maximum values will be used in creating rectangle coordinates to
be used by `map::place_spawns`. Each monster generated from the monster group will be placed in a different random
location within the rectangle. The values in the above example will produce a rectangle for `map::place_spawns` from (
13, 15 ) to ( 15, 15 ) inclusive.

The optional "density" is a floating-point multipier to the "chance" value. If the result is bigger than 100% it
gurantees one spawn point for every 100% and the rest is evaluated by chance (one added or not). Then the monsters are
spawned according to their spawn-point cost "cost_multiplier" defined in the monster groups. Additionally all overmap
densities within a square of raduis 3 (7x7 around player - exact value in mapgen.cpp/MON_RADIUS macro) are added to
this. The "pack_size" modifier in monstergroups is a random multiplier to the rolled spawn point amount.



### Spawn items from a group with "item"

| Field  | Description
| ---    | ---
| item   | (required) Value: "ITEM_GROUP". The item group id, which picks random stuff from a list.
| x, y   | (required) Spawn coordinates. Value from `0-23`, or range `[ 0-23, 0-23 ]` for a random value in that range.
| chance | (required) Percentage chance to spawn.

Example:

```json
{ "item": "livingroom", "x": 12, "y": [ 5, 15 ], "chance": 50 }
```

When using a range for `"x"` or `"y"`, the minimum and maximum values will be used in creating rectangle coordinates to
be used by `map::place_items`. Each item from the item group will be placed in a different random location within the
rectangle. These values in the above example will produce a rectangle for map::place_items from ( 12, 5 ) to ( 12, 15 )
inclusive.


## Spawn a single monster with "place_monster"

**optional** Spawn single monster. Either specific monster or a random monster from a monster group. Is affected by spawn density game setting.

Value: `[ array of {objects} ]: [ { "monster": ... } ]`

| Field       | Description
| ---         | ---
| monster     | ID of the monster to spawn.
| group       | ID of the monster group from which the spawned monster is selected. `monster` and `group` should not be used together. `group` will act over `monster`.
| x, y        | Spawn coordinates ( specific or area rectangle ). Value: 0-23 or `[ 0-23, 0-23 ]` - random value between `[ a, b ]`.
| chance      | Percentage chance to do spawning. If repeat is used each repeat has separate chance.
| repeat      | The spawning is repeated this many times. Can be a number or a range.
| pack_size   | How many monsters are spawned. Can be single number or range like `[1-4]`. Is affected by the chance and spawn density. Ignored when spawning from a group.
| one_or_none | Do not allow more than one to spawn due to high spawn density. If repeat is not defined or pack size is defined this defaults to true true, otherwise this defaults to false. Ignored when spawning from a group.
| friendly    | Set true to make the monster friendly. Default false.
| name        | Extra name to display on the monster.
| target      | Set to true to make this into mission target. Only works when the monster is spawned from a mission.

Note that high spawn density game setting can cause extra monsters to spawn when `monster` is used. When `group` is used
only one monster will spawn.

When using a range for `"x"` or `"y"`, the minimum and maximum values will be used in creating rectangle coordinates to
be used by `map::place_spawns`. Each monster generated from the monster group will be placed in a different random
location within the rectangle. Example: `"x": 12, "y": [ 5, 15 ]` - these values will produce a rectangle for
`map::place_spawns` from ( 12, 5 ) to ( 12, 15 ) inclusive.

Example:
```json
"place_monster": [
    { "group": "GROUP_REFUGEE_BOSS_ZOMBIE", "name": "Sean McLaughlin", "x": 10, "y": 10, "target": true }
]
```

This places a single random monster from group "GROUP_REFUGEE_BOSS_ZOMBIE", sets the name to "Sean McLaughlin", spawns
the monster at coordinate (10, 10) and also sets the monster as the target of this mission.

Example:
```json
"place_monster": [
    { "monster": "mon_secubot", "x": [ 7, 18 ], "y": [ 7, 18 ], "chance": 30, "repeat": [1, 3], "spawn_data": { "ammo": [ { "ammo_id": "556", "qty": [ 20, 30 ] } ] } }
]
```

This places "mon_secubot" at random coordinate (7-18, 7-18). The monster is placed with 30% probablity. The placement is
repeated by random number of times `[1-3]`. The monster will spawn with 20-30 5.56x45mm rounds.


## Spawn specific items with a "place_item" array
**optional** A list of *specific* things to add. WIP: Monsters and vehicles will be here too

Value: `[ array of {objects} ]: [ { "item", ... }, ... ]`

Example:
```json
"place_item": [
    { "item": "weed", "x": 14, "y": 15, "amount": [ 10, 20 ], "repeat": [1, 3], "chance": 20 }
]
```

| Field  | Description
| ---    | ---
| item   | (required) ID of the item to spawn
| x, y   | (required) Spawn coordinates. Value from `0-23`, or range `[ 0-23, 0-23 ]` for a random value in that range.
| amount | (required) Number of items to spawn. Single integer, or range `[ a, b ]` for a random value in that range.
| chance | (optional) One-in-N chance to spawn item.
| repeat | (optional) Value: `[ n1, n2 ]`. Spawn item randomly between `n1` and `n2` times. Only makes sense if the coordinates are random. Example: `[ 1, 3 ]` - repeat 1-3 times.


## Extra map features with specials
**optional** Special map features that do more than just placing furniture / terrain.

Specials can be defined either via a mapping like the terrain / furniture mapping using the "rows" entry above or
through their exact location by its coordinates.

The mapping is defined with a json object like this:

```
"<type-of-special>" : {
    "A" : { <data-of-special> },
    "B" : { <data-of-special> },
    "C" : { <data-of-special> },
    ...
}
```

`"<type-of-special>"` is one of the types listed below. `<data-of-special>` is a json object with content specific to
the special type. Some types require no data at all or all their data is optional, an empty object is enough for those
specials. You can define as many mapping as you want.

Each mapping can be an array, for things that can appear several times on the tile (e.g. items, fields) each entry of
the array is applied in order. For traps, furniture and terrain, one entry is randomly chosen (all entries have the same
chances) and applied.

Example (places grass at 2/3 of all '.' square and dirt at 1/3 of them):

```json
"terrain" : {
    ".": [ "t_grass", "t_grass", "t_dirt" ]
}
```

It is also possible to specify the number of instances (and consequently their chance) directly, which is particularly
useful for rare occurrences (rather than repeating the common value many times):

```json
"terrain" : {
    ".": [ [ "t_grass", 2 ], "t_dirt" ]
}
```


Example (places a blood and a bile field on each '.' square):

```json
"fields" : {
    ".": [ { "field": "fd_blood" }, { "field": "fd_bile" } ]
}
```

Or define the mappings for one character at once:
```json
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

`<x>` and `<y>` define where the special is placed (x is horizontal, y vertical). Valid value are in the range 0..23,
min-max values are also supported: `"x": [ 0, 23 ], "y": [ 0, 23 ]` places the special anyway on the map.

Example with mapping (the characters `"O"` and `";"` should appear in the rows array where the specials should appear):

```json
"gaspumps": {
    "O": { "amount": 1000 }
},
"toilets": {
    ";": { }
}
```

The amount of water to be placed in toilets is optional, an empty entry is therefor completely valid.

Example with coordinates:

```json
"place_gaspumps": [
    { "x": 14, "y": 15, "amount": [ 1000, 2000 ] }
],
"place_toilets": [
    { "x": 19, "y": 22 }
]
```

Terrain, furniture and traps can specified as a single string, not a json object:
```json
"traps" : {
    ".": "tr_beartrap"
}
```
Same as
```json
"traps" : {
    ".": { "trap": "tr_beartrap" }
}
```

### Place smoke, gas, or blood with "fields"

| Field     | Description
| ---       | ---
| field     | (required, string) the field type (e.g. `"fd_blood"`, `"fd_smoke"`)
| density   | (optional, integer) field density. Defaults to 1. Possible values are 1, 2, or 3.
| intensity | (optional, integer) how concentrated the field is, from 1 to 3 or more. See `data/json/field_type.json`
| age       | (optional, integer) field age. Defaults to 0.


### Place NPCs with "npcs"

Example:

```json
"npcs": { "A": { "class": "NC_REFUGEE", "target": true, "add_trait": "ASTHMA" } }
```

| Field     | Description
| ---       | ---
| class     | (required, string) the npc class id, see `data/json/npcs/npc.json` or define your own npc class.
| target    | (optional, bool) this NPC is a mission target.  Only valid for `update_mapgen`.
| add_trait | (optional, string or string array) this NPC gets these traits, in addition to any from the class definition.


### Place signs with "signs"

Places a sign (furniture `f_sign`) with a message written on it. Either "signage" or "snippet" must be defined.  The
message may include tags like `<full_name>`, `<given_name>`, and `<family_name>` that will insert a randomly generated
name, or `<city>` that will insert the nearest city name.

Example:

```json
"signs": { "P": { "signage": "Subway map: <city> stop" } }
```

| Field   | Description
| ---     | ---
| signage | (optional, string) the message that should appear on the sign.
| snippet | (optional, string) a category of snippets that can appear on the sign.


### Place a vending machine and items with "vendingmachines"

Places a vending machine (furniture) and fills it with items from an item group. The machine can sometimes spawn as broken one.

| Field      | Description
| ---        | ---
| item_group | (optional, string) the item group that is used to create items inside the machine. It defaults to either "vending_food" or "vending_drink" (randomly chosen).


### Place a toilet with some amount of water with "toilets"

Places a toilet (furniture) and adds water to it.

| Field  | Description
| ---    | ---
| amount | (optional, integer or min/max array) the amount of water to be placed in the toilet.


### Place a gas or diesel pump with some fuel with "gaspumps"

Places a gas pump with gasoline (or sometimes diesel) in it.

| Field  | Description
| ---    | ---
| amount | (optional, integer or min/max array) the amount of fuel to be placed in the pump.
| fuel   | (optional, string: "gasoline" or "diesel") the type of fuel to be placed in the pump.


### Place items from an item group with "items"

| Field  | Description
| ---    | ---
| item   | (required, string or itemgroup object) the item group to use.
| chance | (optional, integer or min/max array) x in 100 chance that a loop will continue to spawn items from the group (which itself may spawn multiple items or not depending on its type, see `ITEM_SPAWN.md`), unless the chance is 100, in which case it will trigger the item group spawn exactly 1 time (see `map::place_items`).
| repeat | (optional, integer or min/max array) the number of times to repeat this placement, default is 1.


### Place monsters from a monster group with "monsters"

The actual monsters are spawned when the map is loaded. Fields:

| Field   | Description
| ---     | ---
| monster | (required, string) a monster group id, when the map is loaded, a random monsters from that group are spawned.
| density | (optional, float) if defined, it overrides the default monster density at the location (monster density is bigger towards the city centers) (see `map::place_spawns`).
| chance  | (optional, integer or min/max array) one in x chance of spawn point being created (see `map::place_spawns`).


### Place a vehicle by type or group with "vehicles"

| Field    | Description
| ---      | ---
| vehicle  | (required, string) type of the vehicle or id of a vehicle group.
| chance   | (optional, integer or min/max array) x in 100 chance of the vehicle spawning at all. The default is 1 (which means 1% probability that the vehicle spawns, you probably want something larger).
| rotation | (optional, integer) the direction the vehicle faces.
| fuel     | (optional, integer) the fuel status. Default is -1 which makes the tanks 1-7% full. Positive values are interpreted as percentage of the vehicles tanks to fill (e.g. 100 means completely full).
| status   | (optional, integer) default is -1 (light damage), a value of 0 means perfect condition, 1 means heavily damaged.


### Place a specific item with "item"

| Field  | Description
| ---    | ---
| item   | (required, string) the item type id of the new item.
| chance | (optional, integer or min/max array) one in x chance that the item will spawn. Default is 1, meaning it will always spawn.
| amount | (optional, integer or min/max array) the number of items to spawn, default is 1.
| repeat | (optional, integer or min/max array) the number of times to repeat this placement, default is 1.

To use this type with explicit coordinates use the name "place_item" (this if for backwards compatibility) like this:

```json
"item": {
    "x": { "item": "rock" }
},
"place_item": [
    { "x": 10, "y": 1, "item": "rock" }
]
```

### Place a specific monster with "monster"

| Field    | Description
| ---      | ---
| monster  | (required, string) type id of the monster (e.g. `mon_zombie`).
| friendly | (optional, bool) whether the monster is friendly, default is false.
| name     | (optional, string) a name for that monster, optional, default is to create an unnamed monster.
| target   | (optional, bool) this monster is a mission target.  Only valid for `update_mapgen`.


### Place a trap with "traps"

| Field | Description
| ---   | ---
| trap  | (required, string) type id of the trap (e.g. `tr_beartrap`).


### Place furniture with "furniture"

| Field | Description
| ---   | ---
| furn  | (required, string) type id of the furniture (e.g. `f_chair`).


### Place terrain with "terrain"

If the terrain has the value "roof" set and is in an enclosed space, it's indoors.

| Field | Description
| ---   | ---
| ter   | (required, string) type id of the terrain (e.g. `t_floor`).


### Place rubble and smash existing terrain with "rubble"

Creates rubble and bashes existing terrain (this step is applied last, after other things like furniture/terrain have
been set). Creating rubble invokes the bashing function that can destroy terrain and cause structures to collapse.

| Field       | Description
| ---         | ---
| rubble_type | (optional, furniture id, default: `f_rubble`) the type of the created rubble.
| items       | (optional, bool, default: false) place items that result from bashing the structure.
| floor_type  | (optional, terrain id, default: `t_dirt`) only used if there is a non-bashable wall at the location or with overwrite = true.
| overwrite   | (optional, bool, default: false) if true it just writes on top of what currently exists.

To use this type with explicit coordinates use the name "place_rubble" (no plural) like this:

```json
"place_rubble": [
    { "x": 10, "y": 1 }
]
```

### Place spilled liquids with "place_liquids"

Creates a liquid item at the specified location. Liquids can't currently be picked up (except for gasoline in tanks or
pumps), but can be used to add flavor to mapgen.

| Field  | Description
| ---    | ---
| liquid | (required, item id) the item (a liquid)
| amount | (optional, integer/min-max array) amount of liquid to place (a value of 0 defaults to the item's default charges)
| chance | (optional, integer/min-max array) one in x chance of spawning a liquid, default value is 1 (100%)

Example for dropping a default amount of gasoline (200 units) on the ground (either by using a character in the rows
array or explicit coordinates):

```json
"liquids": {
    "g": { "liquid": "gasoline" }
},
"place_liquids": [
    { "liquid": "gasoline", "x": 3, "y": 5 }
],
```

### Place a specific item or an item from a group with "loot"

Places item(s) from an item group, or an individual item. An important distinction between this and `place_item` and
`item`/`items` is that `loot` can spawn a single item from a distribution group (without looping). It can also spawn a
matching magazine and ammo for guns.

**Note**: Either `group` or `item` must be specified, but not both.

| Field    | Description
| ---      | ---
| group    | (string) the item group to use (see `ITEM_SPAWN.md` for notes on collection vs distribution groups)
| item     | (string) the type id of the item to spawn
| chance   | (optional, integer) x in 100 chance of item(s) spawning. Defaults to 100.
| ammo     | (optional, integer) x in 100 chance of item(s) spawning with the default amount of ammo. Defaults to 0.
| magazine | (optional, integer) x in 100 chance of item(s) spawning with the default magazine. Defaults to 0.


### Plant seeds in a planter with "sealed_item"

Places an item or item group inside furniture that has special handling for items.

This is intended for furniture such as `f_plant_harvest` with the `PLANT` flag, because placing items on such furniture
via the other means will not work (since they have the `NOITEM` FLAG).

On such furniture, there is supposed to be a single (hidden) seed item which dictates the species of plant.  Using
`sealed_item`, you can create such plants by specifying the furniture and a seed item.

**Note**: Exactly one of "item" or "items" must be given (but not both).

| Field     | Description
| ---       | ---
| furniture | (string) the id of the chosen furniture.
| item      | spawn an item as the "item" special
| items     | spawn an item group as the "items" special.

Example:

```json
"sealed_item": {
  "p": {
    "items": { "item": "farming_seeds", "chance": 100 },
    "furniture": "f_plant_harvest"
  }
},
```


### Place messages with "graffiti"

Places a graffiti message at the location. Either "text" or "snippet" must be defined. The message may include tags like
`<full_name>`, `<given_name>`, and `<family_name>` that will insert a randomly generated name, or `<city>` that will
insert the nearest city name.

| Field   | Description
| ---     | ---
| text    | (optional, string) the message that will be placed.
| snippet | (optional, string) a category of snippets that the message will be pulled from.


### Place a zone for an NPC faction with "zones"

NPCs in the faction will use the zone to influence the AI.

| Field   | Description
| ---     | ---
| type    | (required, string) Values: `"NPC_RETREAT"`, `"NPC_NO_INVESTIGATE"`, or `"NPC_INVESTIGATE_ONLY"`.
| faction | (required, string) the faction id of the NPC faction that will use the zone.
| name    | (optional, string) the name of the zone.

The `type` field values affect NPC behavior. NPCs will:

- Prefer to retreat towards `NPC_RETREAT` zones.
- Not move to the see the source of unseen sounds coming from `NPC_NO_INVESTIGATE` zones.
- Not move to the see the source of unseen sounds coming from outside `NPC_INVESTIGATE_ONLY` zones.


### Translate terrain type with "translate_ter"

Translates one type of terrain into another type of terrain.  There is no reason to do this with normal mapgen, but it
is useful for setting a baseline with `update_mapgen`.

| Field | Description
| ---   | ---
| from  | (required, string) the terrain id of the terrain to be transformed
| to    | (required, string) the terrain id that the from terrain will transformed into


### Apply mapgen transformation with "ter_furn_transforms"

Run a `ter_furn_transform` at the specified location.  This is mostly useful for applying transformations as part of
an `update_mapgen`, as normal mapgen can just specify the terrain directly.

- "transform": (required, string) the id of the `ter_furn_transform` to run.


## Rotate the map with "rotation"

Rotates the generated map after all the other mapgen stuff has been done. The value can be a single integer or a range
(out of which a value will be randomly chosen). Example: `"rotation": [ 0, 3 ]`

Values are 90° steps.


## Pre-load a base mapgen with "predecessor_mapgen"

Specifying an overmap terrain id here will run the entire mapgen for that overmap terrain type first, before applying
the rest of the mapgen defined here. The primary use case for this is when our mapgen for a location takes place in a
natural feature like a forest, swamp, or lake shore. Many existing JSON mapgen attempt to emulate the mapgen of the type
they're being placed on (e.g. a cabin in the forest has placed the trees, grass and clutter of a forest to try to make
the cabin fit in) which leads to them being out of sync when the generation of that type changes. By specifying the
`predecessor_mapgen`, you can instead focus on the things that are added to the existing location type.

Example: `"predecessor_mapgen": "forest"`


# Using update_mapgen

**update_mapgen** is a variant of normal JSON mapgen.  Instead of creating a new overmap tile, it
updates an existing overmap tile with a specific set of changes.  Currently, it only works within
the NPC mission interface, but it will be expanded to be a general purpose tool for modifying
existing maps.

update_mapgen generally uses the same fields as JSON mapgen, with a few exceptions.  update_mapgen has a few new fields
to support missions, as well as ways to specify which overmap tile will be updated.

## Overmap tile specification
update_mapgen updates an existing overmap tile.  These fields provide a way to specify which tile to update.


### "assign_mission_target"

assign_mission_target assigns an overmap tile as the target of a mission.  Any update_mapgen in the same scope will
update that overmap tile.  The closet overmap terrain with the required terrain ID will be used, and if there is no
matching terrain, an overmap special of om_special type will be created and then the om_terrain within that special will
be used.

| Field      | Description
| ---        | ---
| om_terrain | (required, string) the overmap terrain ID of the mission target
| om_special | (required, string) the overmap special ID of the mission target


### "om_terrain"

The closest overmap tile of type om_terrain in the closest overmap special of type om_special will be used.  The overmap
tile will be updated but will not be set as the mission target.

| Field      | Description
| ---        | ---
| om_terrain | (required, string) the overmap terrain ID of the mission target
| om_special | (required, string) the overmap special ID of the mission target

# Mission specials
update_mapgen adds new optional keywords to a few mapgen JSON items.

### "target"

place_npc, place_monster, and place_computer can take an optional target boolean. If they have `"target": true` and are
invoked by update_mapgen with a valid mission, then the NPC, monster, or computer will be marked as the target of the
mission.

