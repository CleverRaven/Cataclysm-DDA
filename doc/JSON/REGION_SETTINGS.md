# Region Settings

The **region_settings** define the attributes for map generation that apply to an entire region.
The general settings define the default overmap terrain and ground cover. Additional sections are
as follows:

|             Section             |                              Description                              |
| ------------------------------- | --------------------------------------------------------------------- |
| `region_terrain_and_furniture`  | Defines the resolution of regional terrain/furniture to actual types. |
| `field_coverage`                | Defines the flora that cover the `field` overmap terrain.             |
| `overmap_lake_settings`         | Defines parameters for generating lakes in the region.                |
| `overmap_forest_settings`       | Defines parameters for generating forests and swamps in the region.   |
| `forest_mapgen_settings`        | Defines flora (and "stuff") that cover the `forest` terrain types.    |
| `forest_trail_settings`         | Defines the overmap and local structure of forest trails.             |
| `city`                          | Defines the structural compositions of cities.                        |
| `map_extras`                    | Defines the map extra groups referenced by overmap terrains.          |
| `weather`                       | Defines the base weather attributes for the region.                   |
| `overmap_feature_flag_settings` | Defines operations on overmap features based on their flags.          |

Note that for the default region, all attributes and sections are required.

### Fields

|       Identifier        |                            Description                             |
| ----------------------- | ------------------------------------------------------------------ |
| `type`                  | Type identifier. Must be "region_settings".                        |
| `id`                    | Unique identifier for this region.                                 |
| `default_oter`          | Default overmap terrain for this region.                           |
| `default_groundcover`   | List of terrain types and weights applied as default ground cover. |


### Example
```jsonc
{
	"type": "region_settings",
	"id": "default",
	"default_oter": "field",
	"default_groundcover": [
		[ "t_grass", 4 ],
		[ "t_dirt", 1 ]
	]
}
```

## Region Terrain / Furniture

The **region_terrain_and_furniture** section defines the resolution of regional terrain/furniture
to their actual terrain and furniture types for the region, with a weighted list for
terrain/furniture entry that defines the relative weight of a given entry when mapgen resolves the
regional entry to an actual entry.

### Fields

| Identifier  |                            Description                             |
| ----------- | ------------------------------------------------------------------ |
| `terrain`   | List of regional terrain and their corresponding weighted lists.   |
| `furniture` | List of regional furniture and their corresponding weighted lists. |

### Example
```jsonc
{
	"region_terrain_and_furniture": {
		"terrain": {
			"t_region_groundcover": {
				"t_grass": 4,
				"t_grass_long": 2,
				"t_dirt": 1
			}
		},
		"furniture": {
			"f_region_flower": {
				"f_black_eyed_susan": 1,
				"f_lily": 1,
				"f_flower_tulip": 1,
				"f_flower_spurge": 1,
				"f_chamomile": 1,
				"f_dandelion": 1,
				"f_datura": 1,
				"f_dahlia": 1,
				"f_bluebell": 1
			}
		}
	}
}
```


## Field Coverage

The **field_coverage** section defines the furniture and terrain that make up the flora that
cover the `field` overmap terrain.

### Fields

|         Identifier         |                                 Description                                  |
| -------------------------- | ---------------------------------------------------------------------------- |
| `percent_coverage`         | % of tiles in the overmap terrain that have a plant.                         |
| `default_ter`              | Default terrain feature for plants.                                          |
| `other`                    | List of features with % chance when `default_ter` isn't used.                |
| `boost_chance`             | % of field overmap terrains with boosted plant growth.                       |
| `boosted_percent_coverage` | % of tiles in the boosted that have a plant.                                 |
| `boosted_other`            | List of features in the boosted with % chance when `default_ter` isn't used. |
| `boosted_other_percent`    | % of `boosted_percent_coverage` that will be covered by `boosted_other`.     |

### Example
```jsonc
{
	"field_coverage": {
		"percent_coverage": 0.9333,
		"default_ter": "t_shrub",
		"other": {
			"t_shrub_blueberry": 0.4166,
			"t_shrub_strawberry": 0.4166,
			"f_mutpoppy": 8.3333
		},
		"boost_chance": 0.833,
		"boosted_percent_coverage": 2.5,
		"boosted_other": {
			"t_shrub_blueberry": 40.0,
			"f_dandelion": 6.6
		},
		"boosted_other_percent": 50.0
	}
}
```

## Overmap Lake Settings

The **overmap_lake_settings** section defines the attributes used in generating lakes on the
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

### Example

```jsonc
{
	"overmap_lake_settings": {
		"noise_threshold_lake": 0.25,
		"lake_size_min": 20,
		"lake_depth": -5,
		"shore_extendable_overmap_terrain": ["forest_thick", "forest_water", "field"],
		"shore_extendable_overmap_terrain_aliases": [
			{ "om_terrain": "forest", "om_terrain_match_type": "TYPE", "alias": "forest_thick" }
		]
	}
}
```

## Overmap Forest Settings

The **overmap_forest_settings** section defines the attributes used in generating forest and swamps
on the overmap. The actual placement of these features is determined globally across all overmaps
so that the edges of the features align, and these parameters are mostly about how those global
features are interpreted.

### Fields

|               Identifier               |                              Description                               |
| -------------------------------------- | ---------------------------------------------------------------------- |
| `noise_threshold_forest`               | [0, 1], x > value spawns `forest`.                                     |
| `noise_threshold_forest_thick`         | [0, 1], x > value spawns `forest_thick`.                               |
| `noise_threshold_swamp_adjacent_water` | [0, 1], x > value spawns `forest_water` if forest near a waterbody.    |
| `noise_threshold_swamp_isolated`       | [0, 1], x > value spawns `forest_water` if forest isolated from water. |
| `river_floodplain_buffer_distance_min` | Minimum buffer distance in overmap terrains for river floodplains.     |
| `river_floodplain_buffer_distance_max` | Maximum buffer distance in overmap terrains for river floodplains.     |

### Example

```jsonc
{
	"overmap_forest_settings": {
		"noise_threshold_forest": 0.25,
		"noise_threshold_forest_thick": 0.3,
		"noise_threshold_swamp_adjacent_water": 0.3,
		"noise_threshold_swamp_isolated": 0.6,
		"river_floodplain_buffer_distance_min": 3,
		"river_floodplain_buffer_distance_max": 15
	}
}
```

## Forest Map Generation Settings

The **forest_mapgen_settings** section defines the attributes used in generating forest (`forest`,
`forest_thick`, `forest_water`) terrains, including their items, groundcover, terrain and
furniture.

### General Structure

At the top level, the `forest_mapgen_settings` is a collection of named configurations, e.g.
`forest`, `forest_thick`, `forest_water`. It is possible to define settings for overmap terrains
that are not rendered by the forest mapgen, but will be used when blending forest terrains with
other terrain types.

```jsonc
{
	"forest_mapgen_settings": {
		"forest": {},
		"forest_thick": {},
		"forest_water": {}
	}
}
```

Each terrain then has an independent set of configuration values that control the mapgen.

### Fields

|          Identifier           |                                 Description                                  |
| ----------------------------- | ---------------------------------------------------------------------------- |
| `terrains`                    | The overmap terrain IDs which have this biome.                               |
| `sparseness_adjacency_factor` | Value relative to neighbors controls how dense the overmap terrain will be.  |
| `item_group`                  | Item group used to place items randomly within the overmap terrain.          |
| `item_group_chance`           | % chance, between 1 and 100, that an item will be placed.                    |
| `item_spawn_iterations`       | Number of times that the item spawning will be called.                       |
| `clear_groundcover`           | Clear all previously defined `groundcover` for this overmap terrain.         |
| `groundcover`                 | Weighted list of terrains used for base groundcover.                         |
| `clear_components`            | Clear all previously defined `components` for this overmap terrain.          |
| `components`                  | Collection of components that make up the terrains and furniture placed.     |
| `clear_terrain_furniture`     | Clear all previously defined `terrain_furniture` for this overmap terrain.   |
| `terrain_furniture`           | Collection of furniture conditionally placed based on terrain.               |

### Example

```jsonc
{
	"forest": {
		"terrains" : [ "forest" ],
		"sparseness_adjacency_factor": 3,
		"item_group": "forest",
		"item_group_chance": 60,
		"item_spawn_iterations": 1,
		"clear_groundcover": false,
		"groundcover": {
			"t_grass": 3,
			"t_dirt": 1
		},
		"clear_components": false,
		"components": {},
		"clear_terrain_furniture": false,
		"terrain_furniture": {}
	}
}
```

### Components

The components are a collection of named objects with a sequence, chance, and set of types that,
during mapgen, are rolled in sequence to pick a feature to be placed at a given location. The names
for the components are only relevant for the purposes of overriding them in region overlays.

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
	"trees": {
		"sequence": 0,
		"chance": 12,
		"clear_types": false,
		"types": {
			"t_tree_young": 128,
			"t_tree": 32,
			"t_tree_birch": 32,
			"t_tree_pine": 32,
			"t_tree_maple": 32,
			"t_tree_willow": 32,
			"t_tree_hickory": 32,
			"t_tree_blackjack": 8,
			"t_tree_coffee": 8,
			"t_tree_apple": 2,
			"t_tree_apricot": 2,
			"t_tree_cherry": 2,
			"t_tree_peach": 2,
			"t_tree_pear": 2,
			"t_tree_plum": 2,
			"t_tree_deadpine": 1,
			"t_tree_hickory_dead": 1,
			"t_tree_dead": 1
		}
	},
	"shrubs_and_flowers": {
		"sequence": 1,
		"chance": 10,
		"clear_types": false,
		"types": {
			"t_underbrush": 8,
			"t_shrub_blueberry": 1,
			"t_shrub_strawberry": 1,
			"t_shrub": 1,
			"f_chamomile": 1,
			"f_dandelion": 1,
			"f_datura": 1,
			"f_dahlia": 1,
			"f_bluebell": 1,
			"f_mutpoppy": 1
		}
	}
}
```

### Terrain Furniture

The terrain furniture are a collection of terrain ids with a chance of having furniture
picked from a weighted list for that given terrain and placed on it during mapgen after
the normal mapgen has completed. This is used, for example, to place cattails on fresh
water in swamps. Cattails could be simply placed in the `components` section and placed
during the normal forest mapgen, but that would not guarantee their placement on fresh
water only, while this does.

### Fields

|    Identifier     |                            Description                             |
| ----------------- | ------------------------------------------------------------------ |
| `chance`          | One in X chance that furniture from this component will be placed. |
| `clear_furniture` | Clear all previously defined `furniture` for this terrain.         |
| `furniture`       | Weighted list of furniture that will be placed on this terrain.    |

### Example

```jsonc
{
	"t_water_sh" : {
		"chance": 2,
		"clear_furniture": false,
		"furniture": {
			"f_cattails": 1
		}
	}
}
```

## Forest Trail Settings

The **forest_trail_settings** section defines the attributes used in generating trails in the
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
	"forest_trail_settings": {
		"chance": 2,
		"border_point_chance": 2,
		"minimum_forest_size": 100,
		"random_point_min": 4,
		"random_point_max": 50,
		"random_point_size_scalar": 100,
		"trailhead_chance": 1,
		"trailhead_road_distance": 6,
		"trailheads": {
			"trailhead_basic": 50
		}
	}
}
```

## Forest Trail Settings

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

## City

The **city** section defines the possible overmap terrains and specials that may be used as
buildings in a city, their weighted chances of placement, and some attributes that control the
relative placements of various classes of buildings.

### Fields

|       Identifier        |                            Description                             |
| ----------------------- | ------------------------------------------------------------------ |
| `type`                  | City type identifier--currently unused.                            |
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
	"city": {
		"type": "town",
		"shop_radius": 80,
		"park_radius": 90,
		"houses": {
			"house_two_story_basement": 1,
			"house": 1000,
			"house_base": 333,
			"emptyresidentiallot": 20
		},
		"parks": {
			"park": 4,
			"pool": 1
		},
		"shops": {
			"s_gas": 5,
			"s_pharm": 3,
			"s_grocery": 15
		}
	}
}
```

## Map Extras

The **map_extras** section defines the named collections of map extras--special mapgen events
applied on top of the defined mapgen--that may be referenced by the `extras` property of an overmap
terrain. This includes both the chance of an extra occurring as well as the weighted list of extras.

### Fields

| Identifier |                           Description                            |
| ---------- | ---------------------------------------------------------------- |
| `chance`   | One in X chance that the overmap terrain will spawn a map extra. |
| `extras`   | Weighted list of map extras that can spawn.                      |

### Example

```jsonc
{
	"map_extras": {
		"field": {
			"chance": 90,
			"extras": {
				"mx_helicopter": 40,
				"mx_portal_in": 1
			}
		}
	}
}
```

## Weather

The **weather** section defines the base weather attributes used for the region.

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
	"weather": {
		"base_temperature": 6.5,
		"base_humidity": 66.0,
		"base_pressure": 1015.0,
		"base_wind": 5.7,
		"base_wind_distrib_peaks": 30,
		"base_wind_season_variation": 64,
		"weather_black_list": [
			"snowstorm"
    ]
  }
}
```

## Overmap Feature Flag Settings

The **overmap_feature_flag_settings** section defines operations that operate on the flags assigned to overmap features.
This is currently used to provide a mechanism for whitelisting and blacklisting locations on a per-region basis.

### Fields

|    Identifier     |                                        Description                                         |
| ----------------- | ------------------------------------------------------------------------------------------ |
| `clear_blacklist` | Clear all previously defined `blacklist`.                                                  |
| `blacklist`       | List of flags. Any location with a matching flag will be excluded from overmap generation. |
| `clear_whitelist` | Clear all previously defined `whitelist`.                                                  |
| `whitelist`       | List of flags. Only locations with a matching flag will be included in overmap generation. |

### Example

```jsonc
{
	"overmap_feature_flag_settings": {
		"clear_blacklist": false,
		"blacklist": [ "FUNGAL" ],
		"clear_whitelist": false,
		"whitelist": []
	}
}
```

# Region Overlay

A **region_overlay** allows the specification of `region_settings` values which will be applied to
specified regions, merging with or overwriting the existing values. It is only necessary to specify
those values which should be changed.

### Fields

| Identifier |                                         Description                                         |
| ---------- | ------------------------------------------------------------------------------------------- |
| `type`     | Type identifier. Must be "region_overlay".                                                  |
| `id`       | Unique identifier for this region overlay.                                                  |
| `regions`  | A list of regions to which this overlay should be applied. "all" will apply to all regions. |

All additional fields and sections are as defined for a `region_overlay`.

### Example
```jsonc
[
	{
		"type": "region_overlay",
		"id": "example_overlay",
		"regions": [ "all" ],
		"city": {
			"parks": {
				"examplepark": 1
			}
		}
	}
]
```
