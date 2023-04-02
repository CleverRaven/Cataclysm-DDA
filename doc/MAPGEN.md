
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
  * [Set terrain, furniture, traps, or variables with a "set" array](#set-terrain-furniture-or-traps-with-a-set-array)
    * [Set things at a "point"](#set-things-at-a-point)
    * [Set things in a "line"](#set-things-in-a-line)
    * [Set things in a "square"](#set-things-in-a-square)
  * [Spawn a single monster with "place_monster"](#spawn-a-single-monster-with-place_monster)
  * [Spawn an entire group of monsters with "place_monsters"](#spawn-an-entire-group-of-monsters-with-place_monsters)
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
    * [Specify a player spawning location using "zones"](#specify-a-player-spawning-location-using-zones)
    * [Translate terrain type with "translate_ter"](#translate-terrain-type-with-translate_ter)
    * [Apply mapgen transformation with "ter_furn_transforms"](#apply-mapgen-transformation-with-ter_furn_transforms)
  * [Mapgen values](#mapgen-values)
  * [Mapgen parameters](#mapgen-parameters)
  * [Rotate the map with "rotation"](#rotate-the-map-with-rotation)
  * [Pre-load a base mapgen with "predecessor_mapgen"](#pre-load-a-base-mapgen-with-predecessor_mapgen)
* [Palettes](#palettes)
  * [Palette ids as mapgen values](#palette-ids-as-mapgen-values)
* [Using update_mapgen](#using-update_mapgen)
  * [Overmap tile specification](#overmap-tile-specification)
    * ["assign_mission_target"](#assign_mission_target)
    * ["om_terrain"](#om_terrain)
* [Mission specials](#mission-specials)
    * ["target"](#target)
* [Map Extras](#map-extras)
    * ["map_extra"](#map_extra)
    * [Example: mx_science](#example-mx_science)

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

### "om_terrain" for linear terrain

Some overmap terrains are *linear*.  This is used for things like roads,
tunnels, etc. where they form lines which can connect in various ways.  Such
terrains are defined with the `LINEAR` flag in their `overmap_terrain`
definition (see the [OVERMAP docs](OVERMAP.md)).

When defining the JSON mapgen for such terrain, you must define several
instances, for each type of connection that might exist.  Each gets a suffix
added to the `overmap_terrain` id.  The suffixes are: `_end`, `_straight`,
`_curved`, `_tee`, `_four_way`.  For an example, see the definitions for `ants`
in [`ants.json`](../data/json/mapgen/bugs/ants.json).

### Define mapgen "weight"

(optional) When the game randomly picks mapgen functions, each function's weight value determines how rare it is. 1000
is the default, so adding something with weight '500' will make it appear about half as often as others using the default weight. (An insanely high value like 10000000 is useful for testing.)

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
**usually required by "rows"**

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

## Mapgen flags
`"flags"` may provide a list of flags to be applied to the mapgen.

Example:
```json
"flags": [ "ERASE_ALL_BEFORE_PLACING_TERRAIN" ],
```

Currently the defined flags are as follows:

* `ERASE_ALL_BEFORE_PLACING_TERRAIN` and `ALLOW_TERRAIN_UNDER_OTHER_DATA` are
  mutually exclusive flags that can be used with any mapgen which is layered on
  top of existing terrain.  This can be update mapgen, nested mapgen, or
  regular mapgen with a predecessor.  It specifies the behaviour to follow when
  an existing terrain is changed by the update, but the tile has existing
  items, trap, or furniture on it.  If neither flag is provided this is an
  error.  If `ERASE_ALL_BEFORE_PLACING_TERRAIN` is given then any items, trap,
  or furniture will be removed before changing the terrain.  If
  `ALLOW_TERRAIN_UNDER_OTHER_DATA` is given then they will be retained without
  an error.  If you require more fine-grained control over this behaviour than
  can be provided by these flags, then each of these things can be removed
  either individually or together.  See the other entries below, such as
  `remove_all`.
  `NO_UNDERLYING_ROTATE` The map won't be rotated even if the underlying tile is.
  `AVOID_CREATURES` If a creature is present terrain, furniture and traps won't be placed.

## Set terrain, furniture, or traps with a "set" array
**optional** Specific commands to set terrain, furniture, traps, radiation, etc. Array is processed in order.

Value: `[ array of {objects} ]: [ { "point": .. }, { "line": .. }, { "square": .. }, ... ]`

Example:

```json
"set": [
  { "point": "furniture", "id": "f_chair", "x": 5, "y": 10 },
  { "point": "radiation", "id": "f_chair", "x": 12, "y": 12, "amount": 20 },
  { "point": "trap", "id": "tr_beartrap", "x": [ 0, 23 ], "y": [ 5, 18 ], "chance": 10, "repeat": [ 2, 5 ] },
  { "point": "variable", "id": "nether_dungeon_door", "x": 4, "y": 2 }
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
- For other types, requires "id" of terrain, furniture, trap, trap_remove, or name of the global variable

| Field  | Description
| ---    | ---
| point  | Allowed values: `"terrain"`, `"furniture"`, `"trap"`, `"trap_remove"`, `"item_remove"`, `"field_remove"`, `"radiation"`, `"variable"`, `"creature_remove"`
| id     | Terrain, furniture, trap ID or the variable's name. Examples: `"id": "f_counter"`, `"id": "tr_beartrap"`. Omit for "radiation", "item_remove", "creature_remove", and "field_remove". For `trap_remove` if tr_null is used any traps present will be removed.
| x, y   | X, Y coordinates. Value from `0-23`, or range `[ 0-23, 0-23 ]` for a random value in that range. Example: `"x": 12, "y": [ 5, 15 ]`
| amount | Radiation amount. Value from `0-100`.
| chance | (optional) One-in-N chance to apply
| repeat | (optional) Value: `[ n1, n2 ]`. Spawn item randomly between `n1` and `n2` times. Only makes sense if the coordinates are random. Example: `[ 1, 3 ]` - repeat 1-3 times.


### Set things in a "line"

- Requires "line" type, and endpoints "x", "y" and "x2", "y2"
- For "line" type "radiation", requires "amount"
- For other types, requires "id" of terrain, furniture, trap, trap_remove
- creature_remove has no "id" or "amount"

Example:
```json
{ "line": "terrain", "id": "t_lava", "x": 5, "y": 5, "x2": 20, "y2": 20 }
```

| Field  | Description
| ---    | ---
| line   | Allowed values: `"terrain"`, `"furniture"`, `"trap"`, `"radiation"`, `"trap_remove"`, `"item_remove"`, `"field_remove"`, `"creature_remove"`
| id     | Terrain, furniture, or trap ID. Examples: `"id": "f_counter"`, `"id": "tr_beartrap"`. Omit for "radiation", "item_remove", "creature_remove", and "field_remove". For `trap_remove` if tr_null is used any traps present will be removed.
| x, y   | Start X, Y coordinates. Value from `0-23`, or range `[ 0-23, 0-23 ]` for a random value in that range. Example: `"x": 12, "y": [ 5, 15 ]`
| x2, y2 | End X, Y coordinates. Value from `0-23`, or range `[ 0-23, 0-23 ]` for a random value in that range. Example: `"x": 22, "y": [ 15, 20 ]`
| amount | Radiation amount. Value from `0-100`.
| chance | (optional) One-in-N chance to apply
| repeat | (optional) Value: `[ n1, n2 ]`. Spawn item randomly between `n1` and `n2` times. Only makes sense if the coordinates are random. Example: `[ 1, 3 ]` - repeat 1-3 times.


### Set things in a "square"

- Requires "square" type, and opposite corners at "x", "y" and "x2", "y2"
- For "square" type "radiation", requires "amount"
- For other types, requires "id" of terrain, furniture, trap, creature_remove, or trap_remove

The "square" arguments are the same as for "line", but "x", "y" and "x2", "y2" define opposite corners.

Example:
```json
{ "square": "radiation", "amount": 10, "x": [ 0, 5 ], "y": [ 0, 5 ], "x2": [ 18, 23 ], "y2": [ 18, 23 ] }
```

| Field  | Description
| ---    | ---
| square | Allowed values: `"terrain"`, `"furniture"`, `"trap"`, `"radiation"`, `"trap_remove"`, `"item_remove"`, `"field_remove"`, `"creature_remove"`
| id     | Terrain, furniture, or trap ID. Examples: `"id": "f_counter"`, `"id": "tr_beartrap"`. Omit for "radiation", "item_remove", creature_remove, and "field_remove". For `trap_remove` if tr_null is used any traps present will be removed.
| x, y   | Top-left corner of square.
| x2, y2 | Bottom-right corner of square.

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
| one_or_none | Do not allow more than one to spawn due to high spawn density. If repeat is not defined or pack size is defined this defaults to true, otherwise this defaults to false. Ignored when spawning from a group.
| friendly    | Set true to make the monster friendly. Default false.
| name        | Extra name to display on the monster.
| target      | Set to true to make this into mission target. Only works when the monster is spawned from a mission.
| spawn_data  | An optional object that contains additional details for spawning the monster.

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

This places "mon_secubot" at random coordinate (7-18, 7-18). The monster is placed with 30% probability. The placement is
repeated by random number of times `[1-3]`. The monster will spawn with 20-30 5.56x45mm rounds.

### "spawn_data" for monsters
This optional object can have two fields:
| Field       | Description
| ---         | ---
| ammo        | A list of objects, each of which has an `"ammo_id"` field and a `"qty"` list of two integers. The monster will spawn with items of "ammo_id", with at least the first number in the "qty" and no more than the second.
| patrol      | A list of objects, each of which has an `"x"` field and a `"y"` field. Either value can be a range or a single number. The x,y co-ordinates define a patrol point as a relative mapsquare point offset from the (0, 0) local mapsquare of the overmap terrain tile that the monster spawns in. Patrol points are converted to absolute mapsquare tripoints inside the monster generator.

Monsters with a patrol point list will move to each patrol point, in order, whenever they have no more pressing action to take on their turn. Upon reaching the last point in the patrol point list, the monster will continue on to the first point in the list.

Example:
```json
"place_monster": [
    { "monster": "mon_zombie", "x": 12, "y": 12, "spawn_data": { "patrol": [ { "x": 12, "y": 12 } ] } }
]
```

This places a "mon_zombie" at (12, 12). The zombie can move freely to chase after enemies, but will always return to the (12, 12) position if it has nothing else to do.

Example 2:
```json
"place_monster": [
    { "monster": "mon_secubot", "x": 12, "y": 12, "spawn_data": { "ammo": [ { "ammo_id": "556", "qty": [ 20, 30 ] } ], "patrol": [ { "x": -23, "y": -23 }, { "x": 47, "y": -23 }, { "x": 47, "y": 47 },  { "x": 47, "y": -23 } ] } }
]
```
This places a "mon_secubot" at (12,12). It will patrol the four outmost concerns of the diagonally adjacent overmap terrain tiles in a box pattern.

## Spawn an entire group of monsters with "place_monsters"
Using `place_monsters` to spawn a group of monsters works in a similar fashion to `place_monster`. The key difference is that `place_monsters` guarantees that each valid entry in the group is spawned. It is strongly advised that you avoid using this flag with larger monster groups, as the total number of spawns is quite difficult to control.

|Field|Description  |
|--|--|
| monster | The ID of the monster group that you wish to spawn |
| x, y        | Spawn coordinates ( specific or area rectangle ). Value: 0-23 or `[ 0-23, 0-23 ]` - random value between `[ a, b ]`.
| chance      | Represents a 1 in N chance that the entire group will spawn. This is done once for each repeat. If this dice roll fails, the entire group specified will not spawn. Leave blank to guarantee spawns.
| repeat      | The spawning is repeated this many times. Can be a number or a range. Again, this represents the number of times the group will be spawned.
| density | This number is multiplied by the spawn density of the world the player is in and then probabilistically rounded to determine how many times to spawn the group. This is done for each time the spawn is repeated. For instance, if the final multiplier from this calculation ends up being `2`, and the repeat value is `6`, then the group will be spawned `2 * 6` or 12 times.


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
| custom-flags | (optional) Value: `[ "flag1", "flag2" ]`. Spawn item with specific flags.

The special custom flag "ACTIVATE_ON_PLACE" causes the item to be activated as it is placed.  This is useful to have noisemakers that are already turned on as the avatar approaches.  It can also be used with explosives with a 1 second countdown to have locations explode as the avatar approaches, creating uniquely ruined terrain.

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

The amount of water to be placed in toilets is optional, an empty entry is therefore completely valid.

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

Example:

```json
"place_fields": [ { "field": "fd_blood", "x": 0, "y": 0, "intensity": [ 1, 3 ], "repeat": [ 0, 3 ] } ]

"place_fields": [ { "field": "fd_blood", "x": 0, "y": 0, "intensity": 1, "repeat": [ 0, 3 ] } ]
```

| Field     | Description
| ---       | ---
| field     | (required, string) the field type (e.g. `"fd_blood"`, `"fd_smoke"`)
| density   | (optional, integer) field density. Defaults to 1. Possible values are 1, 2, or 3.
| intensity | (optional, integer, array ) how concentrated the field is, from 1 to 3 or more.  Arrays are randomized.  See `data/json/field_type.json`
| age       | (optional, integer) field age. Defaults to 0.
| remove    | (optional, bool) If true the given field will be removed rather than added. Defaults to false.


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
| unique_id | (optional, string) This is a unique id to descibe the npc, if an npc with this id already exists the command will silently fail.


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

Places a vending machine (furniture) and fills it with items from an item group.

| Field      | Description
| ---        | ---
| item_group | (optional, string) the item group that is used to create items inside the machine. It defaults to either "vending_food" or "vending_drink" (randomly chosen).
| reinforced | (optional, bool) setting which will make vending machine spawn as reinforced. Defaults to false.
| lootable   | (optional, bool) setting which indicates whether this particular vending machine should have a chance to spawn ransacked (i.e. broken and with no loot inside). The chance for this is increased with each day passed after the Cataclysm. Valid only if `reinforced` is false. Defaults to false.


### Place a toilet with some amount of water with "toilets"

Places a toilet (furniture) and adds water to it.

| Field  | Description
| ---    | ---
| amount | (optional, integer or min/max array) the amount of water to be placed in the toilet.


### Place a gas or diesel pump with some fuel with "gaspumps"

Places a gas pump with fuel in it.

| Field  | Description
| ---    | ---
| amount | (optional, integer or min/max array) the amount of fuel to be placed in the pump. If not specified, the amount is randomized between 10'000 and 50'000.
| fuel   | (optional, string: "gasoline", "diesel", "jp8", or "avgas") the type of fuel to be placed in the pump. If not specified, the fuel is gasoline (75% chance) or diesel (25% chance).


### Place items from an item group with "items"

| Field   | Description
| ---     | ---
| item    | (required, string or itemgroup object) the item group to use.
| chance  | (optional, integer or min/max array) x in 100 chance that a loop will continue to spawn items from the group (which itself may spawn multiple items or not depending on its type, see `ITEM_SPAWN.md`), unless the chance is 100, in which case it will trigger the item group spawn exactly 1 time (see `map::place_items`).
| repeat  | (optional, integer or min/max array) the number of times to repeat this placement, default is 1.
| faction | (optional, string) the faction that owns these items.


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
| faction  | (optional, string) faction this vehicle belongs to.

Note that vehicles cannot be placed over overmap boundaries. So it needs to be 24 tiles long at most.

```json
"place_vehicles": [
    { "vehicle": "fire_engine", "x": 11, "y": 13, "chance": 30, "rotation": 270 }
]
```

### Remove vehicles by type

| Field    | Description
| ---      | ---
| vehicles  | (optional, string array) types of vehicle to be removed. If left empty all vehicles will be removed.

```json
"remove_vehicles": [
    { "vehicles": [ "fire_engine" ], "x": [ 10, 15 ], "y": [ 10, 15 ] }
]
```

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
| remove| (optional, bool) If true the given trap will be removed rather than added. Defaults to false.

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
pumps, or unless terrain the liquid spilled on has `LIQUIDCONT` flag), but can be used to add flavor to mapgen.

| Field  | Description
| ---    | ---
| liquid | (required, item id) the item (a liquid)
| amount | (optional, integer/min-max array) amount of liquid to place (a value of -1 defaults to the item's default charges)
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
| variant  | (optional, string), itype variant id for the spawned item


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
| type    | (required, string) Values: `"NPC_RETREAT"`, `"NPC_NO_INVESTIGATE"`, or `"NPC_INVESTIGATE_ONLY"`, or `LOOT_xxx`
| faction | (required, string) the faction id of the NPC faction that will use the zone.
| name    | (optional, string) the name of the zone.
| filter  | (optional, string) used as filter for `LOOT_CUSTOM`, or as group id for `LOOT_ITEM_GROUP`

The `type` field values affect NPC behavior. NPCs will:

- Prefer to retreat towards `NPC_RETREAT` zones.
- Not move to see the source of unseen sounds coming from `NPC_NO_INVESTIGATE` zones.
- Not move to see the source of unseen sounds coming from outside of `NPC_INVESTIGATE_ONLY` zones.
- Use `LOOT_xxx` zones for their shop (see [NPCs.md#Shop_restocking](NPCs.md#Shop-restocking))

Single-point loot zones that overlap cargo vehicle parts will be placed as vehicle zones.

Zone placements can be debugged in game by turning on debug mode and changing `F`action in the Zones Manager.

### Specify a player spawning location using "zones"

When designing a scenario you can directly specify where in the map the player will be placed by using a `ZONE_START_POINT` zone. Player will be placed in the center of this zone. A `ZONE_START_POINT` zone will only be considered valid if it belongs to the `your_followers` faction. Keep in mind that no additional checks are conducted when assigning player spawning location using this method, and thus player can spawn in a wall, on open air, and other inappropriate tiles.

### Remove everything with "remove_all"

This has no additional fields, and will remove all fields, items, traps,
graffiti, and furniture from a tile.  This can be useful in e.g. nests or other
update mapgen to clear out existing stuff that might exist but wouldn't make
sense in the nest.


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

### Spawn nested chunks based on overmap neighbors with "place_nested"

Place_nested allows for limited conditional spawning of chunks based on the `"id"`s of their overmap neighbors and the joins that were used in placing a mutable overmap special.  This is useful for creating smoother transitions between biome types or to dynamically create walls at the edges of a mutable structure.

| Field              | Description
| ---                | ---
| chunks/else_chunks | (required, string) the nested_mapgen_id of the chunk that will be conditionally placed. Chunks are placed if the specified neighbor matches, and "else_chunks" otherwise.
| x and y            | (required, int) the cardinal position in which the chunk will be placed.
| neighbors          | (optional) Any of the neighboring overmaps that should be checked before placing the chunk.  Each direction is associated with a list of overmap `"id"` substrings.
| joins              | (optional) Any mutable overmap special joins that should be checked before placing the chunk.  Each direction is associated with a list of join `"id"` strings.


The adjacent overmaps which can be checked in this manner are:
* the direct cardinal neighbors ( `"north"`, `"east"`, `"south"`, `"west"` ),
* the inter cardinal neighbors ( `"north_east"`, `"north_west"`, `"south_east"`, `"south_west"` ),
* the direct vertical neighbors ( `"above"`, `"below"` ).

Joins can be checked only for the cardinal directions, `"above"`, and `"below"`

Example:

```json
  "place_nested": [
    { "chunks": [ "concrete_wall_ew" ], "x": 0, "y": 0, "neighbors": { "north": [ "empty_rock", "field" ] } },
    { "chunks": [ "gate_north" ], "x": 0, "y": 0, "joins": { "north": [ "interior_to_exterior" ] } },
    { "else_chunks": [ "concrete_wall_ns" ], "x": 0, "y": 0, "neighbors": { "north_west": [ "field", "microlab" ] } }
  ],
```
The code excerpt above will place chunks as follows:
* `"concrete_wall_ew"` if the north neighbor is either a field or solid rock
* `"gate_north"` if the join `"interior_to_exterior"` was used to the north
  during mutable overmap placement.
* `"concrete_wall_ns"`if the north west neighbor is neither a field nor any of the microlab overmaps.


### Place monster corpse from a monster group with "place_corpses"

Creates a corpse of a random monster from a monster group.

| Field  | Description
| ---    | ---
| group  | (required, string) a monster group id from which random monster will be selected
| age    | (optional, integer) age (in days) of monster's corpse. If not set, defaults to current turn.

Example for placing a monster corpse (either by using a character in the rows array or explicit coordinates):

```json
"corpses": {
    "g": { "group": "GROUP_PETS", "age": 3 }
},
"place_corpses": [
    { "group": "GROUP_PETS", "x": 3, "y": 5 }
],
```


### Place computer console with "computers" or "place_computers"

Creates a `f_console` furniture. Despite the only required field is `name`, you should also define either `options` or (`eocs` and `chat_topics`) to make interaction with the computer sensible.

| Field  | Description
| ---    | ---
| name          | (required, string) a name for a computer
| options       | (optional, array of objects) set of options available for player. `name` (string) - displayed name for an option. `action` - id of a hardcoded action from `computer_session.cpp`
| failures      | (optional, array of objects) set of failures that could happen in case of unsuccessful hacking attempt. `action` - id of a hardcoded computer failure from `computer_session.cpp`. Only one random failure from the set could happen per one unsuccessful hacking attempt
| security      | (optional, integer) value for determining the difficulty of hacking this computer. It's checked versus player's computer skill and intelligence
| access_denied | (optional, string) message displayed to the player if `security` > 0. Defaults to `"ERROR!  Access denied!"`
| eocs          | (optional, array of strings) a name for an `effect` that will shoot when player examines the computer
| chat_topics   | (optional, array of strings) conversation topics if dialog is opened with the computer


Example for placing computer console (either by using a character in the rows array or explicit coordinates):

```json
"computers": {
  "a": {
    "name": "Test computer 1",
    "security": 3,
    "options": [ { "name": "Test unlock action", "action": "unlock" } ],
    "failures": [ { "action": "shutdown" }, { "action": "alarm" } ],
    "access_denied": "ERROR!  Access denied!  Unauthorized access will be met with lethal force!"
  }
},
"place_computers": [
  {
    "name": "Test computer 2",
    "eocs": [ "EOC_REFUGEE_CENTER_COMPUTER" ],
    "chat_topics": [ "COMP_REFUGEE_CENTER_MAIN" ],
    "x": 20,
    "y": 20
  }
],
```

## Mapgen values

A *mapgen value* can be used in various places where a specific id is expected.
For example, the default value of a parameter, or a terrain id in the
`"terrain"` object.  A mapgen value can take one of three forms:

* A simple string, which should be a literal id.  For example, `"t_flat_roof"`.
* A JSON object containing the key `"distribution"`, whose corresponding value
  is a list of lists, each a pair of a string id and an integer weight.  For
  example:
```
{ "distribution": [ [ "t_flat_roof", 2 ], [ "t_tar_flat_roof", 1 ], [ "t_shingle_flat_roof", 1 ] ] }
```
* A JSON object containing the key `"param"`, whose corresponding value is the
  string name of a parameter as discussed in [Mapgen
  parameters](#mapgen-parameters).  For example, `{ "param": "roof_type" }`.
  You may be required to also supply a fallback value, such as `{ "param":
  "roof_type", "fallback": "t_flat_roof" }`.  The fallback is necessary to
  allow mapgen definitions to change without breaking an ongoing game.
  Different parts of the same overmap special can be generated at different
  times, and if a new parameter is added to the definition part way through the
  generation then the value of that parameter will be missing and the fallback
  will be used.
* A switch statement to select different values depending on the value of some
  other mapgen value.  This would most often be used to switch on the value of
  a mapgen parameter, so as to allow two parts of the mapgen to be consistent.
  For example, the following switch would match a fence gate type to a fence
  type chosen by a mapgen parameter `fence_type`:
```json
{
    "switch": { "param": "fence_type", "fallback": "t_splitrail_fence" },
    "cases": {
        "t_splitrail_fence": "t_splitrail_fencegate_c",
        "t_chainfence": "t_chaingate_c",
        "t_fence_barbed": "t_gate_metal_c",
        "t_privacy_fence": "t_privacy_fencegate_c"
    }
}
```


## Mapgen parameters

(Note that this feature is under development and functionality may not line up exactly
with the documentation.)

Another entry within a mapgen definition or [palette](#palettes) can be a `"parameters"`
key.  For example:
```
"parameters": {
  "roof_type": {
    "type": "ter_str_id",
    "default": { "distribution": [ [ "t_flat_roof", 2 ], [ "t_tar_flat_roof", 1 ], [ "t_shingle_flat_roof", 1 ] ] }
  }
},
```

Each entry in the `"parameters"` JSON object defines a parameter.  The key is
the parameter name.  Each such key should have an associated JSON object.  That
object must provide its type (which should be a type string as for a
`cata_variant`) and may optionally provide a default value.  The default value
should be a [mapgen value](#mapgen-values) as defined above.

At time of writing, the only way for a parameter to get a value is via the
`"default"`, so you probably want to always have one.

The primary application of parameters is that you can use a `"distribution"`
mapgen value to select a value at random, and then apply that value to every
use of that parameter.  In the above example, a random roof terrain is picked.
By using the parameter with some `"terrain"` key, via a `"param"` mapgen value,
you can use a random but consistent choice of roof terrain across your map.
In contrast, placing the `"distribution"` directly in the `"terrain"` object would
cause mapgen to choose a terrain at random for each roof tile, leading to a
mishmash of roof terrains.

By default, the scope of a parameter is the `overmap_special` being generated.
That is, the parameter will have the same value across the `overmap_special`.
When a default value is needed, it will be chosen when the first chunk of that
special is generated, and that value will be saved to be reused for later
chunks.

If you wish, you may specify `"scope": "omt"` to limit the scope to just a
single overmap tile.  Then a default value will be chosen independently for
each OMT.  This has the advantage that you are no longer forced to select a
`"fallback"` value when using that parameter in mapgen.

The third option for scope is `"scope": "nest"`.  This only makes sense when
used in nested mapgen (although it is not an error to use it elsewhere, so that
the same palette may be used for nested and non-nested mapgen).  When the scope
is `nest`, the value of the parameter is chosen for a particular nested chunk.
For example, suppose a nest defines a carpet across several tiles, you can use
a parameter to ensure that the carpet is the same colour for all the tiles
within that nest, but another instance of the same `nested_mapgen_id` elsewhere
in the same OMT might choose a different colour.

To help you debug mapgen parameters and their effect on mapgen, you can see the
chosen values for `overmap_special`-scoped parameters in the overmap editor
(accessible via the debug menu).


## Rotate the map with "rotation"

Rotates the generated map after all the other mapgen stuff has been done. The value can be a single integer or a range
(out of which a value will be randomly chosen). Example: `"rotation": [ 0, 3 ]`

Values are 90° steps.


## Pre-load a base mapgen with `"predecessor_mapgen"`

Specifying an overmap terrain id here will run the entire mapgen for that overmap terrain type first, before applying
the rest of the mapgen defined here. The primary use case for this is when our mapgen for a location takes place in a
natural feature like a forest, swamp, or lake shore. Many existing JSON mapgen attempt to emulate the mapgen of the type
they're being placed on (e.g. a cabin in the forest has placed the trees, grass and clutter of a forest to try to make
the cabin fit in) which leads to them being out of sync when the generation of that type changes. By specifying the
`predecessor_mapgen`, you can instead focus on the things that are added to the existing location type.

Example: `"predecessor_mapgen": "forest"`


## Dynamically use base mapgen with `"fallback_predecessor_mapgen"`

If your map could exist in a variety of surroundings, you might want to
automatically take advantage of the mapgen for whatever terrain was assumed to
be here before this one was set.  For example, overmap specials are always
placed over existing terrain like fields and forests.

Defining `"fallback_predecessor_mapgen"` allows your map to opt-in to
requesting that whatever that previous terrain was, it should be generated
first, before your mapgen is applied on top.  This works the same as for
`"predecessor_mapgen"` above, except that it will pick the correct terrain for
the context automatically.

However, to support savegame migration across changes to mapgen definitions,
you must provide a fallback value which will be used in the event that the game
doesn't know what the previous terrain was.

Example: `"predecessor_mapgen": "field"`


# Palettes

A **palette** provides a way to use the same symbol definitions for different
pieces of mapgen.  For example, most of the houses defined in CDDA us the
`standard_domestic_palette`.  That palette, for example, defines `h` as meaning
`f_chair`, so all the house mapgen can use `h` in its `"rows"` array without
needing to repeat this definition everywhere.  It simply requires a reference
to the palette, achieved by adding

```json
"palettes": [ "standard_domestic_palette" ]
```

to the definition of each house.

Each piece of mapgen can refer to multiple palettes.  When two palettes both
define meanings for the same symbol, both are applied.  In some cases (such as
spawning items) you can see the results of both in the final output.  In other
cases (such as setting terrain or furniture) one result must override the
others.  The rule is that the last palette listed overrides earlier ones, and
definitions in the outer mapgen override anything in the palettes within.

Palette definitions can contain any of the JSON described above for the [JSON
object definition](#json-object-definition) where it is defining a meaning for
a symbol.  They cannot specify anything for a particular location (using `"x"`
and `"y"` coordinates.

Palettes can themselves include other palettes via a `"palettes"` key.  So if
two or more palettes would have many of the same symbols with the same meanings
that common part can be pulled out into a new palette which each of them
includes, so that the definitions need not be repeated.


## Palette ids as mapgen values

The values in the `"palettes"` list need not be simple strings.  They can be
any [mapgen value](#mapgen-values) as described above.  Most importantly, this
means that they can use a `"distribution"` to select from a set of palettes at
random.

This selection works as if it were an overmap special-scoped [mapgen
parameter](#mapgen-parameters).  So, all OMTs within a special will use the
same palette.  Moreover, you can see which palette was chosen by looking at the
overmap special arguments displayed in the overmap editor (accessible via the
debug menu).

For example, the following JSON used in a cabin mapgen definition
```json
      "palettes": [ { "distribution": [ [ "cabin_palette", 1 ], [ "cabin_palette_abandoned", 1 ] ] } ],
```
causes half the cabins generated to use the regular `cabin_palette` and the
other half to use `cabin_palette_abandoned`.

# Using `update_mapgen`

**update_mapgen** is a variant of normal JSON mapgen.  Instead of creating a new overmap tile, it
updates an existing overmap tile with a specific set of changes.

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

# Map Extras

Map extras can be used to place environmental objects and structures that can help create some emergent storytelling. These are placed randomly while exploring the overmap, and can range from simple ponds and groves to full-on crashed spaceships with enemies or NPCs.

### "map_extra"

```JSON
{
  "id": "mx_minefield",
  "type": "map_extra",
  "name": { "str": "Minefield" },
  "description": "Mines are scattered here.",
  "generator": { "generator_method": "map_extra_function", "generator_id": "mx_minefield" },
  "min_max_zlevel": [ 0, 0 ],
  "sym": "M",
  "color": "red",
  "autonote": true,
  "flags": [ "MAN_MADE" ]
}
```

| Field          | Description
| ---            | ---
| generator      | (_optional_) An object defining how and what this extra should spawn.
| min_max_zlevel | (_optional_) A pair of integers defining the minimum and maximum zlevel in which this extra can spawn. Defaults to none (can spawn at any zlevel).
| sym            | (_optional_) The symbol to use when marking this extra on the overmap. Defaults to no symbol.
| color          | (_optional_) The color of the symbol identifying this extra on the overmap. Defaults to white.
| autonote       | (_optional_) Whether to automatically mark this extra on the overmap. Defaults to false.
| flags          | (_optional_) List of flags that identify this extra. These flags can be listed in `overmap_feature_flag_settings` to blacklist or whitelist map extras.

The `generator` can use one of 3 methods:

| Method               | Description
| ---                  | ---
| `map_extra_function` | The `generator_id` points to a builtin function to generate the extra. See the `builtin_functions` function map in map_extras.cpp for the current list.
| `mapgen`             | The `generator_id` points to a `om_terrain` string within a `mapgen` definition.
| `update_mapgen`      | The `generator_id` points to a `update_mapgen_id` string within a `mapgen` definition.

### Example: mx_science

The `mx_science` map extra spawns bodies of scientists as well as a few enemy mobs. The map extra definition is as follows:

```JSON
{
  "id": "mx_science",
  "type": "map_extra",
  "name": { "str": "Scientists" },
  "description": "Several corpses of scientists are here.",
  "generator": { "generator_method": "update_mapgen", "generator_id": "mx_science" },
  "min_max_zlevel": [ -5, 0 ],
  "sym": "s",
  "color": "light_red",
  "autonote": true
}
```

In this case the `generator_id` points to a `mapgen` definition that establishes what objects or structures will spawn at that location:

```JSON
{
  "type": "mapgen",
  "method": "json",
  "update_mapgen_id": "mx_science",
  "object": {
    "rows": [
      "----------------------  ",
      "----------------------  ",
      "----------------------  ",
      "----------------------  ",
      "----------------------  ",
      "----------------------  ",
      "----------------------  ",
      "----------------------  ",
      "----------------------  ",
      "----------------------  ",
      "----------------------  ",
      "-----------1----------  ",
      "----------------------  ",
      "----------------------  ",
      "----------------------  ",
      "----------------------  ",
      "----------------------  ",
      "----------------------  ",
      "----------------------  ",
      "----------------------  ",
      "----------------------  ",
      "----------------------  ",
      "                        ",
      "                        "
    ],
    "monsters": {
      " ": { "monster": "GROUP_NETHER_CAPTURED", "chance": 1, "density": 0.0001 },
      "-": { "monster": "GROUP_NETHER_CAPTURED", "chance": 1, "density": 0.0001 }
    },
    "nested": {
      "-": { "chunks": [ [ "corpse_blood_gibs_science_3x3", 1 ], [ "null", 150 ] ] },
      "1": { "chunks": [ "corpse_blood_gibs_science_3x3" ] }
    }
  }
}
```

The nested chunks define the items and fields that may spawn:

```JSON
{
  "type": "mapgen",
  "method": "json",
  "nested_mapgen_id": "corpse_blood_gibs_science_3x3",
  "object": {
    "mapgensize": [ 3, 3 ],
    "place_items": [ { "item": "map_extra_science", "x": [ 0, 2 ], "y": [ 0, 2 ], "chance": 100 } ],
    "place_fields": [ { "field": "fd_blood", "x": [ 0, 2 ], "y": [ 0, 2 ] }, { "field": "fd_gibs_flesh", "x": [ 0, 2 ], "y": [ 0, 2 ] } ]
  }
}
```

In order for the map extra to be available for spawning, one or more `region_settings` entries need to be created for it:

```JSON
{
  "type": "region_settings",
  "id": "default",
  ...
  "map_extras": {
    "forest": {
      "chance": 20,
      "extras": {
        "mx_helicopter": 1,
        "mx_military": 8,
        "mx_science": 20,
        "mx_collegekids": 25,
        ...
      }
    }
  }
}

```
