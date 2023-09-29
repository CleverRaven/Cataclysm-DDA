## Guide for basic mapgen

This guide will cover the basics of mapgen, which files you need to edit, the tags in each file and the differences in creating specials or regular city buildings.  For full technical information about mapgen entries refer to: [doc/MAPGEN.md](https://github.com/CleverRaven/Cataclysm-DDA/blob/master/doc/MAPGEN.md).

First, lets cover some basic concepts and the files you'll add or edit.

#### General comments:

CDDA mapgen is surprisingly powerful once you get used to working with it.  You can use lots of tricks to add variability and interest to your maps.  Most advanced mapgen techniques will go into a different tutorial.  This one covers basic concepts and how to create a basic single OMT (overmap terrain tile)sized building.  We will touch on palette usage and how to add a roof as well.

#### Specials vs. city buildings:

A special is a building that spawns outside the city and requires additional information to spawn, like its distance from cities and valid OMT terrain types.  They also used to be the only multi-tile buildings in the game until recent changes allowed special type buildings to spawn inside cities.  Examples of specials are: evac shelters, cabins, LMOE, etc.

City buildings can be single or multi-tile in size and have their spawns limited to the city boundaries. A building can be both a city building and a special but would require both sets of entries to spawn for both types.  Some motels are examples of this (see: 2fmotel_city & 2fmotel).

Important policy: since the roof project, all buildings are now multi-tile across z levels.  All new buildings should always get a JSON roof added.  Basements are also custom fit to the ground floor mapgen, so it is good practice to include dedicated downstairs if you want a basement.

#### The Files & their purpose:

1. You will add a new mapgen file in: [data/json/mapgen](https://github.com/CleverRaven/Cataclysm-DDA/tree/master/data/json/mapgen) or one of its sub-folders.  If you are using an existing foundation shape for the building, you may append it to that building's file.
    * This is the blueprint for the building.  It can also hold all the building’s data for adding furniture and loot (see palette for an alternative).

2. You will add entries for each z level you create in the appropriate overmap_terrain file ([data/json/overmap/overmap_terrain](https://github.com/CleverRaven/Cataclysm-DDA/tree/master/data/json/overmap/overmap_terrain)).
    * These entries will define what your building looks like in the overmap, its symbol, color, and spawn requirements like adding sidewalks, it will also control flags for some mapgen functions.

3. You will add an entry into either specials.json or multitile_city_buildings.json depending on if it is a special or a city building.
    * For multitile_city_buildings this will link the various z levels &/or multiple OMTs of your building.
    * For specials, this will link the various z levels or multiple OMTs of your building, define any needed road/subway/etc. connections, and define its spawning parameters.

4. Add an entry into regional_settings.json for city buildings.  This will allow them to spawn in the world.

5. Optional but recommended for any large project: adding a new palette file into mapgen_palettes folder (you may use any existing palette too).
    * This is a file that can be shared among several maps that holds a portion of the mapgen file data.  It is commonly used for defining terrain and furniture.  You can also put in loot, vehicle, monster spawns and any other data that normally goes in the `"object"` tag of the mapgen file.
    * Please avoid editing existing mapgen palettes because you may affect existing maps using a combination of the palette and the mapgen file.

#### Starting the mapgen entry:

When I start a new map project, I generally will add in all the entries I need for it to spawn in game from the outset.  This way I can test it as I work on it and adjust it as needed.  So, I recommend setting up all the needed files when you begin your project.

Before beginning you’ll want to make some decisions:

1. What size will it be overall (how many OMTs?)
2. Where will it spawn?
3. Will I use a palette or put everything in the mapgen file?
    * If you use a palette, define as much of it as possible from the outset.

4. Advanced questions:
    * Will I use nested maps?
    * Do I want it to connect to the subways or roads?
    * Will I be using the mapgen object data in combination with a palette (see the mall 2nd floor if you want a master class in combined usage of both types)?

#### The mapgen map:

This covers the mapgen file map flags and what they do in layman’s terms.  You can get more extensive information from [doc/MAPGEN.md](https://github.com/CleverRaven/Cataclysm-DDA/blob/master/doc/MAPGEN.md).

the mapgen file has some meta data tags and the `"object"` data which defines everything to make the map.

##### The metadata:

Sample:
```
    "type": "mapgen",
    "method": "json",
    "om_terrain": "s_restaurant_coffee",
    "weight": 250,
```
1. `"type"` will always be mapgen (we'll cover other map types in future tutorials), the `"method"` will always be JSON.  This data tells the program how to treat this file.

2. `"om_terrain"`: this is basically your internal name for the map (not the name that shows up on the overmap).  It should usually be unique unless you plan on having multiple variants of the same map which **share the same building foundation shape** (note: I mean the actual shape of the building's foundation).

3. `"weight"`: This entry is the weight of this particular map compared to others **sharing the same om_terrain name**.  So, say you have one version of a house then you make an identical house with different spawns (like a fully furnished house and an abandoned version). This weight will determine how often each spawns in relation to the other.  Say the furnished house is at 100, and the abandoned one is at 20.  So, it'll spawn 5x less than the furnished house.

##### The object data:

This is the section of tags that defines your map, its terrains, furniture, and various spawn types.  There are several ways to place items (and nested maps).  These deserve their own tutorial.  For this document we'll be using "explicit symbol" placement for loot spawns, the easiest to use.  Everything in the object section can be placed in a mapgen_palette except rows and fill_ter.

Sample object segment:  everything in the object needs to fall within the object's {} braces or it won't be included.  If you misplace the end bracket, you probably won't get a loading error.
<!-- {% raw %} -->
Sample:
```
    "object": {
      "fill_ter": "t_floor",
      "rows": [
        "S___________SSTzzzzzzzTS",
        "S_____,_____SSzMMMMMMMzS",
        "S_____,_____SSSSSSSSSMzS",
        "S_____,_____SSSSSSSSSMzS",
        "S_____,_____SSSSSSSSSMzS",
        "S_____,_____SS||V{{{V||S",
        "S_____,_____SS|D     <|S",
        "SSSSSSSSSSSSSS|r      OS",
        "SSSSSSSSSSSSSS|r      |S",
        "SVVVVVVVzMSSzz|   #W##|S",
        "SVD>>>>VzMSSMzV   #ww%|S",
        "SV BBB>VzMSSSS{   xwwF|S",
        "SV   B>VzMSSSS{   flwl|S",
        "SV   B>VzMMMMzV   flwU|S",
        "SV   B>Vzzzzzz|X  #wwG|S",
        "S|B ^||||||||||^  ||I||S",
        "SO6  B|=;|;=|99  r|FwC|S",
        "S|B  6|=A|A=|9   r|Fwc|S",
        "S|   B|+|||+|9   D|!ww|S",
        "S|B               |Lwl|S",
        "SO6   6 6 6   B66B|Lwl|S",
        "S|B ^???????^ B66B|Lwl|S",
        "S|||||||||||||||||||3||S",
        "S4SSSSSSSSSSSSSSSSSSSSSS"
      ],
      "terrain": {
        ".": "t_region_groundcover_urban",
        "M": "t_region_groundcover_urban",
        "z": "t_region_shrub_decorative",
        "_": "t_pavement",
        ",": "t_pavement_y",
        "S": "t_sidewalk",
        " ": "t_floor",
        "!": "t_linoleum_white",
        "#": "t_linoleum_white",
        "%": "t_linoleum_white",
        ";": "t_linoleum_white",
        "=": "t_linoleum_white",
        "A": "t_linoleum_white",
        "C": "t_linoleum_white",
        "F": "t_linoleum_white",
        "G": "t_linoleum_white",
        "L": "t_linoleum_white",
        "U": "t_linoleum_white",
        "W": "t_linoleum_white",
        "c": "t_linoleum_white",
        "f": "t_linoleum_white",
        "l": "t_linoleum_white",
        "w": "t_linoleum_white",
        "+": "t_door_c",
        "3": [ "t_door_locked", "t_door_locked_alarm" ],
        "I": "t_door_locked_interior",
        "O": "t_window",
        "V": "t_wall_glass",
        "{": "t_door_glass_c",
        "|": "t_wall_b",
        "<": "t_stairs_up",
        "4": "t_gutter_downspout",
        "T": "t_tree_coffee"
      },
      "furniture": {
        "M": "f_region_flower",
        "^": [ "f_indoor_plant", "f_indoor_plant_y" ],
        "x": "f_counter",
        "#": "f_counter",
        ">": "f_counter",
        "W": "f_counter_gate_c",
        "%": "f_trashcan",
        "D": "f_trashcan",
        "6": "f_table",
        "?": "f_sofa",
        "B": "f_chair",
        "C": "f_desk",
        "L": "f_locker",
        "A": "f_sink",
        "U": "f_sink",
        "f": "f_glass_fridge",
        "F": "f_fridge",
        "G": "f_oven",
        "X": "f_rack",
        "9": "f_rack",
        "l": "f_rack",
        "r": "f_rack"
      },
      "toilets": { ";": {  } },
      "items": {
        "x": { "item": "cash_register_random", "chance": 100 },
        "#": { "item": "coffee_counter", "chance": 10 },
        "6": { "item": "coffee_table", "chance": 35 },
        "9": { "item": "coffee_display_2", "chance": 85, "repeat": [ 1, 8 ] },
        ";": { "item": "coffee_bathroom", "chance": 15 },
        "=": { "item": "coffee_bathroom", "chance": 35 },
        ">": { "item": "coffee_table", "chance": 25 },
        "A": { "item": "coffee_bathroom", "chance": 35 },
        "C": { "item": "SUS_office_desk", "chance": 33 },
        "D": { "item": "coffee_trash", "chance": 75 },
        "F": { "item": "coffee_fridge", "chance": 80, "repeat": [ 1, 8 ] },
        "G": { "item": "oven", "chance": 35 },
        "L": { "item": "coffee_locker", "chance": 75 },
        "U": { "item": "coffee_dishes", "chance": 75 },
        "X": { "item": "coffee_newsstand", "chance": 90, "repeat": [ 1, 8 ] },
        "f": { "item": "coffee_freezer", "chance": 85, "repeat": [ 1, 8 ] },
        "l": { "item": "coffee_prep", "chance": 50 },
        "r": { "item": "coffee_condiments", "chance": 80, "repeat": [ 1, 8 ] }
      },
      "monster": { "!": { "monster": "mon_zombie", "chance": 10 } },
      "place_monster": [ { "group": "GROUP_ZOMBIE", "x": [ 3, 17 ], "y": [ 13, 18 ], "chance": 80, "repeat": [ 1, 2 ] } ],
      "vehicles": { "c": { "vehicle": "swivel_chair", "chance": 100 } }
    }
```
<!-- {% endraw %} -->

1. The `"fill_ter"`: this tag defines the default terrain/flooring for use under furniture and for undefined symbols in your rows.  Generally, pick the terrain that has the most furniture associated with it.

2. The `"rows"`: this is the actual blueprint for your building.  A standard OMT tile (overmap tile) is 24x24 tiles.  Rows begin their x,y coordinates at 0,0 which is the upper left corner of your map.
    * tip: if you cross stitch or are familiar with cross stitch patterns, this should all look very familiar.  You have the map and the "legend" areas.

3. `"terrain"`: this defines what all those letters in the rows mean when they are terrains.  A symbol can return a single terrain, or, it can offer a chance to spawn from a selection of terrains.  Here are some quick examples:

`"*": "t_door_c",`: this will make every * in the rows a closed wooden door.

`"*": [ [ "t_door_locked_peep", 2 ], "t_door_locked_alarm", [ "t_door_locked", 10 ], "t_door_c" ],`: this array will randomly choose from a selection of doors.  Some are weighted to have a higher chance to spawn then others.  Locked doors will be most common, then peephole doors.  Finally closed & locked/alarmed have the same basic weight and will spawn the least.

*Note: in my example, the fill_ter is for floor.  So if you have furniture that you want to spawn with a different floor, you must use that same symbol that you've given the furniture and also define it as a terrain for your new flooring.  In the above example, several furniture symbols are using white linoleum for their flooring.  If you don't do this step, your furniture will end up having the wrong flooring which will be especially noticeable if you smash it.  I often do a final map check where I go around in game and smash furniture to check their terrains before submitting my maps. This can be quite cathartic.*

4. `"furniture"` tag:  Like terrain, this is a list of the furniture ID's and their map symbols.  It can handle the same sort of arrays as terrain.

5. `"toilets"` and other specially defined furniture: you'll run into some specially defined common furniture which allows for some easier placement.  In our sample map the entry: `"toilets": { ";": {  } },` defines the symbol entry and will also auto-place water in your toilets.  There are a few other specialty furniture entries.

The other most common one is: `"vendingmachines": { "D": { "item_group": "vending_drink" }, "V": { "item_group": "vending_food" } }` this assigns two symbols for vending machines and makes one for food & one for drinks. *note: you can put any item_group into the machines, like those bullet ones*.

6. Item spawns:  There are many ways to place items.  This tutorial will only cover explicit symbol placement which is the easiest.  There is documentation all about loot spawns you can read for further information.  See: [doc/ITEM_SPAWN.md](https://github.com/CleverRaven/Cataclysm-DDA/blob/master/doc/ITEM_SPAWN.md).

our sample uses "items": for its tag.  others include: "place_item", "place_items", "place_loot".  Some of these allow for individual item placement and others groups, or both.  This will be covered in another tutorial.

For now lets break this one apart:
```
"items": {
      "a": { "item": "stash_wood", "chance": 30, "repeat": [ 2, 5 ] },
      "d": [
        { "item": "SUS_dresser_mens", "chance": 50, "repeat": [ 1, 2 ] },
        { "item": "SUS_dresser_womens", "chance": 50, "repeat": [ 1, 2 ] }
       ]
      }
```

"a" in this case is a fireplace as defined in its map.  So, in the fireplace I want to add the stash_wood item_group.  By defining the group under "a", every fireplace in my map will get this group.  This is particularly powerful for common item_groups like in houses where every fridge is going to get the same item_group.  The chance is its spawn chance and repeat means that it will repeat the roll for that chance 2-5 times, so this fireplace can be extra stocked or have a little bit or nothing if it fails its chance rolls.

the "d" entry is a dresser.  I wanted the dressers to pull from two possible item groups, one a man's selection and the other women’s.  So, there is an array [ ... ] which encompasses all possible item_groups for this symbol.

You can add as many item_groups to the array as you'd like.  This is one of my racks in the generic house palette:
```
"q": [
        { "item": "tools_home", "chance": 40 },
        { "item": "cleaning", "chance": 30, "repeat": [ 1, 2 ] },
        { "item": "mechanics", "chance": 1, "repeat": [ 1, 2 ] },
        { "item": "camping", "chance": 10 },
        { "item": "tools_survival", "chance": 5, "repeat": [ 1, 2 ] }
      ],
```

*Note: When using explicit symbol placements, remember your group has a chance to spawn in every furniture using that symbol, so it can end up being quite generous. If you want two bookcases with different item spawns, give each bookcase its own symbol, or, use an alternate item spawn format, like the one using x,y coordinates for placement.*

7. Monster spawns:  our example has two types of monster spawns listed.
```
"monster": { "!": { "monster": "mon_zombie", "chance": 10 } },
"place_monster": [ { "group": "GROUP_ZOMBIE", "x": [ 3, 17 ], "y": [ 13, 18 ], "chance": 80, "repeat": [ 1, 2 ] } ],
```

The first entry is using  that explicit symbol placement technique.  The end entry is using a range x,y coordinates to place a monster.  *Note: We are moving away from putting monsters in the mapgen file in favor of overmap_terrain entries, so only use this if you want a specific monster group to spawn for specific reasons.  See overmap_terrain section for more information.*

8. Vehicle spawns:
```
"vehicles": { "c": { "vehicle": "swivel_chair", "chance": 100 } }
```
Our vehicle happens to be a swivel chair using explicit symbol placement.

This next example uses vehicle_groups and x,y placement.  It also includes rotation and status.  The rotation is which direction the vehicle will spawn on the map, and that status is its overall condition. Fuel is pretty self explanatory.  Always test your vehicle spawns in game, they can be rather picky in their placement and the rotation doesn't really match what you'd expect the numbers to mean.  The 0,0 point of vehicles can vary so you'll have to experiment to get the spawns in the right spots, especially in tight spaces.

```
"place_vehicles": [
        { "chance": 75, "fuel": 0, "rotation": 270, "status": 1, "vehicle": "junkyard_vehicles", "x": 12, "y": 18 },
        { "chance": 75, "fuel": 0, "rotation": 270, "status": 1, "vehicle": "junkyard_vehicles", "x": 19, "y": 18 },
        { "chance": 90, "fuel": 0, "rotation": 0, "status": -1, "vehicle": "engine_crane", "x": 28, "y": 3 },
        { "chance": 90, "fuel": 0, "rotation": 90, "status": -1, "vehicle": "handjack", "x": 31, "y": 3 },
        { "chance": 90, "fuel": 30, "rotation": 180, "status": -1, "vehicle": "welding_cart", "x": 34, "y": 3 },
        { "chance": 75, "fuel": 0, "rotation": 180, "status": -1, "vehicle": "junkyard_vehicles", "x": 31, "y": 9 },
        { "chance": 75, "fuel": 0, "rotation": 270, "status": 1, "vehicle": "junkyard_vehicles", "x": 28, "y": 18 },
        { "chance": 75, "fuel": 0, "rotation": 270, "status": 1, "vehicle": "junkyard_vehicles", "x": 35, "y": 18 }
      ]
 ```

9. Liquids in furniture:  this entry is for a standing tank.  I've defined the tank in the furniture entry as "g".

`"liquids": { "g": { "liquid": "water_clean", "amount": [ 0, 100 ] } },`

This places clean water in the tank, and a range of amount to spawn.

10. There are other specialty placement techniques that you'll pick up as you look at more maps.  One of my favorites is for the new planters:

Since the planter is a "sealed item" you define what's going into that container.  This example places seeds (ready to harvest) in the planters.  Note the first one will place a seedling, the others are harvest ready.  I've given each planter type an explicit symbol for quicker placement.

```
"sealed_item": {
      "1": { "item": { "item": "seed_rose" }, "furniture": "f_planter_seedling" },
      "2": { "item": { "item": "seed_chamomile" }, "furniture": "f_planter_harvest" },
      "3": { "item": { "item": "seed_thyme" }, "furniture": "f_planter_harvest" },
      "4": { "item": { "item": "seed_bee_balm" }, "furniture": "f_planter_harvest" },
      "5": { "item": { "item": "seed_mugwort" }, "furniture": "f_planter_harvest" },
      "6": { "item": { "item": "seed_tomato" }, "furniture": "f_planter_harvest" },
      "7": { "item": { "item": "seed_cucumber" }, "furniture": "f_planter_harvest" },
      "8": { "item": { "item": "soybean_seed" }, "furniture": "f_planter_harvest" },
      "9": { "item": { "item": "seed_zucchini" }, "furniture": "f_planter_harvest" }
    }
 ```

11. Best practices:
  * If you are making a new house please use this palette: "standard_domestic_palette".  The loots are already assigned and it covers a wide range of domestic furniture.  This will keep your house in sync with all the other houses for loot spawns.
  * All buildings should also get roof entries.
  * While entry placement for json doesn't really matter, try to keep your mapgen files ordered like the majority existing maps.  Be kind to future contributors.
    Add meta data at the top of the file.
    Object entry comes second.  Within the object entry it is generally: palette, set_points, terrain, furniture, random special entries like toilets, item spawns, vehicle spawns, monster spawns.
  * All buildings should at least get one "t_gutter_downspout" for roof access.  Players would love at least 2.
    if you're adding gutters to a multi-z level building, don't forget the intermediate floors.  You'll need to stagger the downspouts to players can climb up (like ladders).
  * If you put things on your roof that would be difficult to get up there, make sure to provide better roof access via ladders and stairs.
  * For your basic grass cover outside please use: `"t_region_groundcover_urban",` to maintain consistency across map boundaries.  Here are my standard flora entries for grass, shrubs & trees:

```
 ".": "t_region_groundcover_urban",
 "%": [ "t_region_shrub", "t_region_shrub_fruit", "t_region_shrub_decorative" ],
 "[": [ [ "t_region_tree_fruit", 2 ], [ "t_region_tree_nut", 2 ], "t_region_tree_shade" ],
```

finally for flowers (which are furniture):
```
"!": "f_region_flower"
```

#### Adding the roof!

Almost all CDDA buildings are now roof-capable and we'd love to keep it that way.  Make sure to submit a roof map with your building.  This can go into the same file as your ground floor and any other floors that share the same building shape/foundation.

So, this is super easy compared to the building we just went over.  It has all the same basic components.  I recommend you start by using the rows from your ground floor map and converting it to the `"roof_palette"` symbol set.  Basically your just going to trace the outline in gutters, add a t_gutter_drop next to your t_gutter_downspout below and toss some infrastructure up there.  I used nests extensively in commercial building roofs and we'll cover that in advanced mapgen.

sample roof:
```
  {
    "type": "mapgen",
    "method": "json",
    "om_terrain": "house_01_roof",
    "object": {
      "fill_ter": "t_shingle_flat_roof",
      "rows": [
        "                        ",
        "                        ",
        "                        ",
        "  -------------------   ",
        "  -.................-   ",
        "  -.................-   ",
        "  -.............N...-   ",
        "  5.................-   ",
        "  -.................-   ",
        "  -.................-   ",
        "  -........&........-   ",
        "  -.................-   ",
        "  -......=..........-   ",
        "  -.................-   ",
        "  -----------------5-   ",
        "                        ",
        "                        ",
        "                        ",
        "                        ",
        "                        ",
        "                        ",
        "                        ",
        "                        ",
        "                        "
      ],
      "palettes": [ "roof_palette" ],
      "terrain": { ".": "t_shingle_flat_roof" }
    }
  }
  ```

  1. I always just append `_roof` to the buildings ID.
  2. See how the palette takes the place of all that data from our earlier example.  So clean and easy.
  3. There is no `"weight"` entry because this will only spawn with its building (once linked).
  4. My palette uses "t_flat_roof" as its default roof.  For houses, I wanted shingles. So, I added the "t_shingle_flat_roof" in this mapgen which will override the palettes entry for `".": "t_flat_roof"`.  (more on this in advanced mapgen).

I have a separate roof document at: [doc/JSON_Mapping_Guides/JSON_ROOF_MAPGEN.md](https://github.com/CleverRaven/Cataclysm-DDA/blob/master/doc/JSON_Mapping_Guides/JSON_ROOF_MAPGEN.md).

#### Linking various mapgen maps using multitile_city_buildings.json

  This file is found at: [data/json/overmap/multitile_city_buildings.json](https://github.com/CleverRaven/Cataclysm-DDA/blob/master/data/json/overmap/multitile_city_buildings.json).

  *Remember this file is for city buildings only, not specials*

  A standard entry:
  ```
  {
    "type": "city_building",
    "id": "house_dogs",
    "locations": [ "land" ],
    "overmaps": [
      { "point": [ 0, 0, 0 ], "overmap": "house_dogs_north" },
      { "point": [ 0, 0, 1 ], "overmap": "house_dogs_roof_north" },
      { "point": [ 0, 0, -1 ], "overmap": "house_08_basement_north" }
    ]
  },
  ```

  1. The `"type"` won't change.  It should always be "city_building".
  2. `"id"`: This ID is often the same as your mapgen ID but it doesn't have to be the same.  We could use something more generic like "house".  This ID will be used in regional settings for spawns, so keep in mind how many buildings are using the ID.  I prefer distinct ID's because it makes debug spawning much, much easier.
  3. `"locations"`: defines where this building can be place by overmap terrain type.  Land is the default.
  4. `"overmaps"`: this is the bit where you define how the maps fit together, so lets break it up:
  `{ "point": [ 0, 0, 0 ], "overmap": "house_dogs_north" },`
  point: its point in relation to the other mapgen files for your building.  The coordinates are [ x, y, z ].  In this example, x,y are 0 because we only have one map per z level.  Zero for z means this is the ground level.  Note the roof above is at 1 and the basement is -1.
  5. appending `_north` to the ID's:
      * If your building rotates you need this compass point so the floors can match up correctly.  This is the generic basement mapgen group and thus doesn't get `_north` (this will change as we add dedicated stairs to our houses).

#### Setting overmap spawns using regional_map_settings.json

[data/json/regional_map_settings.json](https://github.com/CleverRaven/Cataclysm-DDA/blob/master/data/json/regional_map_settings.json)

1. For city buildings and houses you'll scroll down to the `"city":` flag.
2. Find your appropriate subtag, `"houses"` or `"shops"` usually.
3. Add your ID from the multitile_city_buildings entry.  This can also accept the mapgen ID and not complain which is why the are often the same name.
4. Choose a good weight for your building.

#### Linking and spawning specials:

Put the entry in: [data/json/overmap/overmap_special/specials.json](https://github.com/CleverRaven/Cataclysm-DDA/blob/master/data/json/overmap/overmap_special/specials.json).

This entry does the job of both the regional_map_settings and multitile_city_buildings plus other fun overmap stuff.

Example:
```
{
    "type": "overmap_special",
    "id": "pump_station",
    "overmaps": [
      { "point": [ 0, 0, 0 ], "overmap": "pump_station_1_north" },
      { "point": [ 0, 0, 1 ], "overmap": "pump_station_1_roof_north" },
      { "point": [ 0, 1, 0 ], "overmap": "pump_station_2_north" },
      { "point": [ 0, 1, 1 ], "overmap": "pump_station_2_roof_north" },
      { "point": [ 0, -1, -1 ], "overmap": "pump_station_3_north" },
      { "point": [ 0, 0, -1 ], "overmap": "pump_station_4_north" },
      { "point": [ 0, 1, -1 ], "overmap": "pump_station_5_north" }
    ],
    "connections": [
      { "point": [ 0, -1, 0 ], "terrain": "road", "connection": "local_road", "from": [ 0, 0, 0 ] },
      { "point": [ 1, -1, -1 ], "terrain": "sewer", "connection": "sewer_tunnel", "from": [ 0, -1, -1 ] },
      { "point": [ -1, -1, -1 ], "terrain": "sewer", "connection": "sewer_tunnel", "from": [ 0, -1, -1 ] },
      { "point": [ -1, 1, -1 ], "terrain": "sewer", "connection": "sewer_tunnel", "from": [ 0, 1, -1 ] }
    ],
    "locations": [ "land" ],
    "city_distance": [ 1, 4 ],
    "city_sizes": [ 4, 12 ],
    "occurrences": [ 0, 1 ],
    "flags": [ "CLASSIC" ]
  }
  ```

  1. `"type"`: is overmap_special.
  2. `"id"` is your buildings ID for the overmap. It also displays on the overmap in game.
  3. `"overmaps"` this works the same way as it does in the city building entries.  Note that the pump station is bigger then 1 OMT on the ground, so the y coordinate changes as well.
  4. `"connections"`: this places road, sewer, subway connections for your map.
  5. `"locations"`: valid OMT types this building can be placed on.
  6. `"city_distance"`, `"city_sizes"` both are parameters for where this spawns in relation to cities.
  7. `"occurrences": [ 0, 1 ],`:  Ok so occurrences can mean two things depending on if it uses the "UNIQUE" flag or not.   When the flag is absent, this simply translates to how many times this special can spawn PER overmap.  So 0 to 1 in this case.
        If you use the UNIQUE flag, this becomes a percentage so [ 1, 10 ] wouldn't be 1 to 10 times per overmap but a 1 in 10% chance to spawn on the overmap.  So 10% chance to spawn once per overmap.
  8. `"flags"`: These are flags you can use to further define the special.  For a list of flags see: [doc/JSON_FLAGS.md](https://github.com/CleverRaven/Cataclysm-DDA/blob/master/doc/JSON_FLAGS.md).

Read: [doc/OVERMAP.md](https://github.com/CleverRaven/Cataclysm-DDA/blob/master/doc/OVERMAP.md) for more details.

#### Overmap_terrain entries:

Choose a file for your building type at: [data/json/overmap/overmap_terrain](https://github.com/CleverRaven/Cataclysm-DDA/tree/master/data/json/overmap/overmap_terrain).

This set of entries defines how your building will look on the overmap.  It supports copy-from.
Example:
```
  {
    "type": "overmap_terrain",
    "id": "s_music",
    "copy-from": "generic_city_building",
    "name": "music store",
    "sym": "m",
    "color": "brown",
    "mondensity": 2,
    "extend": { "flags": [ "SOURCE_LUXURY" ] }
  },
  {
    "type": "overmap_terrain",
    "id": "s_music_roof",
    "copy-from": "generic_city_building",
    "name": "music store roof",
    "sym": "m",
    "color": "brown"
  }
  ```

You need one entry per mapgen ID:
1. `"type"` will always be overmap_terrain.
2. `"id"` will be the same ID you used in your mapgen file.
3. `"copy-from"` this will copy any data from another entry, excepting what you define here.
4. `"name"` how the name displays on the overmap.
5. `"sym"` the symbol displayed on the overmap. If left out, the carrots will be used `v<>^`
6. `"color"` color for overmap symbol.
7. `"mondesntiy"` sets the default monster density for this overmap tile.  You'll use this for general zombie spawns and reserve the mapgen monster entries for special spawns for that location (e.g. a pet store's pets).
8. `"extend"` many of these flags will be used by NPCs in the future for their AI, try to add flags appropriate for your location.  Others further define the mapgen, like having sidewalks generate.

For further information see: [Overmap Terrain section of doc/OVERMAP.md](https://github.com/CleverRaven/Cataclysm-DDA/blob/master/doc/OVERMAP.md#overmap-terrain).

#### Palettes:

As mentioned earlier, palettes can hold almost all the information that the object entry contains, except for `rows` and `fill_ter`.  Their main purpose is to reduce the need to add the same basic data to related maps and to maintain symbol consistency.

Entries that are added to the mapgen file's object will over ride the same symbol used in the palette.  In this way, you can use a palette and make select alterations to each mapgen as needed.  This is especially useful for multiple ground terrains like carpets, concrete, etc.

Terrain works very well when you substitute it via the mapgen file.  I've had less success overriding furniture but need to test it more to clarify when it works as intended.  I have recently noticed that if your palette symbol uses an array of values, the mapgen entry can't override it.

Example:
Entry for the mapgen file object: `"palettes": [ "roof_palette" ],`

The palette metadata:
```
  "type": "palette",
  "id": "roof_palette",
```

Everything else will look like a series of object entries, for example the roof_palette:

```
  {
    "type": "palette",
    "id": "roof_palette",
    "terrain": {
      ">": "t_stairs_down",
      "v": "t_ladder_down",
      ".": "t_flat_roof",
      " ": "t_open_air",
      "o": "t_glass_roof",
      "_": "t_floor",
      "-": "t_gutter",
      "4": "t_gutter_downspout",
      "5": "t_gutter_drop",
      "#": "t_grate",
      "&": "t_null",
      ":": "t_null",
      "X": "t_null",
      "=": "t_null",
      "A": "t_null",
      "b": "t_null",
      "c": "t_null",
      "t": "t_null",
      "r": "t_null",
      "L": "t_null",
      "C": "t_null",
      "Y": "t_null",
      "y": "t_null"
    },
    "furniture": {
      "&": "f_roof_turbine_vent",
      "N": "f_TV_antenna",
      ":": "f_cellphone_booster",
      "X": "f_small_satelitte_dish",
      "~": "f_chimney",
      "=": "f_vent_pipe",
      "A": "f_air_conditioner",
      "M": "f_solar_unit",
      "b": "f_bench",
      "c": "f_counter",
      "t": "f_table",
      "r": "f_rack",
      "L": "f_locker",
      "C": [ "f_crate_c", "f_cardboard_box" ],
      "Y": "f_stool",
      "s": "f_sofa",
      "S": "f_sink",
      "e": "f_oven",
      "F": "f_fridge",
      "Я": "f_standing_tank",
      "y": [ "f_indoor_plant_y", "f_indoor_plant" ]
    },
    "liquids": { "Я": { "liquid": "water_clean", "amount": [ 0, 300 ] } },
    "nested": {
      "5": { "chunks": [ [ "null", 90 ], [ "roof_1x1_birdnest", 10 ] ] },
      "-": { "chunks": [ [ "null", 1000 ], [ "roof_1x1_birdnest", 5 ] ] }
    },
    "toilets": { "T": {  } }
  }
  ```

If you want to look at more complex palettes, the standard_domestic_palette in [data/json/mapgen_palettes/house_general_palette.json](https://github.com/CleverRaven/Cataclysm-DDA/blob/master/data/json/mapgen_palettes/house_general_palette.json) is a good look at a palette designed to work across all CDDA houses.  It includes the loot spawns and accounts for most furniture that will be used in a house.  I also left a list of symbols open to be used in the mapgen file for specific location needs.

Finally, the series of house_w palettes at [data/json/mapgen_palettes/house_w_palette.json](https://github.com/CleverRaven/Cataclysm-DDA/blob/master/data/json/mapgen_palettes/house_w_palette.json) are designed to work together for houses using nested mapgen.  There is a palette devoted to the foundation, another for the nests, and finally another one I've designed for domestic outdoor nested chunks.

#### Final comments:

The information here should be enough for you got get around mapgen and start making maps but there are a lot of variations that will be covered in focused.

Not covered in this document:
  * Nested maps and their placement.
  * NPC spawns.
  * Advanced terrain tricks for complex floor options.
  * traps, terrain and you.
  * update_mapgen (NPC and player triggered map updates).
  * faction camp expansion maps.
  * field emitting furniture.
