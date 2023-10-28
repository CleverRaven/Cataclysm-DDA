# Magic, Spells, and Enchantments

- [How to add magic to a mod](#how-to-add-magic-to-a-mod)
  - [Spells](#spells)
  - [The template spell](#the-template-spell)
  - [Mandatory fields](#mandatory-fields)
    - [Spell effects](#spell-effects)
    - [Spell shape](#spell-shape)
  - [Common fields](#common-fields)
    - [Spell Flags](#spell-flags)
    - [Damage Types](#damage-types)
    - [Spell level](#spell-level)
    - [Learning Spells](#learning-spells)
    - [Extra spell effects](#extra-spell-effects)
  - [Adding spells to professions and NPCs](#adding-spells-to-professions-and-npcs)
  - [Examples](#examples)
    - [Summon spell](#summon-spell)
    - [Typical attack](#typical-attack)
    - [Consecutive spell casting](#consecutive-spell-casting)
    - [Random spell casting](#random-spell-casting)
    - [Repeatedly cast the same spell](#repeatedly-cast-the-same-spell)
    - [A spell that casts a note on the target and an effect on the caster](#a-spell-that-casts-a-note-on-the-target-and-an-effect-on-the-caster)
    - [Monster spells](#monster-spells)
  - [Enchantments](#enchantments)
    - [The `relic_data` field](#the-relic_data-field)
    - [Variables](#variables)
    - [ID values](#id-values)
    - [Enchantment value examples](#enchantment-value-examples)


## Spells

Spells in Cataclysm: Dark Days Ahead consist in actions performed by a character or item, that result on a target or targets receiving an event.

This can be anything from the humble fireball spell or the simple heal, to granting states, mutations, summoning items and vehicles, spawning monsters, exploding monsters, applying auras, crowd-controlling, blinking, transforming terrain, adding or subtracting stats, granting or removing `effect_type`s, and more.

Remarkably, some things that don't seem quite "magical" at first glance may also be handled by spells, like casting Fist, casting Gun, casting Vomit, granting (or removing) `drunk`enness, handling fields (`field_id`s), transforming items, among other things.

By making clever use of JSON fields, interactions, combinations and descriptions, anyone can go wild in spell crafting.  Different spells from the official mods can be used as reference.

## The template spell

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
    "shape": "blast",                                         // the "shape" of the spell's area of effect. uses the aoe stat
    "extra_effects": [ { "id": "fireball", "hit_self": false, "max_level": 3 } ],	// this allows you to cast multiple spells with only one spell
    "affected_body_parts": [ "head", "torso", "mouth", "eyes", "arm_l", "arm_r", "hand_r", "hand_l", "leg_l", "foot_l", "foot_r" ], // body parts affected by effects
    "flags": [ "SILENT", "LOUD", "SOMATIC", "VERBAL", "NO_HANDS", "NO_LEGS", "SPAWN_GROUP" ], // see "Spell Flags" below
    "spell_class": "NONE",                                    //
    "base_casting_time": 1000,                                // this is the casting time (in moves)
    "final_casting_time": 100,
    "casting_time_increment": -50,
    "base_energy_cost": 30,                                   // the amount of energy (of the requisite type) to cast the spell
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
The template spell above shows every JSON field that spells can have.  Most of these values can be set at 0 or "NONE", so you may leave out most of these fields if they do not pertain to your spell.

When deciding values for some of these, it is important to note that some of the formulae are not linear.
For example, this is the formula for spell failure chance:

```( ( ( ( spell_level - spell_difficulty ) * 2 + intelligence + spellcraft_skill ) - 30 ) / 30 ) ^ 2```

Meaning a spell with difficulty 0 cast by a player with 8 intelligence, 0 spellcraft, and level 0 in the spell will have a 53% spell failure chance.
On the other hand, a player with 12 intelligence, 6 spellcraft, and level 6 in the same spell will have a 0% spell failure chance.

However, experience gain is a little more complicated to calculate.  The formula for how much experience you need to get to a level is below:

```e ^ ( ( level + 62.5 ) * 0.146661 ) ) - 6200```

## Mandatory fields

As noted above, few JSON fields are actually required for spells to work.  Some of the mandatory fields are:

Identifier      | Description
---             |---
`id`            |  Unique ID for the spell, used internally. Must be one continuous word, use underscores if necessary.
`type`          | Indicates the JSON object is a `SPELL`.
`name`          | Name of the spell that shows in game.
`description`   | Description of the spell that shows in game.
`valid_targets` | Targets affected by the spell.  If a valid target is not included, you cannot cast the spell on that target.  Additionally, if the valid target is not specified, the spell aoe will not affect it.  Can be `ally`, `field`, `ground`, `hostile`, `item`, `none` or `self`.
`effect`        | Hardcoded spell behaviors, roughly speaking spell "type".  See the list below.
`shape`         | The shape of the spell's area of effect.  See the list below.

Depending on the spell effect, more or less fields will be required.  

For example, a classic `attack` spell needs the `damage_type`, `min_damage`, `max_damage`, `min_range`, `max_range`, and `max_level` fields.  

In contrast, an `attack` spell using `effect_str` to grant a `self` buff (without a damage component) needs the `min_duration`, `max_duration`, and `max_level` fields instead.


### Spell effects

Each spell effect is defined in the `effect` field. For example, the Magus spell "Magic Missile" has the `attack` effect, meaning it deals damage to a specific target:

```json
  {
    "id": "magic_missile",
    "effect": "attack",
    "min_damage": 1
  }
```

while the Druid spell "Nature's Bow" has the `spawn_item` effect, designating the ID of the item to spawn:

```json
  {
    "id": "druid_naturebow1",
    "effect": "spawn_item",
    "effect_str": "druid_recurve"
  }
```

Below is a table of currently implemented effects, along with special rules for how they work:

Effect                 | Description
---                    |---
`area_pull`            | Pulls `valid_targets` in its aoe toward the target location.  Currently, the pull distance is set to 1 (see `directed_push`).
`area_push`            | Pushes `valid_targets` in its aoe away from the target location.  Currently, the push distance is set to 1 (see `directed_push`).
`attack`               | Causes damage to `valid_targets` in its aoe, and applies `effect_str` named effect to targets.  To damage terrain use `bash`.
`banishment`           | Kills any `MONSTER` in the aoe up to damage hp.  Any overflow hp is taken from the caster; if it's more than the caster's hp the spell fails.
`bash`                 | Bashes the terrain at the target.  Uses damage() as the strength of the bash.
`charm_monster`        | Charms a monster that has less hp than damage() for approximately duration().
`dash`                 | Dashes forward up to range and hits targets in a cone at the target.
`directed_push`        | Pushes `valid_targets` in aoe away from the target location, with a distance of damage().  Negative values pull instead.
`effect_on_condition`  | Runs the `effect_on_condition` from `effect_str` on all valid targets.  The EOC will be centered on the player, with the NPC as caster.
`emit`                 | Causes an `emit` at the target.
`explosion`            | Causes an explosion centered on the target.  Uses damage() for power and factor aoe()/10.
`flashbang`            | Causes a flashbang effect is centered on the target.  Uses damage() for power and factor aoe()/10.
`fungalize`            | Fungalizes the target.
`guilt`                | Target gets the guilt morale as if it killed the caster.
`map`                  | Maps out the overmap centered on the player, to a radius of aoe().
`mod_moves`            | Adds damage() moves to the targets.  Negative values "freeze" for that amount of time.
`morale`               | Gives a morale effect to NPCs or the avatar within the aoe.  Uses damage() for the value.  `decay_start` is duration() / 10.
`mutate`               | Mutates the targets.  If `effect_str` is defined, mutates toward that category instead of picking at random.  If the `MUTATE_TRAIT` flag is used, allows `effect_str` to be a specific trait.  Damage() / 100 is the percent chance the mutation will be successful (10000 represents 100.00%).
`noise`                | Causes damage() amount of noise at the target.  Note: the noise can be described further with `sound_type`, `sound_description`, `sound_ambient`, `sound_id` and `sound_variant`.
`pain_split`           | Evens out all of your limbs' damage.
`pull_target`          | Attempts to pull the target towards the caster in a straight line.  If the path is blocked by impassable furniture or terrain, the effect fails.
`recover_energy`       | Recovers an energy source equal to damage of the spell.  The energy source is defined in `effect_str` and may be one of `BIONIC`, `FATIGUE`, `PAIN`, `MANA` or `STAMINA`.
`remove_effect`        | Removes `effect_str` effects from all creatures in the aoe.
`remove_field`         | Removes a `effect_str` field in the aoe.  Causes teleglow of varying intensity and potentially teleportation depending on field density, if the field removed is `fd_fatigue`.
`revive`               | Revives a monster like a zombie necromancer.  The monster must have the `REVIVES` flag.
`short_range_teleport` | Teleports the player randomly range spaces with aoe variation.  See also the `TARGET_TELEPORT` and `UNSAFE_TELEPORT` flags.
`slime_split`          | The slime splits into two large or normal slimes, depending on mass.  Note: hardcoded for `mon_blob`-type enemies, check the monster `death_function` + spell `summon` combination.
`spawn_item`           | Spawns an item that will disappear at the end of its duration.  Default duration is 0.
`summon`               | Summons a `MONSTER` or `monstergroup` from `effect_str` that will disappear at the end of its duration.  Default duration is 0.  See also the `SPAWN_WITH_DEATH_DROPS` flag.
`summon_vehicle`       | Summons a `vehicle` from `effect_str` that will disappear at the end of its duration.  Default duration is 0.
`targeted_polymorph`   | A targeted monster is permanently transformed into the `MONSTER` specified by `effect_str`, if it has less HP than the spell's damage.  If `effect_str` is left empty, the target will transform into a random monster with a similar difficulty rating.  Alternatively, the `POLYMORPH_GROUP` flag can be used to pick a weighted ID from a `monstergroup`.  The player and NPCs are immune to this spell effect.
`ter_transform`        | Transforms the terrain and furniture in its aoe.  The chance of any one of the points in the aoe changing is 1 / (damage).  The `effect_str` is the ID of a `ter_furn_transform`.
`timed_event`          | Adds a timed event to the player only.  Valid timed events are: `amigara`, `artifact_light`, `dim`, `help`, `robot_attack`, `roots_die`, `spawn_wyrms`, `temple_flood`, `temple_open`, `temple_spawn`, `wanted`.  **NOTE**: This was added only for artifact active effects.  Support is limited, use at your own risk.
`translocate`          | Opens up a window that allows the caster to choose a translocation gate to teleport to.
`upgrade`              | Immediately upgrades a target `MONSTER`.
`vomit`                | Any creature within its aoe will instantly vomit, if it's able to do so.


### Spell shape

Another mandatory field is the spell shape. This dictates how the area of effect works:

Shape   | Description
--      | --
`blast` | A circular blast centered on the impact position.  Aoe value is the radius.
`cone`  | Fires a cone with an arc equal to aoe in degrees.
`line`  | Fires a line with a width equal to the aoe.


## Common fields

The following JSON fields are also used in spells and, while optional, greatly expand how spells can behave.

All fields that are numeric can also be a "variable object"( see [NPCs.md](NPCs.md) for full details) instead of a number.  

Field group | Description | Example
---  | --- | ---
`min_X`, `max_X`, ``X_increment`` | Minimun value, maximum value, and the value, that add each level. Note: your spell don't take max_X on the max lvl, it will always add the correct amount of X_increment to your spell - in spell with `min_damage: 0, max_damage: 100, damage_increment: 5` and `max_level: 10`, on level 10 the spell will deal only 50 damage, as [ 0 + (5\*10) ]. Opposite is also true, in spell with `min_damage: 0, max_damage: 25, damage_increment: 5` on level 10 will deal only 25 damage, as the highest value, described in `max_X` | `min_damage`, `min_range`, `min_aoe`
`min_damage`, `max_damage`, `damage_increment` |  By default respond for the damage (or healing, if the damage is negative) of the spell, but, depending on `effect`, may respond for different things. For damage, that do not increase with level (like for a monster spell) you need to use both `min_damage` and `max_damage` of the same value (`"min_damage": 15, "max_damage": 15`), increment isn't nesessary | "min_damage": 0,  <br>"max_damage": 100,  <br>"damage_increment": 5,|
`min_duration`, `max_duration`, `duration_increment` | Respond for the duration of the effect. Writed in moves, so `100` means `1 second`, and `1 minute` described as `6000`.  Usually works with some `effects`, and with `effect_str` | "min_duration": 100, <br>"max_duration": 6000, <br>"duration_increment": 100,
`min_range`, `max_range`, `range_increment` | Respond for the distance caster can use the spell. Can be ignored, if the target is user (giving an buff/debuff, spawning an item). Note: the reality bubble diameter is ~60 | "min_range": 2, <br>"max_range": 10, <br>"range_increment": 0.5,
`min_aoe`, `max_aoe`, `aoe_increment` | Respond for area of effect. | "min_aoe": 0, <br>"max_aoe": 5, <br>"aoe_increment": 0.1, 
`min_accuracy`, `max_accuracy`, `accuracy_increment` | Respond for accuracy of the spell. -20 accuracy will make it always miss, 20 will make it always hit. Doesn't work currently | "min_accuracy" -20, <br>"max_accuracy": 20, <br>"accuracy_increment": 1.5
`min_dot`, `max_dot`, `dot_increment` | Respond for damage over time. Same as damage, may respond both for damage and for healing (except the healing would be too fast for default character) | "min_dot": 0, <br>"max_dot": 2, <br>"dot_increment": 0.1,
`min_pierce`, `max_pierce`, `pierce_increment` | Respond for armor piersing of the spell | "min_pierce": 0, <br>"max_pierce": 1, <br>"pierce_increment": 0.1,
`base_casting_time`, `final_casting_time`, `casting_time_increment` | Respond for amount of time user cast the spell. Same as duration, writed in moves, which allow to make spells, that can be casted for 0.5 or 0.25 seconds. Doesn't work for monsters and for items, that cast spells | "base_casting_time": 1000, <br>"final_casting_time": 100, <br>"casting_time_increment": -50,
`base_energy_cost`, `final_energy_cost`, `energy_increment` | Respond for amount of energy you spend for cast | "base_energy_cost": 30, <br>"final_energy_cost": 100, <br>"energy_increment": -6,
`field_id`, `field_chance`, `min_field_intensity`, `max_field_intensity`, `field_intensity_increment`, `field_intensity_variance` | Respond for field spawn. `field_id` describe what field will be spawned; `field_chance` describe the chance of spawning the field, as "one in `field_chance`" (1 means the probability is 100%, 2 is 50%, 3 is 33% etc.); `min_field_intensity`, `max_field_intensity` and `field_intensity_increment` are respond for the field intensity and it grow depend on level (for example, for fd_electricity intensity 1 represent spark, when intensity 10 is electric cloud); `field_intensity_variance` allow the spell randomly increase and decrease the intensity of the spell as a percent (intencity 10 and variance 0.1 means it can grow or shring at 10%, from 9 to 11)| "field_id": "fd_blood", <br>"field_chance": 100,    <br>"min_field_intensity": 10, <br>"max_field_intensity": 10, <br>"field_intensity_increment": 1, <br>"field_intensity_variance": 0.1
`effect_str` | By default respond for effect, the spell can deal (see [EFFECTS_JSON](EFFECTS_JSON.md)), but can vary depend on effect | "effect_str": "zapped", 
`max_level` | How much you can train the spell. 0 by default | "max_level": 10,
`difficulty` | How hard to cast the spell. Higher difficulty make spell easier to fail, but also train your spell skill to the `difficulty` level (spell with difficulty 10 can train user to spellcasting lvl 10) | "difficulty": 7,
`affected_body_parts` | Respond on which body part `effect_str` will occur. Doesn't respond what body part the spell will target if you damage the target, it always aim a `torso` no matter what | "affected_body_parts": [ "head" ] 
`extra_effects` | Cast `id` spell right after the end of the main. Can cast multiple different spells at once | "extra_effects": [ <br> { <br> "id": "fireball", <br> "hit_self": false, <br> "max_level": 3 <br> }, <br> { "id": "storm_chain_1" } <br>]
`learn_spells` | Allow user to learn this spells, when it will reach the amount of levels ("create_atomic_light": 5 means user can learn create_atomic_light, when it will reach the 5 level of the spell) | "learn_spells": { "create_atomic_light": 5, "megablast": 10 }
`teachable` | Whether it's possible to teach this spell between characters. (Default = true) | `"teachable": true`
`message` | The message, that will be send in log, when you cast a spell. "You cast %s!" by default | "message": "You feel refreshed."
`sound_type`, `sound_description`, `sound_ambient`, `sound_id`, `sound_variant` | Respond for the sound that play when you use the spell. `sound_type` is one of `background`, `weather`, `music`, `movement`, `speech`, `activity`, `destructive_activity`, `alarm`, `combat`, `alert`, `order`; `sound_description` respond for the message in log, as "You hear %s" ("an explosion" by default); `sound_ambient` whether or not this is treated as an ambient sound | "sound_type": "combat", <bp>"sound_description": "a whoosh", <bp>"sound_ambient": true, <bp>"sound_id": "misc", <bp>"sound_variant": "shockwave", 
`targeted_monster_ids` | You can target only `monster_id` | "targeted_monster_ids": [ "mon_hologram" ],
`targeted_monster_species` | You can target only picked species (for list see data/json/species.json) | "targeted_monster_species": [ "ROBOT", "CYBORG" ],
`ignored_monster_species` | The opposite of targeted_monster_species: you can target everything except picked species (for list see data/json/species.json) | "ignored_monster_species": [ "ZOMBIE", "NETHER" ],

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

Flag                       | Description
-------------------------  | ---
`CONCENTRATE`              | Focus affects spell fail %.
`EXTRA_EFFECTS_FIRST`      | The spell's `extra_effects` will happen before the main spell effect.
`FRIENDLY_POLY`            | The target of a `targeted_polymorph` spell will become friendly to the caster if the spell resolves successfully.
`HOSTILE_SUMMON`           | Summon spell always spawns a hostile monster.
`HOSTILE_50`               | Summoned monster spawns friendly 50% of the time.
`IGNITE_FLAMMABLE`         | If the spell area has anything flammable, a fire will be produced
`IGNORE_WALLS`             | Spell's aoe goes through walls.
`LOUD`                     | Spell makes extra noise at target.
`MUST_HAVE_CLASS_TO_LEARN` | The spell is autolearned when you have `spell_class`, and removed when you lost it.
`MUTATE_TRAIT`             | Overrides the `mutate` spell effect to use a specific trait_id instead of a category.
`NO_EXPLOSION_SFX`         | The spell will not generate a visual explosion effect.
`NO_FAIL`                  | This spell cannot fail when cast.
`NO_HANDS`                 | Hands do not affect spell energy cost.
`NO_LEGS`                  | Legs do not affect casting time.
`NO_PROJECTILE`            | The "projectile" portion of the spell phases through walls, the epicenter of the spell effect is exactly where you target it, with no regards to obstacles.
`NON_MAGICAL`              | Ignores spell resistance when calculating damage mitigation.
`PAIN_NORESIST`            | Pain altering spells can't be resisted (like with the deadened trait).
`PERCENTAGE_DAMAGE`        | The spell deals damage based on the target's current hp.  This means that the spell can't directly kill the target.
`PERMANENT`                | Items or creatures spawned with this spell do not disappear and die as normal.  Items can only be permanent at maximum spell level; creatures can be permanent at any spell level.
`PERMANENT_ALL_LEVELS`     | Items spawned with this spell do not disappear even if the spell is not max level.
`POLYMORPH_GROUP`          | A `targeted_polymorph` spell will transform the target into a random monster from the `monstergroup` in `effect_str`.
`RANDOM_AOE`               | Picks random number between (min + increment) * level and max instead of normal behavior.
`RANDOM_CRITTER`           | Same as `RANDOM_TARGET` but ignores ground.
`RANDOM_DAMAGE`            | Picks random number between (min + increment) * level and max instead of normal behavior.
`RANDOM_DURATION`          | Picks random number between (min + increment) * level and max instead of normal behavior.
`RANDOM_TARGET`            | Forces the spell to choose a random valid target within range instead of the caster choosing the target.  This also affects `extra_effects`.
`SILENT`                   | Spell makes no noise at target.
`SOMATIC`                  | Arm encumbrance affects fail % and casting time (slightly).
`SPAWN_GROUP`              | Spawn or summon from an `item_group` or `monstergroup`, instead of the specific IDs.
`SPAWN_WITH_DEATH_DROPS`   | Allows summoned monsters to retain their usual death drops, otherwise they drop nothing.
`SWAP_POS`                 | A projectile spell swaps the positions of the caster and target.
`TARGET_TELEPORT`          | Teleport spell changes to maximum range target with aoe as variation around target.
`UNSAFE_TELEPORT`          | Teleport spell risks killing the caster or others.
`VERBAL`                   | Spell makes noise at caster location, mouth encumbrance affects fail %.
`WONDER`                   | This drastically alters the behavior of the parent spell: The spell itself doesn't cast, but the damage and range information are used to cast the `extra_effects`.  A n number of `extra_effects` will be chosen to be cast at random, where n is the current damage of the spell (stacks with the `RANDOM_DAMAGE` flag), the message of the casted spell will also be displayed.  If this spell's message is not wanted, make sure `message` is an empty string.


### Damage Types

The following are the available damage types, for those spells that have a damaging component:

Damage type  | Description
---          |---
`acid`       | 
`bash`       | 
`biological` | Internal damage such as poison.
`cold`       | 
`cut`        | 
`electric`   | 
`heat`       | 
`pure`       | This damage type goes through armor altogether.  Set by default.
`stab`       | 


### Spell level

Spells can change effects as they level up.  "Effect" in this context can be: 
* accuracy
* aoe (area of effect)
* damage
* dot (damage over time)
* duration
* pierce, and
* range

The effect growth is indicated with the `min_effect`, `max_effect` and `effect_increment` fields:
* `min_effect` is what the spell will do at level 0
* `max_effect` is where it stops growing, the level cap.
* `effect_increment` is how much it changes per level.

Additionally, there are also included: 
* energy cost (as `base_energy_cost`, `final_energy_cost`, `energy_increment`), and
* field intensity (`min_field_intensity`, `max_field_intensity`, `field_intensity_increment` plus `field_intensity_variance`).

For example:

```json
...
    "min_range": 1,
    "max_range": 25,
    "range_increment": 5,
...
```

Min and max values must always have the same sign, but it can be negative e.g. in the case of spells that use a negative 'recover' effect to cause pain or stamina damage. For example:

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

There multiple ways to learn spells: learning a spell from an item (through a `use_action`), from spells that have the `learn_spells` field, and from traits/mutations.  An example is shown below:

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
  }
```

You can study this spellbook for a rate of ~1 experience per turn depending on intelligence, spellcraft, and focus.

Below is an example of `learn_spells` usage:
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
  }
```

Traits/mutations have the `spells_learned` field, see the [JSON_INFO](JSON_INFO.md) documentation for details.


### Extra spell effects

Another two interesting fields are `extra_effects` and `effect_str`:

* `extra_effects` allows to cast one or more spells simultaneously, thus enabling "chain" style casting.

* `effect_str` works as a pointer, it links the main spell to a certain JSON object.  Behavior can vary depending on the spell's `effect`, e.g. it may define which `effect_type` will be applied the target, the ID of the `spawn_item`, `monster` or `vehicle` to spawn, etc.  Do note that some effect types are hardcoded, like `beartrap`, `crushed`, `dazed`, `downed`, `stunned`, etc. (non-exhaustive list!).


## Adding spells to professions and NPCs

You can add spells to professions or NPC class definitions like this:

```json
  {
    "id": "test_profession",
    "type": "profession",
    "name": "Test Professioner",
    "description": "Tests professions",
    "spells": [ { "id": "summon_zombie", "level": 0 }, { "id": "magic_missile", "level": 10 } ],
    ...
  }
```

**Note:** This makes it possible to learn spells that conflict with a class. It also does not give the prompt to gain the class. Be judicious upon adding this to a profession!

## Examples

The following are some spell examples, from simple to advanced:

### Summon spell

```C++
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

Self explanatory: when cast, `test_summon` will silently summon 1 hostile `mon_test_monster` on a random 3x3 location centered around the caster, with a duration of 62.5 seconds, after which it will disappear.

### Typical attack

```C++
  {
    "id": "test_attack",                                     // id of the spell, used internally. not translated
    "type": "SPELL",
    "name": "Ranged Strike",                                 // name of the spell that shows in game
    "description": "Deals damage to the target with 100% accuracy. Will always apply the status effect specified in 'effect_str'.",
    "valid_targets": [ "ground", "hostile" ],                // if a valid target is not included, you cannot cast the spell on that target.
    "effect": "attack",                                      // effects are coded in C++. A list is provided in this document of possible effects that have been coded.
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

Explanation: classic damage spell with designated effect.  After a 5 second cast, a lvl 1 `test_attack` will deal 11 stab damage up to 4 tiles away, and stun the `hostile` target for 2.1 seconds.

Note: `valid_targets` has both `ground` and `hostile` so it can be targeted in an area with no line of sight.


### Consecutive spell casting

```C++
  {
    "id": "test_combo",                                      // id of the spell, used internally. not translated
    "type": "SPELL",
    "name": "Combo Strikes",                                 // name of the spell that shows in game
    "description": "Upon casting this spell, will also activate the spells specified on the 'extra_effects' in descending order.",
    "flags": [ "SILENT", "RANDOM_DAMAGE", "RANDOM_AOE" ],    // see "Spell Flags" in this document
    "valid_targets": [ "hostile", "ground" ],                // if a valid target is not included, you cannot cast the spell on that target.
    "effect": "attack",                                      // effects are coded in C++. A list is provided in this document of possible effects that have been coded.
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

Explanation: If you put two or more spells in `extra_effects`, it will consecutively cast the spells: first `extra_effects`, second `extra_effects`, third `extra_effects`, etc.  If you wish to pick one at random, use the `WONDER` flag (see below).  Additionally, the extra spells will be cast at a level up to the level of the parent spell being cast, unless additional data is added to each `test_atk#`.


### Random spell casting

```C++
  {
    "id": "test_starter_spell",                              // id of the spell, used internally. not translated
    "type": "SPELL",
    "name": "Starter",                                       // name of the spell that shows in game
    "description": "Upon casting this spell, randomly selects one spell specified in 'extra_effects' to cast. The spell damage shows how many times it will randomly select from the list",
    "flags": [ "SILENT", "WONDER", "RANDOM_DAMAGE" ],        // see "Spell Flags" in this document
    "valid_targets": [ "hostile" ],                          // if a valid target is not included, you cannot cast the spell on that target.
    "effect": "attack",                                      // effects are coded in C++. A list is provided in this document of possible effects that have been coded.
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

Explanation: The `WONDER` flag does wonders by turning the spell into a dice: When cast, the main spell will pick the subspells in `extra_effects`.  The amount of "rolls" is specified via `min_damage` and `max_damage` plus the `RANDOM_DAMAGE` flag.  In this example, casting `test_starter_spell` will cause any one of each `test_atk#` subspells to be selected, this repeats 3 to 5 times.

Note: There must be a minimum of one spell in `extra_effects` for the `WONDER` flag to work properly.


### Repeatedly cast the same spell

```C++
  {
    "type": "SPELL",
    "id": "test_attack_repeat",                              // id of the spell, used internally. not translated
    "name": "a spell",                                       // name of the spell that shows in game
    "description": "Upon casting this spell it will repeat the spell specified in `extra_effects` - the amount of repetitions is the interval `min_damage`-`max_damage` ",
    "extra_effects": [ { "id": "test_attack" } ],            // this allows you to cast multiple spells with only one spell
    "flags": [ "SILENT", "WONDER", "RANDOM_DAMAGE" ],        // see "Spell Flags" in this document
    "valid_targets": [ "hostile" ],                          // if a valid target is not included, you cannot cast the spell on that target.
    "effect": "attack",                                      // effects are coded in C++. A list is provided in this document of possible effects that have been coded.
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

Explanation: Notice a different approach for the `WONDER` and `RANDOM_DAMAGE` combo: `min_damage` set to 5 and `max_damage` set to 7 will cause the main spell to "roll" `extra_effects` 5 - 7 times, but this time there's a single `test_attack`.  Because `WONDER` has a 100% chance of picking a spell, `test_attack`, will be repeated 5 to 7 times.


### A spell that casts a note on the target and an effect on the caster

```C++
  {
    "id": "test_attack_note",                                // id of the spell, used internally. not translated
    "type": "SPELL",
    "name": "a note",                                        // name of the spell that shows in game
    "description": "This spell applies a harmless status effect to notify the player about the spell that the user has cast.",
    "flags": [ "SILENT" ],                                   // see "Spell Flags" in this document
    "valid_targets": [ "hostile" ],                          // if a valid target is not included, you cannot cast the spell on that target.
    "effect": "attack",                                      // effects are coded in C++. A list is provided in this document of possible effects that have been coded.
    "extra_effects": [ { "id": "sacrifice_spell", "hit_self": true }, { "id": "test_attack" } ],     // this allows you to cast multiple spells with only one spell
    "effect_str": "eff_test_note",                           // varies, see table of implemented effects in this document
    "min_aoe": 6,                                            // area of effect, or range of variance
    "max_aoe": 6,
    "min_duration": 1,                                       // duration of spell effect in moves (if the spell has a special effect)
    "max_duration": 1
  }
```

Explanation: Here we have one main spell with two subspells: one on the caster and the other on the target.  To do this, you must specify the ID of whatever spells you're using with the `extra_effects` field.  In this case, `sacrifice_spell` is stated with `"hit_self": true` and will "hit" the caster, while the second spell will be cast as normal on the `hostile` target.  This is only necessary if we need an effect that is cast on a target and a secondary effect cast on the caster.


### Monster spells

See [Monster special attacks - Spells](MONSTER_SPECIAL_ATTACKS.md#spell-monster-spells).


## Enchantments

Enchantments are another layer of enhancements, similar to `effect_type` and `mutation`.  Unlike these which are carried by the avatar and similar entities, enchantments are bound to "containers" such as items, mutations, bionics and effects.  This allows some flexibility in customizing effects and interactions according to the current state of the container, the spell being activated if any, what exactly is being granted, and more.

Identifier                  | Description
---                         |---
`id`                        | Unique ID.  Must be one continuous word, use underscores if necessary.
`has`                       | How an enchantment determines if it is in the right location in order to qualify for being active.  `WIELD` when wielded in your hand, `WORN` when worn as armor, `HELD` when in your inventory.
`condition`                 | Determines the environment where the enchantment is active.  `ALWAYS` is active always and forevermore, `ACTIVE` whenever the item, mutation, bionic, or whatever the enchantment is attached to is active, `INACTIVE` whenever the item, mutation, bionic, or whatever the enchantment is attached to is inactive.  `DIALOG_CONDITION - ACTIVE` whenever the dialog condition in `condition` is true.
`hit_you_effect`            | A spell that activates when you `melee_attack` a creature.  The spell is centered on the location of the creature unless `"hit_self": true`, then it is centered on your location.  Follows the template for defining `fake_spell`.
`hit_me_effect`             | A spell that activates when you are hit by a creature.  The spell is centered on your location.  Follows the template for defining `fake_spell`
`intermittent_activation`   | Spells that activate centered on you depending on the duration.  The spells follow the `fake_spell` template.
`values`                    | Numbers that can be modified (see [list](#id-values)).  `add` is added to the base value, `multiply` is **also added** and treated as percentage: 2.5 is +250% and -1 is -100%.  `add` is always applied before `multiply`.  Either `add` or `multiply` can be a variable_object/arithmetic expression (see [below](#variables) for syntax and application, and [NPCs](NPCs.md) for the in depth explanation).
`emitter`                   | Grants the emit_id.
`modified_bodyparts`        | Modifies the body plan (standard is human).  `gain` adds body_part_id, `lose` removes body_part_id.  Note: changes done this way stay even after the item/effect/mutation carrying the enchantment is removed.
`mutations`                 | Grants the mutation/trait ID.  Note: enchantments effects added this way won't stack, due how mutations work.
`ench_effects`              | Grants the effect_id.  Requires the `intensity` for the effect.


There are two possible syntaxes.  The first is by defining an enchantment object and then referencing the ID, the second is by directly defining the effects as an inline enchantment of something (in this case, an item):

```json
  {
    "type": "enchantment",
    "id": "ENCH_INVISIBILITY",
    "condition": "ALWAYS",
    "has": "WIELD",
    "hit_you_effect": [ { "id": "AEA_FIREBALL", "hit_self": true, "once_in": 12 } ],
    "hit_me_effect": [ { "id": "AEA_HEAL" } ],
    "values": [ { "value": "STRENGTH", "multiply": 1.1, "add": -5 } ],
    "emitter": "emit_AEP_SMOKE",
    "modified_bodyparts": [ { "gain": "test_corvid_beak" }, { "lose": "torso" } ],
    "mutations": [ "GILLS", "MEMBRANE", "AMPHIBIAN", "WAYFARER", "WILDSHAPE:FISH" ],
    "ench_effects": [ { "effect": "invisibility", "intensity": 1 } ],
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
Note: all fields except for `type` and `id` are optional.

```json
  {
    "copy-from": "mring_silver",
    "type": "TOOL_ARMOR",
    "id": "mring_wizardry_lesser",
    "price_postapoc": "2000 USD",
    "name": { "str": "lesser ring of wizardry", "str_pl": "lesser rings of wizardry" },
    "description": "A thin silver band ring, engraved with two sealed scrolls.  Increases mana capacity somewhat.",
    "relic_data": { "passive_effects": [ { "has": "WORN", "condition": "ALWAYS", "values": [ { "value": "MAX_MANA", "add": 400 } ] } ] }
  }
```


### The `relic_data` field

As seen in the last example, enchantments are added to the item as `passive_effects` inside the `relic_data` field.  Items with this data are turned into a relic or artifact, being displayed as magenta inside the inventory view.

Also supported is `charge_info`, which allows automatic charge regeneration.  This in turn enables active magical items that cast spells on use:

```json
...
    "use_action": { "type": "cast_spell", "spell_id": "conj_throwing_blade3", "no_fail": true, "level": 1, "need_worn": true },
    "extend": { "flags": [ "NO_UNLOAD", "NO_RELOAD" ] },
    "charges_per_use": 1,
    "relic_data": { "charge_info": { "recharge_type": "periodic", "time": "1 h", "regenerate_ammo": true } },
    "pocket_data": [ { "pocket_type": "MAGAZINE", "holster": true, "ammo_restriction": { "crystallized_mana": 5 } } ]
...
```

The item consumes 1 charge per spell cast.  It can't be recharged or unloaded, relying on ammo regeneration over time for use.

Another example is a `GUN` type item (e.g. a firearm).  As this is a weapon that consumes ammo per use, `use_action` can be omitted:

```json
...
    "clip_size": 5,
    "flags": [ "NO_UNLOAD", "NO_RELOAD" ],
    "relic_data": {
      "charge_info": { "regenerate_ammo": true, "recharge_type": "periodic", "time": "20 s" },
      "passive_effects": [ { "id": "ENCH_INVISIBILITY" } ]
    },
    "pocket_data": [ { "pocket_type": "MAGAZINE", "rigid": true, "ammo_restriction": { "ammo_magic_bullet": 5 } } ],
...
```

This weapon consumes "magic bullet" ammo every time it's fired.  Note how `charge_info` and `passive_effects` can be used together.


The field `charge_info` supports the following:

Identifier           | Description
---                  |---
`regenerate_ammo`    | `true`.
`recharge_type`      | Can be one of: `lunar`, `periodic`, `solar_cloudy`, `solar_sunny`, or `none`.
`time`               | Time required per charge.
`recharge_condition` | (optional) Similar to `has` from enchantments: can be one of `held`, `worn`, `wield`.  If omitted, the item recharges regardless, even if dropped.


### Variables

From now, EOC variables can be used inside enchantments, including predefined (see [NPCs.md](NPCs.md#dialogue-conditions) for examples), custom variables or [math equasions](NPCs.md#math).  Here are some examples:

```json
  {
    "type": "enchantment",
    "id": "MON_NEARBY_STR",
    "has": "WIELD",
    "condition": "ALWAYS",
    "values": [ { "value": "STRENGTH", "add": { "math": [ "u_val('dexterity') + 1" ] } } ]
  }
```

This enchantment adds the dexterity value to strength plus one: a character with str 8 and dex 10 will result with str 19 and dex 10.


```json
  {
    "type": "enchantment",
    "id": "MON_NEARBY_LUMINATION",
    "has": "WIELD",
    "condition": "ALWAYS",
    "values": [
      {
        "value": "LUMINATION",
        "add": { "math": [ "u_monsters_nearby('radius': 25) * 20" ] }
      }
    ]
  }
```

This enchantment checks the amount of monsters near the character (in a 25 tile range), then multiplies that number by 20, and adds the value as lumination: more monsters nearby = more light produced.


```json
  {
    "type": "enchantment",
    "id": "MOON_STR",
    "has": "WORN",
    "condition": "ALWAYS",
    "values": [ { "value": "STRENGTH", "add": { "math": [ "IS_UNDER_THE_MOON * 4" ] } } ]
  }
```

Here's an enchantment that relies on a custom variable check, the full power of EOCs in your hand.

First, the custom variable IS_UNDER_THE_MOON is set behind the scenes, it checks if the character is under the moon's rays (by a combination of `{ "not": "is_day" }` and `"u_is_outside"`): if true value is 1, otherwise is 0.  Then, the custom variable is used inside an arithmetic operation that multiplies the truth value by 4: character is granted [ 1 * 4 ] = 4 additional strength if outside and during the night, or [ 0 * 4 ] = 0 additional strength otherwise.

### ID values

The following is a list of possible enchantment `values`:

Character status value  | Description
---                     |---
`ARMOR_ACID`            | Negative values give armor against the damage, positive values make you accept more damage of this type
`ARMOR_BASH`            | 
`ARMOR_BIO`             | 
`ARMOR_BULLET`          | 
`ARMOR_COLD`            | 
`ARMOR_CUT`             | 
`ARMOR_ELEC`            | 
`ARMOR_HEAT`            | 
`ARMOR_STAB`            | 
`ATTACK_NOISE`          | Affects the amount of noise you make while melee attacking.
`ATTACK_SPEED`          | Affects attack speed of item even if it's not the one you're wielding.
`BIONIC_POWER`          |
`BONUS_BLOCK`           | Affects the number of blocks you can perform.
`BONUS_DODGE`           | Affects the number of dodges you can perform.
`CARRY_WEIGHT`          | Affect the summary weight player can carry. `"add": 1000` adds 1 kg of weight to carry.
`COMBAT_CATCHUP`        | Affects the rate at which you relearn combat skills (multiplier)
`CLIMATE_CONTROL_HEAT`  | Moves body temperature up towards comfortable by number of warmth units up to value.
`CLIMATE_CONTROL_CHILL` | Moves body temperature down towards comfortable by number of warmth units up to value.
`DEXTERITY`             | Affects the dexterity stat.
`INTELLIGENCE`          | Affects the intelligence stat.
`PERCEPTION`            | Affects the perception stat.
`STRENGTH`              | Affects the strength stat.
`SPEED`                 | 
`EFFECTIVE_HEALTH_MOD`  | If this is anything other than zero (which it defaults to) you will use it instead of your actual health mod.
`EXTRA_ACID`            | EXTRA_TYPE increases received damage of the selected type.
`EXTRA_BASH`            | 
`EXTRA_BIO`             | 
`EXTRA_BULLET`          | 
`EXTRA_COLD`            | 
`EXTRA_CUT`             | 
`EXTRA_ELEC`            | 
`EXTRA_HEAT`            | 
`EXTRA_STAB`            | 
`EXTRA_ELEC_PAIN`       | Multiplier on electric damage received, the result is applied as extra pain.
`EVASION`               | Flat chance for your character to dodge incoming attacks regardless of other modifiers. From 0.0 (no evasion chance) to 1.0 (100% evasion chance).
`FALL_DAMAGE`           | Affects the amount of fall damage you take.
`FATIGUE`               | 
`FOOTSTEP_NOISE`        | 
`FORCEFIELD`            | Chance your character reduces incoming damage to 0. From 0.0 (no chance), to 1.0 (100% chance to avoid attacks).
`HUNGER`                | 
`KNOCKBACK_RESIST`      | The amount knockback effects you, 0 is the regular amount, -100 would be double effect, 100 would be no effect
`KNOCKDOWN_RESIST`      | The amount knockdown effects you, currently *only* having 100 or greater knockdown_resist makes you immune to knockdown
`LEARNING_FOCUS`        | Amount of bonus focus you have for learning purposes.
`LUMINATION`            | Character produces light.
`MAX_HP`                | 
`MAX_MANA`              | 
`MAX_STAMINA`           | 
`MELEE_DAMAGE`          | 
`RANGED_DAMAGE`         | Adds damage to ranged attacks
`METABOLISM`            | 
`MOD_HEALTH`            | If this is anything other than zero (which it defaults to) you will to mod your health to a max/min of `MOD_HEALTH_CAP` every half hour.
`MOD_HEALTH_CAP`        | If this is anything other than zero (which it defaults to) you will cap your `MOD_HEALTH` gain/loss at this every half hour.
`MOVE_COST`             | 
`OVERKILL_DAMAGE`       | multiplies or contributes to the damage to an enemy corpse after death. The lower the number, the more damage caused
`PAIN`                  | When gaining pain the amount gained will be modified by this much.  You will still always gain at least 1 pain.
`PAIN_REMOVE`           | When pain naturally decreases every five minutes the chance of pain removal will be modified by this much.  You will still always have at least a chance to reduce pain.
`SHOUT_NOISE`           | 
`SIGHT_RANGE_ELECTRIC`  | How many tiles away is_electric() creatures are visible from.
`SIGHT_RANGE_NETHER`    | How many tiles away is_nether() creatures are visible from.
`SIGHT_RANGE_MINDS`     | How many tiles away humans or creatures with the HAS_MIND flag are visible from.
`MOTION_VISION_RANGE `  | Reveals all monsters as a red `?` within the specified radius.
`SLEEPY`                | The higher this the easier you fall asleep.
`SKILL_RUST_RESIST`     | Chance / 100 to resist skill rust.
`SOCIAL_INTIMIDATE`     | Affects your ability to intimidate.
`SOCIAL_LIE`            | Affects your ability to lie.
`SOCIAL_PERSUADE`       | Affects your ability to persuade.
`RANGE`                 | Modifies your characters range with firearms
`READING_EXP`           | Changes the minimum you learn from each reading increment.
`RECOIL_MODIFIER`       | Affects recoil when shooting a gun. Positive value increase the dispersion, negative decrease one.
`REGEN_HP`              | Affects the rate you recover hp.
`REGEN_MANA`            | 
`REGEN_STAMINA`         | 
`THIRST`                | 
`WEAPON_DISPERSION`     | Positive value increase the dispersion, negative decrease one.


Melee-only enchantment values | Description
---                           |---
`ITEM_DAMAGE_ACID`            | 
`ITEM_DAMAGE_BASH`            | 
`ITEM_DAMAGE_BIO`             | 
`ITEM_DAMAGE_BULLET`          | 
`ITEM_DAMAGE_COLD`            | 
`ITEM_DAMAGE_CUT`             | 
`ITEM_DAMAGE_ELEC`            | 
`ITEM_DAMAGE_HEAT`            | 
`ITEM_DAMAGE_PURE`            | 
`ITEM_DAMAGE_STAB`            | 


Enchanted item value | Description
---                  |---
`ITEM_ARMOR_ACID`    | 
`ITEM_ARMOR_BASH`    | 
`ITEM_ARMOR_BIO`     | 
`ITEM_ARMOR_BULLET`  | 
`ITEM_ARMOR_COLD`    | 
`ITEM_ARMOR_CUT`     | 
`ITEM_ARMOR_ELEC`    | 
`ITEM_ARMOR_HEAT`    | 
`ITEM_ARMOR_STAB`    | 
`ITEM_ATTACK_SPEED`  | 


### Enchantment value examples

```C++
  { "value": "ARMOR_ELEC", "add": -20 }       // subtracts 20 points of incoming electrical damage
  { "value": "ATTACK_SPEED", "add": -60 }     // subtracts 60 attack moves, making the attacker faster
  { "value": "ARMOR_COLD", "multiply": -0.4 } // subtracts 40% of incoming cold damage
  { "value": "ARMOR_HEAT", "multiply": 0.4 }  // increases damage taken from fire by 40%
  { "value": "ARMOR_CUT", "add": 2 }          // increases incoming cut damage by 2
  { "value": "ARMOR_BIO", "multiply": -1.4 }  // subtracts 100 percent of incoming biological damage, heals for the remaining 40%
  { "value": "ARMOR_ACID", "multiply": 1.4 }  // increases incoming acid damage by 140%
```
