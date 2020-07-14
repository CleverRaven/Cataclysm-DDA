# Artifacts

## Introduction

An "artifact" is a special game item with unique "magic" effects.  It uses a base item id and enhances it with further abilities.

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

As the names suggest, these are *passive* benefits/penalties to having the artifact (ie. always present without activating the artifact's abilities).  **Add** values add or subtract from existing scores, and **mult** values multiply them.  These are entered as a list of possible 'abilities' the artifact could get. It does not by default get all these abilities, rather when it spawns it selects from the list provided.

- **weight:** the weight of this value in the list, to be chosen randomly
- **min_value:** the minimum possible value for this value type. for add must be an integer, for mult it can be a float
- **max_value:** the maximum possible value for this value type. for add must be an integer, for mult it can be a float
- **type:** the type of enchantment value. see MAGIC.md for detailed documentation on enchantment values
- **increment:** the increment that is used for the power multiplier
- **power_per_increment:** the power value per increment

### type_weights
This determines the relative weight of the 'add' and 'mult' types.  When generated, an artifact first decides if it is going to apply an 'add' or a 'mult' ability based on the type_weights of each.  Then it uses the weights of the entries under the selected type to pick an ability.  This continues cycling until the artifact reaches the defined power level.  Possible values right now that are functional are:
- passive_enchantment_add
- passive_enchantment_mult

This must be included in a dataset or it could cause a crash.

### items
This provides a list of possible items that this artifact can spawn as, if it appears randomly in a hard-coded map extra.

## Power Level
An artifact's power level is a summation of its attributes. For example, each point of strength addition in the above object, the artifact is a +250 power, so an artifact with +2 strength would have a power level of 500. similarly, if an artifact had a strength multiplier of 0.8, it would have a power level of -500.
