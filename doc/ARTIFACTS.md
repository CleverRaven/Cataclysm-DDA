# Artifacts

## Generation

The procedural generation of artifacts is defined in Json. The object looks like the following:
```json
{
    "type": "relic_procgen_data",
    "id": "cult",
    "passive_add_procgen_values": [
        {
            "weight": 100,
            "min_value": -1,
            "max_value": 1,
            "type": "STRENGTH",
            "increment": 1,
            "power_per_increment": 250
        }
    ],
    "passive_mult_procgen_values": [
        {
            "weight": 100,
            "min_value": -1.5,
            "max_value": 1.5,
            "type": "STRENGTH",
            "increment": 0.1,
            "power_per_increment": 250
        }
    ],
    "type_weights": [ { "weight": 100, "value": "passive_enchantment_add" } ],
    "items": [ { "weight": 100, "item": "spoon" } ]
  }
```

### passive_add_procgen_values and passive_mult_procgen_values
weight: the weight of this value in the list, to be chosen randomly\n
min_value: the minimum possible value for this value type. for add must be an integer, for mult it can be a float\n
max_value: the maximum possible value for this value type. for add must be an integer, for mult it can be a float\n
type: the type of enchantment value. see MAGIC.md for detailed documentation on enchantment values\n
increment: the increment that is used for the power multiplier\n
power_per_increment: the power value per increment

### type_weights
The weights for the "type" of attribute that is rolled. posisble values right now that are functional are:
- passive_enchantment_add
- passive_enchantment_mult

This must be included in a dataset or it could cause a crash.

### items
The items that are allowed to be spawned in map extras (hardcode). May have additional applications in the future.

## Power Level
An artifact's power level is a summation of its attributes. For example, each point of strength addition in the above object, the artifact is a +250 power, so an artifact with +2 strength would have a power level of 500. similarly, if an artifact had a strength multiplier of 0.8, it would have a power level of -500.
