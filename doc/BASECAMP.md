# Adding alternate basecamp upgrade paths

This doesn't work yet.

A basecamp upgrade path is a series of basecamp upgrade missions that upgrade the camp.  Upgrade missions are generally performed sequentially, but there is an option to have them branch.  Branched missions optionally can have further missions that require missions from other branches.

Bascamp upgrade paths are defined by several related files:
* The recipe JSONs that define what the material, tool, and skill requirements to perform an upgrade mission and the blueprint mapgen, blueprint requirements, blueprint provides, and blueprint resources associated with each upgrade mission.
* The mapgen_update JSONs that define how the map will change when the upgrade mission is complete.  These may include shared instances of nested mapgen, such a standard room or tent.
* The recipe_group JSONs that define what recipes can be crafted after completing the upgrade mission.

## recipe JSONs
The recipe JSONs are standard recipe JSONs, with the addition of a few fields.

New field | Description
-- | --
`"construction_blueprint"` | string, the `"update_mapgen_id"` of the mapgen_update JSON that will be peformed when the upgrade mission is complete.
`"construction_name"` | string, a short description/name of the upgrade mission that is displayed in the base camp mission selector.  The recipe's `"description"` field has the extended description that explains why a player might want to have an NPC perform this upgrade mission.
`"blueprint_provides"` | array of objects, with each object having an `"id"` string and an optional `"amount"` integer.  These are the camp features that are available when the upgrade mission is complete.  Every upgrade mission provides its recipe `"result"` with an amount of 1 automatically and that string does not need to be listed in `"blueprint_provides"`.
`"blueprint_requires"` | array of objects, with each object having an `"id"` string and an optional `"amount"` integer.  These are the camp features that are required before the upgrade mission can be attempted.
`"blueprint_resources"` | array of `"itype_id"`s.  Items with those ids will be added to the camp inventory after the upgrade mission is completed and can be used for crafting or additional upgrade missions.

### blueprint requires and provides
blueprint requires and blueprint provides are abstract concepts or flags that an upgrade mission requires to start, or that are provided by a previous upgrade mission to satisfy the blueprint requirements of a current upgrade mission.  Each one has an `"id"` and an `"amount"`.  Multiple requires or provides with the same `"id"` sum their `"amount"` if they're on the same basecamp expansion.

These are arbitrary strings and can be used to control the branching of the upgrade paths.  However, some strings have meaning within the basecamp code:

provides `"id"` | meaning
-- | --
`"bed"` | every 2 `"amount"`' of `"bed"` allows another expansion in the camp, to a maximum of 8, not include the camp center.
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
