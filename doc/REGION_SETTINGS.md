# Region Settings

The **region_settings** define the attributes for map generation that apply to an entire region.
The general settings define the default overmap terrain and ground cover, as well as the factors
that control forest and swamp growth. Additional sections are as follows:

|     Section      |                         Description                          |
| ---------------- | ------------------------------------------------------------ |
| `field_coverage` | Defines the flora that cover the `field` overmap terrain.    |
| `city`           | Defines the structural compositions of cities.               |
| `map_extras`     | Defines the map extra groups referenced by overmap terrains. |
| `weather`        | Defines the base weather attributes for the region.          |

Note that for the default region, all attributes and sections are required.

### Fields

|       Identifier        |                            Description                             |
| ----------------------- | ------------------------------------------------------------------ |
| `type`                  | Type identifier. Must be "region_settings".                        |
| `id`                    | Unique identfier for this region.                                  |
| `default_oter`          | Default overmap terrain for this region.                           |
| `default_groundcover`   | List of terrain types and weights applied as default ground cover. |
| `num_forests`           | Number of forest "chunks".                                         |
| `forest_size_min`       | Minimum size for a forest chunk, in # of overmap terrains.         |
| `forest_size_max`       | Maximum size for a forest chunk, in # of overmap terrains          |
| `swamp_maxsize`         | Maximum size for a swamp chunk, in # of overmap terrains.          |
| `swamp_river_influence` | Impacts swamp chance near rivers. Higher = more.                   |
| `swamp_spread_chance`   | One in X chance that a swamp is created outside a forest or field. |


### Example
```json
{
	"type": "region_settings",
	"id": "default",
	"default_oter": "field",
	"default_groundcover": [
		["t_grass", 4],
		["t_dirt", 1]
	],
	"num_forests": 250,
	"forest_size_min": 15,
	"forest_size_max": 40,
	"swamp_maxsize": 4,
	"swamp_river_influence": 5,
	"swamp_spread_chance": 8500
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
```json
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
| `house_basement_chance` | One in X chance that a house has a basement.                       |
| `houses`                | Weighted list of overmap terrains and specials used for houses.    |
| `basements`             | Weighted list of overmap terrains and specials used for basements. |
| `parks`                 | Weighted list of overmap terrains and specials used for parks.     |
| `shops`                 | Weighted list of overmap terrains and specials used for shops.     |

### Placing shops, parks, and houses

When picking a building to place in a given location, the game considers the city size, the
location's distance from the city center, and finally the `shop_radius` and `park_radius` values for
the region. It then tries to place a shop, then a park, and finally a house, where the chance to
place the shop or park are based on the formula `rng( 0, 99 ) > X_radius * distance from city center
/ city size`.

### Example
```json
{
	"city": {
		"type": "town",
		"shop_radius": 80,
		"park_radius": 90,
		"house_basement_chance": 5,
		"houses": {
			"house_two_story_basement": 1,
			"house": 1000,
			"house_base": 333,
			"emptyresidentiallot": 20
		},
		"basements": {
			"basement": 1000,
			"basement_hidden_lab_stairs": 50,
			"basement_bionic": 50
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

```json
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

|     Identifier     |                              Description                              |
| ------------------ | --------------------------------------------------------------------- |
| `base_temperature` | Base temperature for the region in degrees Celsius.                   |
| `base_humidity`    | Base humidity for the region in relative humidity %                   |
| `base_pressure`    | Base pressure for the region in millibars.                            |
| `base_acid`        | Base acid for the region in ? units. Value >= 1 is considered acidic. |

### Example

```json
{
	"weather": {
		"base_temperature": 6.5,
		"base_humidity": 66.0,
		"base_pressure": 1015.0,
		"base_acid": 0.0
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
| `id`       | Unique identfier for this region overlay.                                                   |
| `regions`  | A list of regions to which this overlay should be applied. "all" will apply to all regions. |

All additional fields and sections are as defined for a `region_overlay`.

### Example
```json
[{
	"type": "region_overlay",
	"id": "example_overlay",
	"regions": ["all"],
	"city": {
		"parks": {
			"examplepark": 1
		}
	}
}]
```