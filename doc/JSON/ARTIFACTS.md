# Artifacts

## Introduction

An "artifact" is a special game item with unique "magic" effects.  It uses a base item id and enhances it with further abilities.

Artifacts may have active or passive effects, requiring them to be worn, held in inventory, or wielded for their effects to work. Active effects may require 'charges', they may come with charges, or generate charges at certain intervals or when certain events occur. Passive effects apply regardless of charges if the artifact is correctly worn/held/wielded.

For Dark Days Ahead, it is intended that effects of artifacts are not knowable without using them. Artifacts are intended to require the player to use and experiment with them to determine what effects, if any, they might have. Some effects are intentionally subtle or difficult to figure out, which is part of the intended gameplay.

Mods are permitted to remove, add, or change this secrecy as they see fit, but there is not currently a way for them to do so.

## Generation

The procedural generation of artifacts is defined in Json. The object looks like the following:
```jsonc
  {
    "type": "relic_procgen_data",
    "id": "cult",
    "charge_types": [
      {
        "weight": 100,
        "charges": { "range": [ 0, 3 ], "power": 25 },
        "charges_per_use":{ "range": [ 1, 1 ], "power": 25 },
        "max_charges": { "range": [ 1, 3 ], "power": 25 },
        "recharge_type": "periodic",
        "time": [ "3 h", "6 h" ]
      }
    ],
    "active_procgen_values": [ { "weight": 100, "spell_id": "AEA_PAIN" } ],
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

### charge_types

The various ways this artifact can charge and use charges.

- **charges** the number of charges this artifact starts with - a random value between the two ones in 'range' are picked, and power is the power value.
- **charges_per_use** how many charges you spend with each use of this artifact - a random value between the two ones in 'range' are picked, and power is the power value.
- **max_charges** The maximum number of charges this artifact can have. - a random value between the two ones in 'range' are picked, and power is the power value.
- **recharge_type** How this artifact recharges
- **recharge_condition** Condition of recharging; whether item must be held, worn or wield
- **time** The amount of time this artifact takes to recharge - a random value between the two ones provided is picked.

#### recharge_types

- **none** This artifact does not recharge
- **periodic** This artifact takes 'time' amount of time to recharge
- **solar_sunny** This artifact takes 'time' amount of time to recharge, only recharges if weather is sunny and character is outside
- **solar_cloudy** This artifact takes 'time' amount of time to recharge, only recharges if weather is cloudy and character is outside
- **lunar** This artifact takes 'time' amount of time to recharge, only recharges at night time.
- **full_moon** This artifact takes 'time' amount of time to recharge, only recharges on the nights of the full moon
- **new_moon** This artifact takes 'time' amount of time to recharge, only recharges on the nights of the new moon

#### recharge_conditions

- **wield** This artifact only recharges if is wielded
- **worn** This artifact only recharges if is wielded or worn
- **held** This artifact only recharges if is wielded, worn or in inventory

### passive_add_procgen_values and passive_mult_procgen_values

As the names suggest, these are *passive* benefits/penalties to having the artifact (i.e. always present without activating the artifact's abilities).  **Add** values add or subtract from existing scores, and **mult** values multiply them.  A multiply value of -1 is -100% and a multiply of 2.5 is +250%. These are entered as a list of possible 'abilities' the artifact could get. It does not by default get all these abilities, rather when it spawns it selects from the list provided.

- **weight:** the weight of this value in the list, to be chosen randomly
- **min_value:** the minimum possible value for this value type. for add must be an integer, for mult it can be a float
- **max_value:** the maximum possible value for this value type. for add must be an integer, for mult it can be a float
- **type:** the type of enchantment value. see MAGIC.md for detailed documentation on enchantment values
- **increment:** the increment that is used for the power multiplier
- **power_per_increment:** the power value per increment
- **ench_has:** where the artifact must be in inventory for the enchantment to take effect

#### ench_has

- **wield** This enchantment or spell only takes effect while the artifact is wielded
- **worn** This enchantment or spell only takes effect while the artifact is wielded or worn
- **held** This enchantment or spell only takes effect while the artifact is wielded, worn, or in inventory

### type_weights
This determines the relative weight of the 'add' and 'mult' types.  When generated, an artifact first decides if it is going to apply an 'add' or a 'mult' ability based on the type_weights of each.  Then it uses the weights of the entries under the selected type to pick an ability.  This continues cycling until the artifact reaches the defined power level.  Possible values right now that are functional are:
- passive_enchantment_add
- passive_enchantment_mult

This must be included in a dataset or it could cause a crash.

### items
This provides a list of possible items that this artifact can spawn as, if it appears randomly in a hard-coded map extra.

## Power Level
An artifact's power level is a summation of its attributes. For example, each point of strength addition in the above object, the artifact is a +250 power, so an artifact with +2 strength would have a power level of 500. similarly, if an artifact had a strength multiplier of 0.8, it would have a power level of -500.

## Resonance
Resonance is an enchantment assigned to artifacts, and is always equal to the artifact's power, being assigned according to the final power of the artifact after its main properties have been generated.  Although artifacts do not have to have resonance (and will not have resonance unless explicitly assigned) all vanilla artifacts with the sole exception of the "Exposed-Wiring Prototype" have resonance, as should any future artifacts added to the game.  Resonance is always at least zero, so a purely detrimental artifact cannot be used to "lower" your resonance although a mixed artifact is less resonant than a purely positive one.

Resonance is assigned to artifacts in the JSON responsible for adding them to mapgen, where artifacts are given a power level and a set of procgen data to draw from. If flagged as true, this artifact will have resonance. If it does not include this optional flag, it will default to false.

Resonance represents the potential dangers of artifacts on the human body to counterbalance their positive effects.  For the purposes of lore, artifacts should primarily cause such resonance effects when they are affecting a human body - such as when carried or wielded by the PC.  Resonance effects are currently hardcoded, and scale with the amount of resonance you have, beginning at 2,000 resonance (2,000 total power of artifacts carried).  Resonance of 2,000 causes an effect roughly every hour, but effect also occur more frequently the more resonance you have, becoming 100% more common for every whole 2,000 resonance. There are 4 "tiers" of effects in increasing severity kicking in at 2,000, 4,500, 7,500, 12,500 resonance. Each "tier" of resonance has a 50% chance to downgrade to a lower tier of severity. Example: you have 8,000 resonance. There is a 50% chance you will suffer a tier 3 resonance effect, a 25% chance you will suffer a tier 2 resonance effect, and a 25% chance you will suffer a tier 1 effect. If you had 4,000 resonance it would be a 50% chance to get a tier 2 effect and a 50% chance to get a tier 1 effect.

These effects should be relatively vague - it should not be explicitly clear that carrying multiple artifacts is what's causing a player's misfortune, but it should be implied due to the effects not happening until the player picks up a bunch of cool-looking napkins and Rubik's cubes.  Tier 1 effects are nausea, low-strength hallucinations, and minor pain. Tier 2 effects are minor radiation (blocked by rad resistant gear), safe micro-teleports and a very loud noise.  Tier 3 effects include severe pain, severe radiation (not fully blockable by gear), and unsafe teleportation.  Tier 4 effects include hounds of Tindalos, incorporeal, and creating a tear in reality.

The resonance factor of an artifact is intended to be measurable, but only at some measurable opportunity cost. There is currently one way to do so ingame, but others could be added.
