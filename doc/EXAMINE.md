# Terrain/Furniture examination actions

These are actions that will be performed when a terrain/furniture is examined.
The hardcoded examine actions specified as a `"examine_action": "ACTION"`, where `ACTION` is replaced with one of the strings from the list below.
The examine actors are specified as JSON objects with a `type` corresponding to a certain type of action, and other members filling in the data for how the action functions.

## Hardcoded Examine Actions

- ```aggie_plant``` Harvest plants.
- ```autodoc``` Brings the autodoc consoles menu. Needs the ```AUTODOC``` flag to function properly and an adjacent furniture with the ```AUTODOC_COUCH``` flag.
- ```autoclave_empty``` Start the autoclave cycle if it contains filthy CBM, and the player has enough water.
- ```autoclave_full``` Check on the progress of the cycle, and collect sterile CBM once cycle is completed.
- ```bars``` Take advantage of AMORPHOUS and slip through the bars.
- ```bulletin_board``` Use this to arrange tasks for your faction camp.
- ```chainfence``` Hop over the chain fence.
- ```controls_gate``` Controls the attached gate.
- ```dirtmound``` Plant seeds and plants.
- ```elevator``` Use the elevator to change floors.
- ```fault``` Displays descriptive message, but otherwise unused.
- ```flower_poppy``` Pick the mutated poppy.
- ```fswitch``` Flip the switch and the rocks will shift.
- ```fungus``` Release spores as the terrain crumbles away.
- ```gaspump``` Use the gas-pump.
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
- ```shrub_marloss``` Pick a marloss bush.
- ```shrub_wildveggies``` Pick a wild veggies shrub.
- ```slot_machine``` Gamble.
- ```toilet``` Either drink or get water out of the toilet.
- ```water_source``` Drink or get water from a water source.

## Examine Actors

### `cardreader`

#### `flags`
Mandatory.
Array of strings.
List of item flags that, when on an item, mean that the item can be used as card.

#### `consume_card`
Optional, defaults to true.
Boolean (true/false).
Whether or not to consume the item used to activite this cardreader.

#### `allow_hacking`
Optional, defaults to true.
Boolean (true/false).
Whether or not to allow hacking this door with an electrohack.
If this allows hacking, it will ignore the data specified here, and transform all `t_door_metal_locked` to `t_door_metal_c` in a 3 tile radius.

#### `despawn_monsters`
Optional, defaults to true.
Boolean (true/false).
Whether or not to remove hostile monsters with the `ID_CARD_DESPAWN` flag.

### `mapgen_id`
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
What message to print to the log when this is sucessfully activated.

#### `redundant_msg`
Mandatory.
String.
What message to print when attempting to activate the cardreader after it has already been activated.

#### Example
```
{
  "type": "cardreader",
  "flags": [ "SCIENCE_CARD" ],
  "consume_card": true,
  "allow_hacking": true,
  "despawn_monsters": true,
  "radius": 3,
  "terrain_changes": { "t_door_metal_locked": "t_door_metal_c" },
  "furn_changes": { "f_crate_c": "f_crate_o" },
  "query": true,
  "query_msg": "Are you sure you want to open this door?",
  "success_msg": "You opened the door!",
  "redundant_msg": "The door is already open."
}
```
