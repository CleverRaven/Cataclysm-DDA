# Vehicle prototypes

Vehicle prototypes are used to spawn stock vehicles. After a vehicle has been spawned, it is saved in a different format.

`copy-from` may be used in a limited form to copy fields from a "parent" definition; first the copy-from data is applied and then the parts, items and zones fields are appended on top of the `copy-from` data.

## Vehicle prototypes

```jsonc
"type": "vehicle",
"id": "sample_vehicle",                    // Unique ID. Must be one continuous word,
                                           // use underscores if necessary.
"name": "Sample Vehicle",                  // In-game name displayed
"blueprint": [                             // Preview of vehicle - ignored by the code,
    "o#o",                                 // so use only as documentation
    "o#o"
],
"parts": [                                 // Parts list
    { "x": 0, "y": 0, "parts": [ "frame" ] },   // Part definition, positive x direction is up,
    { "x": 0, "y": 0, "parts": [ "seat" ] },    // positive y is to the right
    { "x": 0, "y": 0, "parts": [ "controls" ] }, // See vehicle_parts.json for part ids
    { "x": 1, "y": 1, "parts": [ "veh_tools_workshop", "tools": [ "welder" ] ] },  // spawn attached tools
    { "x": 0, "y": 1, "parts": [ "frame", "seat" ] },
    { "x": 0, "y": 1, "parts": [ { "part": "tank", "fuel": "gasoline" }, "battery_car" ] },
    { "x": 0, "y": 1, "parts": [ "stereo" ] }, // order matters - see below
    { "x": 1, "y": 0, "parts": [ "frame#cover", "wheel_mount_medium", "wheel" ] },
    { "x": 1, "y": 1, "parts": [ "frame#cross", "wheel_mount_medium", "wheel" ] },
    { "x": -1, "y": 0, "parts": [ "frame#cover", "wheel_mount_medium", "wheel" ] },
    { "x": -1, "y": 1, "parts": [ "frame#cross", "wheel_mount_medium", "wheel" ] }
],
"items": [                                 // Item spawn list
    { "x": 0, "y": 0, "items": "helmet_army" },   // individual item
    { "x": 0, "y": 0, "item_groups": "army_uniform" }, // item or items from an item_group
    { "x": 0, "y": 1, "items": [ "matchbook", "2x4" ] }, // all items in the list spawn
    { "x": 0, "y": 0, "item_groups": [ "army_uniform", "rare_guns" ] } // all item_groups are processed
]
"zones": [
    { "x": -3, "y": 0, "type": "LOOT_AMMO" }
]
```

### Part order

.* Important! *.
Vehicle parts must be defined in the same order you would install them in the game: a wheel hub must be added prior to the wheel, but after the frame.  You also cannot break the normal rules of installation (you can't stack non-stackable part flags).

### Parts list
The part list contains an arbitrary number of lines. Each line is of the form:
{ "x": X, "y": Y, "parts": [ "PARTID1", { "part": "PARTID2", "fuel": "gasoline", ... }, "PARTID3"... ] }

The line defines parts at location X,Y. Strings, like "PARTID1" and "PARTID3", spawn parts with no extra options. PARTID2 is using object form, this way optional keys can be used to define extra options (see below for a list).

Several different lines can have the same X, Y co-ordinates and each one adds additional parts to that location.

### Extra options

#### Turrets

`ammo` - integer in range of [0-100], percent chance of turret spawning with ammo, 100 always spawns ammo.

`ammo_types` - an array of itype_id strings, random ammo type will be chosen to spawn

`ammo_qty` - an array of 2 integers, representing min and max amount of ammo to spawn

#### Fuel tanks

`fuel`, an itype_id string - for liquid tank parts, which liquid type to fill the tank with

#### VEH_TOOLS parts

`tools`, an array of itype_id strings - for VEH_TOOLS parts, which tool types should spawn attached

### Part variants
Some parts can have multiple variants; each variant can define the symbols and broken symbols, also each variant is a tileset sprite, if the tileset defines one for the variant.

If a part has variants, the specific variant can be specified in the vehicle prototype by appending the variant to the part id after a `#` symbol.  Thus, `"frame#cross"` is the `"cross"` variant of the `"frame"` part.

Variants perform a mini-lookup chain by slicing variant string until the next `_` from the right until a match is found.
For example the tileset lookups for `seat_leather#windshield_left` are as follows:

* `vp_seat_leather_windshield_left`

* `vp_seat_leather_windshield`

( At this point variant is completely gone and default tile is looked for: )

* `vp_seat_leather`

( If still no match is found then the `looks_like` field of `vp_seat_leather` is used and tileset looks for: )

* `vp_seat`

### Items list
The items list contains an arbitrary number of lines. Each line is of the form:
    { "x": X, "y": Y, TYPE: DATA },
and describes the items that may spawn at that location.
TYPE and DATA may be one of:
```jsonc
"items": "itemid"                                   // single item of that type
"items": [ "itemid1", "itemid2", /* ... */ ]        // all the items in the array
"item_groups": "groupid"                            // one or more items in the group, depending on
                                                    // whether the group is a collection or distribution
"item_groups": [ "groupid1", "groupid2" /* ... */ ] // one or more items for each group
```
the optional keyword "chance" provides an X in 100 chance that a particular item definition will spawn.

If a single item is specified through `"items"`, an itype variant for it can be specified through `"variant"`.

Multiple lines of items may share the same X and Y values.

### Zones list
The zones list contains an arbitrary number of lines. Each line is of the form:
    { "x": X, "y": Y, "type": ZONE_ID }
where ZONE_ID is any valid zone id such as `LOOT_UNSORTED`.
These zones are only placed if the vehicle has a faction owner.

## Export vehicle prototypes using debug menu
There's an function in debug menu to generate a prototype from a vehicle. To use this, step on any vehicle in the game, then go to debug menu "Vehicle -> Export vehicle as prototype".
By using this, you can visually and interactively edit the vehicle in the game, then export the prototype as a json file. The file needs to be processed by the json formatter for readability.
Supported functionality: Exporting the parts, their variants, ammo of the turrets, fuel of the tanks, attached tools, loot zones bound to the vehicle, the blueprint ( the vehicle's appearance )and simple top-level items in the cargo space.
Note that support for exporting items is rather imperfect. Do not rely on this for complicated spawning of items.
Example of exported json ( after formatting ):
```jsonc
{
  "//1": "Although this vehicle can be readily spawned in game without further tweaking,",
  "//2": "this is auto-generated by program, so please check against it before proceeding.",
  "//3": "Only simple top-level items can be exported by this function. Do not rely on this for exporting comestibles, containers, etc.",
  "id": "/TO_BE_REPLACED/",
  "type": "vehicle",
  "name": "/TO_BE_REPLACED/",
  "blueprint": [
    "t&"
  ],
  "parts": [
    {
      "x": 0,
      "y": 0,
      "parts": [
        "frame#vertical_T_left",
        { "part": "tank_medium", "fuel": "diesel" },
        "diesel_engine_v6",
        "alternator_car",
        "turret_mount",
        { "part": "turret_m240", "ammo": 100, "ammo_types": "762_51", "ammo_qty": [ 500, 500 ] },
        "seat#swivel_chair_front"
      ]
    },
    {
      "x": 0,
      "y": 1,
      "parts": [ "frame#vertical_T_right", { "part": "veh_tools_workshop", "tools": [ "water_purifier", "pot", "pan" ] } ]
    }
  ],
  "items": [
    { "x": 0, "y": 0, "chance": 100, "items": [ "saw" ] }
  ],
  "zones": [
    { "x": 0, "y": 0, "type": "LOOT_GUNS" }
  ]
}

```
