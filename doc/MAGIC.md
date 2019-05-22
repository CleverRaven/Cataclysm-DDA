# How to add magic to a mod

### Spells

In \data\mods\Magiclysm there is a template spell, copied here for your perusal:

```C++
{
    // This spell exists in json as a template for contributors to see the possible values of the spell
	"id": "example_template",                                 // id of the spell, used internally. not translated
	"type": "SPELL",
	"name": "Template Spell",                                 // name of the spell that shows in game
	"description": "This is a template to show off all the available values",
	"valid_targets": [ "hostile", "ground", "self", "ally" ], // if a valid target is not included, you cannot cast the spell on that target.
	"effect": "shallow_pit",                                  // effects are coded in C++. A list will be provided below of possible effects that have been coded.
	"effect_str": "template"                                  // special. see below
	"effected_body_parts": [ "HEAD", "TORSO", "MOUTH", "EYES", "ARM_L", "ARM_R", "HAND_R", "HAND_L", "LEG_L", "FOOT_L", "FOOT_R" ], // body parts affected by effects
	"spell_class": "NONE"                                     // 
	"base_casting_time": 100,                                 // this is the casting time (in moves)
	"base_energy_cost": 10,                                   // the amount of energy (of the requisite type) to cast the spell
	"energy_source": "MANA",                                  // the type of energy used to cast the spell. types are: MANA, BIONIC, HP, STAMINA, NONE (none will not use mana)
	"difficulty": 12,                                         // the difficulty to learn/cast the spell
	"max_level": 10,                                          // maximum level you can achieve in the spell
	"min_damage": 0,                                          // minimum damage (or "starting" damage)
	"max_damage": 100,                                        // maximum damage the spell can achieve
	"damage_increment": 2.5,                                  // to get damage (and any of the other below stats) multiply this by spell's level and add to minimum damage
	"min_aoe": 0,                                             // area of effect (currently not implemented)
	"max_aoe": 5,
	"aoe_increment": 0.1,
	"min_range": 1,                                           // range of the spell
	"max_range": 10,
	"range_increment": 2,
	"min_dot": 0,                                             // damage over time (currently not implemented)
	"max_dot": 2,
	"dot_increment": 0.1,
	"min_duration": 0,                                        // duration of spell effect (if the spell has a special effect)
	"max_duration": 1000,
	"duration_increment": 4,
	"min_pierce": 0,                                          // how much of the spell pierces armor (currently not implemented)
	"max_pierce": 1,
	"pierce_increment": 0.1
  }
```
Most of the default values for the above are either 0 or "NONE", so you may leave out most of the values if they do not pertain to your spell.

When deciding values for some of these, it is important to note that some of the formulae are not linear.
For example, this is the formula for spell failure chance: 

```( ( ( ( spell_level - spell_difficulty ) * 2 + intelligence + spellcraft_skill ) - 30 ) / 30 ) ^ 2```

Meaning a spell with difficulty 0 cast by a player with 8 intelligence, 0 spellcraft, and level 0 in the spell will have a 53% spell failure chance.
On the other hand, a player with 12 intelligence, 6 spellcraft, and level 6 in the same spell will have a 0% spell failure chance.

However, experience gain is a little more complicated to calculate.  The formula for how much experience you need to get to a level is below:

```e ^ ( ( level + 62.5 ) * 0.146661 ) ) - 6200```

#### Currently Implemented Effects and special rules

"pain_split" - makes all of your limbs' damage even out.

"move_earth" - "digs" at the target location. some terrain is not diggable this way.

"target_attack" - deals damage to a target (ignores walls).  If "effect_str" is included, it will add that effect (defined elsewhere in json) to the targets if able, to the body parts defined in effected_body_parts.
Any aoe will manifest as a circular area centered on the target, and will only deal damage to valid_targets. (aoe does not ignore walls)

"projectile_attack" - similar to target_attack, except the projectile you shoot will stop short at impassable terrain.  If "effect_str" is included, it will add that effect (defined elsewhere in json) to the targets if able, to the body parts defined in effected_body_parts.

"cone_attack" - fires a cone toward the target up to your range.  The arc of the cone in degrees is aoe.  Stops at walls.  If "effect_str" is included, it will add that effect (defined elsewhere in json) to the targets if able, to the body parts defined in effected_body_parts.

"line_attack" - fires a line with width aoe toward the target, being blocked by walls on the way.  If "effect_str" is included, it will add that effect (defined elsewhere in json) to the targets if able, to the body parts defined in effected_body_parts.

"spawn_item" - spawns an item that will disappear at the end of its duration.  Default duration is 0.

"teleport_random" - teleports the player randomly range spaces with aoe variation

#####
For Spells that have an attack type, these are the available damage types:
"fire"
"acid"
"bash"
"bio" - internal damage such as poison
"cold"
"cut"
"electric"
"stab"
"none" - this damage type goes through armor altogether. it is the default.

#### Learning Spells

Currently there is only one way of learning spells that is implemented: learning a spell from an item, through a use_action.  An example is shown below:
```C++
{
"id": "DEBUG_spellbook",
"type": "GENERIC",
"name": "A Technomancer's Guide to Debugging C:DDA",
"description": "static std::string description( spell sp ) const;",
"weight": 1,
"volume": "1 ml",
"symbol": "?",
"color": "magenta",
"use_action": {
  "type": "learn_spell",
  "spells": [ "debug_hp", "debug_stamina", "example_template", "debug_bionic", "pain_split", "fireball" ] // this is a list of spells you can learn from the item
}
},
```
You can study this spellbook for a rate of ~1 experience per turn depending on intelligence, spellcraft, and focus.