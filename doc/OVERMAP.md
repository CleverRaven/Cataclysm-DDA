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

```json
{
    "type": "overmap_terrain",
    "id": "forest",
    "name": "forest",
    "sym": "F",
    "color": "green"
}
```

and the `^` is a house which has a definition like this:

```json
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
| `type`            | Must be `"overmap_terrain"`.                                                                     |
| `id`              | Unique id.                                                                                       |
| `name`            | Name for the location shown in game.                                                             |
| `sym`             | Symbol used when drawing the location, like `"F"` (or you may use an ASCII value like `70`).     |
| `color`           | Color to draw the symbol in. See [COLOR.md](COLOR.md).                                           |
| `looks_like`      | Id of another overmap terrain to be used for the graphical tile, if this doesn't have one.       |
| `connect_group`   | Specify that this overmap terrain might be graphically connected to its neighbours, should a tileset wish to.  It will connect to any other `overmap_terrain` with the same `connect_group`. |
| `see_cost`        | Affects player vision on overmap. Higher values obstruct vision more.                            |
| `travel_cost`     | Affects pathfinding cost. Higher values are harder to travel through (reference: Forest = 10 )   |
| `extras`          | Reference to a named `map_extras` in region_settings, defines which map extras can be applied.   |
| `mondensity`      | Summed with values for adjacent overmap terrains to influence density of monsters spawned here.  |
| `spawns`          | Spawns added once at mapgen. Monster group, % chance, population range (min/max).                |
| `flags`           | See `Overmap terrains` in [JSON_FLAGS.md](JSON_FLAGS.md).                                        |
| `mapgen`          | Specify a C++ mapgen function. Don't do this--use JSON.                                          |
| `mapgen_straight` | Specify a C++ mapgen function for a LINEAR feature variation. Prefer JSON instead.               |
| `mapgen_curved`   | Specify a C++ mapgen function for a LINEAR feature variation. Prefer JSON instead.               |
| `mapgen_end`      | Specify a C++ mapgen function for a LINEAR feature variation. Prefer JSON instead.               |
| `mapgen_tee`      | Specify a C++ mapgen function for a LINEAR feature variation. Prefer JSON instead.               |
| `mapgen_four_way` | Specify a C++ mapgen function for a LINEAR feature variation. Prefer JSON instead.               |

### Example

A real `overmap_terrain` wouldn't have all these defined at the same time, but in the interest of
an exhaustive example...

```json
{
    "type": "overmap_terrain",
    "id": "field",
    "name": "field",
    "sym": ".",
    "color": "brown",
    "looks_like": "forest",
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

### Special placement

There are a finite number of sectors in which overmap specials can be placed during overmap
generation, each being a square with side length equal to `OMSPEC_FREQ` (defined in omdata.h; at
the time of writing `OMSPEC_FREQ`= 15 meaning each overmap has 144 sectors for placing specials). 
At the beginning of overmap generation, a list of eligable map specials is built 
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
than once per overmap please use the "UNIQUE" flag.

### Occurrences ( UNIQUE )

When the special has the "UNIQUE" flag, instead of defining the minimum and maximum number placed 
the occurrences field defines the chance of the special to be included in any one given overmap.
Before any placement rolls, all specials with this flag have to succeed in an x_in_y (first value, second
value) roll to be included in the `overmap_special_batch` for the currently generated overmap;
any special that failed this roll will never be considered for placement.  Currently all UNIQUE specials
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
| `occurrences`   | Min/max number of occurrences when placing the special. If UNIQUE flag is set, becomes X of Y chance. |
| `flags`         | See `Overmap specials` in [JSON_FLAGS.md](JSON_FLAGS.md).                                             |
| `rotate`        | Whether the special can rotate. True if not specified.                                                |

Depending on the subtype, there are further relevant fields:

#### Further fields for fixed overmap specials

|   Identifier    |                                              Description                                              |
| --------------- | ----------------------------------------------------------------------------------------------------- |
| `overmaps`      | List of overmap terrains and their relative `[ x, y, z ]` location within the special.                |
| `connections`   | List of overmap connections and their relative `[ x, y, z ]` location within the special.             |

#### Further fields for mutable overmap specials

|   Identifier          |                                              Description                                              |
| --------------------- | ----------------------------------------------------------------------------------------------------- |
| `check_for_locations` | List of pairs `[ [ x, y, z ], [ locations, ... ] ]` defining the locations that must exist for initial placement. |
| `connections`         | List of connections and their relative `[ x, y, z ]` location within the special. |
| `overmaps`            | Definitions of the various overmaps and how they join to one another. |
| `root`                | The initial overmap from which the mutable overmap will be grown. |
| `phases`              | A specification of how to grow the overmap special from the root OMT. |

### Example fixed special

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

### Fixed special overmaps

| Identifier  |                                Description                                 |
| ----------- | -------------------------------------------------------------------------- |
| `point`     | `[ x, y, z]` of the overmap terrain within the special.                    |
| `overmap`   | Id of the `overmap_terrain` to place at the location.                      |
| `locations` | List of `overmap_location` ids that this overmap terrain may be placed on. |

### Connections

|  Identifier  |                                           Description                                              |
| ------------ | -------------------------------------------------------------------------------------------------- |
| `point`      | `[ x, y, z]` of the connection end point. Cannot overlap an overmap terrain entry for the special. |
| `terrain`    | Will go away in favor of `connection` eventually. Use `road`, `subway`, `sewer`, etc.              |
| `connection` | Id of the `overmap_connection` to build. Optional for now, but you should specify it explicitly.   |
| `from`       | Optional point `[ x, y, z]` within the special to treat as the origin of the connection.           |
| `existing`   | Boolean, default false. If the special requires a preexisting terrain to spawn.					|

### Example mutable special

```json
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
        { "overmap": "corner", "max": 5 },
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

After all the joins and overmaps are defined, the manner in which the special
is laid out is given by `root` and `phases`.

`root` specifies the overmap which is placed first, at the origin point for
this special.

Then `phases` gives a list of growth phases used to place more overmaps.  These
phases are processed strictly in order.

Each *phase* is a list of rules.  Each *rule* specifies an overmap and an
integer `max` and/or `weight`.

Within each phase, the game looks for unsatisfied joins from the existing
overmaps and attempts to find an overmap from amongst those available in its
rules to satisfy that join.

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

To help avoid these errors, two additional features of the mutable special
placement can help you.

Firstly, `check_for_locations` defines a list of extra constraints that are
checked before the special is attempted to be placed.  Each constraint is a
pair of a position (relative to the root) and a set of locations.  The existing
OMT in each postion must fall into one of the given locations, else the
attempted placement is aborted.

Secondly, each join also has an associated list of locations.  This defaults to
the locations for the special, but it can be overridden for a particular join
like this:

```json
"joins": [
  { "id": "surface_to_surface", "into_locations": [ "land" ] },
  "tunnel_to_tunnel"
]
```

For an overmap to be placed, it is not sufficient for it to satisfy the
existing joins adjacent to a particular location.  Any residual joins it
possesses beyond those which already match up must point to OMTs with
terrains consistent with that join's locations.

For the particular case of the anthill example above, we can see how these two
additions ensure that placement is always successful and no unsatisfied joins
remain.

The `check_for_locations` constraints ensure that the `below_entrance` overmap
can be placed below the root and that all four cardinal-adjacent OMTs are
`subterranean_empty`, which is needed to add any further overmaps satisfying
the four other joins of `below_entrance`.

The next few phases of placement will attempt to place various tunnels.  The
join constraints will ensure that the unsatisfied joins (the open ends of
tunnels) will always point into `subterranean_empty` OMTs.

Then, in the final phase, we have five different rules intended to cap off any
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

### Joins

A join definition can be a simple string, which will be its id.  Alternatively,
it can be a dictionary with some of these keys:

| Identifier  |                                Description                                 |
| ----------- | -------------------------------------------------------------------------- |
| `id`        | Id of the join being defined. |
| `into_locations` | List of `overmap_location` ids that this join may point towards. |

### Mutable special overmaps

The overmaps are a JSON dictionary.  Each overmap must have an id (local to
this special) which is the JSON dictionary key, and then the fields within the
value may be:

| Identifier  |                                Description                                 |
| ----------- | -------------------------------------------------------------------------- |
| `overmap`   | Id of the `overmap_terrain` to place at the location. |
| `locations` | List of `overmap_location` ids that this overmap terrain may be placed on.  If not specified, defaults to the `locations` value from the special definition. |
| `north`     | Id of the join which must align with the north edge of this OMT |
| `east`      | Id of the join which must align with the east edge of this OMT |
| `south`     | Id of the join which must align with the south edge of this OMT |
| `west`      | Id of the join which must align with the west edge of this OMT |
| `above`     | Id of the join which must link this to the OMT above |
| `below`     | Id of the join which must link this to the OMT below |

### Generation rules

| Identifier  |                                Description                                 |
| ----------- | -------------------------------------------------------------------------- |
| `overmap`   | Id of the `overmap` to place. |
| `max`       | Maximum number of times this rule should be used. |
| `weight`    | Weight with which to select this rule. |

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
| `flags`      | See `Overmap connections` in [JSON_FLAGS.md](JSON_FLAGS.md).                                               |

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
