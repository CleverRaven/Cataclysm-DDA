# Adding Json Roof Guide

Adding json roofs to a building involves using a few more files to link the roof and building together during mapgen.

Files that will be edited:

`data/json/mapgen/[name of building].json` : the map of the building and roof.

`data/json/overmap_terrain.json` : defines the building's overmap characteristics.

`data/json/regional_map_settings.json` : defines the building's overmap spawn settings.

`data/json/overmap/multietile_city_buildings.json` : links the building layers together.

## Making the Roof Map

Refer to [MAPGEN.md](https://github.com/CleverRaven/Cataclysm-DDA/blob/master/doc/MAPGEN.md) for creating the map if you are new to map creation.

Open the file that contains the map for the building `data/json/mapgen/[name of building].json`
Add a new entry for the roof.  You can copy the building entry since you want the same foundation footprint for the roof.

Give the roof a unique om_terrain ID.  Below is an example of the main floor and roof om_terrain IDs

````json
"om_terrain": [ "abstorefront" ],
"om_terrain": [ "abstorefront_roof" ],
````

Note: If you are adding a roof to an existing building that shares a common om_terrain ID with other maps, you will need to change the om_terrain ID for the existing floor to be unique.

Keep the outline of the walls from the original floor,
Add `t_open_air` outside the building and a `t_flat_roof` over the building's footprint.
There are a few flat roof terrains to choose from in terrain.json.

There are a number of terrains and furniture structures for roofs including gutters, chimneys, and roof turbine vents.
Browse json/terrain.json and furniture.json for ideas.  Consider roof access, you can use ladders, stairs and gutters.  Some furniture is also climbable.

There is a set of optional nested map chunks at `data/json/mapgen/nested_chunks_roof.json` if you'd like to incorporate them.  Add any new nested chunks for roofs here as well.

Sample roof entry:
````json
  {
    "type": "mapgen",
    "method": "json",
    "om_terrain": "abstorefront_roof",
    "weight": 200,
    "object": {
      "fill_ter": "t_flat_roof",
      "rows": [
        "                        ",
        " |....................3 ",
        " |....................3 ",
        " |....................3 ",
        " |....................3 ",
        " |....................3 ",
        " |....................3 ",
        " |....................3 ",
        " |....................3 ",
        " |....................3 ",
        " |....................3 ",
        " |....................3 ",
        " |......&.............3 ",
        " |....................3 ",
        " |....................3 ",
        " |-----------------5--3 ",
        "                        ",
        "                        ",
        "                        ",
        "                        ",
        "                        ",
        "                        ",
        "                        ",
        "                        "
      ],
      "terrain": {
        ".": "t_flat_roof",
        " ": "t_open_air",
        "|": "t_gutter_west",
        "-": "t_gutter_south",
        "3": "t_gutter_east",
        "5": "t_gutter_drop"
      },
      "furniture": { "&": "f_roof_turbine_vent" },
      "place_items": [ { "item": "roof_trash", "x": [ 2, 21 ], "y": [ 3, 14 ], "chance": 50, "repeat": [ 1, 3 ] } ],
      "place_nested": [
        {
          "chunks": [
            [ "null", 50 ],
            [ "roof_4x4_party", 15 ],
            [ "roof_4x4_holdout", 5 ],
            [ "roof_4x4_utility", 40 ],
            [ "roof_5x5_coop", 5 ]
          ],
          "x": [ 3, 15 ],
          "y": [ 3, 7 ]
        }
      ]
    }
}
````

## Linking the main floor and the roof
Navigate to `json/overmap/multitile_city_buildings.json` or `json/overmap/multitile_buildings_terrain.json` for buildings taking up more then one overmap tile per z level (schools, mansions).
Add an entry for the main floor.  The `point` coordinates define to the x, y, z positions of the building.  The 1 places the roof one z level above the ground floor.
Append north for rotating buildings to orient the z levels.

````json
  {
    "type": "city_building",
    "id": "abstorefront",
    "locations": [ "land" ],
    "overmaps": [
      { "point": [ 0, 0, 0 ], "overmap": "abstorefront_north" },
      { "point": [ 0, 0, 1 ], "overmap": "abstorefront_roof_north" }
    ]
  }
````
## Overmap Specials

Overmap specials are handled a little differently.  They use `json/overmap/specials.json` for both linking z levels and overmap spawning.  A special won't need an entry in `data/json/regional_map_settings.json`

Overmap special example:
````json
  {
        "type" : "overmap_special",
        "id" : "Evac Shelter",
        "overmaps" : [
            { "point":[0,0,0], "overmap": "shelter"},
            { "point":[0,0,-1], "overmap": "shelter_under"},
            { "point":[0,0,1], "overmap": "shelter_roof"}
        ],
        "connections" : [
            { "point" : [0,-1,0], "terrain" : "road" }
        ],
        "locations" : [ "wilderness" ],
        "city_distance" : [5, 10],
        "city_sizes" : [4, 12],
        "occurrences" : [1, 3],
        "rotate" : false,
        "flags" : [ "CLASSIC" ]
  }
````
## Adding the overmap_terrain entry

Navigate to `data/json/overmap_terrain.json`
Every z level gets an entry that defines how it appears on the overmap.
The name field will determine what is displayed in the in-game overmap.
The entries should share the same color and symbol.

````json
  {
    "type": "overmap_terrain",
    "id": "abandonedwarehouse",
    "copy-from": "generic_city_building",
    "name": "abandoned warehouse",
    "sym": 119,
    "color": "brown"
  },
  {
    "type": "overmap_terrain",
    "id": "abandonedwarehouse_roof",
    "copy-from": "generic_city_building",
    "name": "abandoned warehouse roof",
    "sym": 119,
    "color": "brown"
}
````
## Adding to regional_map_settings
Navigate to `data/json/regional_map_settings.json`

This determines the spawn frequency and location of non-special buildings.
Find the appropriate category for your building and add either the overmap_special ID or the city_building ID and include a spawn weight.

````json
""abandonedwarehouse": 200,
````
When testing you can increase the spawn rate if you want to survey your work using natural spawns.

Finally, always [lint](http://dev.narc.ro/cataclysm/format.html) your additions before submitting.
