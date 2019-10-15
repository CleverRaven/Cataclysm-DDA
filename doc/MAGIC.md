# How to add magic to a mod

### Spells

In `data/mods/Magiclysm` there is a template spell, copied here for your perusal:

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
	"extra_effects": [ { "id": "fireball", "hit_self": false, "max_level": 3 } ],	// this allows you to cast multiple spells with only one spell
	"effected_body_parts": [ "HEAD", "TORSO", "MOUTH", "EYES", "ARM_L", "ARM_R", "HAND_R", "HAND_L", "LEG_L", "FOOT_L", "FOOT_R" ], // body parts affected by effects
	"spell_class": "NONE"                                     // 
	"base_casting_time": 100,                                 // this is the casting time (in moves)
	"base_energy_cost": 10,                                   // the amount of energy (of the requisite type) to cast the spell
	"energy_source": "MANA",                                  // the type of energy used to cast the spell. types are: MANA, BIONIC, HP, STAMINA, FATIGUE, NONE (none will not use mana)
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
	"pierce_increment": 0.1,
	"field_id": "fd_blood",                                   // the string id of the field (currently hardcoded)
	"field_chance": 100,                                      // one_in( field_chance ) chance of spawning a field per tile in aoe
	"min_field_intensity": 10,                                // field intensity of fields generated
	"max_field_intensity": 10,
	"field_intensity_increment": 1,
	"field_intensity_variance": 0.1                           // the field can range in intensity from -variance as a percent to +variance as a percent i.e. this spell would be 9-11
	"sound_type": "combat",                                   // the type of sound. possible types are: background, weather, music, movement, speech, activity, destructive_activity, alarm, combat, alert, order
	"sound_description": "a whoosh",                          // the sound description. in the form of "You hear %s" by default it is "an explosion"
	"sound_ambient": true,                                    // whether or not this is treated as an ambient sound or not
	"sound_id": "misc",                                       // the sound id
	"sound_variant": "shockwave"                              // the sound variant
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

* "pain_split" - makes all of your limbs' damage even out.

* "move_earth" - "digs" at the target location. some terrain is not diggable this way.

* "target_attack" - deals damage to a target (ignores walls).  If "effect_str" is included, it will add that effect (defined elsewhere in json) to the targets if able, to the body parts defined in effected_body_parts.
Any aoe will manifest as a circular area centered on the target, and will only deal damage to valid_targets. (aoe does not ignore walls)

* "projectile_attack" - similar to target_attack, except the projectile you shoot will stop short at impassable terrain.  If "effect_str" is included, it will add that effect (defined elsewhere in json) to the targets if able, to the body parts defined in effected_body_parts.

* "cone_attack" - fires a cone toward the target up to your range.  The arc of the cone in degrees is aoe.  Stops at walls.  If "effect_str" is included, it will add that effect (defined elsewhere in json) to the targets if able, to the body parts defined in effected_body_parts.

* "line_attack" - fires a line with width aoe toward the target, being blocked by walls on the way.  If "effect_str" is included, it will add that effect (defined elsewhere in json) to the targets if able, to the body parts defined in effected_body_parts.

* "spawn_item" - spawns an item that will disappear at the end of its duration.  Default duration is 0.

* "teleport_random" - teleports the player randomly range spaces with aoe variation

* "recover_energy" - recovers an energy source (defined in the effect_str, shown below) equal to damage of the spell
- "MANA"
- "STAMINA"
- "FATIGUE"
- "PAIN"
- "BIONIC"

* "ter_transform" - transform the terrain and furniture in an area centered at the target.  The chance of any one of the points in the area of effect changing is one_in( damage ).  The effect_str is the id of a ter_furn_transform.

* "vomit" - any creature within its area of effect will instantly vomit, if it's able to do so.

* "timed_event" - adds a timed event to the player only. valid timed events: "help", "wanted", "robot_attack", "spawn_wyrms", "amigara", "roots_die", "temple_open", "temple_flood", "temple_spawn", "dim", "artifact_light" NOTE: This was added only for artifact active effects. support is limited, use at your own risk.

* "explosion" - an explosion is centered on the target, with power damage() and factor aoe()/10

* "flashbang" - a flashbang effect is centered on the target, with poewr damage() and factor aoe()/10

* "mod_moves" - adds damage() moves to the target. can be negative to "freeze" the target for that amount of time

* "map" - maps the overmap centered on the player out to a radius of aoe()

* "morale" - gives a morale effect to all npcs or avatar within aoe, with value damage(). decay_start is duration() / 10.

* "charm_monster" - charms a monster that has less hp than damage() for approximately duration()

* "mutate" - mutates the target(s). if effect_str is defined, mutates toward that category instead of picking at random. the "MUTATE_TRAIT" flag allows effect_str to be a specific trait instead of a category. damage() / 100 is the percent chance the mutation will be successful (a value of 10000 represents 100.00%)

* "bash" - bashes the terrain at the target. uses damage() as the strength of the bash.

* "WONDER" - Unlike the above, this is not an "effect" but a "flag".  This alters the behavior of the parent spell drastically: The spell itself doesn't cast, but its damage and range information is used in order to cast the extra_effects.  N of the extra_effects will be chosen at random to be cast, where N is the current damage of the spell (stacks with RANDOM_DAMAGE flag) and the message of the spell cast by this spell will also be displayed.  If this spell's message is not wanted to be displayed, make sure the message is an empty string.

* "RANDOM_TARGET" - A special spell flag (like wonder) that forces the spell to choose a random valid target within range instead of the caster choosing the target. This also affects extra_effects.

##### For Spells that have an attack type, these are the available damage types:
* "fire"
* "acid"
* "bash"
* "bio" - internal damage such as poison
* "cold"
* "cut"
* "electric"
* "stab"
* "none" - this damage type goes through armor altogether. it is the default.

#### Spells that level up

Spells that change effects as they level up must have a min and max effect and an increment. The min effect is what the spell will do at level 0, and the max effect is where it stops growing.  The increment is how much it changes per level. For example:

```json
"min_range": 1,
"max_range": 25,
"range_increment": 5,
```

Min and max values must always have the same sign, but it can be negative eg. in the case of spells that use a negative 'recover' effect to cause pain or stamina damage. For example:

```json
  {
    "id": "stamina_damage",
    "type": "SPELL",
    "name": "Tired",
    "description": "decreases stamina",
    "valid_targets": [ "hostile" ],
    "min_damage": -2000,
    "max_damage": -10000,
    "damage_increment": -3000,
    "max_level": 10,
    "effect": "recover_energy",
    "effect_str": "STAMINA"
  }
```

### Learning Spells

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


#### Spells in professions

You can add a "spell" member to professions like so:
```json
"spells": [ { "id": "summon_zombie", "level": 0 }, { "id": "magic_missile", "level": 10 } ]
```
NOTE: This makes it possible to learn spells that conflict with a class. It also does not give the prompt to gain the class. Be judicious upon adding this to a profession!


#### Monsters

You can assign a spell as a special attack for a monster.
```json
{ "type": "spell", "spell_id": "burning_hands", "spell_level": 10, "cooldown": 10 }
```
* spell_id: the id for the spell being cast.
* spell_level: the level at which the spell is cast. Spells cast by monsters do not gain levels like player spells.
* cooldown: how often the monster can cast this spell

