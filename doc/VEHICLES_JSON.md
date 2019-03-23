# Vehicle prototypes JSON file contents

Vehicle prototypes are used to spawn stock vehicles. After a vehicle has been spawned, it is saved in a different format.

Vehicle prototypes do not currently accept copy-from

## vehicle prototypes

```C++
"type": "vehicle",
"id": "sample_vehicle",                    // Unique ID. Must be one continuous word,
                                           // use underscores if necessary.
"name": "Sample Vehicle",                  // In-game name displayed
"blueprint": [                             // Preview of vehicle - ignored by the code,
    "o#o",                                 // so use only as documentation
    "o#o"
],
"parts": [                                 // Parts list
    { "x": 0, "y": 0, "part": "frame" },   // Part definition, positive x direction is to the left,
    { "x": 0, "y": 0, "part": "seat" },    // positive y is to the right
    { "x": 0, "y": 0, "part": "controls"}, // See vehicle_parts.json for part ids
    { "x": 0, "y": 1, "parts: [ "frame", "seat" ] }, // Arrays of parts on the same space
    { "x": 0, "y": 1, "parts: [ { "part": "tank", "fuel": "gasoline" }, "battery_car" },
    { "x": 0, "y": 1, "part": "stereo" },  // parts arrays and part may be mixed on the same space
    { "x": 1, "y": 0, "parts: [ "frame, "wheel" ] },
    { "x": 1, "y": 1, "parts: [ "frame, "wheel" ] },
    { "x": -1, "y": 0, "parts: [ "frame, "wheel" ] },
    { "x": -1, "y": 1, "parts: [ "frame, "wheel" ] }
],
"items": [                                 // Item spawn list
    { "x": 0, "y": 0, "items": "helmet_army" },   // individual item
    { "x": 0, "y": 0, "item_groups": "army_uniform" }, // item or items from an item_group
    { "x": 0, "y": 1, "items": [ "matchbook", "two_by_four" ] }, // all items in the list spawn
    { "x": 0, "y": 0, "item_groups": [ "army_uniform", "rare_guns" ] } all item_groups are processed
]
```

.* Important! *. Vehicle parts must be defined in the same order you would install them in the game (ie, frames and mount points first).  You also cannot break the normal rules of installation (you can't stack non-stackable part flags).

### Parts list
The part list contains an arbitary number of lines. Each line is of the form:
    { "x": X, "y": Y, "part": PARTID, ... }
or
    { "x": X, "y": Y, "parts": [ PARTID1, ... ] }

In the first form, the line defines a single part at location X,Y of vehicle part type PARTID. It can have the optional "ammo", "ammo_types", "ammo_qty", or "fuel" keys with an appropriate value following.

In the second form, the line defines several parts at location X, Y. Each part is either defined by its PARTID string, or can be an object of the form
    { "part": PARTID, ... }
with any of the optional keys  "ammo", "ammo_types", "ammo_qty", or "fuel" as above.

Several different lines can have the same X, Y co-ordinates and each one adds additional parts to that location.

### Items list
The items list contains an arbitrary number of lines. Each line is of the form:
    { "x": X, "y": Y, TYPE: DATA },
and describes the items that may spawn at that location.
TYPE and DATA may be one of:
```C++
"items": "itemid"                              // single item of that type
"items": [ "itemid1", "itemid2", ... ]         // all the items in the array
"item_groups": "groupid"                       // one or more items in the group, depending on
                                               // whether the group is a collection or distribution
"item_groups": [ "groupid1", "groupid2" ... ]  // one or more items for each group
```
the optional keyword "chance" provides an X in 100 chance that a particular item definition will spawn.

Multiple lines of items may share the same X and Y values.
