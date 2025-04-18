# Terrain/Furniture Examination Actions

These are actions that will be performed when a terrain/furniture is examined.
The hardcoded examine actions specified as a `"examine_action": "ACTION"`, where `ACTION` is replaced with one of the strings from the list below.
The examine actors are specified as JSON objects with a `type` corresponding to a certain type of action, and other members filling in the data for how the action functions.

## Hardcoded Examine Actions

- ```aggie_plant``` Harvest plants.
- ```autodoc``` Brings the Autodoc consoles menu. Needs the ```AUTODOC``` flag to function properly and an adjacent furniture with the ```AUTODOC_COUCH``` flag.
- ```autoclave_empty``` Start the autoclave cycle if it contains filthy CBM, and the player has enough water.
- ```autoclave_full``` Check on the progress of the cycle, and collect sterile CBM once cycle is completed.
- ```bars``` Take advantage of AMORPHOUS and slip through the bars.
- ```bulletin_board``` Use this to arrange tasks for your faction camp.
- ```chainfence``` Hop over the chain fence.
- ```controls_gate``` Controls the attached gate.
- ```dirtmound``` Plant seeds and plants.
- ```elevator``` Use the elevator to change floors.
- ```finite_water_source``` Drink or get liquid from this terrain/furniture. Unlike ordinary `water_source`, terrain with this examine action will get liquid from a finite source (liquid is placed on that tile as an item during the mapgen) and will stop functioning if said liquid if exhausted on that tile. Should be used in pair with `liquid_source`
- ```flower_poppy``` Pick the mutated poppy.
- ```fswitch``` Flip the switch and the rocks will shift.
- ```fungus``` Release spores as the terrain crumbles away.
- ```gaspump``` Use the gas-pump.
- ```harvest_plant_ex``` Harvest a harvestable plant. Works on field crops and planters. (corn, wheat, etc.)
- ```locked_object``` Locked, but can be pried open. Adding 'PICKABLE' flag allows opening with a lockpick as well. Prying/lockpicking results are hardcoded.
- ```locked_object_pickable``` Locked, but can be opened with a lockpick. Requires 'PICKABLE' flag, lockpicking results are hardcoded.
- ```none``` None
- ```pedestal_temple``` Opens the temple if you have a petrified eye.
- ```pedestal_wyrm``` Spawn wyrms.
- ```pit_covered``` Uncover the pit.
- ```pit``` Cover the pit if you have some planks of wood.
- ```portable_structure``` Take down a tent or similar portable structure.
- ```recycle_compactor``` Compress pure metal objects into basic shapes.
- ```rubble``` Clear up the rubble if you have a shovel.
- ```safe``` Attempt to crack the safe.
- ```shelter``` Take down the shelter.
- ```shrub_marloss``` Pick a Marloss bush.
- ```shrub_wildveggies``` Pick a wild veggies shrub.
- ```slot_machine``` Gamble.
- ```water_source``` Drink or get liquid from a furniture/terrain. Should be used in pair with `liquid_source`
- ```mortar``` Shoot a projectile.

## Examine Actors

### `appliance_convert`

#### `furn_set`
Optional, defaults to no change.
String.
Furniture id that this tile will be set to after placing the appliance.

#### `ter_set`
Optional, defaults to no change.
String.
Terrain id that this tile will be set to after placing the appliance.

#### `item`
Mandatory.
String.
Item id of the base item of this appliance.

#### Example
```jsonc
  {
    "type": "appliance_convert",
    "furn_set": "f_null",
    "ter_set": "t_floor",
    "item": "fridge"
  }
```

### `cardreader`

#### `flags`
Mandatory.
Array of strings.
List of item flags that, when on an item, mean that the item can be used as card.

#### `consume_card`
Optional, defaults to true.
Boolean (true/false).
Whether or not to consume the item used to activate this cardreader.

#### `allow_hacking`
Optional, defaults to true.
Boolean (true/false).
Whether or not to allow hacking this door with an electrohack.
If this allows hacking, it will ignore the data specified here, and transform all `t_door_metal_locked` to `t_door_metal_c` in a 3 tile radius.

#### `despawn_monsters`
Optional, defaults to true.
Boolean (true/false).
Whether or not to remove hostile monsters with the `ID_CARD_DESPAWN` flag.

#### `omt_allowed_radius`
Optional, defaults to infinity.
Integer (0 or greater).
For cards with the `PRESERVE_SPAWN_OMT` flag, how many overmap tiles away a card can spawn and be accepted for this cardreader.
For cards without the flag, this field is ignored.

#### `mapgen_id`
Optional.
String.
Update mapgen ID to apply on opening the door.
Conflicts with `radius`, `terrain_changes`, and `furn_changes`.

#### `radius`
Optional, defaults to 3.
Integer.
What area around the cardreader to apply changes to.
With a radius of three, it will go 3 tiles out from the reader in every direction.

#### `terrain_changes`
Optional, defaults to nothing.
JSON Object.
Each key in this JSON Object corresponds to a terrain, and the value is a terrain that that terrain will be transformed into.
Multiple items can be specified by adding more keys, e.g. `{ "a": "b", "x": "y" }`

#### `furn_changes`
Optional, defaults to nothing.
JSON Object.
Each key in this JSON Object corresponds to a furniture, and the value is a furniture that that furniture will be transformed into.
Multiple items can be specified by adding more keys, e.g. `{ "a": "b", "x": "y" }`

#### `query`
Optional, defaults to true.
Boolean (true/false).
Whether or not to query the player before activating and potentially consuming a card.

#### `query_msg`
Optional, defaults to nothing.
String.
What message to display when querying the player on whether or not to activate the cardreader.

#### `success_msg`
Mandatory.
String.
What message to print to the log when this is successfully activated.

#### `redundant_msg`
Mandatory.
String.
What message to print when attempting to activate the cardreader after it has already been activated.

#### Example
```jsonc
  {
    "type": "cardreader",
    "flags": [ "SCIENCE_CARD" ],
    "consume_card": true,
    "allow_hacking": true,
    "despawn_monsters": true,
    "omt_allowed_radius": 3,
    "radius": 3,
    "terrain_changes": { "t_door_metal_locked": "t_door_metal_c" },
    "furn_changes": { "f_crate_c": "f_crate_o" },
    "query": true,
    "query_msg": "Are you sure you want to open this door?",
    "success_msg": "You opened the door!",
    "redundant_msg": "The door is already open."
  }
```
### `effect_on_condition`

#### `effect_on_conditions`
Mandatory.
Array of strings and or effect_on_condition objects.
Run all of the eocs upon being examined with u as the examiner and npc as null. See [EFFECT_ON_CONDITION.md](EFFECT_ON_CONDITION.md)

### `mortar`

#### `ammo`
Mandatory.
Array of strings.
List of ammo types, that can be used in this mortar.

#### `range`
Mandatory
Integer
How far this mortar can shoot, in meters.

#### `condition`
Optional, defaults to true.
Condition
Can be used to add custom conditions for a turret to be used; character must pass the condition, otherwise they won't be able to use a turret.

#### `condition_fail_msg`
Optional, defaults to `You cannot use this mortar.`
String.
If `condition` is failed, the game print this message

#### `aim_deviation`
Optional, defaults to 0.
Integer or variable object
Percent of the distance between you and aiming point, that you may miss; aim_deviation of `0.02` when you aim 1000 meters afar would mean you can miss for 20 meters, 10 meters in either coordinate.

#### `aim_duration`
Optional, defaults to 0 seconds.
Duration or variable object
How long you gonna aim before actually shooting a projectile.

#### `flight_time`
Optional, defaults to 0 seconds.
Duration or variable object
How long the projectile gonna fly before explosion occur.

#### `effect_on_conditions`
Optional
JSON Object.
Effect on condition, that would be fired in the end of shooting. Additional context variables sent: 
`this`, string, furniture id; 
`pos`; string, coordinates of the furniture; 
`target`, string, coordinates of picked tile

#### Example
```jsonc
    "examine_action": {
      "type": "mortar",
      "ammo": [ "mortar_60mm" ],
      "condition": { "math": [ "u_skill('launcher') >= 1" ] },
      "condition_fail_msg": "You have absolutely no idea how to use this mortar.",
      "range": 3490,
      "aim_duration": {
        "math": [
          "u_effect_duration('aimed_60mm') > 1",
          " ? max(5, 16 / u_skill('launcher'))",
          " : max(60, 190 / u_skill('launcher'))"
        ]
      },
      "aim_deviation": { "math": [ "max(0.02, 0.1 / u_skill('launcher'))" ] },
      "flight_time": "15 seconds",
      "effect_on_conditions": [
        {
          "id": "EOC_60MM_SHOT",
          "effect": [
            { "u_add_effect": "aimed_60mm", "duration": "3 minutes" },
            { "math": [ "u_skill_exp('launcher', 'format': 'percentage')++" ] },
            { "if": { "math": [ "distance('u', _target) <= 400 " ] }, "then": { "u_spawn_item": "60mm_propellant" } },
            { "if": { "math": [ "distance('u', _target) <= 1340" ] }, "then": { "u_spawn_item": "60mm_propellant" } },
            { "if": { "math": [ "distance('u', _target) <= 2150" ] }, "then": { "u_spawn_item": "60mm_propellant" } },
            { "if": { "math": [ "distance('u', _target) <= 2890" ] }, "then": { "u_spawn_item": "60mm_propellant" } }
          ]
        }
      ]
    }
```