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
    "effect_str": "template",                                 // special. see below
    "effect_on_conditions": ["template"],                     // special. see below
    "shape": "blast",                                         // the "shape" of the spell's area of effect. uses the aoe stat
    "extra_effects": [ { "id": "fireball", "hit_self": false, "max_level": 3 } ],	// this allows you to cast multiple spells with only one spell
    "affected_body_parts": [ "head", "torso", "mouth", "eyes", "arm_l", "arm_r", "hand_r", "hand_l", "leg_l", "foot_l", "foot_r" ], // body parts affected by effects
    "flags": [ "SILENT", "LOUD", "SOMATIC", "VERBAL", "NO_HANDS", "NO_LEGS", "SPAWN_GROUP" ], // see "Spell Flags" below
    "spell_class": "NONE",                                    //
    "base_casting_time": 1000,                                // this is the casting time (in moves)
    "final_casting_time": 100,
    "casting_time_increment": -50,
    "base_energy_cost": 30,                                  // the amount of energy (of the requisite type) to cast the spell
    "final_energy_cost": 100,
    "energy_increment": -6,
    "energy_source": "MANA",                                  // the type of energy used to cast the spell. types are: MANA, BIONIC, HP, STAMINA, NONE (none will not use mana)
    "components": [requirement_id]                            // an id from a requirement, like the ones you use for crafting. spell components require to cast.
    "difficulty": 12,                                         // the difficulty to learn/cast the spell
    "max_level": 10,                                          // maximum level you can achieve in the spell
    "min_accuracy" -20,                                       // the accuracy bonus of the spell. around -15 and it gets blocked all the time
    "max_accuracy": 20,                                       // around 20 accuracy and it's basically impossible to block
    "accuracy_increment": 1.5
    "min_damage": 0,                                          // minimum damage (or "starting" damage)
    "max_damage": 100,                                        // maximum damage the spell can achieve
    "damage_increment": 2.5,                                  // to get damage (and any of the other below stats) multiply this by spell's level and add to minimum damage
    "min_aoe": 0,                                             // area of effect, or range of variance
    "max_aoe": 5,
    "aoe_increment": 0.1,
    "min_range": 1,                                           // range of the spell
    "max_range": 10,
    "range_increment": 2,
    "min_dot": 0,                                             // damage over time
    "max_dot": 2,
    "dot_increment": 0.1,
    "min_duration": 0,                                        // duration of spell effect in moves (if the spell has a special effect)
    "max_duration": 1000,
    "duration_increment": 4,
    "min_pierce": 0,                                          // how much of the spell pierces armor
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
    "sound_variant": "shockwave",                             // the sound variant
    "learn_spells": { "create_atomic_light": 5, "megablast": 10 }   // the caster will learn these spells when the current spell reaches the specified level. should be a map of spell_type_id and the level at which the new spell is learned.
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

### Spell effects and rules

The value of the `"effect"` string in the spell's JSON data says what effect the spell has. For example, the Magus spell "Magic Missile" has a `target_attack` effect, meaning it deals damage to a specific target:

```json
{
  "id": "magic_missile",
  "effect": "attack",
  "min_damage": 1
}
```

while the Druid spell "Nature's Bow" has a `spawn_item` effect, and requires the name of the item to spawn:

```json
{
  "id": "druid_naturebow1",
  "effect": "spawn_item",
  "effect_str": "druid_recurve"
}
```

Below is a table of currently implemented effects, along with special rules for how they work:

| Effect                   | Description
|---                       |---
| `area_pull` | Pulls `valid_targets` in its aoe toward the target location.  Currently, the pull distance is set to 1 (see `directed_push`).
| `area_push` | Pushes `valid_targets` in its aoe away from the target location.  Currently, the push distance is set to 1 (see `directed_push`).
| `attack` | Causes damage to `valid_targets` in its aoe, and applies `effect_str` named effect to targets.  To damage terrain use `bash`.
| `banishment` | Kills any `MONSTER` in the aoe up to damage hp.  Any overflow hp is taken from the caster; if it's more than the caster's hp the spell fails.
| `bash` | Bashes the terrain at the target.  Uses damage() as the strength of the bash.
| `charm_monster` | Charms a monster that has less hp than damage() for approximately duration().
| `dash` | Dashes forward up to range and hits targets in a cone at the target.
| `directed_push` | Pushes `valid_targets` in aoe away from the target location, with a distance of damage().  Negative values pull instead.
| `effect_on_condition` | Runs the `effect_on_condition` from `effect_str` on all valid targets.  The EOC will be centered on the player, with the NPC as caster.
| `emit` | Causes an `emit` at the target.
| `explosion` | Causes an explosion centered on the target.  Uses damage() for power and factor aoe()/10.
| `flashbang` | Causes a flashbang effect is centered on the target.  Uses damage() for power and factor aoe()/10.
| `fungalize` | Fungalizes the target.
| `guilt` | Target gets the guilt morale as if it killed the caster.
| `map` | Maps out the overmap centered on the player, to a radius of aoe().
| `mod_moves` | Adds damage() moves to the targets.  Negative values "freeze" for that amount of time.
| `morale` | Gives a morale effect to NPCs or the avatar within the aoe.  Uses damage() for the value.  `decay_start` is duration() / 10.
| `mutate` | Mutates the targets.  If `effect_str` is defined, mutates toward that category instead of picking at random.  If the `MUTATE_TRAIT` flag is used, allows `effect_str` to be a specific trait.  Damage() / 100 is the percent chance the mutation will be successful (10000 represents 100.00%).
| `pain_split` | Evens out all of your limbs' damage.
| `pull_target` | Attempts to pull the target towards the caster in a straight line.  If the path is blocked by impassable furniture or terrain, the effect fails.
| `recover_energy` | Recovers an energy source equal to damage of the spell.  The energy source is defined in `effect_str` and may be one of `BIONIC`, `FATIGUE`, `PAIN`, `MANA` or `STAMINA`.
| `remove_effect` | Removes `effect_str` effects from all creatures in aoe.
| `remove_field` | Removes a `effect_str` field in aoe.  Causes teleglow of varying intensity and potentially teleportation depending on field density, if the field removed is `fd_fatigue`.
| `revive` | Revives a monster like a zombie necromancer.  The monster must have the `REVIVES` flag.
| `short_range_teleport` | Teleports the player randomly range spaces with aoe variation.  See also the `TARGET_TELEPORT` and `UNSAFE_TELEPORT` flags.
| `spawn_item` | Spawns an item that will disappear at the end of its duration.  Default duration is 0.
| `summon` | Summons a `MONSTER` or `monstergroup` from `effect_str` that will disappear at the end of its duration.  Default duration is 0.  See also the `SPAWN_WITH_DEATH_DROPS` flag.
| `summon_vehicle` | Summons a `vehicle` from `effect_str` that will disappear at the end of its duration.  Default duration is 0.
| `targeted_polymorph` | A targeted monster is permanently transformed into the `MONSTER` specified by `effect_str`, if it has less HP than the spell's damage.  If `effect_str` is left empty, the target will transform into a random monster with a similar difficulty rating.  Alternatively, the `POLYMORPH_GROUP` flag can be used to pick a weighted ID from a `monstergroup`.  The player and NPCs are immune to this spell effect.
| `ter_transform` | Transforms the terrain and furniture in its aoe.  The chance of any one of the points in the aoe changing is 1 / (damage).  The `effect_str` is the ID of a `ter_furn_transform`.
| `timed_event` | Adds a timed event to the player only.  Valid timed events are: `amigara`, `artifact_light`, `dim`, `help`, `robot_attack`, `roots_die`, `spawn_wyrms`, `temple_flood`, `temple_open`, `temple_spawn`, `wanted`.  **NOTE**: This was added only for artifact active effects.  Support is limited, use at your own risk.
| `translocate` | Opens up a window that allows the caster to choose a translocation gate to teleport to.
| `upgrade` | Immediately upgrades a target `MONSTER`.
| `vomit` | Any creature within its aoe will instantly vomit, if it's able to do so.


Another mandatory member is spell "shape". This dictates how the area of effect works.

| Shape | Description
| --    | --
| `blast` | A circular blast centered on the impact position.  Aoe value is the radius.
| `cone` | Fires a cone with an arc equal to aoe in degrees.
| `line` | Fires a line with a width equal to the aoe.


### Spell Flags

Flags allow you to provide additional customizations for spell effects, behavior, and limitations.
Spells may have any number of flags, for example:

```json
 {
    "id": "bless",
    "//": "Encumbrance on the mouth (verbal) or arms (somatic) affect casting success, but not legs.",
    "flags": [ "VERBAL", "SOMATIC", "NO_LEGS" ]
 }
```

| Flag | Description
| ---  | ---
| `CONCENTRATE` | Focus affects spell fail %.
| `EXTRA_EFFECTS_FIRST` | The spell's `extra_effects` will happen before the main spell effect.
| `FRIENDLY_POLY` | The target of a `targeted_polymorph` spell will become friendly to the caster if the spell resolves successfully.
| `HOSTILE_SUMMON` | Summon spell always spawns a hostile monster.
| `HOSTILE_50` | Summoned monster spawns friendly 50% of the time.
| `IGNORE_WALLS` | Spell's aoe goes through walls.
| `LOUD` | Spell makes extra noise at target.
| `MUTATE_TRAIT` | Overrides the `mutate` spell effect to use a specific trait_id instead of a category.
| `NO_EXPLOSION_SFX` | The spell will not generate a visual explosion effect.
| `NO_HANDS` | Hands do not affect spell energy cost.
| `NO_LEGS` | Legs do not affect casting time.
| `NO_PROJECTILE` | The "projectile" portion of the spell phases through walls, the epicenter of the spell effect is exactly where you target it, with no regards to obstacles.
| `NON_MAGICAL` | Ignores spell resistance when calculating damage mitigation.
| `PAIN_NORESIST` | Pain altering spells can't be resisted (like with the deadened trait).
| `PERMANENT` | Items or creatures spawned with this spell do not disappear and die as normal.  Items can only be permanent at maximum spell level; creatures can be permanent at any spell level.
| `PERMANENT_ALL_LEVELS` | Items spawned with this spell do not disappear even if the spell is not max level.
| `POLYMORPH_GROUP` | A `targeted_polymorph` spell will transform the target into a random monster from the `monstergroup` in `effect_str`.
| `RANDOM_AOE` | Picks random number between (min + increment)*level and max instead of normal behavior.
| `RANDOM_DAMAGE` | Picks random number between (min + increment) * level and max instead of normal behavior.
| `RANDOM_DURATION` | Picks random number between (min + increment) * level and max instead of normal behavior.
| `RANDOM_TARGET` | Forces the spell to choose a random valid target within range instead of the caster choosing the target.  This also affects `extra_effects`.
| `SILENT` | Spell makes no noise at target.
| `SOMATIC` | Arm encumbrance affects fail % and casting time (slightly).
| `SPAWN_GROUP` | Spawn or summon from an `item_group` or `monstergroup`, instead of the specific IDs.
| `SPAWN_WITH_DEATH_DROPS` | Allows summoned monsters to retain their usual death drops, otherwise they drop nothing.
| `SWAP_POS` | A projectile spell swaps the positions of the caster and target.
| `TARGET_TELEPORT` | Teleport spell changes to maximum range target with aoe as variation around target.
| `UNSAFE_TELEPORT` | Teleport spell risks killing the caster or others.
| `VERBAL` | Spell makes noise at caster location, mouth encumbrance affects fail %.
| `WITH_CONTAINER` | Items spawned with container.
| `WONDER` | This drastically alters the behavior of the parent spell: The spell itself doesn't cast, but its damage and range information is used in order to cast the `extra_effects`.  N of the `extra_effects` will be chosen to be cast at random, where N is the current damage of the spell (stacks with the `RANDOM_DAMAGE` flag) and the message of the spell cast by this spell will also be displayed.  If this spell's message is not wanted, make sure `message` is an empty string.


### Damage Types

For spells that have an attack type, these are the available damage types:

* "acid"
* "bash"
* "biological" - internal damage such as poison
* "cold"
* "cut"
* "electric"
* "heat"
* "pure" - this damage type goes through armor altogether. it is the default.
* "stab"


### Spells that level up

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

Currently there multiple ways of learning spells that are implemented: learning a spell from an item(through a use_action), from spells that have the learn_spells property and from traits/mutations.  An example of an use item is shown below:

```json
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
    "//": "list of spells you can learn from the item",
    "spells": [ "debug_hp", "debug_stamina", "example_template", "debug_bionic", "pain_split", "fireball" ]
  }
},
```

You can study this spellbook for a rate of ~1 experience per turn depending on intelligence, spellcraft, and focus.

Below is an example of the learn_spells property:
```json
  {
    "id": "phase_door",
    "type": "SPELL",
    "name": "Phase Door",
    "description": "Teleports you in a random direction a short distance.",
    "effect": "short_range_teleport",
    "shape": "blast",
    "valid_targets": [ "none" ],
    "max_level": 10,
    "difficulty": 2,
    "spell_class": "MAGUS",
    "learn_spells": { "dimension_door": 10 }
  },
```
Traits/mutations have the spells_learned property, see the [JSON_INFO](JSON_INFO.md) documentation for details.


### Spells in professions and NPC classes

You can add a "spell" member to professions or an NPC class definition like so:

```json
"spells": [ { "id": "summon_zombie", "level": 0 }, { "id": "magic_missile", "level": 10 } ]
```

NOTE: This makes it possible to learn spells that conflict with a class. It also does not give the prompt to gain the class. Be judicious upon adding this to a profession!

### Spell examples

Below you can see the proper examples of monster spells - most common types and even some advanced ones:

Spell types:

1) Summon:
```
    {
    "type": "SPELL",
    "id": "test_summon",                                     // id of the spell, used internally. not translated
    "name": "Summon",                                        // name of the spell that shows in game
    "description": "Summons the creature specified in 'effect_str'",
    "flags": [ "SILENT", "HOSTILE_SUMMON" ],  // see "Spell Flags" in this document
    "valid_targets": [ "ground" ],                           // if a valid target is not included, you cannot cast the spell on that target.
    "min_damage": 1,                                         // minimum number of creatures summoned (or "starting" number of creatures summoned)
    "max_damage": 1,                                         // maximum number of creatures summoned the spell can achieve
    "min_aoe": 3,                                            // area of effect of the spell, in this case the area the summons can appear in
    "max_aoe": 3,
    "effect": "summon",                                      // effects are coded in C++. A list is provided in this document of possible effects that have been coded.
    "effect_str": "mon_test_monster",                        // varies, see table of implemented effects in this document
    "min_duration": 6250,                                    // duration of spell effect in moves (if the spell has a special effect)
    "max_duration": 6250
    }
  ```


2) Typical attack:
```
    {
    "id": "test_attack",                                     // id of the spell, used internally. not translated
    "type": "SPELL",
    "name": "Ranged Strike",                                 // name of the spell that shows in game
    "description": "Deals damage to the target with 100% accuracy. Will always apply the status effect specified in 'effect_str'.",
    "valid_targets": [ "ground", "hostile" ],                // if a valid target is not included, you cannot cast the spell on that target.
    "effect": "projectile_attack",                           // effects are coded in C++. A list is provided in this document of possible effects that have been coded.
    "effect_str": "stunned",                                 // varies, see table of implemented effects in this document
    "min_damage": 10,                                        // minimum damage (or "starting" damage)
    "max_damage": 20,                                        // maximum damage the spell can achieve
    "damage_increment": 1.0,                                 // How much damage increases per spell level increase
    "min_range": 4,                                          // range of the spell
    "max_range": 4,
    "base_casting_time": 500,                                // this is the casting time (in moves)
    "min_duration": 200,                                     // duration of spell effect in moves (if the spell has a special effect)
    "max_duration": 300,
    "duration_increment": 10,                                // How much longer the spell lasts per spell level
    "damage_type": "stab"                                    // type of damage
    }
  ```
  Note: Uses both `ground` and `hostile` in `valid_targets` so it can be targeted in an area with no line of sight.


3) Consecutively cast spells:
```
    {
    "id": "test_combo",                                      // id of the spell, used internally. not translated
    "type": "SPELL",
    "name": "Combo Strikes",                                 // name of the spell that shows in game
    "description": "Upon casting this spell, will also activate the spells specified on the 'extra_effects' in descending order.",
    "flags": [ "SILENT", "RANDOM_DAMAGE", "RANDOM_AOE" ],    // see "Spell Flags" in this document
    "valid_targets": [ "hostile", "ground" ],                // if a valid target is not included, you cannot cast the spell on that target.
    "effect": "projectile_attack",                           // effects are coded in C++. A list is provided in this document of possible effects that have been coded.
    "effect_str": "downed",                                  // varies, see table of implemented effects in this document
    "extra_effects": [ { "id": "test_atk1" }, { "id": "test_atk2" } ],               // this allows you to cast multiple spells with only one spell
    "min_damage": 7,                                         // minimum damage (or "starting" damage)
    "max_damage": 14,                                        // maximum  damage the spell can achieve
    "damage_increment": 0.7                                  // damage increase per spell level
    "min_aoe": 2,                                            // area of effect
    "max_aoe": 4,
    "aoe_increment": 0.2,                                    // how much wider the area of effect gets per spell level
    "min_range": 10,                                         // range of the spell
    "max_range": 10,
    "base_casting_time": 750,                                // this is the casting time (in moves)
    "min_duration": 325,                                     // duration of spell effect in moves (if the spell has a special effect)
    "max_duration": 325,
    "damage_type": "stab"                                    // type of damage
    }
  ```
  Explanation: If you put two or more spells in `extra_effects`, it will consecutively cast the spells: first `extra_effects`, second `extra_effects`, third `extra_effects`, etc.  If you wish to pick one at random, use the `WONDER` flag (see below).  Additionally, the extra spells will be cast at a level up to the level of the parent spell being cast, unless additional data is added to the fake_spell.


4) Randomly cast spells:
```
    {
    "id": "test_starter_spell",                              // id of the spell, used internally. not translated
    "type": "SPELL",
    "name": "Starter",                                       // name of the spell that shows in game
    "description": "Upon casting this spell, randomly selects one spell specified in 'extra_effects' to cast. The spell damage shows how many times it will randomly select from the list",
    "flags": [ "SILENT", "WONDER", "RANDOM_DAMAGE" ],        // see "Spell Flags" in this document
    "valid_targets": [ "hostile" ],                          // if a valid target is not included, you cannot cast the spell on that target.
    "effect": "projectile_attack",                           // effects are coded in C++. A list is provided in this document of possible effects that have been coded.
    "extra_effects": [                                       // this allows you to cast multiple spells with only one spell
      { "id": "test_atk1" },                                 // id of the spell, used internally. not translated
      { "id": "test_atk2" },                                 // id of the spell, used internally. not translated
      { "id": "test_atk3" },                                 // id of the spell, used internally. not translated
      { "id": "test_atk4" },                                 // id of the spell, used internally. not translated
      { "id": "test_atk5" },                                 // id of the spell, used internally. not translated
      { "id": "test_atk6" }                                  // id of the spell, used internally. not translated
    ],
    "min_damage": 3,                                         // minimum damage (or "starting" damage)
    "max_damage": 5,                                         // maximum damage the spell can achieve
    "damage_increment": 0.2                                  // damage increase per spell level
    "min_range": 10,                                         // range of the spell
    "max_range": 10
    }
  ```
  Explanation: The `WONDER` flag does wonders by turning the spell into a dice: When cast, the main spell will pick the subspells in `extra_effects`.  The amount of "rolls" is specified via `min_damage` and `max_damage` plus the `RANDOM_DAMAGE` flag.  In this example, casting `test_starter_spell` will cause any one of each `test_atk#` subspells to be selected, this repeats 3 to 5 times.  There must be a minimum of one spell in `extra_effects` for the `WONDER` flag to work properly.


5) Repeatedly cast the same spell:
```
    {
    "type": "SPELL",
    "id": "test_attack_repeat",                              // id of the spell, used internally. not translated
    "name": "a spell",                                       // name of the spell that shows in game
    "description": "Upon casting this spell it will repeat the spell specified in `extra_effects` - the amount of repetitions is the interval `min_damage`-`max_damage` ",
    "extra_effects": [ { "id": "test_attack" } ],            // this allows you to cast multiple spells with only one spell
    "flags": [ "SILENT", "WONDER", "RANDOM_DAMAGE" ],        // see "Spell Flags" in this document
    "valid_targets": [ "hostile" ],                          // if a valid target is not included, you cannot cast the spell on that target.
    "effect": "target_attack",                               // effects are coded in C++. A list is provided in this document of possible effects that have been coded.
    "effect_str": "target_message",                          // varies, see table of implemented effects in this document
    "min_damage": 5,                                         // minimum (starting damage)
    "max_damage": 7,                                         // maximum damage the spell can achieve
    "damage_increment": 0.2                                  // damage increase per spell level
    "min_range": 10,                                         // range of the spell
    "max_range": 10,
    "min_duration": 1,                                       // duration of spell effect in moves (if the spell has a special effect)
    "max_duration": 1
    }
  ```
  Explanation: Notice the `WONDER` and `RANDOM_DAMAGE` combo, with a different approach:  `min_damage` set to 5 and `max_damage` set to 7 will cause the main spell to "roll" `extra_effects` 5 - 7 times, but this time there's a single `test_attack`.  Because `WONDER` has a 100% chance of picking a spell, `test_attack`, will be repeated 5 to 7 times.


6) A spell that casts a note on a target and an effect on itself:
```
    {
    "id": "test_attack_note",                                // id of the spell, used internally. not translated
    "type": "SPELL",
    "name": "a note",                                        // name of the spell that shows in game
    "description": "This spell applies a harmless status effect to notify the player about the spell that the user has cast.",
    "flags": [ "SILENT" ],                                   // see "Spell Flags" in this document
    "valid_targets": [ "hostile" ],                          // if a valid target is not included, you cannot cast the spell on that target.
    "effect": "target_attack",                               // effects are coded in C++. A list is provided in this document of possible effects that have been coded.
    "extra_effects": [ { "id": "sacrifice_spell", "hit_self": true }, { "id": "test_attack" } ],     // this allows you to cast multiple spells with only one spell
    "effect_str": "eff_test_note",                           // varies, see table of implemented effects in this document
    "min_aoe": 6,                                            // area of effect, or range of variance
    "max_aoe": 6,
    "min_duration": 1,                                       // duration of spell effect in moves (if the spell has a special effect)
    "max_duration": 1
    }
  ```
  Explanation: Here we have one main spell with two subspells: one on the caster and the other on the target.  To do this, you must specify the `id` of whatever spells you're using with the `extra_effects` field, in this case `sacrifice_spell` is stated with `"hit_self": true` and will hit the caster, while the second spell will be cast as normal.
  This is only necessary if we need an effect that is cast on a target and a second effect that is cast on the caster.


#### Monsters

Monster creatures can also cast spells.  To do this, you need to assign the spells in `special_attacks`.  Spells with `target_self: true` will only target the monster itself, and will be casted only if the monster has a hostile target.

```json 
    { 
    "type": "spell", 
    "spell_data": { "id": "cone_cold", "min_level": 4 }, 
    "monster_message": "%1$s casts %2$s at %3$s!", 
    "cooldown": 25 
    }
```
| Identifier              | Description
|---                      |---
| `spell_data`            | List of spell properties for the attack.
| `min_level`             | The level at which the spell is cast. Spells cast by monsters do not gain levels like player spells.
| `cooldown `             | How often the monster can cast this spell
| `monster_message`       | Message to print when the spell is cast, replacing the `message` in the spell definition. Dynamic fields correspond to `<Monster Display Name> / <Spell Name> / <Target name>`.
| `forbidden_effects_any` | Array of effect ids, if the monster has any one the attack can't trigger.
| `forbidden_effects_all` | Array of effect ids, if the monster has every effect the attack can't trigger.
| `required_effects_any`  | Array of effect ids, the monster needs any one for the attack to trigger.
| `required_effects_all`  | Array of effect ids, the monster needs every effect for the attack to trigger.
| `allow_no_target`       | Bool, default false. If true the monster will cast it even without a hostile target.

### Enchantments
| Identifier                  | Description
|---                          |---
| `id`                        | Unique ID. Must be one continuous word, use underscores if necessary.
| `has`                       | How an enchantment determines if it is in the right location in order to qualify for being active. "WIELD" - when wielded in your hand * "WORN" - when worn as armor * "HELD" - when in your inventory
| `condition`                 | How an enchantment determines if you are in the right environments in order for the enchantment to qualify for being active. * "ALWAYS" - Always and forevermore * "DIALOG_CONDITION" - ACTIVE whenever the dialog condition in `condition` is true * "ACTIVE" - whenever the item, mutation, bionic, or whatever the enchantment is attached to is active. * "INACTIVE" - whenever the item, mutation, bionic, or whatever the enchantment is attached to is inactive.
| `hit_you_effect`            | A spell that activates when you melee_attack a creature.  The spell is centered on the location of the creature unless self = true, then it is centered on your location.  Follows the template for defining "fake_spell"
| `hit_me_effect`             | A spell that activates when you are hit by a creature.  The spell is centered on your location.  Follows the template for defining "fake_spell"
| `intermittent_activation`   | Spells that activate centered on you depending on the duration.  The spells follow the "fake_spell" template.
| `values`                    | Anything that is a number that can be modified.  The id field is required, and "add" and "multiply" are optional.  A "multiply" value of -1 is -100% and a multiply of 2.5 is +250%.  Add is always before multiply. See allowed id below.


```json
  {
    "type": "enchantment",
    "id": "MEP_INK_GLAND_SPRAY",
    "hit_me_effect": [
      {
        "id": "generic_blinding_spray_1",
        "hit_self": false,
        "once_in": 15,
        "message": "Your ink glands spray some ink into %2$s's eyes.",
        "npc_message": "%1$s's ink glands spay some ink into %2$s's eyes."
      }
    ]
  },
  {
    "type": "enchantment",
    "id": "ENCH_INVISIBILITY",
    "condition": "ALWAYS",
    "ench_effects": [ { "effect": "invisibility", "intensity": 1 } ],
    "has": "WIELD",
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
        }
      ]
    }
  }

```

To add the enchantment to the item, you need to write the id of your enchantment inside `relic_data` field. For example:

```json
"relic_data": { "passive_effects": [ { "id": "ench_fishform" } ] },

```

If your enchantment is relatively small, you can write it right in the thing json, using the same syntaxys as the common enchantment. For example:

```json
"relic_data": {
  "passive_effects": [
    {
      "has": "WORN",
      "condition": "ALWAYS",
      "values": [
        { "value": "ARMOR_CUT", "add": -4 },
        { "value": "ARMOR_BASH", "add": -4 },
        { "value": "ARMOR_STAB", "add": -4 },
        { "value": "ARMOR_BULLET", "add": -2 }
      ]    
    }
  ]
}
```


### Allowed id for values

The allowed values are as follows:

Effects for the character that has the enchantment:

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
* HUNGER
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
* WEAPON_DISPERSION
* SOCIAL_LIE
* SOCIAL_PERSUADE
* SOCIAL_INTIMIDATE
* SLEEPY : The higher this is the more easily you fall asleep.
* LUMINATION : The character produces light
* EFFECTIVE_HEALTH_MOD : If this is anything other than zero(which it defaults to) you will use it instead of your actual health mod
* MOD_HEALTH : If this is anything other than zero(which it defaults to) you will to mod your health to a max/min of MOD_HEALTH_CAP every half hour
* MOD_HEALTH_CAP : If this is anything other than zero(which it defaults to) you will cap your MOD_HEALTH gain/loss at this every half hour
* MAP_MEMORY : How many map tiles you can remember.
* READING_EXP : Changes the minimum you learn from each reading increment.
* SKILL_RUST_RESIST : Chance out of 100 to resist skill rust.
* LEARNING_FOCUS : Amount of bonus focus you have for learning purposes.
* ARMOR_BASH
* ARMOR_CUT
* ARMOR_STAB
* ARMOR_HEAT
* ARMOR_COLD
* ARMOR_ELEC
* ARMOR_ACID
* ARMOR_BIO

Effects for the item that has the enchantment:

* ITEM_DAMAGE_PURE
* ITEM_DAMAGE_BASH
* ITEM_DAMAGE_CUT
* ITEM_DAMAGE_STAB
* ITEM_DAMAGE_HEAT
* ITEM_DAMAGE_COLD
* ITEM_DAMAGE_ELEC
* ITEM_DAMAGE_ACID
* ITEM_DAMAGE_BIO

The damage enchantment values are for melee only.

* ITEM_DAMAGE_AP
* ITEM_DAMAGE_PURE
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

Examples
    { "value": "ARMOR_ELEC", "add": -20 } subtracts 20 points of electrical damage
    { "value": "ATTACK_SPEED", "add": -60 } subtracts 60 moves from attacking making the attack faster
    { "value": "ARMOR_COLD", "multiply": -0.4 } subtracts 40 percent of the any cold damage
    { "value": "ARMOR_HEAT", "multiply": 0.4 } increases damage taken from fire by 40 percent
    { "value": "ARMOR_CUT", "add": 2 } increases cut damage taken by 2
    { "value": "ARMOR_BIO", "multiply": -1.4 } subtracts 140 percent of the any bio damage giving 40% of damage dealt as increased health
    { "value": "ARMOR_ACID", "multiply": 1.4 } increases damage taken from acid by 140 percent
