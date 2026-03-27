# Dimensions
Dimensions are stored within a save folder, they are mostly disconnected from the "main" dimension, aka the dimension stored above `/dimensions` folder. 
When travelling between them, you aren't travelling as much as you are "switching" which dimension data folder is loaded right now.

There's almost no distinction between creating a dimension and loading into it. `u_travel_to_dimension` will just create a dimension if the ID you gave it doesn't correspond to an existing dimension.
The one exception is `region_type`, you're only allowed to change a dimension's `region_settings` object when creating a new dimension. If you're loading into an already existing dimension, the `region_type` variable in `u_travel_to_dimension` will be ignored.  

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

## NPCs

When using `u_travel_to_dimension` EOC, you can choose to take the nearby NPCs with you. In technical terms, When the dimension is being unloaded, the chosen NPCs aren't unloaded and they get to come with you.

Travel to dimension `test`, take any follower within 10 tiles of your avatar with you.
```jsonc
{"u_travel_to_dimension":"test","npc_travel_radius":10,"npc_travel_filter":"follower"}
```
There are also `all` and `enemy` filters you can use.

There's a quirk with NPCs right now, that causes issues when attempting to take them with you to another dimension and then attempt to teleport them to some location within the same turn, you probably have to delay the npc teleportation stage by a turn to get around this. This might get fixed in the future.

## Region Settings

When generating a new dimension, you have the option to supply a [region_settings](REGION_SETTINGS.md) object which the dimension will use to generate it's terrain.

Creating a new dimension `test_dimension` with the `region_settings` object with id `test`.
```jsonc
{"u_travel_to_dimension":"test_dimension","region_type":"test"}
```

If travelling to an already created dimension, the `region_type` variable won't change the dimension's `region_settings` object. If you want that, you need to `clear_dimension` the dimension first.
## Cleanup

If you're going for a temporary dimension or maybe you want the overworld terrain to be regenerated. You can use `clear_dimension` EOC func.

```jsonc
{"clear_dimension":"test"}
```

This will delete the dimension's folder inside `/SAVEFOLDER/dimensions/<dimension_id>`.
You can still `u_travel_to_dimension` into it, but the overworld terrain, items, vehicles, monsters, NPCs, faction hubs and all that will be regenerated in different locations.
You can also supply a different `region_type` value when re-generating a dimension, if for some reason you want that.