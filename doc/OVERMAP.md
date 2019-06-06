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
..........FFF│FF
.v>│<...FFFFF│FF
──>│<...FFFFF│FF
<.>│<...FFFFF│FF
O.v│vv.vvv┌──┘FF
───┼──────┘.FFFF
```

So for example, the `F` is a forest which has a definition like this:

```json
{
    "type": "overmap_terrain",
    "id": "forest",
    "name": "forest",
    "sym": 70,
    "color": "green"
}
```

and the `v` is a house which has a definition like this:

```json
{
    "type": "overmap_terrain",
    "id": "house",
    "name": "house",
    "sym": 94,
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

```json
{
    "type": "overmap_location",
    "id": "forest",
    "terrains": ["forest"]
},
{
    "type": "overmap_location",
    "id": "wilderness",
    "terrains": ["forest", "field"]
}
```

The value of these is that they allow for a given overmap terrain to belong to several different
locations and provide a level of indirection so that when new overmap terrains are added (or
existing ones removed), only the relevant **overmap_location** entries need to be changed rather
than every **overmap_connection**, **overmap_special**, and **city_building** that needs those
groups.

For example, with the addition of a new forest overmap terrain `forest_thick`, we only have to
update these definitions as follows:

```json
{
    "type": "overmap_location",
    "id": "forest",
    "terrains": ["forest", "forest_thick"]
},
{
    "type": "overmap_location",
    "id": "wilderness",
    "terrains": ["forest", "forest_thick", "field"]
}
```

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
| `type`            | Must be "overmap_terrain".                                                                       |
| `id`              | Unique id.                                                                                       |
| `name`            | Name for the location shown in game.                                                             |
| `sym`             | Symbol used when drawing the location. ASCII (e.g. F is 70) plus some specials for line drawing. |
| `color`           | Color to draw the symbol in. See COLOR.md.                                                       |
| `see_cost`        | Affects player vision on overmap. Higher values obstruct vision more.                            |
| `travel_cost`     | Affects pathfinding cost. Higher values are harder to travel through (reference: Forest = 10 )   |
| `extras`          | Reference to a named `map_extras` in region_settings, defines which map extras can be applied.   |
| `mondensity`      | Summed with values for adjacent overmap terrains to influence density of monsters spawned here.  |
| `spawns`          | Spawns added once at mapgen. Monster group, % chance, population range (min/max).                |
| `flags`           | See `Overmap terrains` in JSON_FLAGS.md.                                                         |
| `mapgen`          | Specify a C++ mapgen function. Don't do this--use JSON.                                          |
| `mapgen_straight` | Specify a C++ mapgen function for a LINEAR feature variation.                                    |
| `mapgen_curved`   | Specify a C++ mapgen function for a LINEAR feature variation.                                    |
| `mapgen_end`      | Specify a C++ mapgen function for a LINEAR feature variation.                                    |
| `mapgen_tee`      | Specify a C++ mapgen function for a LINEAR feature variation.                                    |
| `mapgen_four_way` | Specify a C++ mapgen function for a LINEAR feature variation.                                    |

### Example

A real `overmap_terrain` wouldn't have all these defined at the same time, but in the interest of
an exhaustive example...

```json
{
    "type": "overmap_terrain",
    "id": "field",
    "name": "field",
    "sym": 46,
    "color": "brown",
    "see_cost": 2,
    "extras": "field",
    "mondensity": 2,
    "spawns": { "group": "GROUP_FOREST", "population": [ 0, 1 ], "chance": 13 },
    "flags": [ "NO_ROTATE" ],
    "mapgen": [ { "method": "builtin", "name": "bridge" } ],
    "mapgen_straight": [ { "method": "builtin", "name": "road_straight" } ],
    "mapgen_curved": [ { "method": "builtin", "name": "road_curved" } ],
    "mapgen_end": [ { "method": "builtin", "name": "road_end" } ],
    "mapgen_tee": [ { "method": "builtin", "name": "road_tee" } ],
    "mapgen_four_way": [ { "method": "builtin", "name": "road_four_way" } ]
}
```

## Overmap Special

An overmap special is an entity that is placed in the overmap after the city generation process has
completed--they are the "non-city" counterpart to the **city_building** type. They commonly are
made up of multiple overmap terrains (though not always), may have overmap connections (e.g. roads,
sewers, subways), and have JSON-defined rules guiding their placement.

### Mandatory Overmap Specials

There are a finite number of "slots" in which overmap specials can be placed during overmap
generation, defined by the width of the overmap, height of the overmap, and an "overmap special
frequency" (at the time of writing there are 72 "slots" per overmap). As a result, you are
encouraged to exercise restraint when specifying some attributes of the overmap special, such as
required minimum occurrences. The game gives precedence to "mandatory overmap specials" (e.g. those
with a minimum greater than 0) and consequently too many mandatory overmap specials may exhaust the
number of slots before any optional specials can even attempt placement. As a general rule, the
minimum should be 0.

### Rotation

In general, it is desirable to define your overmap special as allowing rotation--this will provide
variety in the game world and allow for more possible valid locations when attempting to place the
overmap special. A consequence of the relationship between rotating an overmap special and rotating
the underlying overmap terrains that make up the special is that the overmap special should reference
a specific rotated version of the associated overmap terrain--generally, this is the `_north` rotation
as it corresponds to the way in which the JSON for mapgen is defined.

### Locations

The overmap special has two mechanisms for specifying the valid locations (`overmap_location`) that
it may be placed on. Each individual entry in `overmaps` may have the valid locations specified,
which is useful if different parts of a special are allowed on different types of terrain (e.g. a
dock that should have one part on the shore and one part in the water). If all values are the same,
locations may instead be specified for the entire special using the top level `locations` key, The
value for an individual entry takes precedence over the top level value, so you may define the top
level value and then only specify it for individual entries that differ.

### Fields

|   Identifier    |                                              Description                                              |
| --------------- | ----------------------------------------------------------------------------------------------------- |
| `type`          | Must be "overmap_special".                                                                            |
| `id`            | Unique id.                                                                                            |
| `overmaps`      | List of overmap terrains and their relative location within the special. Location is [ x, y, z ].     |
| `connections`   | List of overmap connections and their relative location within the special. Location is [ x, y, z ].  |
| `locations`     | List of `overmap_location` ids that the special may be placed on.                                     |
| `city_distance` | Min/max distance from a city that the special may be placed. Use -1 for unbounded.                    |
| `city_sizes`    | Min/max city size for a city that the special may be placed near. Use -1 for unbounded.               |
| `occurrences`   | Min/max number of occurrences when placing the special. If UNIQUE flag is set, becomes X of Y chance. |
| `flags`         | See `Overmap specials` in JSON_FLAGS.md.                                                              |
| `rotate`        | Whether the special can rotate. True if not specified.                                                |

### Example

```json
[
  {
    "type": "overmap_special",
    "id": "campground",
    "overmaps": [
      { "point": [ 0, 0, 0 ], "overmap": "campground_1a_north", "locations": [ "forest_edge" ] },
      { "point": [ 1, 0, 0 ], "overmap": "campground_1b_north" },
      { "point": [ 0, 1, 0 ], "overmap": "campground_2a_north" },
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

### Overmaps

| Identifier  |                                Description                                 |
| ----------- | -------------------------------------------------------------------------- |
| `point`     | [ x, y, z] of the overmap terrain within the special.                      |
| `overmap`   | Id of the `overmap_terrain` to place at the location.                      |
| `locations` | List of `overmap_location` ids that this overmap terrain may be placed on. |

### Connections

|  Identifier  |                                           Description                                            |
| ------------ | ------------------------------------------------------------------------------------------------ |
| `point`      | [ x, y, z] of the connection end point. Cannot overlap an overmap terrain entry for the special. |
| `terrain`    | Will go away in favor of `connection` eventually. Use `road`, `subway`, `sewer`, etc.            |
| `connection` | Id of the `overmap_connection` to build. Optional for now, but you should specify it explicitly. |
| `from`       | Optional point [ x, y, z] within the special to treat as the origin of the connection.           |

## City Building

A city building is an entity that is placed in the overmap during the city generation process--they
are the "city" counterpart to the **overmap_special** type. The definition for a city building is a subset
of that for an overmap special, and consequently will not be repeated in detail here.

### Mandatory Overmap Specials / Region Settings

City buildings are not subject to the same quantity limitations as overmap specials, and in fact
the occurrences attribute does not apply at all. Instead, the placement of city buildings is driven
by the frequency assigned to the city building within the `region_settings`. Consult
REGION_SETTINGS.md for more details.

### Fields

| Identifier  |                                   Description                                    |
| ----------- | -------------------------------------------------------------------------------- |
| `type`      | Must be "city_building".                                                         |
| `id`        | Unique id.                                                                       |
| `overmaps`  | As in `overmap_special`, with one caveat: all point x and y values must be >= 0. |
| `locations` | As in `overmap_special`.                                                         |
| `flags`     | As in `overmap_special`.                                                         |
| `rotate`    | As in `overmap_special`.                                                         |

### Example

```json
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

```json
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
| `flags`      | See `Overmap connections` in JSON_FLAGS.md.                                                                |

## Overmap Location

### Fields

| Identifier |                               Description                               |
| ---------- | ----------------------------------------------------------------------- |
| `type`     | Must be "overmap_location".                                             |
| `id`       | Unique id.                                                              |
| `terrains` | List of `overmap_terrain` that can be considered part of this location. |


### Example

```json
[
  {
    "type": "overmap_location",
    "id": "wilderness",
    "terrains": [ "forest", "forest_thick", "field", "forest_trail" ]
  }
]

```
