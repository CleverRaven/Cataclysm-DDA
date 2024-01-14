# Points, tripoints, and coordinate systems

## Axes

The game is three-dimensional, with the axes oriented as follows:
* The **x-axis** goes from left to right across the display (in non-isometric
  views).
* The **y-axis** goes from top to bottom of the display.
* The **z-axis** is vertical, with negative z pointing underground and positive
  z pointing to the sky.

## Coordinate systems

CDDA uses a variety of coordinate systems for different purposes.  These differ
by scale and origin.

The most precise coordinates are **map square** (ms) coordinates.  These refer to
the tiles you see normally when playing the game.

Two origins for map square coordinates are common:
* **Absolute** coordinates, sometimes called global, which are a global system
  for the whole game, relative to a fixed origin.
* **Local** coordinates, which are relative to the corner of the current "reality
  bubble", or `map` roughly centered on the avatar.  In local map square
  coordinates, `x` and `y` values will both fall in the range [0,120).

The next scale is **submap** (sm) coordinates.  One submap is 12x12
(`SEEX`x`SEEY`) map squares.  Submaps are the scale at which chunks of the map
are loaded or saved as they enter or leave the reality bubble.

Next comes **overmap terrain** (omt) coordinates.  One overmap terrain is 2x2
submaps.  Overmap terrains correspond to a single tile on the map view in-game,
and are the scale of chunk of mapgen.

Largest are **overmap** (om) coordinates.  One overmap is 180x180
(`OMAPX`x`OMAPY`) overmap terrains.  Large-scale mapgen (e.g. city layout)
happens one overmap at a time.

Lastly, these is a system called **segment** (seg) coordinates.  These are only
used in saving/loading submaps and you are unlikely to encounter them.

As well as absolute and local coordinates, sometimes we need to use coordinates
relative so some larger scale.  For example, when performing mapgen for a
single overmap, we want to work with coordinates within that overmap.  This
will be an overmap terrain-scale point relative to the corner of its containing
overmap, and so typically take `x` and `y` values in the range [0,180).

## Vertical coordinates

Although `x` and `y` coordinates work at all these various scales, `z`
coordinates are consistent across all contexts.  They lie in the range
[-`OVERMAP_DEPTH`,`OVERMAP_HEIGHT`].

## Vehicle coordinates

Each vehicle has its own origin point, which will be at a particular part of
the vehicle (e.g. it might be at the driver's seat).  The origin can move if
the vehicle is damaged and all the vehicle parts at that location are
destroyed.

Vehicles use two systems of coordinates relative to their origin:

* **mount** coordinates provide a location for vehicle parts that does not
  change as the vehicle moves.  It is the map square of that part, relative to
  the vehicle origin, when the vehicle is facing due east.

* **map square** is the map square, relative to the origin, but accounting for
  the vehicle's current facing.

Vehicle facing is implemented via a combination of rotations (by quarter turns)
and shearing to interpolate between quarter turns.  The logic to convert
between vehicle mount and map square coordinates is complicated and handled by
the `vehicle::coord_translate()` and `vehicle::mount_to_tripoint()` families of
functions.

Currently, vehicle mount coordinates do not have a z-level component, but
vehicle map square coordinates do. The z coordinate is relative to the vehicle
origin.

## Point types

To work with these coordinate systems we have a variety of types.  These are
defined in [`coordinates.h`](../src/coordinates.h).  For example, we have
`point_abs_ms` for absolute map-square coordinates.  The four parts of the
type name are *dimension*`_`*origin*`_`*scale*(_ib).

* **dimension** is either `point` for two-dimensional or `tripoint` for
  three-dimensional.
* **origin** specifies what the value is relative to, and can be:
  * `rel` means relative to some arbitrary point.  This is the result of
    subtracting two points with a common origin.  It would be used for example
    to represent the offset between the avatar and a monster they are shooting
    at.
  * `abs` means global absolute coordinates.
  * `sm` means relative to a corner of a submap.
  * `omt` means relative to a corner of an overmap terrain.
  * `om` means relative to a corner of an overmap.
  * `bub` means local coordinates, relative to the corner of the reality bubble (`get_map()`).
  * `veh` means relative to a vehicle origin.
* **scale** means the scale as discussed above.
  * `ms` for map square.
  * `sm` for submap.
  * `omt` for overmap terrain.
  * `seg` for segment.
  * `om` for overmap.
  * `mnt` for vehicle mount coordinates (only relevant for the `veh` origin).
* The optional **_ib** suffix denotes that the type is guaranteed to be inbounds
  for the given origin. It is only meaningful for `bub` and `sm` origins.

## Raw point types

As well as these types with origin and scale encoded into the type, there are
simple raw point types called just `point` and `tripoint`.  These can be used
when no particular game scale is intended.

At time of writing we are still in the process of transitioning the codebase
away from using these raw point types everywhere, so you are likely to see
legacy code using them in places where the more type-safe points might seem
appropriate.

New code should prefer to use the types which include their coordinate system
where feasible.

## Converting between point types

### Changing scale

To change the scale of a point without changing its origin, use `project_to`.
For example:

```c++
point_abs_ms pos_ms = get_avatar()->global_square_location().xy();
point_abs_omt pos_omt = project_to<coords::omt>( pos_ms );
assert( pos_omt == get_avatar()->global_omt_location().xy() );
```

The same function `project_to` can be used for scaling up or down.  When
converting to a coarser coordinate system precision is of course lost.  If you
care about the remainder then you must instead use `project_remain`.

`project_remain` allows you to convert to a coarser coordinate system and also
capture the remainder relative to that coarser point.  It returns a helper
struct intended to be used with
[`std::tie`](https://en.cppreference.com/w/cpp/utility/tuple/tie) to capture
the two parts of the result.  For example, suppose you want to know which
overmap the avatar is in, and which overmap terrain they are in within that
overmap.

```c++
point_abs_omt abs_pos = get_avatar()->global_omt_location().xy();
point_abs_om overmap;
point_om_omt omt_within_overmap;
std::tie( overmap, omt_within_overmap ) = project_remain<coords::om>( abs_pos );
```

That makes sense for two-dimensional `point` types, but how does it handle
`tripoint`?  Recall that the z-coordinates do not scale along with the
horizontal dimensions, so `z` values are unchanged by `project_to` and
`project_remain`.  However, for `project_remain` we don't want to duplicate the
z-coordinate in both parts of the result, so you must choose exactly one to be
a `tripoint`.  In the example above, z-coordinates do not have much meaning at
the overmap scale, so you probably want the z-coordinate in
`omt_within_overmap`.  Than can be done as follows:

```c++
tripoint_abs_omt abs_pos = get_avatar()->global_omt_location();
point_abs_om overmap;
tripoint_om_omt omt_within_overmap;
std::tie( overmap, omt_within_overmap ) = project_remain<coords::om>( abs_pos );
```

The last available operation for rescaling points is `project_combine`.  This
performs the opposite operation from `project_remain`.  Given two points where
the origin of the second matches the scale of the first, you can combine them
into a single value.  As you might expect from the above discussion, one of
these two can be a `tripoint`, but not both.

```c++
tripoint_abs_omt abs_pos = get_avatar()->global_omt_location();
point_abs_om overmap;
tripoint_om_omt omt_within_overmap;
std::tie( overmap, omt_within_overmap ) = project_remain<coords::om>( abs_pos );
tripoint_abs_omt abs_pos_again = project_combine( overmap, omt_within_overmap );
assert( abs_pos == abs_pos_again );
```

### Changing origin

`project_remain` and `project_combine` facilitate some changes of origin, but
only those origins specifically related to rescaling.  To convert to or from
local or vehicle coordinates requires a specific `map` or `vehicle` object.

For example, to convert between global to local coordinates:
```c++
tripoint_bub_ms local_pos = get_map().bub_from_abs( global_pos );
tripoint_abs_ms global_pos = get_map().getglobal( local_pos );
```

TODO: write some vehicle examples once this is implemented.

## Point operations

We provide standard arithmetic operations as overloaded operators, but limit
them to prevent bugs.  For example, most point types cannot be multiplied by a
constant, but ones with the `rel` origin can (it makes sense to say "half as
far in the same direction").

Similarly, you can't generally add two points together, but you can when one of
them has the `rel` origin, or if one of them is a raw point type.

For computing distances a variety of functions are available, depending on your
requirements: `square_dist`, `trig_dist`, `rl_dist`, `manhattan_dist`.  Other
related utility functions include `direction_from` and `line_to`.

To iterate over nearby points of the same type you can use
`closest_points_first`.
