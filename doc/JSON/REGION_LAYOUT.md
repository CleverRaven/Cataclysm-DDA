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
