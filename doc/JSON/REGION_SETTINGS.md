<!-- TODO: ToC -->

# Region Settings

The **region_settings** define the attributes for map generation that apply to an entire region.
A region spans the entire game world but is handled on a per-overmap basis (see doc/JSON/OVERMAP.md).

The `region_settings` type has the following fields. 
NOTE: most of the below settings are stored in separate types and are referred to by id; 
see their info later in this document. 

### Fields

|       Identifier         |               Type                  |                            Description                                |
| ------------------------ | ----------------------------------- | --------------------------------------------------------------------- |
| `type`                   |                                     | Type identifier. Must be "region_settings".                           |
| `id`                     |                                     | Unique identifier for this region.                                    |
| `tags`                   | array of string                     | An arbitrary list of tags for overlays to apply to.                   |
| `place_swamps`           | boolean                             | Controls whether or not swamps will be placed (requires forests to be placed) |
| `place_roads`            | boolean                             | Whether or not to generate road connections |
| `place_railroads`        | boolean                             | Whether or not to generate railroad connections |
| `place_railroads_before_roads` | boolean                       | Generates railroads before roads if true |
| `place_specials`         | boolean                             | Controls placement of overmap specials |
| `neighbor_connections`   | boolean                             | Generate connections between neighboring overmaps |
| `max_urbanity`           | integer                             | Max urbanity (a multiplier on city size) |
| `urbanity_increase`      | array of numbers                    | Increase in urbanity to the [ north, east, south, west ] of overmap (0,0) |
| `rivers`                 | `region_settings_river`             | River generation parameters; Use `null` to disable river generation.  |
| `lakes`                  | `region_settings_lake`              | Defines parameters for generating lakes in the region. `null` to disable. |
| `ocean`                  | `region_settings_ocean`             | Defines parameters for generating oceans in the region. `null` to disable. |
| `ravines`                | `region_settings_ravine`            | Defines parameters for generating ravines in the region. `null` to disable. |
| `forests`                | `region_settings_forest`            | Defines parameters for generating forests and swamps in the region. `null` to disable forest generation. |
| `forest_composition`     | `region_settings_forest_mapgen`     | Defines flora (and "stuff") that cover the `forest` terrain types.    |
| `forest_trails`          | `region_settings_forest_trail`      | Defines the overmap and local structure of forest trails. `null` to disable. |
| `highways`               | `region_settings_highway`           | Defines parameters for generating highways in the region. `null` to disable. |
| `cities`                 | `region_settings_city`              | Defines the structural compositions of cities. `null` to disable city generation. |
| `map_extras`             | `region_settings_map_extras`        | Defines the map extra groups referenced by overmap terrains.          |
| `terrain_furniture`      | `region_settings_terrain_furniture` | Defines the resolution of regional terrain/furniture to actual types. |
| `weather`                | `weather_generator`                 | Defines the base weather attributes for the region.                   |
| `default_oter`           |  array of `oter_id`                 | Default overmap terrain for this region, listed from z=10 to z=-10    |
| `default_groundcover`    |  weighted list of `oter_id`         | List of terrain types and weights applied as default ground cover.    |
| `feature_flag_settings`  |  two lists of strings               | Defines operations on overmap features based on their flags.          |
| `connections`            | `overmap_connection_id` fields      | Defines what overmap connections this region uses.                    |

### Example
```jsonc
{
    "type": "region_settings",
    "id": "default",
    "rivers": "default",
    "lakes": "default",
    "ocean": "default",
    "ravines": "default",
    "forests": "default",
    "forest_composition": "default",
    "forest_trails": "default",
    "highways": "default",
    "cities": "default",
    "map_extras": "default",
    "terrain_furniture": "default",
    "weather": "default",
    "default_oter": [
      "open_air",
      ...
    ],
    "default_groundcover": [ [ "t_region_groundcover", 1 ] ],
    "feature_flag_settings": { "blacklist": [  ], "whitelist": [  ] }
  },
```

# Region Overlay

Loading `region_settings` poses an issue: what if we only want to add a map extra to 
a set of regions instead of redefining the entire `region_settings` object? We can `copy-from`,
but if more than one mod is loaded, we would have multiple separate `region_settings` objects
but only one region available to use for the world.

We get around these issues by using a `region_overlay`.

A `region_overlay` is capable of extending the following types:
- `region_settings_city`
- `region_settings_highway`
- `region_settings_forest_trail`
- `region_settings_terrain_furniture`
- `region_settings_map_extras`*
- `region_settings_forest_composition`* 

NOTE: the types with asterisks have collections of other types that must 
be extended separately using "overlay_id". These types are:

- `map_extra_collection`
- `forest_biome`
- `forest_biome_component`

Study the below examples carefully; they are subtly different.

The following example will append city building "dinozoo" to all 
`region_settings_city` parks in all regions.

### Example
```jsonc
{
    "type": "region_overlay",
    "id": "default_dinomod",
	"apply_to_tags": [ "all" ],
    "cities": "default_dinomod"
},
{
    "type": "region_settings_city",
    "id": "default_dinomod",
    "parks": [ [ "dinozoo", 25 ] ]
}
```

The following example will append the map extra *collections* in "default_crazy" 
to map extra *collections* in all regions.
### Example
```jsonc
{
    "type": "region_overlay",
    "id": "default_crazy",
    "apply_to_tags": [ "all" ],
    "map_extras": "default_crazy"
},
{
    "type": "region_settings_map_extras",
    "id": "default_crazy",
    "extras": [ "forest_crazy" ]
},
{
    "type": "map_extra_collection",
    "id": "forest_crazy",
    "extras": [ [ "mx_shia", 1 ] ]
}
```

By adding "overlay_id": "forest" to the `map_extra_collection`, the above example would
extend `map_extra_collection` "id": "forest", spawning "mx_shia" along with the
other map extras in "forest".

### Example
```jsonc
{
    "type": "map_extra_collection",
    "id": "forest_crazy",
	"overlay_id": "forest",
    "extras": [ [ "mx_shia", 1 ] ]
}
```

### Tags

The "apply_to_tags" field will apply the overlay to any `region_settings` with
matching `tags`. Currently, using the tag "all" will apply the overlay to all regions:
```jsonc
{
    "apply_to_tags": [ "all" ],
}
```

If you wanted to apply the overlay only for regions with a "default" tag:
```jsonc
{
    "apply_to_tags": [ "default" ],
}
```

Tags are arbitary strings and are only used to avoid referencing a likely-growing number
of region_settings ids.

## Region Terrain / Furniture

**region_terrain_furniture** defines the resolution of regional terrain/furniture
(with flag REGION_PSEUDO) to their actual terrain and furniture types for the region, with a weighted list for
terrain/furniture entry that defines the relative weight of a given entry when mapgen resolves the
regional entry to an actual entry.

A list of all regional terrain/furniture can be found in:
data/json/furniture_and_terrain/furniture_regional_pseudo.json
data/json/furniture_and_terrain/terrain_regional_pseudo.json

Mod developers should define a mapping for every listed regional terrain/furniture;
they will be placed regardless of whether they're defined or not!

`region_terrain_furniture` contains the terrain/furniture id and the weighted lists they resolve to:
NOTE: only one of `ter_id` or `furn_id` and their respective lists can be used; terrain cannot map
to furniture and vice-versa.

| Identifier               |                            Description                                      |
| ------------------------ | --------------------------------------------------------------------------- |
| `ter_id`                 | Regional terrain id.                                                        |
| `replace_with_terrain`   | Weighted list of actual terrain replacing "ter_id".                         |
| `furn_id`                | Regional furniture id.                                                      |
| `replace_with_furniture` | Weighted list of actual furniture replacing "furn_id".                      |

### Example
```jsonc
{
    "type": "region_terrain_furniture",
    "id": "default_t_groundcover",
    "ter_id": "t_region_groundcover",
    "replace_with_terrain": [ [ "t_grass", 12000 ], [ "t_grass_dead", 2000 ], [ "t_dirt", 1000 ] ]
}
```

`region_settings_terrain_furniture` is simply a list of all `region_terrain_furniture`
mappings for the region:

### Fields

| Identifier  |                            Description                                      |
| ----------- | --------------------------------------------------------------------------- |
| `ter_furn`  | List of regional terrain/furniture and their corresponding weighted lists.  |

### Example
```jsonc
{
    "type": "region_settings_terrain_furniture",
    "id": "default",
    "ter_furn": [
      "default_t_groundcover",
      "default_t_groundcover_urban",
      "default_t_groundcover_forest",
	  ...
	]
}
```

## Region Lake Settings

**region_settings_lake** defines the attributes used in generating lakes on the
overmap. The actual placement of these features is determined globally across all overmaps so that
the edges of the features align, and these parameters are mostly about how those global features
are interpreted.

### Fields

|                 Identifier                 |                                 Description                                 |
| ------------------------------------------ | --------------------------------------------------------------------------- |
| `noise_threshold_lake`                     | [0, 1], x > value spawns a `lake_surface` or `lake_shore`.                  |
| `lake_size_min`                            | Minimum size of the lake in overmap terrains for it to actually spawn.      |
| `lake_depth`                               | Depth of lakes, expressed in Z-levels (e.g. -1 to -10).                     |
| `shore_extendable_overmap_terrain`         | List of overmap terrains that can be extended to the shore if adjacent.     |
| `shore_extendable_overmap_terrain_aliases` | Overmap terrains to treat as different overmap terrain for extending shore. |
| `invert_lakes`                             | Invert drawing of lakes. What would be a lake is land, what would be land is a lake. |
| `shore_ter`                                | Overmap terrain id of shore terrain place on boundary with land. |
| `surface_ter`                              | Overmap terrain id of surface terrain, placed on z = 0. |
| `interior_ter`                             | Overmap terrain id of interior terrain, placed between surface terrain and bed terrain. |
| `bed_ter`                                  | Overmap terrain id of bed terrain, placed on bottom of the lake. |

### Example

```jsonc
{
    "type": "region_settings_lake",
    "id": "default",
    "noise_threshold_lake": 0.25,
    "lake_size_min": 20,
    "lake_depth": -5,
    "shore_extendable_overmap_terrain": [ "forest", "forest_thick", "forest_water", "field" ],
    "shore_extendable_overmap_terrain_aliases": [
      { "om_terrain": "island_forest", "om_terrain_match_type": "TYPE", "alias": "forest" },
      { "om_terrain": "island_forest_thick", "om_terrain_match_type": "TYPE", "alias": "forest_thick" },
      { "om_terrain": "island_forest_water", "om_terrain_match_type": "TYPE", "alias": "forest_water" },
      { "om_terrain": "island_field", "om_terrain_match_type": "TYPE", "alias": "field" }
    ]
}
```

## Region Forest Settings

**region_settings_forest** defines the attributes used in *broadly* generating forest and swamps
on the overmap. The actual placement of these features is determined globally across all overmaps
so that the edges of the features align, and these parameters are mostly about how those global
features are interpreted.

Two noise functions with values between 0 and 1 are generated over the entire map. The noise threshold values control which values from the noise function correspond to what terrain.

### Fields

|               Identifier               |                              Description                               |
| -------------------------------------- | ---------------------------------------------------------------------- |
| `noise_threshold_forest`               | [0, 1], x > value spawns `forest`.                                     |
| `noise_threshold_forest_thick`         | [0, 1], x > value spawns `forest_thick`.                               |
| `noise_threshold_swamp_adjacent_water` | [0, 1], x > value spawns `forest_water` if forest near a waterbody.    |
| `noise_threshold_swamp_isolated`       | [0, 1], x > value spawns `forest_water` if forest isolated from water. |
| `river_floodplain_buffer_distance_min` | Minimum buffer distance in overmap terrains for river floodplains.     |
| `river_floodplain_buffer_distance_max` | Maximum buffer distance in overmap terrains for river floodplains.     |
| `forest_threshold_limit`               | Number. Maximum increase in forest threshold value due to distance from overmap (0,0) |
| `forest_threshold_increase`               | Array of numbers. [ north, east, south, west ] change in forest noise threshold per overmap from overmap (0,0) |

### Example

```jsonc
{
    "type": "region_settings_forest",
    "id": "default",
    "noise_threshold_forest": 0.2,
    "noise_threshold_forest_thick": 0.25,
    "noise_threshold_swamp_adjacent_water": 0.3,
    "noise_threshold_swamp_isolated": 0.6,
    "river_floodplain_buffer_distance_min": 3,
    "river_floodplain_buffer_distance_max": 15
}
```

## Region Forest Map Generation Settings

**region_settings_forest_mapgen** defines the attributes used in generating forest terrains, 
including their items, groundcover, terrain and furniture.

### Biomes

At the top level, `forest_mapgen_settings` is a collection of named configurations we
will call `forest_biome_mapgen` (not to be confused with regions; these are strictly for forests). 

It is possible to define settings for overmap terrains that are not rendered by the forest mapgen, 
but will be used when blending forest terrains with other terrain types.

```jsonc
{
    "type": "region_settings_forest_mapgen",
    "id": "default",
    "biomes": [ "biome_forest_default", "biome_forest_thick_default", "biome_forest_water_default" ]
  },
```

`forest_biome_mapgen` fields include:

### Fields

|          Identifier           |                                 Description                                  |
| ----------------------------- | ---------------------------------------------------------------------------- |
| `terrains`                    | The overmap terrain IDs which have this biome.                               |
| `sparseness_adjacency_factor` | Value relative to neighbors controls how dense the overmap terrain will be.  |
| `item_group`                  | Item group used to place items randomly within the overmap terrain.          |
| `item_group_chance`           | % chance, between 1 and 100, that an item will be placed.                    |
| `item_spawn_iterations`       | Number of times that the item spawning will be called.                       |
| `groundcover`                 | Weighted list of terrains used for base groundcover.                         |
| `components`                  | Collection of components that make up the terrains and furniture placed.     |
| `terrain_furniture`           | Collection of furniture conditionally placed based on terrain.               |

### Example

```jsonc
{
    "type": "forest_biome_mapgen",
    "id": "biome_forest_water_default",
    "terrains": [ "forest_water", "hunter_shack", "hunter_shack_1", "shipwreck_river_1", "shipwreck_river_2" ],
    "sparseness_adjacency_factor": 2,
    "item_group": "forest",
    "item_group_chance": 60,
    "item_spawn_iterations": 1,
    "groundcover": [ [ "t_region_groundcover_swamp", 1 ] ],
    "components": [ "trees_water", "flowers_and_shrubs_water", "clutter_water", "water_water" ],
    "terrain_furniture": { 
	  "t_water_murky": { 
	    "chance": 2, 
		"furniture": [ [ "f_region_water_plant", 1 ] ] 
		} 
	}
}
```

### Components

The "components" field of forest biomes (`forest_biome_component`) are a collection of named objects with a 
sequence, chance, and set of types that, during mapgen, are rolled in sequence to pick a feature to be 
placed at a given location. The names for the components are only relevant for the purposes of overriding 
them in region overlays.

### Fields

|  Identifier   |                             Description                              |
| ------------- | -------------------------------------------------------------------- |
| `sequence`    | Sequence in which components are processed.                          |
| `chance`      | One in X chance that something from this component will be placed.   |
| `clear_types` | Clear all previously defined `types` for this component.             |
| `types`       | Weighted list of terrains and furniture that make up this component. |

### Example

```jsonc
{
    "type": "forest_biome_component",
    "id": "clutter_default",
    "sequence": 2,
    "chance": 80,
    "types": [
      [ "t_trunk", 128 ],
      [ "t_dirtmound", 128 ],
      [ "f_boulder_small", 128 ],
      [ "f_rubble_rock", 32 ],
      [ "f_boulder_medium", 8 ],
      [ "f_boulder_large", 1 ],
      [ "t_pit", 1 ],
      [ "t_pit_shallow", 1 ]
    ]
}
```

### Forest Biome Terrain-Conditional Furniture

Distinct from `region_settings_terrain_furniture`, the forest biome "terrain_furniture" field
picks furniture to place only on forest terrain. 
In the above biome example, "f_region_water_plant" is placed on "t_water_murky". 
NOTE: "f_region_water_plant" could be placed in the `components` section, 
but that would not guarantee its placement on "t_water_murky" only.

### Fields

|    Identifier     |                            Description                             |
| ----------------- | ------------------------------------------------------------------ |
| `chance`          | One in X chance that furniture from this component will be placed. |
| `furniture`       | Weighted list of furniture that will be placed on this terrain.    |

## Region Forest Trail Settings

**region_settings_forest_trail** defines the attributes used in generating trails in the
forests, including their likelihood of spawning, their connectivity, their chance for spawning
trailheads, and some general tuning of the actual trail width/position in mapgen.

### Fields

|         Identifier         |                                         Description                                         |
| -------------------------- | ------------------------------------------------------------------------------------------- |
| `chance`                   | One in X chance a contiguous forest will have a trail system.                               |
| `border_point_chance`      | One in X chance that the N/S/E/W-most point of the forest will be part of the trail system. |
| `minimum_forest_size`      | Minimum contiguous forest size before a trail system can be spawned.                        |
| `random_point_min`         | Minimum # of random points from contiguous forest used to form trail system.                |
| `random_point_max`         | Maximum # of random points from contiguous forest used to form trail system.                |
| `random_point_size_scalar` | Forest size is divided by this and added to the minimum number of random points.            |
| `trailhead_chance`         | One in X chance a trailhead will spawn at end of trail near field.                          |
| `trailhead_road_distance`  | Maximum distance trailhead can be from a road and still be created.                         |
| `trailheads`               | Weighted list of overmap specials / city buildings that will be placed as trailheads.       |

### Example

```jsonc
{
    "type": "region_settings_forest_trail",
    "id": "default",
    "chance": 2,
    "border_point_chance": 2,
    "minimum_forest_size": 100,
    "random_point_min": 4,
    "random_point_max": 50,
    "random_point_size_scalar": 100,
    "trailhead_chance": 1,
    "trailhead_road_distance": 6,
    "trailheads": { "trailhead_basic": 1, "trailhead_outhouse": 1, "trailhead_shack": 1 }
}
```

## Overmap Connection Settings

The **overmap_connection_settings** section defines the `overmap_connection_id`s used in hardcoded placement.

### Fields

|          Identifier          |                                                    Description                                     |
| ---------------------------- | -------------------------------------------------------------------------------------------------- |
| `intra_city_road_connection` | overmap_connection id used within cities. Should include locations for road and road_nesw_manhole. |
| `inter_city_road_connection` | overmap_connection id used between cities. Should include locations for the intra city terrains.   |
| `trail_connection`           | overmap_connection id used for forest trails.                                                      |
| `sewer_connection`           | overmap_connection id used for sewer connections.                                                  |
| `subway_connection`          | overmap_connection id used for for both z-2 and z-4 subway connections.                            |
| `rail_connection`            | overmap_connection id used for rail connections. ( Unused in vanilla as of 0.H )                   |

### Example

```jsonc
{
	"overmap_connection_settings": {
		"intra_city_road_connection": "cobbled_road",
		"inter_city_road_connection": "rural_road"
	}
}
```


## Overmap Highway Settings

The **overmap_highway_settings** section defines the attributes used in generating highways
on the overmap including the specials containing the maps used.

### Fields

|               Identifier          |                              Description                               
| --------------------------------- | -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `grid_column_seperation`          | The distance between north-south highways in overmaps, with the whole overmap gap being `grid_column_seperation` - 1.                                                                                                                        |
| `grid_row_seperation`             | The distance between east-west highways in overmaps, with the whole overmap gap being `grid_row_seperation` - 1.                                                                                                                             |
| `width_of_segments`               | The width of the segments defined below in `om_terrain`s. Used to tell the C++ what width the segments provided are, not to change the width placed.                                                                                         |
| `straightness_chance`             | For one overmap, the chance for a highway's end points to (mostly) line up.                                                                                                                                                                  |
| `intersection_max_radius`         | The maximum number of overmaps that an intersection can deviate from its gridded position. Cannot be greater than or equal to row / 2 or column / 2, may cause bugs for > row / 4, column / 4.                                               |
| `reserved_terrain_id`             | The `om_terrain` used to reserve land and air for highways before their actual `om_terrain` placement.                                                                                                                                       |
| `reserved_terrain_water_id`       | The `om_terrain` used to reserve water for highways before their actual `om_terrain` placement.                                                                                                                                              |
| `four_way_intersections`          | An object with a list of specials and their respective weights to place at four way highway intersections. The [0,0,0] point should be the NW corner of the intersection formed before placement.                                            |
| `three_way_intersections`         | An object with a list of specials and their respective weights to place at three way highway intersections, currently only found near ocean. The [0,0,0] point is the NW corner of the intersection formed before placement before rotation. |
| `bends`                           | An object with a list of specials and their respective weights to place at highway bends. The [0,0,0] point is the NW corner of the intersection formed before placement before rotation. All specials in this set MUST be square for X/Y!   |
| `road_connections`                | An object with a list of specials and their respective weights to potentially place for each compass direction of placed highway for road connections and rest stops. The [0,0,0] point is the west side of the highway before rotation.     |
| `segment_flat_special`            | The (width_of_segments x 1 x 1) special that corresponds to a flat section of highway.                                                                                                                                                       |
| `segment_road_bridge_special`     | The (width_of_segments x 1 x 2) special that corresponds to a flat section of highway with a road crossing.                                                                                                                                  |
| `segment_bridge_special`          | The (width_of_segments x 1 x 2) special that corresponds to an elevated section of highway for crossing water.                                                                                                                               |
| `segment_bridge_supports_special` | The (width_of_segments x 1 x 1) special that corresponds to bridge supports to place below segment_bridge_special down to the base of the water.                                                                                             |
| `segment_overpass_special`        | The (width_of_segments x 1 x 2) special that corresponds to an elevated section of highway for crossing cities.                                                                                                                              |
| `fallback_onramp_special`         | The (width_of_segments x 1 x 1) special used to end highways if they can't be drawn, like for near oceans.                                                                                                                                   |
| `clockwise_slant_special`         | Special for meandering (in cases where bends can't) in a clockwise direction relative to direction of highway generation.                                                                                                                    |
| `counterclockwise_slant_special`  | Special for meandering (in cases where bends can't) in a counterclockwise direction relative to direction of highway generation.                                                                                                             |
| `fallback_supports`               | Dummy special; during finalization, places (1 x 1) special "highway_support_mutable".                                                                                                                                                        |

### Example

```json
    "overmap_highway_settings": {
      "grid_column_seperation": 1,
      "grid_row_seperation": 1,
      "width_of_segments": 2,
      "reserved_terrain_id": "highway_reserved",
      "reserved_terrain_water_id": "highway_reserved_water",
      "four_way_intersections": { "Highway crossroad": 1 },
      "three_way_intersections": { "Highway tee": 1 },
      "bends": { "Highway bend": 1 },
      "road_connections": { "Highway service station": 1, "Highway on/off ramps": 3 },
      "segment_flat_special": "highway_segment_flat",
      "segment_road_bridge_special": "highway_segment_road_bridge",
      "segment_bridge_special": "highway_segment_bridge",
      "segment_bridge_supports_special": "highway_segment_bridge_supports",
      "segment_overpass_special": "highway_segment_overpass",
      "symbolic_ramp_up_id": "highway_symbolic_ramp_up",
      "symbolic_ramp_down_id": "highway_symbolic_ramp_down",
      "symbolic_overpass_road_id": "highway_symbolic_road"
    },
```

## Region City Settings

**region_settings_city** defines the possible overmap terrains and specials that may be used as
buildings in a city, their weighted chances of placement, and some attributes that control the
relative placements of various classes of buildings.

### Fields

|       Identifier        |                            Description                             |
| ----------------------- | ------------------------------------------------------------------ |
| `name_snippet`          | Snippet used to generate city names. Default is `<city_name>`.     |
| `shop_radius`           | Radial frequency of shop placement. Smaller number = more shops.   |
| `park_radius`           | Radial frequency of park placement. Smaller number = more parks.   |
| `houses`                | Weighted list of overmap terrains and specials used for houses.    |
| `parks`                 | Weighted list of overmap terrains and specials used for parks.     |
| `shops`                 | Weighted list of overmap terrains and specials used for shops.     |

### Placing shops, parks, and houses

When picking a building to place in a given location, the game considers the city size, the
location's distance from the city center, and finally the `shop_radius` and `park_radius` values for
the region. It then tries to place a shop, then a park, and finally a house, where the chance to
place the shop or park are based on the formula `rng( 0, 99 ) > X_radius * distance from city center
/ city size`.

### Example
```jsonc
{
    "type": "region_settings_city",
    "id": "default",
    "shop_radius": 30,
    "shop_sigma": 50,
    "park_radius": 20,
    "park_sigma": 80,
	"houses": [
		["house_two_story_basement", 1],
		["house", 1000],
		["house_base", 333],
		["emptyresidentiallot", 20]
	],
	"parks": [
		["park", 4],
		["pool", 1]
	],
	"shops": [
		["s_gas", 5],
		["s_pharm", 3],
		["s_grocery", 15]
	]
}
```

## Region Map Extras

**region_settings_map_extras** section defines the named collections of map extras -- special mapgen events
applied on top of the defined mapgen -- that may be referenced by the `extras` property of an overmap
terrain. This includes both the chance of an extra occurring as well as the weighted list of extras.

`region_settings_map_extras` is a list of map extra collections:

### Fields

| Identifier |             Description                 |
| ---------- | ----------------------------------------|
| `extras`   | List of `map_extra_collection` ids.     |

### Example

```jsonc
{
    "type": "region_settings_map_extras",
    "id": "default",
    "extras": [
      "forest",
      "forest_thick",
      "forest_water",
      "field",
      ...
    ]
}
```

### Map Extra Collections

See above.

### Fields

| Identifier |                           Description                            |
| ---------- | ---------------------------------------------------------------- |
| `chance`   | One in X chance that the overmap terrain will spawn a map extra. |
| `extras`   | Weighted list of map extras that can spawn.                      |

### Example

```jsonc
{
    "type": "map_extra_collection",
    "id": "forest",
    "chance": 40,
    "extras": [
      [ "mx_blackberry_patch", 1000 ],
      [ "mx_point_dead_vegetation", 500 ],
      [ "mx_point_burned_ground", 500 ],
      [ "mx_grove", 500 ],
      ...
    ]
}
```

## Weather

**weather** defines the base weather attributes used for the region.

### Fields

|     Identifier                 |                              Description                              |
| ------------------------------ | --------------------------------------------------------------------- |
| `base_temperature`             | Base temperature for the region in degrees Celsius.                   |
| `base_humidity`                | Base humidity for the region in relative humidity %                   |
| `base_pressure`                | Base pressure for the region in millibars.                            |
| `base_wind`                    | Base wind for the region in mph units. Roughly the yearly average.    |
| `base_wind_distrib_peaks`      | How high the wind peaks can go. Higher values produce windier days.   |
| `base_wind_season_variation`   | How the wind varies with season. Lower values produce more variation  |
| `weather_black_list`           | Ids of weather types not allowed in this region.                      |
| `weather_white_list`           | Ids of the only weather types allowed in this region.                 |

### Example

```jsonc
{
    "type": "weather_generator",
    "id": "default",
    "base_temperature": 6.5,
    "base_humidity": 70.0,
    "base_pressure": 1015.0,
    "base_wind": 3.4,
    "base_wind_distrib_peaks": 80,
    "base_wind_season_variation": 50
}
```

## Region Feature Flag Settings

**feature_flag_settings** defines flags assigned to overmap features.
This is currently used to provide a mechanism for whitelisting and blacklisting locations on a per-region basis.

### Fields

|    Identifier     |                                        Description                                         |
| ----------------- | ------------------------------------------------------------------------------------------ |
| `blacklist`       | List of flags. Any location with a matching flag will be excluded from overmap generation. |
| `whitelist`       | List of flags. Only locations with a matching flag will be included in overmap generation. |

### Example

```jsonc
{
	"overmap_feature_flag_settings": {
		"blacklist": [ "FUNGAL" ],
		"whitelist": []
	}
}
```
