# Adding alternate basecamp upgrade paths

A basecamp upgrade path is a series of basecamp upgrade missions that upgrade the camp.  Upgrade missions are generally performed sequentially, but there is an option to have them branch.  Branched missions optionally can have further missions that require missions from other branches.

Bascamp upgrade paths are defined by several related files:
* The recipe JSONs that define what the material, tool, and skill requirements to perform an upgrade mission and the blueprint mapgen, blueprint requirements, blueprint provides, and blueprint resources associated with each upgrade mission.
* The mapgen_update JSONs that define how the map will change when the upgrade mission is complete.  These may include shared instances of nested mapgen, such a standard room or tent.
* The recipe_group JSONs that define what recipes can be crafted after completing the upgrade mission and what camps and expansions are avialable.

## recipe JSONs
The recipe JSONs are standard recipe JSONs, with the addition of a few fields.

New field | Description
-- | --
`"construction_blueprint"` | string, the `"update_mapgen_id"` of the mapgen_update JSON that will be peformed when the upgrade mission is complete.
`"construction_name"` | string, a short description/name of the upgrade mission that is displayed in the base camp mission selector.  The recipe's `"description"` field has the extended description that explains why a player might want to have an NPC perform this upgrade mission.
`"blueprint_provides"` | array of objects, with each object having an `"id"` string and an optional `"amount"` integer.  These are the camp features that are available when the upgrade mission is complete.  Every upgrade mission provides its recipe `"result"` with an amount of 1 automatically and that string does not need to be listed in `"blueprint_provides"`.
`"blueprint_requires"` | array of objects, with each object having an `"id"` string and an optional `"amount"` integer.  These are the camp features that are required before the upgrade mission can be attempted.
`"blueprint_excludes"` | array of objects, with each object having an `"id"` string and an optional `"amount"` integer.  These are the camp features that prevent the upgrade mission from being attempted if they exist.
`"blueprint_resources"` | array of `"itype_id"`s.  Items with those ids will be added to the camp inventory after the upgrade mission is completed and can be used for crafting or additional upgrade missions.

### blueprint requires, provides, and excludes
blueprint requires, blueprint provides, and blueprint exlcudes are abstract concepts or flags that an upgrade mission requires to start, or that are provided by a previous upgrade mission to satisfy the blueprint requirements of a current upgrade mission, or that prevent an upgrade mission from being available.  Each one has an `"id"` and an `"amount"`.  Multiple requires, provides, or excludes with the same `"id"` sum their `"amount"` if they're on the same basecamp expansion.

Every upgrade mission has its recipe `"result"` as a blueprint_provides and a blueprint_excludes, so upgrade missions will automatically prevent themselves from being repatable.

These are arbitrary strings and can be used to control the branching of the upgrade paths.  However, some strings have meaning within the basecamp code:

provides `"id"` | meaning
-- | --
`"bed"` | every 2 `"amount"`' of `"bed"` allows another expansion in the camp, to a maximum of 8, not include the camp center.
`"tool_storage"` | after this upgrade mission is complete, the Store Tools mission will be available
.
`"radio"` | after this upgrade mission is complete, two way radios communicating to the camp have extended range.
`"pantry"` | after this upgrade mission is complete, the Distribute Food mission is more efficient when dealing with short term spoilage items.
`"gathering"` | after this upgrade mission is complete, the Gather Materials, Distribute Food, and Reset Sort Points basecamp missions will be available.
`"firewood"` | after this upgrade mission is complete, the Gather Firewood basecamp mission will be available.
`"sorting"` | after this upgrade mission is complete, the Menial Labor basecamp mission will be available.
`"logging"` | after this upgrade mission is complete, the Cut Logs and Clear a Forest basecamp missions will be available.
`"relaying"` | after this upgrade mission is complete, the Setup Hide Site and Relay Hide Site basecamp missions will be available.
`"foraging"` | after this upgrade mission is complete, the Forage for Plants basecamp mission will be available.
`"trapping"` |  after this upgrade mission is complete, the Trap Small Game basecamp mission will be available.
`"hunting"` | after this upgrade mission is complete, the Hunt Large Animals basecamp mission will be available.
`"walls"` | after this upgrade mission is complete, the Construct Map Fortifications basecamp mission will be available.
`"recruiting"` | after this upgrade mission is complete, the Recruit Companions basecamp mission will be available.
`"scouting"` | after this upgrade mission is complete, the Scout Mission basecamp mission will be available.
`"patrolling"` | after this upgrade mission is complete, the Combat Patrol basecamp mission will be available.
`"dismantling"` | after this upgrade mission is complete, the Chop Shop basecamp mission will be available.
`"farming"` | after this upgrade mission is complete, the Plow Fields, Plant Fields, Fertilize Fields, and Harvest Fields basecamp missions will be available.

### Sample recipe JSON
```JSON
  {
    "type": "recipe",
    "result": "faction_base_camp_8",
    "description": "We need to expand our base to include basic dining facilities.",
    "category": "CC_BUILDING",
    "subcategory": "CSC_BUILDING_BASES",
    "skill_used": "fabrication",
    "difficulty": 5,
    "autolearn": false,
    "never_learn": true,
    "comment": "2hrs*4wall + .5 hr*2tables + .5hr*4benches+ 1hrs pits = 12 hrs (12hrs on/off) 1 days total",
    "time": "1440 m",
    "construction_blueprint": "faction_base_field_camp_8",
    "blueprint_name": "basic central kitchen",
    "blueprint_resources": [ "fake_stove" ],
    "blueprint_provides": [ { "id": "trapping", "amount": 1 }, { "id": "hunting", "amount": 1 }, { "id": "walls", "amount": 1 }, { "id": "recruiting", "amount": 1 }
    ],
    "blueprint_requires": [ { "id": "faction_base_camp_6", "amount": 1 } ],
    "qualities": [ [ { "id": "DIG", "level": 1 } ], [ { "id": "SAW_M", "level": 1 } ], [ { "id": "HAMMER", "level": 2 } ] ],
    "components": [ [ [ "2x4", 28 ] ], [ [ "log", 16 ] ], [ [ "nail", 56 ] ], [ [ "stick", 24 ] ], [ [ "metal_tank", 1 ] ], [ [ "pipe", 1 ] ]
    ]
  },
```

The `"faction_base_camp_8"` upgrade mission can be performed after `"faction_base_camp_6"` and enables the trapping, hunting, fortification, and recruiting basecamp missions.  It adds a camp stove to the camp's inventory to represent the iron stove.

## mapgen_update JSON

These are standard mapgen_update JSON; see doc/MAPGEN.md for more details.  Each one should change a localized portion of the camp map and should, as much as possible, be independent of any other pieces of mapgen_update for the basecamp upgrade path.  Obviously, some bits are going to expand other bits, in which case their `"blueprint_requires"` should include the `"blueprint_provides"` of the previous upgrade missions.

### Sample mapgen_update JSON

```json
  {
    "type": "mapgen",
    "update_mapgen_id": "faction_base_field_camp_7",
    "method": "json",
    "object": { "place_nested": [ { "chunks": [ "basecamp_large_tent_east" ], "x": 2, "y": 10 } ] }
  },
  {
    "type": "mapgen",
    "method": "json",
    "nested_mapgen_id": "basecamp_large_tent_east",
    "object": {
      "mapgensize": [ 5, 5 ],
      "rows": [
        "WWWWW",
        "Wbb$W",
        "Wr;;D",
        "Wbb$W",
        "WWWWW"
      ],
      "palettes": [ "acidia_camp_palette" ]
    }
  },
```

This mapgen_update places a large tent in the west central portion of the camp.  The `"place_nested"` references a standard map used in the primitive field camp.

## Recipe groups
Recipe groups serve two purposes: they indicate what recipes can produced by the camp after an upgrade mission is completed, and they indicate what upgrade paths are available and where camps can be placed.

### Upgrade Paths and Expansions
There are two special recipe groups, `"all_faction_base_types"` and `"all_faction_base_expansions"`.  They both look like this:
```json
  {
    "type": "recipe_group",
    "name": "all_faction_base_expansions",
    "building_type": "NONE",
    "recipes": [
      { "id": "faction_base_farm_0", "description": "Farm", "om_terrains": [ "field" ] },
      { "id": "faction_base_garage_0", "description": "Garage", "om_terrains": [ "field" ] },
      { "id": "faction_base_kitchen_0", "description": "Kitchen", "om_terrains": [ "field" ] },
      { "id": "faction_base_blacksmith_0", "description": "Blacksmith Shop", "om_terrains": [ "field" ] }
    ]
  },
```

Each entry in the `"recipes"` array must be a dictionary with the `"id"`, `"description"`, and `"om_terrains"` fields.  `"id"` is the recipe `"id"` of the recipe that starts that basecamp or basecamp expansion upgrade path.  `"description"` is a short name of the basecamp or basecamp expansion.  `"om_terrains"` is a list of overmap terrain ids which can be used as the basis for the basecamp or basecamp expansion.

All recipes that start an upgrade path or expansion should have a blueprint requirement that can never be met, such as "not_an_upgrade", to prevent them from showing up as available upgrades.

If the player attempts to start a basecamp on an overmap terrain that has two or more valid basecamp expansion paths, she will allowed to choose which path to start.

## Sample basecamp upgrade path

The primitive field camp has the following upgrade path:
1. `"faction_base_camp_0"` - the initial camp survey and the bulletin board.
2. `"faction_base_camp_1"` - setting up the northeast tent
3. `"faction_base_camp_2"` - digging the fire pit and constructing a bed
4. `"faction_base_camp_3"` - adding the bookshelves to store stuff
5. `"faction_base_camp_4"` - adding the second bed to the tent
6. any of:
`"faction_base_camp_5"`, `"faction_base_camp_7"`, `"faction_base_camp_9"`, `"faction_base_camp_15"`, `"faction_base_camp_18"` - add additional tents with beds in any order.
or `"faction_base_camp_6"` - build the central kitchen.  `"faction_base_camp_8"`, `"faction_base_camp_10"`, `"faction_base_camp_11"`, `"faction_base_camp_16"` must be built in that sequence afterwards, though interleaved with the other optional upgrade missions after `"faction_base_camp_4"`.
or `"faction_base_camp_12"` - the central camp well.
or `"faction_base_camp_13"` - building the camp's outer wall, which can be followed by `"faction_base_camp_14"` to complete the wall.
or `"faction_base_camp_19"` - building the camp's radio tower, which can be followed by `"faction_base_camp_20"` to build the radio tower console and make it operational.
7.  `"faction_base_camp_17"` - adding better doors to camp wall and the central building must be built after `"faction_base_camp_14"` and `"faction_base_camp_16"`.

After setting up the first tent, the player has a lot of options: they can build additional tents to enable expansions, they can build the well for water, they can build as much or as little of the central kitchen as they desire, or add the wall to give the camp defenses, or the radio tower to improve the range of their two-way radios.

## Modular Basecamp conventions
The modular basecamp is a structure for designing basecamp upgrade paths.  You don't have to use it, but elements of the design might get more code support in the future.

### Layout
A modular camp is laid out on a 24x24 overmap tile.  The outer 3 map squares on each side are reserved for fortifications and movement corridors, and the inner 18x18 map squares are divided into a 3x3 grid of 6x6 areas.

Area | upper left position | lower right position
-- | -- | --
northwest | 3, 3 | 8, 8
north | 9, 3 | 14, 8
northeast | 15, 3 | 20, 8
west | 3, 9 | 8, 14
center | 9, 9 | 14, 14
east | 15, 9, | 20, 14
southwest | 3, 15 | 8, 20
south | 9, 15 | 14, 20
southeast | 15, 15 | 20, 20

Ideally, nested mapgen chunks should fit entirely into a 6x6 area.

### Naming scheme
Modular bases use the following naming scheme for recipes.  Each element is separated by an `_`

* `"faction_base"` <-- encouraged for all basecamp recipes, _and_
* `"modular"` <-- indicates a modular construction, _and_
* overmap terrain id <-- the base terrain that this modular base is built upon, _and_
* DESCRIPTOR <-- arbitrary string that describes what is being built.  For buildings, "room" is used to mean a construction that is intended to be part of a larger building and might share walls with other parts of the building in adjacent areas; "shack" means a free standing building.  Numbers indicate stages of construction, with 4 usually meaning a complete structure, with walls and roof, _and_
* MATERIAL <-- the type of material used in construction, such as metal, wood, brick, or wad (short for wattle-and-daub), _and_
* AREA <-- the area in the 3x3 grid of the modular camp layout.

blueprint keywords follow a similar scheme, but `"faction_base_modular"` is collapsed into `"fbm"` and the overmap terrain id is collapsed into a short identifier.  ie, `"fbmf"` is the keyword identifier for elements of the modular field base.
