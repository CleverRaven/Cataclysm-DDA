# relic_data

relic_data is a replacement of the previous artifact_data on items.  It is an object that can be included with an itype (an item in JSON, if you prefer) it has several members that it contains to give it various characteristics, and different objects that also have their own characteristics.  Here is an example of a relic_data json object (which would be inside an item json object in order to make that item a relic).

```json
"relic_data": {
  "passive_effects": [
    {
      "has": "WIELD",
      "condition": "ALWAYS",
      "hit_you_effect": [ { "id": "AEA_FIREBALL" } ],
      "hit_me_effect": [ { "id": "AEA_HEAL" } ],
      "values": [ { "value": "STRENGTH", "multiply": 1.1, "add": -5 } ],
      "intermittent_activation": {
        "effects": [ 
          {
            "frequency": "1 hour",
            "spell_effects": [
              { "id": "AEA_ADRENALINE" }
            ]
        ]
      }
    }
  ],
  "active_effects": [
    { "id": "AEA_DIM" }
  ]
  "moves": 200,
  "charges_per_activation": 2
}
```

### passive_effects (a.k.a. enchantments)

relics have a list of passive_effects, which is a list of enchantments.  Enchantments can be defined as their own object for artifact procgen purposes, but when assigning enchantments to a relic you need to define all of its parts.  This section will list the available values for each field.

##### "has"

The "has" is how an enchantment determines if it is in the right location in order to qualify for being active.

* "WIELD" - when wielded in your hand
* "WORN" - when worn as armor
* "HELD" - when in your inventory

##### "condition"

The "condition" is how an enchantment determines if you are in the right environments in order for the enchantment to qualify for being active.

* "ALWAYS" - Always and forevermore
* "UNDERGROUND" - When the owner of the item is below Z-level 0
* "UNDERWATER" - When the owner is in swimmable terrain

##### "hit_me_effect"

The "hit_me_effect" is a spell that activates when you are hit by a creature.  The spell is centered on your location.  Follows the template for defining "fake_spell"

##### "hit_you_effect"

The "hit_you_effect" is a spell that activates when you melee_attack a creature.  The spell is centered on the location of the creature unless self = true, then it is centered on your location.  Follows the template for defining "fake_spell"

##### "values"

"values" are anything that is a number that can be modified.  The id field is required, and "add" and "multiply" are optional.  A "multiply" value of -1 is -100% and a multiply of 2.5 is +250%.  Add is always before multiply.  The allowed values are as follows:

* STRENGTH
* DEXTERITY
* PERCEPTION
* INTELLIGENCE
* SPEED
* ATTACK_COST
* ATTACK_SPEED
* MOVE_COST
* METABOLISM
* MAX_MANA
* REGEN_MANA
* BIONIC_POWER
* MAX_STAMINA
* REGEN_STAMINA
* MAX_HP
* REGEN_HP
* THIRST
* FATIGUE
* PAIN
* BONUS_DODGE
* BONUS_BLOCK
* BONUS_DAMAGE
* ATTACK_NOISE
* SPELL_NOISE
* SHOUT_NOISE
* FOOTSTEP_NOISE
* SIGHT_RANGE
* CARRY_WEIGHT
* CARRY_VOLUME
* SOCIAL_LIE
* SOCIAL_PERSUADE
* SOCIAL_INTIMIDATE
* ARMOR_BASH
* ARMOR_CUT
* ARMOR_STAB
* ARMOR_HEAT
* ARMOR_COLD
* ARMOR_ELEC
* ARMOR_ACID
* ARMOR_BIO
effects for the item that has the enchantment
* ITEM_DAMAGE_BASH
* ITEM_DAMAGE_CUT
* ITEM_DAMAGE_STAB
* ITEM_DAMAGE_HEAT
* ITEM_DAMAGE_COLD
* ITEM_DAMAGE_ELEC
* ITEM_DAMAGE_ACID
* ITEM_DAMAGE_BIO
* ITEM_DAMAGE_AP
* ITEM_ARMOR_BASH
* ITEM_ARMOR_CUT
* ITEM_ARMOR_STAB
* ITEM_ARMOR_HEAT
* ITEM_ARMOR_COLD
* ITEM_ARMOR_ELEC
* ITEM_ARMOR_ACID
* ITEM_ARMOR_BIO
* ITEM_WEIGHT
* ITEM_ENCUMBRANCE
* ITEM_VOLUME
* ITEM_COVERAGE
* ITEM_ATTACK_SPEED
* ITEM_WET_PROTECTION

##### "intermittent_activation"

These are spells that activate centered on you depending on the duration.  The spells follow the "fake_spell" template.

##### "active_effects"

These are spells you cast when you activate the relic.  The amount of time to cast the spell is based on the "moves" of the artifact, and the charges_per_activation determine the charges used up when casting the spell.  The activation will use the spell targeting feature.
