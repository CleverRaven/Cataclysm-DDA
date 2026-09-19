# Region Layout

Region layouts provide handling for assigning region settings by overmap at a world scale. The simplest example:

```json
{
	"type": "dimension_region_layout",
	"id": "default",
	"generation_mode": "UNIFORM",
	"uniform_region": "default"
}
```

DISCLAIMER: as of July 2026, overmap feature generation (lakes, oceans, highways, etc.) does not take into account adjacent overmaps that may be a different region. ESPECIALLY DO NOT add multiple regions into the default dimension without sufficient testing first! Mods are fine, as long as you're willing to maintain them.

Region layouts have a secondary type: "generation_mode", which defines how the layout is generated.

There are two broad types of region layouts: STATIC and DYNAMIC.
- DYNAMIC layouts are determined upon generating an overmap.
- STATIC layouts have predetermined bounds and are generated along with the first overmap, 
using a DYNAMIC layout to generate out-of-bounds overmaps.

Listed below is documentation for each region layout mode's fields:

## UNIFORM (Type: DYNAMIC)
All overmaps are generated with a single region. See the top of this document for an example.

|       Identifier       |                            Description                                        |
| ---------------------- | ----------------------------------------------------------------------------- |
| `uniform_region`       | region_id -- all overmaps will generate with this region.                     |

## RANDOM (Type: DYNAMIC)
All overmaps are generated with a random region, selected from a weighted list.

|       Identifier       |                            Description                                        |
| ---------------------- | ----------------------------------------------------------------------------- |
| `random_regions`       | array of (region_id, int) pairs -- list of regions to pick from               |

```json
{
	"type": "dimension_region_layout",
	"id": "test_layout",
	"generation_mode": "RANDOM",
	"random_regions": [ ["default", 4],["highlands", 1],["spider_dimension", 1],["string_dimension", 1],["radiosphere", 1] ]
}
```

The above example may output a world map like so:

ACCCDAACC
DBDBACCCA
CABCBBDCC
CACCCCCEE
CCCCBBCEB
CCDEBDBCC
CDBCCCADB
CDEDCCBCC
CCADDCDAA

C = default
A = highlands
B = spider_dimension
D = radiosphere
E = string_dimension

## ANGLES (Type: DYNAMIC)
Overmaps are generated based on their angle relative to overmap (0,0).
Sectors are made from (360 / N)-degree wedges, where N is the count of regions provided.
Regions can be provided more than once to form larger sectors.

|       Identifier       |                            Description                                         |
| ---------------------- | ------------------------------------------------------------------------------ |
| `angles_regions`       | region_id -- up to 16 region_ids, clockwise starting from north                |
| `angles_offset`        | angle in degrees -- up to 180 degrees, offset from 0 degrees north, default 0  |

```json
{
	"type": "dimension_region_layout",
	"id": "test_layout",
	"generation_mode": "ANGLES",
	"angle_regions": ["default", "string_dimension", "default", "string_dimension", "default", "string_dimension", "default", "string_dimension" ]
}
```

The above example may output a world map like so:

AAAABBBBA
BAAABBBAA
BBAABBAAA
BBBABAAAA
BBBBBBBBB
AAAABABBB
AAABBAABB
AABBBAAAB
ABBBBAAAA

A = string_dimension
B = default

## MANUAL_VORONOI (Type: STATIC)
Uses a Voronoi diagram generator with interchangeable point sets to generate blobs of regions.
About Voronoi diagrams: https://en.wikipedia.org/wiki/Voronoi_diagram
A decent drafter for your intended layout: https://alexbeutel.com/webgl/voronoi.html

|       Identifier          |                            Description                                        |
| ------------------------- | ----------------------------------------------------------------------------- |
| `generated_bounds_min`    | tripoint_abs_om -- minimum layout overmap bound                               |
| `generated_bounds_max`    | tripoint_abs_om -- maximum layout overmap bound                               |
| `layout_out_of_bounds`    | region_layout_id -- the DYNAMIC layout used for out_of_bounds generation      |
| `region_point_sets`       | array of region_point_set -- the point sets used for Voronoi generation       |

This layout mode uses `region_point_set`, which are defined as follows:

|       Identifier          |                            Description                                        |
| ------------------------- | ----------------------------------------------------------------------------- |
| `region_points`           | array of tripoint_abs_om -- all overmap points in this set                    |
| `regions_weighted`        | array of (region_id, int) pairs -- regions picked and assigned to points      |
| `remove_region`           | boolean -- whether to remove picked regions from `regions_weighted`           |
| `default_region`          | region_id -- used if `regions_weighted` is empty                              |
| `region_point_variance`   | int -- maximum random offset from `region_points`                             |

```json
{
	"type": "dimension_region_layout",
	"id": "test_layout",
	"generation_mode": "MANUAL_VORONOI",
	"layout_out_of_bounds": "default",
	"generated_bounds_min": [-10,-10, 0],
	"generated_bounds_max": [10, 10, 0],
	"region_point_sets": [
	{
		"regions_weighted": [["default", 1],["highlands", 1],["spider_dimension", 1],["string_dimension", 1],["radiosphere", 1] ],
		"region_points": [[0, 0, 0], [-5, -5, 0], [5, 5, 0], [5, -5, 0], [-5, 5, 0]],
		"remove_region": true,
		"default_region": "default",
		"region_point_variance": 2
	},
	{
		"regions_weighted": [["highlands", 1],["spider_dimension", 1],["string_dimension", 1],["radiosphere", 1] ],
		"region_points": [[-8, -8, 0], [8, 8, 0], [8, -8, 0], [-8, 8, 0]],
		"remove_region": true,
		"default_region": "default",
		"region_point_variance": 1
	}
	]
}
```

The above example may output a world map like so (note that both highlands points merged!):

AAAAAAAABBBCCCBBBBBBB
AAAAAAABBBBCCCBBBBBBB
AAAAAABBBBBCCCCBBBBBB
AAAAABBBBBBCCCCBBBBBB
AAAABBBBBBBCCCCCBBBBB
AAABBBBBBBBCCCCCCBBBB
AABBBBBBBBBCCCCCCBBBB
ABBBBBBBBBDCCCCCCCBBB
BBBBBBBBBBDDDCCCCCBBB
BBBBBBBBBBDDDDDCCCCBB
CBBBBBBBBDDDDDDDDEEEE
CCCBBBBBADDDDDDDEEEEE
CCCCCCAAAADDDDEEEEEEE
CCCCCCAAAAADDEEEEEEEE
CCCCCCAAAAAAEEEEEEEEE
CCCCCCAAAAAAEEEEEEEEE
CCCCCCAAAAAAEEEEEEEEE
CCCCCCAAAAAAEEEEEEEEE
CCCCCCAAAAAAEEEEEEEEE
CCCCCCAAAAAAEEEEEEEEE
CCCCCCAAAAAAAEEEEEEEE
A = radiosphere
B = string_dimension
C = spider_dimension
D = default
E = highlands

