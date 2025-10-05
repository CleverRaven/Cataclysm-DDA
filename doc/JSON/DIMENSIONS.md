# Dimensions
Dimensions are worlds that are stored within a save folder, they are mostly disconnected from the "main dimension." 
When travelling between them, you aren't travelling as much as you are "switching" which world data folder is loaded right now.
By design, you aren't allowed to alter world data of unloaded dimensions. Meaning you can't place down structures in unloaded dimensions, place down NPCs, assign missions requiring a location inside the dimension, you can't even view the unloaded dimensions on an in-game map.

To get around this, we need to do all our world alterations/queries when travelling into the dimension.

Warping player to a dimension `portal_cell`, updating the location they teleported with a portal_cell structure and then teleporting the player inside:
```jsonc
    { "u_location_variable": { "global_val": "cell_loc" }, "z_adjust": 0, "z_override": true },
    { "u_location_variable": { "global_val": "cell_roof_loc" }, "z_adjust": 1, "z_override": true },
    { "u_travel_to_dimension": "portal_cell" },
    { "mapgen_update": "portal_cell_roof", "target_var": { "global_val": "cell_roof_loc" } },
    { "mapgen_update": "portal_cell", "target_var": { "global_val": "cell_loc" } },
    {
    "u_location_variable": { "context_val": "player_teleport_loc" },
    "terrain": "t_floor_warped",
    "target_max_radius": 60
    },
    { "u_teleport": { "context_val": "player_teleport_loc" } }
```

## NPCS




There's a quirk with NPCs right now, that causes issues when attempting to take them with you to another dimension and then attempt to teleport them to some location within the same turn, that might get fixed in the future.