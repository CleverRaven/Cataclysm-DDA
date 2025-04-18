# Overmap Generation

## Overview

The **overmap** is what you see in the game when you use the _view map (m)_ command.

It looks something like this:

```
...>│<......................│..............P.FFFF├─FF...
..┌─┼─┐.....................│..............FFFFF┌┘FFFF..
..│>│^│.....................│..............FF┌─┬┘FFFFF.F
──┘...└──┐..............>│<.│..............F┌┘F│FFFFFFF─
.........└──┐........vvO>│v.│............FF┌┘FF│FFFFFFFF
............└┐.......─┬──┼─>│<........┌─T──┘FFF└┐FFFFFFF
.............│.......>│<$│S.│<.......┌┘..FFFFFFF│FFF┌─┐F
..O.........┌┴──┐....v│gF│O.│<v......│...FFFFFF┌┘F.F│F└─
.OOOOO.....─┘...└─────┼──┼──┼────────┤....FFFFF│FF..│FFF
.O.O.....dddd.......^p│^>│OO│<^......└─┐FFFFFFF├────┴─FF
.........dddd........>│tt│<>│..........│FFF..FF│FFFFF.FF
.........dddd......┌──┘tT├──...........│FFF..F┌┘FFFFFFFF
.........dddd......│....>│^^..H........└┐...FF│FFFFFFFFF
..................┌┘.....│<.............└─┐FF┌┘FFFFFFFFF
..................│......│................│FF│FFFFFFFFFF
.................┌┘......│................│F┌┘FFFFFFFFFF
...FFF...........│......┌┘...............┌┘.│FFFFFFFFFFF
FFFFFF...........│.....┌┘................│FF│FFFFFFFFFFF
FFFFFF..FFFF.....│.....B..........─┐.....│┌─┘F..FFFFFFFF
```

In the context of **overmap generation**, the **overmap** is the collection of data and features
that define the locations in the game world at a more abstract level than the local map that the
player directly interacts with, providing the necessary context for local map generation to occur.

By example, the overmap tells the game:

* where the cities are
* where the roads are
* where the buildings are
* what types the buildings are

but it does not tell the game:

* what the actual road terrain looks like
* what the actual building layout looks like
* what items are in the building

It can sometimes be useful to think of the overmap as the outline for the game world which will then
be filled in as the player explores. The rest of this document is a discussion of how we can create
that outline.

### Overmap generation

Generating an overmap happens in the following sequence of functions ( see generate::overmap in overmap.cpp ):

	populate_connections_out_from_neighbors( north, east, south, west );
    place_rivers( north, east, south, west );
    place_lakes();
    place_forests();
    place_swamps();
    place_ravines();
    place_cities();
    place_forest_trails();
    place_roads( north, east, south, west );
    place_specials( enabled_specials );
    place_forest_trailheads();
    polish_river();
    place_mongroups();
    place_radios();

This has some consequences on overmap special spawning, as discussed in the relevant section.

## Terminology and Types

First we need to briefly discuss some of the data types used by the
overmap.

### overmap_terrain

The fundamental unit of the overmap is the **overmap_terrain**, which defines the id, name, symbol, color
(and more, but we'll get to that) used to represent a single location on the overmap. The overmap is 180
overmap terrains wide, 180 overmap terrains tall, and 21 overmap terrains deep (these are the z-levels).

In this example snippet of an overmap, each character corresponds one entry in the overmap which
references a given overmap terrain:

```
.v>│......FFF│FF
──>│<....FFFF│FF
<.>│<...FFFFF│FF
O.v│vv.vvv┌──┘FF
───┼──────┘.FFFF
F^^|^^^.^..F.FFF
```

So for example, the `F` is a forest which has a definition like this:

```jsonc
{
    "type": "overmap_terrain",
    "id": "forest",
    "name": "forest",
    "sym": "F",
    "color": "green"
}
```

and the `^` is a house which has a definition like this:

```jsonc
{
    "type": "overmap_terrain",
    "id": "house",
    "name": "house",
    "sym": "^",
    "color": "light_green"
}
```

It's important to note that a single overmap terrain definition is referenced for every usage of that
overmap terrain in the overmap--if we were to change the color of our forest from `green` to `red`, every
forest in the overmap would be red.

### overmap_special / city_building

The next important concept for the overmap is that of the **overmap_special** and
**city_building**. These types, which are similar in structure, are the mechanism by which multiple
overmap terrains can be collected together into one conceptual entity for placement on the overmap.
If you wanted to have a sprawling mansion, a two story house, a large pond, or a secret lair placed
under an unassuming bookstore, you would need to use an **overmap_special** or a **city_building**.

These types are effectively a list of the overmap terrains that compose them, the placement of
those overmap terrains relative to each other, and some data used to drive the placement of the
overmap special / city building (e.g. how far from a city, should be it connected to a road, can it
be placed in a forest/field/river, etc).

When retiring or migrating an overmap special, the id should be defined in a new `overmap_special_migration`,
usually defined in `data/json/overmap/overmap_special/overmap_special_migration.json` as follows:

```jsonc
{
  "type": "overmap_special_migration",
  "id": "Military Bunker",
  "new_id": "military_bunker"
}
```

Specials that have been removed (not just renamed) should have a blank `new_id`, or just omit
the `new_id` field altogether.

### overmap_connection

Speaking of roads, the concept of linear features like roads, sewers, subways, railroads, and
forest trails are managed through a combination of overmap terrain attributes and another type
called an **overmap_connection**.

An overmap connection effectively defines the types of overmap terrain on which a given connection
can be made, the "cost" to make that connection, and the type of terrain to be placed when making
the connection. This, for example, is what allows us to say that:

* a road may be placed on fields, forests, swamps and rivers
* fields are preferred over forests, which are preferred over swamps, which are preferred over rivers
* a road crossing a river will be a bridge

The **overmap_connection** data will then be used to create a linear road feature connecting two points,
applying the previously defined rules.

### overmap_location

We've previously mentioned defining valid overmap terrain types for the placement of overmap specials,
city buildings, and overmap connections, but one thing to clarify is these actually leverage another
type called an **overmap_location** rather than referencing **overmap_terrain** values directly.

Simply put, an **overmap_location** is just a named collection of **overmap_terrain** values.

For example, here are two simple definitions.

```jsonc
{
    "type": "overmap_location",
    "id": "forest",
    "terrains": [ "forest" ]
},
{
    "type": "overmap_location",
    "id": "wilderness",
    "terrains": [ "forest", "field" ]
}
```

The value of these is that they allow for a given overmap terrain to belong to several different
locations and provide a level of indirection so that when new overmap terrains are added (or
existing ones removed), only the relevant **overmap_location** entries need to be changed rather
than every **overmap_connection**, **overmap_special**, and **city_building** that needs those
groups.

For example, with the addition of a new forest overmap terrain `forest_thick`, we only have to
update these definitions as follows:

```jsonc
{
    "type": "overmap_location",
    "id": "forest",
    "terrains": [ "forest", "forest_thick" ]
},
{
    "type": "overmap_location",
    "id": "wilderness",
    "terrains": [ "forest", "forest_thick", "field" ]
}
```

### oter_vision

When the overmap is displayed to the player through the map screen, tiles can be displayed with
varying degrees of information. There are five levels of overmap vision, each providing progressively
more detail about the shown tile. They are as follows:

- unseen, no information is known about the tile
- vague, only cursory details such as from a quick glance - broad features
- outlines, able to distinguish large/visible features
- details, features that are harder to spot become visible
- full, all information provided in the **overmap_terrain** is provided

The information on how to display the middle three vision levels is provided in a **oter_vision**
definition.

## Overmap Terrain

### Rotation

If an overmap terrain can be rotated (i.e. it does not have the `NO_ROTATE` flag), then when
the game loads the definition from JSON, it will create the rotated definitions automatically,
suffixing the `id` defined here with `_north`, `_east`, `_south` or `_west`. This will be
particularly relevant if the overmap terrains are used in **overmap_special** or **city_building**
definitions, because if those are allowed to rotate, it's desirable to specify a particular
rotation for the referenced overmap terrains (e.g. the `_north` version for all).

### Fields

|    Identifier     |                                           Description                                            |
| ----------------- | ------------------------------------------------------------------------------------------------ |
| `type`            | Must be `"overmap_terrain"`.                                                                     |
| `id`              | Unique id.                                                                                       |
| `name`            | Name for the location shown in game.                                                             |
| `sym`             | Symbol used when drawing the location, like `"F"` (or you may use an ASCII value like `70`).     |
| `color`           | Color to draw the symbol in. See [COLOR.md](COLOR.md).                                           |
| `looks_like`      | Id of another overmap terrain to be used for the graphical tile, if this doesn't have one.       |
| `vision_levels`   | Id of a `oter_vision` that describes how this overmap terrain will be displayed when there is not full vision of the tile.
| `connect_group`   | Specify that this overmap terrain might be graphically connected to its neighbours, should a tileset wish to.  It will connect to any other `overmap_terrain` with the same `connect_group`. |
| `see_cost`        | Affects player vision on overmap.  See table below for possible values.                          |
| `travel_cost_type` | How to treat this location when planning a route using autotravel on the overmap. Valid values are `road`,`field`,`dirt_road`,`trail`,`forest`,`shore`,`swamp`,`water`,`air`,`impassable`,`other`. Some types are harder to travel through with different types of vehicles, or on foot. |
| `extras`          | Reference to a named `map_extras` in region_settings, defines which map extras can be applied.   |
| `mondensity`      | Summed with values for adjacent overmap terrains to influence density of monsters spawned here.  |
| `spawns`          | Spawns added once at mapgen. Monster group, % chance, population range (min/max).                |
| `flags`           | See `Overmap terrains` in [JSON_FLAGS.md](JSON_FLAGS.md).                                        |
| `uniform_terrain` | Specifies the terrain that should fill this overmap terrain in place of a JSON mapgen with fill ter for very common maps to save on load time. |
| `mapgen`          | Specify a C++ mapgen function. Don't do this--use JSON.                                          |
| `mapgen_straight` | Specify a C++ mapgen function for a LINEAR feature variation. Prefer JSON instead.               |
| `mapgen_curved`   | Specify a C++ mapgen function for a LINEAR feature variation. Prefer JSON instead.               |
| `mapgen_end`      | Specify a C++ mapgen function for a LINEAR feature variation. Prefer JSON instead.               |
| `mapgen_tee`      | Specify a C++ mapgen function for a LINEAR feature variation. Prefer JSON instead.               |
| `mapgen_four_way` | Specify a C++ mapgen function for a LINEAR feature variation. Prefer JSON instead.               |
| `eoc`             | Supply an effect_on_condition id or an inline effect_on_condition.  The condition of the eoc will be tested to see if the special can be placed.  The effect of the eoc will be run when the special is placed.  See [effect_on_condition.md](effect_on_condition.md). |
| `entry_eoc`       | An effect on condition ID that will run when you enter this location.                            |
| `exit_eoc`        | An effect on condition ID that will run when you exit this location.                             |

### `see_cost` values

| name | role |
|------|------|
| `"all_clear"` | This tile has no or minimal horizontal obstacles and can be seen down through |
| `"none"` | This tile has no or minimal horizontal obstacles - most flat terrain |
| `"low"` | This tile has low horizontal obstacles or few higher obstacles |
| `"medium"` | This tile has medium horizontal obstacles |
| `"spaced_high"` | This tile has high obstacles, but they are spaced and have several gaps - a forest |
| `"high"` | This tile has high obstacles, but still allows some sight around it - most buildings |
| `"full_high"` | This tile has high obstacles, and effectively cannot be seen through - multi-tile buildings |
| `"opaque"` | This tile cannot be seen through under any circumstance |

### Example

A real `overmap_terrain` wouldn't have all these defined at the same time, but in the interest of
an exhaustive example...

```jsonc
{
    "type": "overmap_terrain",
    "id": "field",
    "name": "field",
    "sym": ".",
    "color": "brown",
    "looks_like": "forest",
    "vision_levels": "open_land",
    "connect_group": "forest",
    "see_cost": "spaced_high",
    "travel_cost_type": "field",
    "extras": "field",
    "mondensity": 2,
    "spawns": { "group": "GROUP_FOREST", "population": [ 0, 1 ], "chance": 13 },
    "flags": [ "NO_ROTATE" ],
    "uniform_terrain": "t_grass",
    "mapgen": [ { "method": "builtin", "name": "bridge" } ],
    "mapgen_straight": [ { "method": "builtin", "name": "road_straight" } ],
    "mapgen_curved": [ { "method": "builtin", "name": "road_curved" } ],
    "mapgen_end": [ { "method": "builtin", "name": "road_end" } ],
    "mapgen_tee": [ { "method": "builtin", "name": "road_tee" } ],
    "mapgen_four_way": [ { "method": "builtin", "name": "road_four_way" } ],
    "eoc": {
      "id": "EOC_REFUGEE_CENTER_GENERATE", 
      "condition": { "math": [ "refugee_centers < 1" ] }, 
      "effect": [ { "math": [ "refugee_centers", "++" ] } ]
    },
    "entry_eoc": "EOC_ENTERED_SECRET_FIELD",
    "exity_eoc": "EOC_LEFT_SECRET_FIELD"
}
```

## Overmap Vision

### Fields

| Identifier | Description |
|------------|-------------|
| type       | Must be `oter_vision` |
| id         | Identifier of this `oter_vision`. Cannot contain a `$`. |
| levels     | Array of vision levels. Between 0 and 3 can be specified. The information is specified in the order of vague, outlines, detailed |

For levels, each entry is a JSON object with the following fields

| Identifier | Description |
|------------|-------------|
| name       | Same as an overmap_terrain name |
| sym        | Same as an overmap_terrain sym |
| color      | Same as an overmap_terrain color |
| looks_like | overmap_terrain id that will be drawn if there is no tile drawn |
| blends_adjacent | If true, the other fields will be ignored and instead of drawing this tile, the most common adjacent tile will be selected and drawn instead |

For tilesets, the id for each level will be specified as: `id$VISION_LEVEL`, where `VISION_LEVEL` is
replaced by one of `vague`, `outlines`, or `details`.

### Example

```jsonc
{
  "type": "oter_vision",
  "id": "example_vision",
  "levels": [
    { "blends_adjacent": true },
    { "name": "example", "sym": "&", "color": "white" }
  ]
}
```

## Overmap Special

An overmap special is an entity that is placed in the overmap after the city generation process has
completed--they are the "non-city" counterpart to the **city_building** type. They commonly are
made up of multiple overmap terrains (though not always), may have overmap connections (e.g. roads,
sewers, subways), and have JSON-defined rules guiding their placement.

### Special placement

There are a finite number of sectors in which overmap specials can be placed during overmap
generation, each being a square with side length equal to `OMSPEC_FREQ` (defined in omdata.h; at
the time of writing `OMSPEC_FREQ`= 15 meaning each overmap has 144 sectors for placing specials).
At the beginning of overmap generation, a list of eligible map specials is built
(`overmap_special_batch`).  Next, a free sector is chosen where a special will be placed.  A random
point in that sector is checked against a random rotation of a random special from the special batch
to see if it can be placed there. If not, a new random point in the sector is checked against a new
special, and so on until a valid spawn is rolled.

### Fixed vs mutable specials

There are two subtypes of overmap special: fixed and mutable.  Fixed overmap
specials have a fixed defined layout which can span many OMTs, and can rotate
(see below) but will always look essentially the same.

Mutable overmap specials have a more flexible layout.  They are defined in
terms of a collection of overmap terrains and the ways they can fit together,
like a jigsaw puzzle where pieces can fit together in multiple ways.  This can
be used to form more organic shapes, such as tunnels dug underground by giant
ants, or larger sprawling buildings with mismatched wings and extensions.

Mutable specials require a lot more care to design such that they can be
reliably placed without error.

### Rotation

In general, it is desirable to define your overmap special as allowing rotation--this will provide
variety in the game world and allow for more possible valid locations when attempting to place the
overmap special. A consequence of the relationship between rotating an overmap special and rotating
the underlying overmap terrains that make up the special is that the overmap special should reference
a specific rotated version of the associated overmap terrain--generally, this is the `_north` rotation
as it corresponds to the way in which the JSON for mapgen is defined.

### Connections

The overmap special can be connected to the road, subway or sewer networks. Specifying a connection
point causes the appropriate connection to be automatically generated from the nearest matching terrain
, unless `existing` is set to true.

Connections with `existing` set to true are used to test the validity of an overmap special's
placement. Unlike normal connection points these do not have to reference road/tunnel terrain types.
They will not generate new terrain, and may even be overwritten by the overmap special's terrain.
However, since the overmap special algorithm considers a limited number of random locations per overmap,
the use of `existing` connections that target a rare terrain lowers the chances of the special
spawning considerably.

### Occurrences (default)

Occurrences is the way to set the baseline rarity of a special. The field can behave in two ways:
by default, it sets the minimum and maximum occurrences of the special per overmap. Currently all
overmap specials have a minimum occurrence of 0, to keep the overmaps from being too similar to each
other. In addition, there are no specials with a maximum occurrence of 1. This is important because
each normal special has a very high chance of being placed at least once per overmap, owing to some
quirks of the code (most notably, the number of specials is only slightly more than the number of slots per
overmap, specials that failed placement don't get disqualified and can be rolled for again, and placement iterates
until all sectors are occupied). For specials that are not common enough to warrant appearing more
than once per overmap please use the "OVERMAP_UNIQUE" flag. For specials that should only have one instance
per world use "GLOBALLY_UNIQUE".

### Occurrences ( OVERMAP_UNIQUE, GLOBALLY_UNIQUE )

When the special has the "OVERMAP_UNIQUE" or "GLOBALLY_UNIQUE" flag, instead of defining the minimum and maximum
number placed.  the occurrences field defines the chance of the special to be included in any one given overmap.
Before any placement rolls, all specials with this flag have to succeed in an x_in_y (first value, second
value) roll to be included in the `overmap_special_batch` for the currently generated overmap;
any special that failed this roll will never be considered for placement.  Currently all OVERMAP_UNIQUE specials
use [x, 100] occurrences - percentages - for ease of understanding, but this is not required.


### Locations

The overmap special has two mechanisms for specifying the valid locations (`overmap_location`) that
it may be placed on. Each individual entry in `overmaps` may have the valid locations specified,
which is useful if different parts of a special are allowed on different types of terrain (e.g. a
dock that should have one part on the shore and one part in the water). If all values are the same,
locations may instead be specified for the entire special using the top level `locations` key, The
value for an individual entry takes precedence over the top level value, so you may define the top
level value and then only specify it for individual entries that differ.

### City distance and size

During generation of a new overmap, cities and their connecting roads will be generated before
specials are placed. Each city gets assigned a size at generation and will begin its life as a single
intersection. The city distance field specifies the minimum and maximum distance the special can be
placed from the edge of the urban radius of a city, and not from the center of the city.
Both city size and city distance requirements are only checked for the "nearest" city, measured from the
original intersection.


### Fields

|   Identifier    |                                              Description                                              |
| --------------- | ----------------------------------------------------------------------------------------------------- |
| `type`          | Must be `"overmap_special"`.                                                                          |
| `id`            | Unique id.                                                                                            |
| `subtype`       | Either `"fixed"` or `"mutable"`.  Defaults to `"fixed"` if not specified.                             |
| `locations`     | List of `overmap_location` ids that the special may be placed on.                                     |
| `city_distance` | Min/max distance from a city edge that the special may be placed. Use -1 for unbounded.               |
| `city_sizes`    | Min/max city size for a city that the special may be placed near. Use -1 for unbounded.               |
| `occurrences`   | Min/max number of occurrences when placing the special. If either of OVERMAP_UNIQUE or GLOBALLY_UNIQUE flags is set, becomes X of Y chance. |
| `priority`      | **Warning: Do not use this unnecessarily.** The generation process is executed in the order of specials with the highest value. Can be used when maps are difficult to generate. (large maps, maps that are or require dependencies etc) It is **strongly recommended** to set it to 1 (HIGH priority) or -1 (LOW priority) if used. (default = 0).  It is adviced that map specials marked with the SAFE_AT_WORLDGEN are given a priority of 1 unless there are reasons not to, in order to exist when specials spawning monsters in an area are spawned, as that will cause the latter not to spawn that close. |
| `flags`         | See `Overmap specials` in [JSON_FLAGS.md](JSON_FLAGS.md).                                             |
| `rotate`        | Whether the special can rotate. True if not specified.                                                |

Depending on the subtype, there are further relevant fields:

#### Further fields for fixed overmap specials

|   Identifier    |                                              Description                                              |
| --------------- | ----------------------------------------------------------------------------------------------------- |
| `overmaps`      | List of overmap terrains and their relative `[ x, y, z ]` location within the special.                |
| `connections`   | List of overmap connections and their relative `[ x, y, z ]` location within the special.             |

#### Further fields for mutable overmap specials

|        Identifier          |                                                    Description                                                     |
| -------------------------- | ------------------------------------------------------------------------------------------------------------------ |
| `check_for_locations`      | List of pairs `[ [ x, y, z ], [ locations, ... ] ]` defining the locations that must exist for initial placement.  |
| `check_for_locations_area` | List of check_for_locations area objects to be considered in addition to the explicit `check_for_locations` pairs. |
| `overmaps`                 | Definitions of the various overmaps and how they join to one another.                                              |
| `root`                     | The initial overmap from which the mutable overmap will be grown.                                                  |
| `phases`                   | A specification of how to grow the overmap special from the root OMT.                                              |

### Example fixed special

```jsonc
[
  {
    "type": "overmap_special",
    "id": "campground",
    "overmaps": [
      { "point": [ 0, 0, 0 ], "overmap": "campground_1a_north", "locations": [ "forest_edge" ] },
      { "point": [ 1, 0, 0 ], "overmap": "campground_1b_north" },
      { "point": [ 0, 1, 0 ], "overmap": "campground_2a_north", "camp": "isherwood_family", "camp_name": "Campground camp" },
      { "point": [ 1, 1, 0 ], "overmap": "campground_2b_north" }
    ],
    "connections": [ { "point": [ 1, -1, 0 ], "terrain": "road", "connection": "local_road", "from": [ 1, 0, 0 ] } ],
    "locations": [ "forest" ],
    "city_distance": [ 10, -1 ],
    "city_sizes": [ 3, 12 ],
    "occurrences": [ 0, 5 ],
    "flags": [ "CLASSIC" ],
    "rotate" : true
  }
]
```

### Fixed special overmaps

| Identifier  |                                                                                      Description                                                                                           |
| ----------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| `point`     | `[ x, y, z]` of the overmap terrain within the special.                                                                                                                                    |
| `overmap`   | Id of the `overmap_terrain` to place at the location. If omitted no overmap_terrain is placed but the point will still be checked for valid locations when deciding if placement is valid. |
| `locations` | List of `overmap_location` ids that this overmap terrain may be placed on. Overrides the specials overall `locations` field.                                                               |
| `camp`      | Will make a NPC-owned camp spawn here when given a value. The entered value is the ID of the faction that owns this camp.                                                                  |
| `camp_name` | Name that will be displayed on the overmap for the camp.                                                                                                                                   |

### Connections

| Identifier   |                                           Description                                              |
| ------------ | -------------------------------------------------------------------------------------------------- |
| `point`      | `[ x, y, z]` of the connection end point. Cannot overlap an overmap terrain entry for the special. |
| `terrain`    | Will go away in favor of `connection` eventually. Use `road`, `subway`, `sewer`, etc.              |
| `connection` | Id of the `overmap_connection` to build. Optional for now, but you should specify it explicitly.   |
| `from`       | Optional point `[ x, y, z]` within the special to treat as the origin of the connection.           |
| `existing`   | Boolean, default false. If the special requires a preexisting terrain to spawn.                    |

### Example mutable special

```jsonc
[
  {
    "type": "overmap_special",
    "id": "anthill",
    "subtype": "mutable",
    "locations": [ "subterranean_empty" ],
    "city_distance": [ 25, -1 ],
    "city_sizes": [ 0, 20 ],
    "occurrences": [ 0, 1 ],
    "flags": [ "CLASSIC", "WILDERNESS" ],
    "//example": "The following check_for_locations_area field is superfluous in this example, and serves as a proof of concept",
    "check_for_locations_area": [
      { "type": [ "subterranean_empty" ], "from": [ -1, -1, -1 ], "to": [ 1, 1, -1 ] }
    ],
    "check_for_locations": [
      [ [ 0, 0, 0 ], [ "land" ] ],
      [ [ 0, 0, -1 ], [ "subterranean_empty" ] ],
      [ [ 1, 0, -1 ], [ "subterranean_empty" ] ],
      [ [ 0, 1, -1 ], [ "subterranean_empty" ] ],
      [ [ -1, 0, -1 ], [ "subterranean_empty" ] ],
      [ [ 0, -1, -1 ], [ "subterranean_empty" ] ]
    ],
    "joins": [ "surface_to_tunnel", "tunnel_to_tunnel" ],
    "overmaps": {
      "surface": { "overmap": "anthill", "below": "surface_to_tunnel", "locations": [ "land" ] },
      "below_entrance": {
        "overmap": "ants_nesw",
        "above": "surface_to_tunnel",
        "north": "tunnel_to_tunnel",
        "east": "tunnel_to_tunnel",
        "south": "tunnel_to_tunnel",
        "west": "tunnel_to_tunnel"
      },
      "crossroads": {
        "overmap": "ants_nesw",
        "north": "tunnel_to_tunnel",
        "east": "tunnel_to_tunnel",
        "south": "tunnel_to_tunnel",
        "west": "tunnel_to_tunnel"
      },
      "tee": { "overmap": "ants_nes", "north": "tunnel_to_tunnel", "east": "tunnel_to_tunnel", "south": "tunnel_to_tunnel" },
      "straight_tunnel": { "overmap": "ants_ns", "north": "tunnel_to_tunnel", "south": "tunnel_to_tunnel" },
      "corner": { "overmap": "ants_ne", "north": "tunnel_to_tunnel", "east": "tunnel_to_tunnel" },
      "dead_end": { "overmap": "ants_end_south", "north": "tunnel_to_tunnel" },
      "queen": { "overmap": "ants_queen", "north": "tunnel_to_tunnel" },
      "larvae": { "overmap": "ants_larvae", "north": "tunnel_to_tunnel" },
      "food": { "overmap": "ants_food", "north": "tunnel_to_tunnel" }
    },
    "root": "surface",
    "phases": [
      [ { "overmap": "below_entrance", "max": 1 } ],
      [
        { "overmap": "straight_tunnel", "max": 20 },
        { "overmap": "corner", "max": { "poisson": 5 } },
        { "overmap": "tee", "max": 10 }
      ],
      [ { "overmap": "queen", "max": 1 } ],
      [ { "overmap": "food", "max": 5 }, { "overmap": "larvae", "max": 5 } ],
      [
        { "overmap": "dead_end", "weight": 2000 },
        { "overmap": "straight_tunnel", "weight": 100 },
        { "overmap": "corner", "weight": 100 },
        { "overmap": "tee", "weight": 10 },
        { "overmap": "crossroads", "weight": 1 }
      ]
    ]
  }
]
```

### How mutable specials are placed

#### Overmaps and joins

A mutable special has a collection of *overmaps* which define the OMTs used to
build it and *joins* which define the way in which they are permitted to
connect with one another.  Each overmap may specify a join for each of its
edges (four cardinal directions, above, and below).  These joins must match the
opposite join for the adjacent overmap in that direction.

In the above example, we see that the `surface` overmap specifies `"below":
"surface_to_tunnel"`, meaning that the join below it must be
`surface_to_tunnel`.  So, the overmap below must specify `"above":
"surface_to_tunnel"`.  Only the `below_entrance` overmap does that, so we know
that overmap must be placed beneath the `surface` overmap.

Overmaps can always be rotated, so a `north` constraint can correspond to other
directions.  So, the above `dead_end` overmap can represent a dead end tunnel
in any direction, but it's important that the chosen OMT `ants_end_south` is
consistent with the `north` join for the generated map to make sense.

Overmaps can also specify connections.  For example, an overmap might be
defined as:

```jsonc
"where_road_connects": {
  "overmap": "road_end_north",
  "west": "parking_lot_to_road",
  "connections": { "north": { "connection": "local_road" } }
}
```

Once the mutable special placement is complete, a `local_road` connection will
be built from the north edge of this overmap (again, 'north' is a relative
term, that will rotate as the overmap is rotated) in the same way as
connections are built for fixed specials.  'Existing' connections are not
supported for mutable specials.

#### Layout phases

After all the joins and overmaps are defined, the manner in which the special
is laid out is given by `root` and `phases`.

`root` specifies the overmap which is placed first, at the origin point for
this special.

Then `phases` gives a list of growth phases used to place more overmaps.  These
phases are processed strictly in order.

Each *phase* is a list of rules.  Each *rule* specifies an overmap and an
integer `max` and/or `weight`.

Weight must always be a simple integer, but `max` may also be an object or array
defining a probability distribution over integers.  Each time the special is
spawned, a value is sampled from that distribution.  Currently uniform, binomial
and Poisson distributions are supported.  Uniform is specified by using an
array such as `"max": [ 1, 5 ]` resulting in an evenly distributed 20% chance for each
number from 1 to 5 being picked as the max.  Poisson and binomial are specified
via an object such as `"max": { "poisson": 5 }` where 5 will be the mean of the
poisson distribution (λ) or `"max": { "binomial": [ 5, 0.3 ] }` where 5 will be
the number of trials and 0.3 the success probability of the binomial distribution
( n, p ).  Both have a higher chance for values to fall closer to the mean
(λ in the case of Poisson and n x p in the case of binomial) with Poisson being more
symetrical while binomial can be skewed by altering p to result in most values being
on the lower end of the range 0-n or vice versa useful to produce more consistent results
with rarer chances of large or small values or just one kind respectively.  Hard bounds
may be specified in addition to poisson or binomial to limit the range of possibilities,
useful for guarenteeing a max of at least 1 or to prevent large values making overall
size management difficult.  To do this add an array `bounds` such as
`"max": { "poisson": 5, "bounds": [ 1, -1 ] }` in this case guarenteeing at least a max
of 1 without bounding upper values.  Any value that would fall outside of these bounds
becomes the relevant bound (as opposed to being rerolled).

Within each phase, the game looks for unsatisfied joins from the existing
overmaps and attempts to find an overmap from amongst those available in its
rules to satisfy that join.  Priority is given to whichever joins are listed
first in the list which defines the joins for this special, but if multiple
joins of the same (highest priority) id are present then one is chosen at
random.

First the rules are filtered to contain only those which can satisfy the joins
for a particular location, and then a weighted selection from the filtered list
is made.  The weight is given by the minimum of `max` and `weight` specified in
the rule.  The difference between `max` and `weight` is that each time a rule
is used, `max` is decremented by one.  Therefore, it limits the number of times
that rule can be chosen.  Rules which only specify `weight` can be chosen an
arbitrary number of times.

If no rule in the current phase is able to satisfy the joins for a particular
location, that location is set aside to be tried again in later phases.

Once all joins are either satisfied or set aside, the phase ends and generation
proceeds to the next phase.

If all phases complete and unsatisfied joins remain, this is considered an
error and a debugmsg will be displayed with more details.

#### Chunks

A placement rule in the phases can specify multiple overmaps to be placed in a
particular configuration.  This is useful if you want to place some feature
that's larger than a single OMT.  Here is an example from the microlab:

```jsonc
{
  "name": "subway_chunk_at_-2",
  "chunk": [
    { "overmap": "microlab_sub_entry", "pos": [ 0, 0, 0 ], "rot": "north" },
    { "overmap": "microlab_sub_station", "pos": [ 0, -1, 0 ] },
    { "overmap": "microlab_subway", "pos": [ 0, -2, 0 ] }
  ],
  "max": 1
}
```

The `"name"` of a chunk is only for debugging messages when something goes
wrong.  `"max"` and `"weight"` are handled as above.

The new feature is `"chunks"` which specifies a list of overmaps and their
relative positions and rotations.  The overmaps are taken from the ones defined
for this special.  Rotation of `"north"` is the default, so specifying that has
no effect, but it's included here to demonstrate the syntax.

The positions and rotations are relative.  The chunk can be placed at any offset
and rotation, so long as all the overmaps are shifted and rotated together like
a rigid body.

#### Techniques to avoid placement errors

To help avoid these errors, some additional features of the mutable special
placement can help you.

##### `check_for_locations`

`check_for_locations` defines a list of extra constraints that are
checked before the special is attempted to be placed.  Each constraint is a
pair of a position (relative to the root) and a set of locations.  The existing
OMT in each position must fall into one of the given locations, else the
attempted placement is aborted.

The `check_for_locations` constraints ensure that the `below_entrance` overmap
can be placed below the root and that all four cardinal-adjacent OMTs are
`subterranean_empty`, which is needed to add any further overmaps satisfying
the four other joins of `below_entrance`.

##### `check_for_locations_area`

`check_for_locations_area` defines a list of extra, rectangular regions of constraints
to be considered in addition to the `check_for_locations` constraints. It is an array
of objects that define a singular location, as well as a `from` and `to` tripoint that
define the square region of constraints, like this:

```jsonc
"check_for_locations_area": [
  { "type": [ "subterranean_empty" ], "from": [ -1, -1, -1 ], "to": [ 1, 1, -1 ] }
],
```
For the above, the `check_for_locations_area` field is equivalent to manually adding
the following constraints to the `check_for_locations` array:
```jsonc
[ [ 0, 0, -1 ], [ "subterranean_empty" ] ],
[ [ 1, 0, -1 ], [ "subterranean_empty" ] ],
[ [ 0, 1, -1 ], [ "subterranean_empty" ] ],
[ [ -1, 0, -1 ], [ "subterranean_empty" ] ],
[ [ 0, -1, -1 ], [ "subterranean_empty" ] ],
[ [ 1, 1, -1 ], [ "subterranean_empty" ] ],
[ [ 1, -1, -1 ], [ "subterranean_empty" ] ],
[ [ -1, 1, -1 ], [ "subterranean_empty" ] ],
[ [ -1, -1, -1 ], [ "subterranean_empty" ] ]
```

The `check_for_locations_area` field in the example mutable special is superfluous and
serves only to illustrate the syntax of the field.

Look at /json/overmap/overmap_mutable/nether_monster_corpse.json for an application
of this field in a real mutable special tile.

##### `into_locations`

Each join also has an associated list of locations.  This defaults to
the locations for the special, but it can be overridden for a particular join
like this:

```jsonc
"joins": [
  { "id": "surface_to_surface", "into_locations": [ "land" ] },
  "tunnel_to_tunnel"
]
```

For an overmap to be placed when satisfying an unresolved
join, it is not sufficient for it to satisfy the
existing joins adjacent to a particular location.  Any residual joins it
possesses beyond those which already match up must point to OMTs with
terrains consistent with that join's locations.

For the particular case of the anthill example above, we can see how these two
additions ensure that placement is always successful and no unsatisfied joins
remain.

The next few phases of placement will attempt to place various tunnels.  The
join constraints will ensure that the unsatisfied joins (the open ends of
tunnels) will always point into `subterranean_empty` OMTs.

##### Ensuring complete coverage in the final phase

In the final phase, we have five different rules intended to cap off any
unsatisfied joins without growing the anthill further.  It is important that
the rules whose overmaps have fewer joins get higher weights.  In the normal
case, every unsatisfied join will be simply closed off using `dead_end`.
However, we also have to cater for the possibility that two unsatisfied joins
point to the same OMT, in which case `dead_end` will not fit (and will be
filtered out), but `straight_tunnel` or `corner` will, and one of those will
likely be chosen since they have the highest weights.  We don't want `tee` to
be chosen (even though it might fit) because that would lead to a new
unsatisfied join and further grow the tunnels.  But it's not a big problem if
`tee` is chosen occasionally in this situation; the new join will likely simply
be satisfied using `dead_end`.

When designing your own mutable overmap specials, you will have to think
through these permutations to ensure that all joins will be satisfied by the
end of the last phase.

#### Optional joins

Rather than having lots of rules designed to satisfy all possible situations in
the final phase, in some situations you can make this easier using optional
joins.  This feature can also be used in other phases.

When specifying the joins associated with an overmap in a mutable special, you
can elaborate with a type, like this example from the [`homeless_camp_mutable`](../data/json/overmap/overmap_mutable/homeless_camp_mutable.json) overmap special:

```jsonc
"overmaps": {
      "camp_core": {
        "overmap": "homelesscamp_north",
        "north": "camp_to_camp",
        "east": "camp_to_camp",
        "south": "camp_to_camp",
        "west": "camp_to_camp"
      },
      "camp_edge": {
        "overmap": "homelesscamp_north",
        "north": "camp_to_camp",
        "east": { "id": "camp_to_camp", "type": "available" },
        "south": { "id": "camp_to_camp", "type": "available" },
        "west": { "id": "camp_to_camp", "type": "available" }
      }
```

The definition of `camp_edge` has one mandatory join to the north, and three
'available' joins to the other cardinal directions.  The semantics of an
'available' join are that it will not be considered an unresolved join, and
therefore will never cause more overmaps to be placed, but it can satisfy other
joins into a particular tile when necessary to allow an existing unresolved
join to be satisfied.

The overmap will always be rotated in such a way that as many of its mandatory
joins as possible are satisfied and available joins are left to point in other
directions that don't currently need joins.

As such, this `camp_edge` overmap can satisfy any unresolved joins for the
`homeless_camp_mutable` special without generating any new unresolved joins of its own.  This
makes it great to finish off the special in the final phase.

#### Asymmetric joins

Sometimes you want two different OMTs to connect, but wouldn't want either to
connect with themselves.  In this case you wouldn't want to use the same join
on both.  Instead, you can define two joins which form a pair, by specifying
one as the opposite of the other.

Another situation where this can arise is when the two sides of a join need
different location constraints.  For example, in the anthill, the surface and
subterranean components need different locations.  We could improve the
definition of its joins by making the join between surface and tunnels
asymmetric, like this:

```jsonc
"joins": [
  { "id": "surface_to_tunnel", "opposite": "tunnel_to_surface" },
  { "id": "tunnel_to_surface", "opposite": "surface_to_tunnel", "into_locations": [ "land" ] },
  "tunnel_to_tunnel"
],
```

As you can see, the `tunnel_to_surface` part of the pair needs to override the
default value of `into_locations` because it points towards the surface.

#### Alternative joins

Sometimes you want the next phase(s) of a mutable special to be able to link to
existing unresolved joins without themselves generating any unresolved joins of
that type.  This helps to create a clean break between the old and the new.

For example, this happens in the `microlab_mutable` special.  This special has
some structured `hallway` OMTs surrounded by a clump of `microlab` OMTs.  The
hallways have `hallway_to_microlab` joins pointing out to their sides, so we
need `microlab` OMTs to have `microlab_to_hallway` joins (the opposite of
`hallway_to_microlab`) in order to match them.

However, we don't want the unresolved edges of a `microlab` OMT to require more
hallways all around, so we mostly want them to use `microlab_to_microlab`
joins.  How can we satisfy these apparently conflicting requirements without
making many different variants of `microlab` with different numbers of each
type of join?  Alternative joins can help us here.

The definition of the `microlab` overmap might look like this:

```jsonc
"microlab": {
  "overmap": "microlab_generic",
  "north": { "id": "microlab_to_microlab", "alternatives": [ "microlab_to_hallway" ] },
  "east": { "id": "microlab_to_microlab", "alternatives": [ "microlab_to_hallway" ] },
  "south": { "id": "microlab_to_microlab", "alternatives": [ "microlab_to_hallway" ] },
  "west": { "id": "microlab_to_microlab", "alternatives": [ "microlab_to_hallway" ] }
},
```

This allows it to join with hallways which are already placed on the overmap,
but new unresolved joins will only match more `microlab`s.

#### Testing your new mutable special

If you want to exhaustively test your mutable special for placement errors, and
you are in a position to compile the game, then an easy way to do so is to use
the existing test in `tests/overmap_test.cpp`.

In that file, look for `TEST_CASE( "mutable_overmap_placement"`.  At the start
of that function there is a list of mutable special ids that tests tries
spawning.  Replace one of them with your new special's id, recompile and run
the test.

The test will attempt to place your special a few thousand times, and should
find most ways in which placement might fail.

### Joins

A join definition can be a simple string, which will be its id.  Alternatively,
it can be a dictionary with some of these keys:

| Identifier  |                                Description                                 |
| ----------- | -------------------------------------------------------------------------- |
| `id`        | Id of the join being defined.                                              |
| `opposite`  | Id of the join which must match this one from the adjacent terrain.        |
| `into_locations` | List of `overmap_location` ids that this join may point towards.      |

### Mutable special overmaps

The overmaps are a JSON dictionary.  Each overmap must have an id (local to
this special) which is the JSON dictionary key, and then the fields within the
value may be:

| Identifier  |                                Description                                 |
| ----------- | -------------------------------------------------------------------------- |
| `overmap`   | Id of the `overmap_terrain` to place at the location.                      |
| `locations` | List of `overmap_location` ids that this overmap terrain may be placed on.  If not specified, defaults to the `locations` value from the special definition. |
| `north`     | Join which must align with the north edge of this OMT                      |
| `east`      | Join which must align with the east edge of this OMT                       |
| `south`     | Join which must align with the south edge of this OMT                      |
| `west`      | Join which must align with the west edge of this OMT                       |
| `above`     | Join which must link this to the OMT above                                 |
| `below`     | Join which must link this to the OMT below                                 |

Each join associated with a direction can be a simple string, interpreted as a
join id.  Alternatively it can be a JSON object with the following keys:

| Identifier     |                                Description                                 |
| -------------- | -------------------------------------------------------------------------- |
| `id`           | Id of the join used here.                                                  |
| `type`         | Either `"mandatory"` or `"available"`.  Default: `"mandatory"`.            |
| `alternatives` | List of join ids that may be used instead of the one listed under `id`, but only when placing this overmap.  Unresolved joins created by its placement will only be the primary join `id`. |

### Generation rules

| Identifier  |                                Description                                 |
| ----------- | -------------------------------------------------------------------------- |
| `overmap`   | Id of the `overmap` to place.                                              |
| `max`       | Maximum number of times this rule should be used.                          |
| `weight`    | Weight with which to select this rule.                                     |

One of `max` and `weight` must be specified.  `max` will be used as the weight
when `weight` is not specified.

## City Building

A city building is an entity that is placed in the overmap during the city generation process--they
are the "city" counterpart to the **overmap_special** type. The definition for a city building is a subset
of that for an overmap special, and consequently will not be repeated in detail here.

### Mandatory Overmap Specials / Region Settings

City buildings are not subject to the same quantity limitations as overmap specials, and in fact
the occurrences attribute does not apply at all. Instead, the placement of city buildings is driven
by the frequency assigned to the city building within the `region_settings`. Consult
[REGION_SETTINGS.md](REGION_SETTINGS.md) for more details.

### Fields

| Identifier  |                                   Description                                    |
| ----------- | -------------------------------------------------------------------------------- |
| `type`      | Must be "city_building".                                                         |
| `id`        | Unique id.                                                                       |
| `overmaps`  | As in `overmap_special`, with one caveat: all point x and y values must be >= 0. |
| `locations` | As in `overmap_special`.                                                         |
| `flags`     | As in `overmap_special`.                                                         |
| `rotate`    | As in `overmap_special`.                                                         |
| `city_sizes`| As in `overmap_special`.                                                         |

### Example

```jsonc
[
  {
    "type": "city_building",
    "id": "zoo",
    "locations": [ "land" ],
    "overmaps": [
      { "point": [ 0, 0, 0 ], "overmap": "zoo_0_0_north" },
      { "point": [ 1, 0, 0 ], "overmap": "zoo_1_0_north" },
      { "point": [ 2, 0, 0 ], "overmap": "zoo_2_0_north" },
      { "point": [ 0, 1, 0 ], "overmap": "zoo_0_1_north" },
      { "point": [ 1, 1, 0 ], "overmap": "zoo_1_1_north" },
      { "point": [ 2, 1, 0 ], "overmap": "zoo_2_1_north" },
      { "point": [ 0, 2, 0 ], "overmap": "zoo_0_2_north" },
      { "point": [ 1, 2, 0 ], "overmap": "zoo_1_2_north" },
      { "point": [ 2, 2, 0 ], "overmap": "zoo_2_2_north" }
    ],
    "flags": [ "CLASSIC" ],
    "rotate" : true
  }
]
```

## Overmap Connection

### Fields

| Identifier |                                           Description                                           |
| ---------- | ----------------------------------------------------------------------------------------------- |
| `type`     | Must be "overmap_connection".                                                                   |
| `id`       | Unique id.                                                                                      |
| `subtypes` | List of entries used to determine valid locations, terrain cost, and resulting overmap terrain. |


### Example

```jsonc
[
  {
    "type": "overmap_connection",
    "id": "local_road",
    "subtypes": [
      { "terrain": "road", "locations": [ "field", "road" ] },
      { "terrain": "road", "locations": [ "forest_without_trail" ], "basic_cost": 20 },
      { "terrain": "road", "locations": [ "forest_trail" ], "basic_cost": 25 },
      { "terrain": "road", "locations": [ "swamp" ], "basic_cost": 40 },
      { "terrain": "road_nesw_manhole", "locations": [  ] },
      { "terrain": "bridge", "locations": [ "water" ], "basic_cost": 120 }
    ]
  },
  {
    "type": "overmap_connection",
    "id": "subway_tunnel",
    "subtypes": [ { "terrain": "subway", "locations": [ "subterranean_subway" ], "flags": [ "ORTHOGONAL" ] } ]
  }
]
```

### Subtypes

|  Identifier  |                                                Description                                                 |
| ------------ | ---------------------------------------------------------------------------------------------------------- |
| `terrain`    | `overmap_terrain` to be placed when the placement location matches `locations`.                            |
| `locations`  | List of `overmap_location` that this subtype applies to. Can be empty; signifies `terrain` is valid as is. |
| `basic_cost` | Cost of this subtype when pathfinding a route. Default 0.                                                  |
| `flags`      | See `Overmap connections` in [JSON_FLAGS.md](JSON_FLAGS.md).                                               |

## Overmap Location

### Fields

| Identifier |                               Description                               |
| ---------- | ----------------------------------------------------------------------- |
| `type`     | Must be "overmap_location".                                             |
| `id`       | Unique id.                                                              |
| `terrains` | List of `overmap_terrain` that can be considered part of this location. |


### Example

```jsonc
[
  {
    "type": "overmap_location",
    "id": "wilderness",
    "terrains": [ "forest", "forest_thick", "field", "forest_trail" ]
  }
]

```

## Cities

Object of `city` type define cities that would appear on overmap in specific location.
Note that no random cities would be spawned if there is at least one city defined.

### Fields

|    Identifier     |                                        Description                                         |
| ----------------- | ------------------------------------------------------------------------------------------ |
| `id`              | Id of the city.                                                                            |
| `database_id`     | Id of the city in MA database.                                                             |
| `name`            | Name of the city.                                                                          |
| `population`      | Original population of the city.                                                           |
| `size`            | Size of the city.                                                                          |
| `pos_om`          | location of the city (in overmap coordinates).                                             |
| `pos`             | location of the city (in overmap terrain coordinates).                                     |

### Example

```jsonc
{
  {
    "type": "city",
    "id": "35",
    "database_id": 35,
    "name": "BOSTON",
    "population": 617594,
    "size": 64,
    "pos_om": [ 46, 15 ],
    "pos": [ 57, 94 ]
  }
}
```
