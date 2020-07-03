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
	"extra_effects": [ { "id": "fireball", "hit_self": false, "max_level": 3 } ],	// this allows you to cast multiple spells with only one spell
	"affected_body_parts": [ "HEAD", "TORSO", "MOUTH", "EYES", "ARM_L", "ARM_R", "HAND_R", "HAND_L", "LEG_L", "FOOT_L", "FOOT_R" ], // body parts affected by effects
	"flags": [ "SILENT", "LOUD", "SOMATIC", "VERBAL", "NO_HANDS", "NO_LEGS", "SPAWN_GROUP" ], // see "Spell Flags" below
  "spell_class": "NONE",                                    //
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

### Spell effects and rules

The value of the `"effect"` string in the spell's JSON data says what effect the spell has. For example, the Magus spell "Magic Missile" has a `target_attack` effect, meaning it deals damage to a specific target:

```json
{
  "id": "magic_missile",
  "effect": "target_attack",
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
| `pain_split` | makes all of your limbs' damage even out
| `move_earth` | "digs" at the target location. some terrain is not diggable this way.
| `target_attack` | deals damage to a target (ignores walls).  If "effect_str" is included, it will add that effect (defined elsewhere in json) to the targets if able, to the body parts defined in affected_body_parts. Any aoe will manifest as a circular area centered on the target, and will only deal damage to valid_targets. (aoe does not ignore walls)
| `projectile_attack` | similar to target_attack, except the projectile you shoot will stop short at impassable terrain.  If "effect_str" is included, it will add that effect (defined elsewhere in json) to the targets if able, to the body parts defined in affected_body_parts.
| `cone_attack` | fires a cone toward the target up to your range.  The arc of the cone in degrees is aoe.  Stops at walls.  If "effect_str" is included, it will add that effect (defined elsewhere in json) to the targets if able, to the body parts defined in affected_body_parts.
| `line_attack` | fires a line with width aoe toward the target, being blocked by walls on the way.  If "effect_str" is included, it will add that effect (defined elsewhere in json) to the targets if able, to the body parts defined in affected_body_parts.
| `spawn_item` | spawns an item that will disappear at the end of its duration.  Default duration is 0.
| `summon` | summons a monster ID or group ID from `effect_str` that will disappear at the end of its duration.  Default duration is 0.
| `teleport_random` | teleports the player randomly range spaces with aoe variation
| `recover_energy` | recovers an energy source equal to damage of the spell. The energy source recovered is defined in "effect_str" and may be one of "MANA", "STAMINA", "FATIGUE", "PAIN", "BIONIC"
| `ter_transform` | transform the terrain and furniture in an area centered at the target.  The chance of any one of the points in the area of effect changing is one_in( damage ).  The effect_str is the id of a ter_furn_transform.
| `vomit` | any creature within its area of effect will instantly vomit, if it's able to do so.
| `timed_event` | adds a timed event to the player only. valid timed events: "help", "wanted", "robot_attack", "spawn_wyrms", "amigara", "roots_die", "temple_open", "temple_flood", "temple_spawn", "dim", "artifact_light" NOTE: This was added only for artifact active effects. support is limited, use at your own risk.
| `explosion` | an explosion is centered on the target, with power damage() and factor aoe()/10
| `flashbang` | a flashbang effect is centered on the target, with poewr damage() and factor aoe()/10
| `mod_moves` | adds damage() moves to the target. can be negative to "freeze" the target for that amount of time
| `map` | maps the overmap centered on the player out to a radius of aoe()
| `morale` | gives a morale effect to all npcs or avatar within aoe, with value damage(). decay_start is duration() / 10.
| `charm_monster` | charms a monster that has less hp than damage() for approximately duration()
| `mutate` | mutates the target(s). if effect_str is defined, mutates toward that category instead of picking at random. the "MUTATE_TRAIT" flag allows effect_str to be a specific trait instead of a category. damage() / 100 is the percent chance the mutation will be successful (a value of 10000 represents 100.00%)
| `bash` | bashes the terrain at the target. uses damage() as the strength of the bash.

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
| `WONDER` | This alters the behavior of the parent spell drastically: The spell itself doesn't cast, but its damage and range information is used in order to cast the extra_effects.  N of the extra_effects will be chosen at random to be cast, where N is the current damage of the spell (stacks with RANDOM_DAMAGE flag) and the message of the spell cast by this spell will also be displayed.  If this spell's message is not wanted to be displayed, make sure the message is an empty string.
| `RANDOM_TARGET` | Forces the spell to choose a random valid target within range instead of the caster choosing the target. This also affects extra_effects.
| `RANDOM_DURATION` | picks random number between min+increment*level and max instead of normal behavior
| `RANDOM_DAMAGE` | picks random number between min+increment*level and max instead of normal behavior
| `RANDOM_AOE` | picks random number between min+increment*level and max instead of normal behavior
| `PERMANENT` | items or creatures spawned with this spell do not disappear and die as normal
| `IGNORE_WALLS` | spell's aoe goes through walls
| `SWAP_POS` | a projectile spell swaps the positions of the caster and target
| `HOSTILE_SUMMON` | summon spell always spawns a hostile monster
| `HOSTILE_50` | summoned monster spawns friendly 50% of the time
| `SILENT` | spell makes no noise at target
| `LOUD` | spell makes extra noise at target
| `VERBAL` | spell makes noise at caster location, mouth encumbrance affects fail %
| `SOMATIC` | arm encumbrance affects fail % and casting time (slightly)
| `NO_HANDS` | hands do not affect spell energy cost
| `NO_LEGS` | legs do not affect casting time
| `CONCENTRATE` | focus affects spell fail %
| `MUTATE_TRAIT` | overrides the mutate spell_effect to use a specific trait_id instead of a category
| `PAIN_NORESIST` | pain altering spells can't be resisted (like with the deadened trait)
| `WITH_CONTAINER` | items spawned with container
| `UNSAFE_TELEPORT` | teleport spell risks killing the caster or others
| `SPAWN_GROUP` | spawn or summon from an item or monster group, instead of individual item/monster ID


### Damage Types

For Spells that have an attack type, these are the available damage types:

* "fire"
* "acid"
* "bash"
* "bio" - internal damage such as poison
* "cold"
* "cut"
* "electric"
* "stab"
* "none" - this damage type goes through armor altogether. it is the default.

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

Currently there is only one way of learning spells that is implemented: learning a spell from an item, through a use_action.  An example is shown below:

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


### Spells in professions and NPC classes

You can add a "spell" member to professions or an NPC class definition like so:

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

### Monster spells examples

Below you can see the proper examples of monster spells - most common types and even some advanced ones:

```
Spell types:

1) Summon:
    {
    "type": "SPELL",
    "id": "test_summon",
    "name": "Summon",
    "description": "Summons the creature specified in 'effect_str'",
    "flags": [ "SILENT", "HOSTILE_SUMMON", "RANDOM_TARGET" ],
    "valid_targets": [ "ground" ],  
	
	"note": " `RANDOM_TARGET` flag+ `ground` in `valid_targets` allows spawning the summoned creature in absolutely any random point on the surface.This is also done to prevent AOE explosion effect, thus hampering the performance of the game",
	
    "min_damage": 1,
    "max_damage": 1,
    "min_range": 3,
    "max_range": 3,
    "effect": "summon",
    "effect_str": "mon_test_monster",
    "min_duration": 6250,
    "max_duration": 6250
  } ;
2) Typical attack:
    {
    "id": "test_attack",
    "type": "SPELL",
    "name": "Ranged Strike",
    "description": "Deals damage to the target with 100% accuracy. Will always apply the status effect specified in 'effect_str'.",
    "valid_targets": [ "ground", "hostile" ],
    "note": "Uses both `ground` and `hostile` in `valid_targets` as well for better efficiency",
    "effect": "projectile_attack",
    "effect_str": "stunned",
    "min_damage": 10,
    "max_damage": 20, 
    "min_range": 4,
    "max_range": 4,
    "base_casting_time": 500,
    "min_duration": 200,
    "max_duration": 300,
    "damage_type": "stab"
  } ;
3) Consecutively cast spells:
    {
    "id": "test_combo",
    "type": "SPELL",
    "name": "Combo Strikes",
    "description": "Upon casting this spell, will also activate the spells specified on the 'extra_effects' in descending order.",
    "flags": [ "SILENT", "RANDOM_DAMAGE", "RANDOM_AOE" ], /"Notice, now `WONDER` flag here"
    "valid_targets": [ "hostile", "ground" ],
    "effect": "projectile_attack",
    "effect_str": "downed",
    "extra_effects": [ { "id": "test_atk1" }, { "id": "test_atk2" } ], 
	
	"note": "If you put two or more spells in `extra_effects` WITHOUT `WONDER` flag, it will simply cast all the spells consecutively - first spell in `extra_effects`, second in `extra_effects`, third in `extra_effects` and etc",
	
    "min_damage": 7,
    "max_damage": 14,
    "min_aoe": 2,
    "max_aoe": 4,
    "min_range": 10,
    "max_range": 10,
    "base_casting_time": 750,
    "min_duration": 325,
    "max_duration": 325,
    "damage_type": "stab"
  } ;
4) Randomly cast spells:
  {
    "id": "test_starter_spell",
    "type": "SPELL",
    "name": "Starter",
    "description": "Upon casting this spell, randomly selects one spell specified in'extra_effects' to cast. This spell's damage counts how many times it will randomly select from the list",
    "flags": [ "SILENT", "WONDER", "RANDOM_DAMAGE" ], 
	
	"note": " `WONDER` flag does wonders here (hehehe, a pun), it works as a dice emulator IF AND ONLY IF there are more than 1 spells specified in `extra_effects` - it basically picks one out of these spells (IT CANNOT FAIL, it will ALWAYS pick ONE RANDDOM spell if there are multiple spells specified in `extra_effects`), the amount of `rolls` of this dice are specified via `min_damage` and `max_damage` with the help of `RANDOM_DAMAGE` flag of course - in this example, this spell will be repeated MINIMUM 3 times and MAXIMUM 5 times thus it will ALWAYS be cast 3-5 times",
	
    "valid_targets": [ "hostile" ],
    "effect": "projectile_attack",
    "extra_effects": [
      { "id": "test_atk1" },
      { "id": "test_atk2" },
      { "id": "test_atk3" },
      { "id": "test_atk4" },
      { "id": "test_atk5" },
      { "id": "test_atk6" }
    ],
    "min_damage": 3,
    "max_damage": 5,
    "min_range": 10,
    "max_range": 10
  } ;
5) Repeatedly cast same spell:
	{
    "type": "SPELL",
    "id": "test_attack_repeat",
    "name": "a spell",
    "description": "Upon casting this spell it will repeat the spell specified in `extra_effects` - the amount of repetitions is the interval `min_damage`-`max_damage` ",
    "extra_effects": [ { "id": "test_attack" } ],
    "flags": [ "SILENT", "WONDER", "RANDOM_DAMAGE" ],
	
	"note": "Notice how we have `WONDER`, `RANDOM_DAMAGE` combo again - we have `min_damage` set to 5, `max_damage` set to 7 so that means that the `dice` will `roll` 5-7 times, BUT THIS TIME we have ONLY ONE SPELL in `extra_effects`, considering that `WONDER` has 100% chance of picking a spell if there is only 1 spell in `extra_effets`, it will SIMPLY REPEAT CASTING THE SPELL 5-7 times, oh and `valid_targets` is [ `hostile` ], though it's just something to note",
	
    "valid_targets": [ "hostile" ],
    "effect": "target_attack",
    "effect_str": "target_message",
    "min_damage": 5,
    "max_damage": 7,
    "min_range": 10,
    "max_range": 10,
    "min_duration": 1,
    "max_duration": 1
  } ;
6) Nested spells with proper notice about casted spell's result:
    {
    "id": "test_note",
    "type": "SPELL",
    "name": "Note",
    "description": "This spell applies a harmless status effect to notify the player about the spell that the user has casted.",
    "flags": [ "SILENT" ],
    "valid_targets": [ "hostile" ],
    "effect": "target_attack",
    "extra_effects": [ { "id": "test_atk1" } ],
    "effect_str": "eff_test_note",
    
    "note": "You need to make a new status effect with `apply_message` which describes the casted spell to the player, then insert it to the `effect_str` of the spell",
    
    "min_aoe": 69,
    "max_aoe": 69,
    "min_duration": 1,
    "max_duration": 1
  } ;
7) "hit_self" use in spells:
	{
    "id": "test_attack_note",
    "type": "SPELL",
    "name": "a note",
    "description": "This spell applies a harmless status effect to notify the player about the spell that the user has casted.",
    "flags": [ "SILENT" ],
    "valid_targets": [ "hostile" ],
    "effect": "target_attack",
    "extra_effects": [ { "id": "sacrifice_spell", "hit_self": true }, { "id": "test_attack" } ],
    "effect_str": "eff_test_note",
	
	"note": "Look at the code above - here we have to use one spell on the monster itself and the other on hostile target aka player or hostile faction monsters, in order to make the monster cast some spell on itself - you must specify the `id` of whatever spell you're using and write near it in the same brackets `hit_self`: true, the second spell will affect only the hostile to monster target aka the player or hostile faction monster
	NOTE: This is used ONLY if we are dealing with 2 or more spells which separately affect monster itself and the hostile to monster being - IF AND ONLY IF you want the monster to use spell on ITSELF ONLY, you don't need to use `hit_self` but only specify `valid_targets` : [ `self` ], if you want the monster to affect hostile creature then it's `valid_targets` : [ `hostile` ] ",
	
    "min_aoe": 69,
    "max_aoe": 69,
    "min_duration": 1,
    "max_duration": 1
  } ;
8) Monster transformation upon death spell:
	{
    "type": "SPELL",
    "id": "test_summon",
    "name": "Summon",
    "description": "Summons the creature specified in 'effect_str' ",
    "flags": [ "SILENT", "PERMANENT", "HOSTILE_SUMMON" ],
    "valid_targets": [ "ground" ],
    "min_damage": 1,
    "max_damage": 1,
    "effect": "summon",
    "effect_str": "mon_test_monster",
	
	"note": "Provides foundation for the new monster upon the death of the old one"
	
  },
  {
    "type": "SPELL",
    "id": "test_upon_death_summon",
    "name": "a spell",
    "description": "Summons a new monster on the place of old monster, instantly killing the old monster sort of doing an instant evolution type of a spell",
    "extra_effects": [ { "id": "sacrifice_spell", "hit_self": true }, { "id": "test_summon", "hit_self": true } ],
    "flags": [ "SILENT" ],
    "valid_targets": [ "ally", "hostile", "ground" ],
	
	"note": "This spell is very interesting - the trick is that we use two spells - one - `sacrifice_spell`+`hit_self` (as explained in 7) ) will kill the old monster, after that due to consecutive nature of this type of spell casting (as explained in 3) ) and thanks to the combination of summon spell above+`hit_self` (first spell in 8), `test_summon` ), it will successfully first kill the original monster and spawn the new one on it's place",
	
    "effect": "target_attack",
    "min_range": 1,
    "max_range": 1
  } ;
9) Targeting in more peculiar cases like 2 spells - one of which is a buff for monster and other is targeting against hostile to monster creatures:
	{
    "type": "SPELL",
    "id": "mon_test_monster_speed_buff",
    "name": "Monster speed buff",
    "description": "Increases the movement speed of a certain monster",
    "flags": [ "SILENT" ],
    "valid_targets": [ "ground", "ally" ],
    "effect": "target_attack",
    "effect_str": "mon_test_buff",
	"note": "The foundation for the buff of the monster - this one increases it's movement speed",
    "min_duration": 1500,
    "max_duration": 1500
  },
  {
    "type": "SPELL",
    "id": "mon_test_monster_speed_buff_target",
    "name": "a spell",
    "description": "Applies speed buff to the monster and makes sure that the monster targets all the hostile creatures properly after that",
    "extra_effects": [
      {
        "id": "mon_test_monster_speed_buff",
        "hit_self": true,
        "note": " `hit_self`, so it forces the monster to cast the spell with 0 range value to itself, receiving (de)buff, damage or healing spell effect."
      }
    ],
    "flags": [ "SILENT" ],
    "valid_targets": [ "ground", "hostile" ], 
	
	"note":  "Once again we're using 'hit_self' here just like in 7) for trivial and obvious reasons (we don't want to use for some reason 'valid_targets': [ 'self' ] in the first spell), now for targeting `valid_targets`: [ `ground`, `hostile` ] is the MOST IMPORTANT LINE here - because the first spell does not specify WHO the monster is buffing up against, but the second spell (`mon_test_monster_speed_buff_target`) clearly specifies with it's `valid_targets` line WHO the monster is going to chase",
    
    "effect": "target_attack",
    "effect_str": "target_message",
    "min_range": 69,
    "max_range": 69,
    "min_duration": 1,
    "max_duration": 1
  }
```

### Enchantments
| Identifier                  | Description
|---                          |---
| `id`                        | Unique ID. Must be one continuous word, use underscores if necessary.
| `has`                       | How an enchantment determines if it is in the right location in order to qualify for being active. "WIELD" - when wielded in your hand * "WORN" - when worn as armor * "HELD" - when in your inventory
| `condition`                 | How an enchantment determines if you are in the right environments in order for the enchantment to qualify for being active. * "ALWAYS" - Always and forevermore * "UNDERGROUND" - When the owner of the item is below Z-level 0 * "UNDERWATER" - When the owner is in swimmable terrain
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

Effects for the item that has the enchantment:

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
