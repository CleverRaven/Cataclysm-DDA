# Terrain/Furniture examination actions

These are actions that will be performed when a terrain/furniture is examined.
These are specified as a `"examine_action": "ACTION"`, where `ACTION` is replaced with one of the strings from the list below.

## Examine Actions

- ```aggie_plant``` Harvest plants.
- ```autodoc``` Brings the autodoc consoles menu. Needs the ```AUTODOC``` flag to function properly and an adjacent furniture with the ```AUTODOC_COUCH``` flag.
- ```autoclave_empty``` Start the autoclave cycle if it contains filthy CBM, and the player has enough water.
- ```autoclave_full``` Check on the progress of the cycle, and collect sterile CBM once cycle is completed.
- ```bars``` Take advantage of AMORPHOUS and slip through the bars.
- ```bulletin_board``` Use this to arrange tasks for your faction camp.
- ```cardreader``` Use the cardreader with a valid card, or attempt to hack.
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
