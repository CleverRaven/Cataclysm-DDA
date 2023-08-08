
# Effect On Condition

An effect_on_condition is an object allowing the combination of dialog conditions and effects with their usage outside of a dialog.  When invoked, they will test their condition; on a pass, they will cause their effect. They can be activated automatically with any given frequency.  (Note: effect_on_conditions use the npc dialog conditions and effects syntax, which allows checking related to, or targeting an effect at, an npc (for example: `npc_has_trait`).  Using these commands in an effect_on_condition is not supported.)

## Fields

| Identifier            | Type      | Description |
|--------------------- | --------- | ----------- |
|`recurrence`          | int or variable object or array | The effect_on_condition is automatically invoked (activated) with this many seconds in-between. If it is an object it must have strings `name`, `type`, and `context`. `default` can be either an int or a string describing a time span. `global` is an optional bool (default false), if it is true the variable used will always be from the player character rather than the target of the dialog.  If it is an array it must have two values which are either ints or varible_objects.
|`condition`           | condition  | The condition(s) under which this effect_on_condition, upon activation, will cause its effect.  See the "Dialogue conditions" section of [NPCs](NPCs.md) for the full syntax.
| `deactivate_condition`| condition  | *optional* When an effect_on_condition is automatically activated (invoked) and fails its condition(s), `deactivate_condition` will be tested if it exists and there is no `false_effect` entry.  If it returns true, this effect_on_condition will no longer be invoked automatically every `recurrence` seconds.  Whenever the player/npc gains/loses a trait or bionic all deactivated effect_on_conditions will have `deactivate_condition` run; on a return of false, the effect_on_condition will start being run again.  This is to allow adding effect_on_conditions for specific traits or bionics that don't waste time running when you don't have the target bionic/trait.  See the "Dialogue conditions" section of [NPCs](NPCs.md) for the full syntax.
| `required_event`      | cata_event | The event that when it triggers, this EOC does as well. Only relevant for an EVENT type EOC.
| `effect`              | effect     | The effect(s) caused if `condition` returns true upon activation.  See the "Dialogue Effects" section of [NPCs](NPCs.md) for the full syntax.
| `false_effect`        | effect     | The effect(s) caused if `condition` returns false upon activation.  See the "Dialogue Effects" section of [NPCs](NPCs.md) for the full syntax.
| `global`              | bool       | If this is true, this recurring eoc will be run on the player and every npc from a global queue.  Deactivate conditions will work based on the avatar. If it is false the avatar and every character will have their own copy and their own deactivated list. Defaults to false.
| `run_for_npcs`        | bool       | Can only be true if global is true. If false the EOC will only be run against the avatar. If true the eoc will be run against the avatar and all npcs.  Defaults to false.
| `EOC_TYPE`            | string     | Can be one of `ACTIVATION`, `RECURRING`, `SCENARIO_SPECIFIC`, `AVATAR_DEATH`, `NPC_DEATH`, `OM_MOVE`, `PREVENT_DEATH`, `EVENT` (see details below). It defaults to `ACTIVATION` unless `recurrence` is provided in which case it defaults to `RECURRING`.

 ### EOC types

`EOC_TYPE` can be any of:

* `ACTIVATION` - activated manually.
* `RECURRING` - activated automatically on schedule (see `recurrence`)
* `SCENARIO_SPECIFIC` - automatically invoked once on scenario start.
* `AVATAR_DEATH` - automatically invoked whenever the current avatar dies (it will be run with the avatar as `u`), if after it the player is no longer dead they will not die, if there are multiple EOCs they all be run until the player is not dead.
* `NPC_DEATH` - EOCs can only be assigned to run on the death of an npc, in which case u will be the dying npc and npc will be the killer.
* `OM_MOVE` - EOCs trigger when the player moves overmap tiles
* `PREVENT_DEATH` - whenever the current avatar dies it will be run with the avatar as `u`, if after it the player is no longer dead they will not die, if there are multiple they all be run until the player is not dead.
* `EVENT` - EOCs trigger when a specific event given by "required_event" takes place. 


 ### Reactivation Support
Important to remember that **reactivated EOCs currently lose all context variables and conditions**. Fixing this is a desired feature.

## Examples:
```JSON
  {
    "type": "effect_on_condition",
    "id": "test_deactivate",
    "recurrence": 1,
    "condition": { "u_has_trait": "SPIRITUAL" },
    "deactivate_condition": { "not":{ "u_has_trait": "SPIRITUAL" } },
    "effect": { "u_add_effect": "infection", "duration": 1 }
  },
  {
    "type": "effect_on_condition",
    "id": "test_stats",
    "recurrence": [ 1, 10 ],
    "condition": { "not": { "u_has_strength": 7 } },
    "effect": { "u_add_effect": "infection", "duration": 1 }
  }
```

# Reusable EOCs:
The code base supports the use of reusable EOCs, you can use these to get guaranteed effects by passing in specific variables. The codebase supports the following:

EOC Name | Description | Variables |
--------------------- | --------- | ----------- |
EOC_RandEnc | Spawns a random encounter at the specified `omt` with mapgen update `map_update` that is later removed with `map_removal`. It has a 1 in `chance` chance of happening and can only occur after `days_till_spawn`. Can optionally only happen if `random_enc_condition` is true | `map_update`: a mapgen update ID <br/> `omt`: overmap tile ID where this happens <br/> `map_removal`: a mapgen update ID <br/> `chance`: an integer <br/> `days_till_spawn`: an integer <br/> `random_enc_condition`: a set condition
# EVENT EOCs:
EVENT EOCs trigger on in game events specified in the event_type enum in `event.h`. When an EVENT EOC triggers it tries to perform the EOC on the NPC that is the focus of the event and if it cannot determine one, triggers on the avatar. So any cata_event that has a field for "avatar_id", "character", "attacker", "killer", "npc" will potentially resolve to another npc rather than the avatar, based on who the event triggers for.

## Event Context Vars:
Every event EOC passes context vars with each of their key value pairs that the event has in C++.  They are all converted to strings when made context variables but their original types are included for reference below.  At some point this documentation may go out of sync, The up to date info for each event can be found in event.h

## Event EOC Types:

| EVENT            | Description | Context Variables |
| --------------------- | --------- | ----------- |
| activates_artifact | Triggers when the player activates an artifact | { "character", `character_id` }, { "item_name", `string` }, |
| activates_mininuke | Triggers when any character arms a mininuke | { "character", `character_id` } |
| administers_mutagen |  | { "character", `character_id` }, { "technique", `mutagen_technique` }, |
| angers_amigara_horrors | Triggers when amigara horrors are spawned as part of a mine finale | NONE |
| avatar_enters_omt |  | { "pos", `tripoint` }, { "oter_id", `oter_id` }, |
| avatar_moves |  | { "mount", `mtype_id` }, { "terrain", `ter_id` }, { "movement_mode", `move_mode_id` }, { "underwater", `bool` }, { "z", `int` }, |
| avatar_dies |  | NONE |
| awakes_dark_wyrms | Triggers when `pedestal_wyrm` examine action is used | NONE |
| becomes_wanted | Triggered when copbots/riot bots are spawned as part of a timed event after mon/char is photo'd by eyebot | { "character", `character_id` } |
| broken_bone | Triggered when any body part reaches 0 hp | { "character", `character_id` }, { "part", `body_part` }, |
| broken_bone_mends | Triggered when `mending` effect is removed by expiry (Character::mend) | { "character", `character_id` }, { "part", `body_part` }, |
| buries_corpse | Triggers when item with flag CORPSE is located on same tile as construction with post-special `done_grave` is completed | { "character", `character_id` }, { "corpse_type", `mtype_id` }, { "corpse_name", `string` }, |
| causes_resonance_cascade | Triggers when resonance cascade option is activated via "old lab" finale's computer | NONE |
| character_casts_spell |  | { "character", `character_id` }, { "spell", `spell_id` }, { "difficulty", `int` }, { "cost", `int` }, { "cast_time", `int` }, { "damage", `int` }, |
| character_consumes_item |  | { "character", `character_id` }, { "itype", `itype_id` }, |
| character_eats_item |  | { "character", `character_id` }, { "itype", `itype_id` }, |
| character_finished_activity | Triggered when character finished or chanceled activity | { "character", `character_id` }, { "activity", `activity_id` }, { "canceled", `bool` } |
| character_forgets_spell |  | { "character", `character_id` }, { "spell", `spell_id` } |
| character_gains_effect |  | { "character", `character_id` }, { "effect", `efftype_id` }, |
| character_gets_headshot |  | { "character", `character_id` } |
| character_heals_damage |  | { "character", `character_id` }, { "damage", `int` }, |
| character_kills_character |  | { "killer", `character_id` }, { "victim", `character_id` }, { "victim_name", `string` }, |
| character_kills_monster |  | { "killer", `character_id` }, { "victim_type", `mtype_id` }, |
| character_learns_spell |  | { "character", `character_id` }, { "spell", `spell_id` } |
| character_loses_effect |  | { "character", `character_id` }, { "effect", `efftype_id` }, |
| character_melee_attacks_character |  | { "attacker", `character_id` }, { "weapon", `itype_id` }, { "hits", `bool` }, { "victim", `character_id` }, { "victim_name", `string` }, |
| character_melee_attacks_monster | | { "attacker", `character_id` }, { "weapon", `itype_id` }, { "hits", `bool` }, { "victim_type", `mtype_id` },|
| character_ranged_attacks_character | |  { "attacker", `character_id` },  { "weapon", `itype_id` }, { "victim", `character_id` }, { "victim_name", `string` }, |
| character_ranged_attacks_monster | | { "attacker", `character_id` }, { "weapon", `itype_id` }, { "victim_type", `mtype_id` }, |
| character_smashes_tile | | { "character", `character_id` },  { "terrain", `ter_str_id` },  { "furniture", `furn_str_id` }, |
| character_starts_activity | Triggered when character starts or resumes activity | { "character", `character_id` }, { "activity", `activity_id` }, { "resume", `bool` } |
| character_takes_damage | | { "character", `character_id` }, { "damage", `int` }, |
| character_triggers_trap | | { "character", `character_id` }, { "trap", `trap_str_id` }, |
| character_wakes_up | | { "character", `character_id` }, |
| character_wields_item | | { "character", `character_id` }, { "itype", `itype_id` }, |
| character_wears_item | | { "character", `character_id` }, { "itype", `itype_id` }, |
| consumes_marloss_item | | { "character", `character_id` }, { "itype", `itype_id` }, |
| crosses_marloss_threshold | | { "character", `character_id` } |
| crosses_mutation_threshold | | { "character", `character_id` }, { "category", `mutation_category_id` }, |
| crosses_mycus_threshold | | { "character", `character_id` } |
| cuts_tree | | { "character", `character_id` } |
| dermatik_eggs_hatch | | { "character", `character_id` } |
| dermatik_eggs_injected | | { "character", `character_id` } |
| destroys_triffid_grove | Triggered *only* via spell with effect_str `ROOTS_DIE` (currently via death spell of triffid heart) | NONE |
| dies_from_asthma_attack | | { "character", `character_id` } |
| dies_from_drug_overdose | | { "character", `character_id` }, { "effect", `efftype_id` }, |
| dies_from_bleeding | | { "character", `character_id` }  |
| dies_from_hypovolemia | | { "character", `character_id` }  |
| dies_from_redcells_loss | | { "character", `character_id` }  |
| dies_of_infection | | { "character", `character_id` }  |
| dies_of_starvation | | { "character", `character_id` }  |
| dies_of_thirst | | { "character", `character_id` }  |
| digs_into_lava | | NONE  |
| disarms_nuke | Triggered via disarm missile computer action in missile silo special | NONE  |
| eats_sewage | Triggered via use action `SEWAGE` | NONE  |
| evolves_mutation | | { "character", `character_id` }, { "from_trait", `trait_id` }, { "to_trait", `trait_id` }, |
| exhumes_grave | Triggers when construction with post-special `done_dig_grave` or `done_dig_grave_nospawn` is completed | { "character", `character_id` } |
| fails_to_install_cbm | | { "character", `character_id` }, { "bionic", `bionic_id` }, |
| fails_to_remove_cbm | | { "character", `character_id` }, { "bionic", `bionic_id` }, |
| falls_asleep_from_exhaustion | | { "character", `character_id` } |
| fuel_tank_explodes | Triggers when vehicle part (that is watertight container/magazine with magazine pocket/or is a reactor) is sufficiently damaged | { "vehicle_name", `string` }, |
| gains_addiction | |  { "character", `character_id` }, { "add_type", `addiction_id` }, |
| gains_mutation | |  { "character", `character_id` }, { "trait", `trait_id` }, |
| gains_skill_level | | { "character", `character_id` }, { "skill", `skill_id` }, { "new_level", `int` }, |
| game_avatar_death | Triggers during bury screen with ASCII grave art is displayed (when avatar dies, obviously) | { "avatar_id", `character_id` }, { "avatar_name", `string` }, { "avatar_is_male", `bool` }, { "is_suicide", `bool` }, { "last_words", `string` }, |
| game_avatar_new | Triggers when new character is controlled and during new game character initialization  | { "is_new_game", `bool` }, { "is_debug", `bool` }, { "avatar_id", `character_id` }, { "avatar_name", `string` }, { "avatar_is_male", `bool` }, { "avatar_profession", `profession_id` }, { "avatar_custom_profession", `string` }, |
| game_load | Triggers only when loading a saved game (not a new game!) | { "cdda_version", `string` }, |
| game_begin | Triggered during game load and new game start | { "cdda_version", `string` }, |
| game_over | Triggers after fully accepting death, epilogues etc have played (probably not useable for eoc purposes?) | { "total_time_played", `chrono_seconds` }, |
| game_save | | { "time_since_load", `chrono_seconds` }, { "total_time_played", `chrono_seconds` }, |
| game_start | Triggered only during new game character initialization | { "game_version", `string` }, |
| installs_cbm | | { "character", `character_id` }, { "bionic", `bionic_id` }, |
| installs_faulty_cbm | | { "character", `character_id` }, { "bionic", `bionic_id` }, |
| learns_martial_art | |  { "character", `character_id` }, { "martial_art", `matype_id` }, |
| loses_addiction | | { "character", `character_id` }, { "add_type", `addiction_id` }, |
| npc_becomes_hostile | Triggers when NPC's attitude is set to `NPCATT_KILL` via dialogue effect `hostile` | { "npc", `character_id` }, { "npc_name", `string` }, |
| opens_portal | Triggers when TOGGLE PORTAL option is activated via ("old lab" finale's?) computer | NONE |
| opens_spellbook | Triggers when player opens the spell menu OR when NPC evaluates spell as best weapon(in preparation to use it) | { "character", `character_id` } |
| opens_temple | Triggers when `pedestal_temple` examine action is used to consume a petrified eye | NONE |
| player_fails_conduct | | { "conduct", `achievement_id` }, { "achievements_enabled", `bool` }, |
| player_gets_achievement | | { "achievement", `achievement_id` }, { "achievements_enabled", `bool` }, |
| player_levels_spell | | { "character", `character_id` }, { "spell", `spell_id` }, { "new_level", `int` }, |
| reads_book | | { "character", `character_id` } |
| releases_subspace_specimens | Triggers when Release Specimens option is activated via ("old lab" finale's?) computer | NONE |
| removes_cbm | |  { "character", `character_id` }, { "bionic", `bionic_id` }, |
| seals_hazardous_material_sarcophagus | Triggers via `srcf_seal_order` computer action | NONE |
| spellcasting_finish | | { "character", `character_id` }, { "spell", `spell_id` }, { "school", `trait_id` }  |
| telefrags_creature | (Unimplemented) | { "character", `character_id` }, { "victim_name", `string` }, |
| teleglow_teleports | Triggers when character(only avatar is actually eligible) is teleported due to teleglow effect | { "character", `character_id` } |
| teleports_into_wall | Triggers when character(only avatar is actually eligible) is teleported into wall | { "character", `character_id` }, { "obstacle_name", `string` }, |
| terminates_subspace_specimens | Triggers when Terminate Specimens option is activated via ("old lab" finale's?) computer | NONE |
| throws_up | | { "character", `character_id` } |
| triggers_alarm | Triggers when alarm is sounded as a failure state of hacking, prying, using a computer, or (if player is sufficiently close)damaged terrain with ALARMED flag | { "character", `character_id` } | 
| uses_debug_menu | | { "debug_menu_option", `debug_menu_index` }, | 
| u_var_changed | | { "var", `string` }, { "value", `string` }, |
| vehicle_moves | | { "avatar_on_board", `bool` }, { "avatar_is_driving", `bool` }, { "avatar_remote_control", `bool` }, { "is_flying_aircraft", `bool` }, { "is_floating_watercraft", `bool` }, { "is_on_rails", `bool` }, { "is_falling", `bool` }, { "is_sinking", `bool` }, { "is_skidding", `bool` }, { "velocity", `int` }, // vehicle current velocity, mph * 100 { "z", `int` }, |

## Context Variables For Other EOCs
Other EOCs have some variables as well that they have access to, they are as follows:

| EOC            | Context Variables |
| --------------------- | ----------- |
| mutation: "activated_eocs" | { "this", `mutation_id` } |
| mutation: "processed_eocs" | { "this", `mutation_id` } |
| mutation: "deactivated_eocs" | { "this", `mutation_id` } |
| damage_type: "ondamage_eocs" | { "bp", `bodypart_id` }, { "damage_taken", `double` damage the character will take post mitigation }, { "total_damage", `double` damage pre mitigation } |


## Character effects


#### `u_mutate`, `npc_mutate`

Your character or the NPC will attempt to mutate; used in mutation system, for other purposes it's better to use [`u_add_trait`](#`u_add_trait`, `npc_add_trait`)

| Syntax | Optionality | Value  | Info |
| --- | --- | --- | --- | 
| "u_mutate" / "npc_mutate" | **mandatory** | int, float or [variable object](#variable-object) | one in `int` chance of using the highest category, with 0 never using the highest category |
| "use_vitamins" | optional | boolean | default true; if true, mutation require vitamins to work | 

##### Valid talkers:

| Avatar | Character | NPC | Monster |  Furniture | Item |
| ------ | --------- | --------- | ---- | ------- | --- | 
| ✔️ | ✔️ | ✔️ | ❌ | ❌ | ❌ |

##### Examples
```json
{ "u_mutate": 0 }
```

```json
{ "npc_mutate": { "math": [ "1+1" ] }, "use_vitamins": false }
```


#### `u_mutate_category`, `npc_mutate_category`

Similar to `u_mutate` but takes category as a parameter and guarantees mutation.

| Syntax | Optionality | Value  | Info |
| --- | --- | --- | --- | 
| "u_mutate_category" / "npc_mutate_category" | **mandatory** | string or [variable object](#variable-object) | mutation category |
| "use_vitamins" | optional | boolean | same as in `u_mutate` | 

##### Examples

```json
{ "u_mutate_category": "PLANT" }
```

```
{ "u_mutate_category": { "global_val": "next_mutation" }
```


#### `u_add_effect`, `npc_add_effect`

Some effect would be applied on you or NPC

| Syntax | Optionality | Value  | Info |
| --- | --- | --- | --- | 
| "u_add_effect" / "npc_add_effect" | **mandatory** | string or [variable object](#variable-object) | id of effect to give |
| "duration" | optional | int, duration or [variable object](#variable-object) | 0 by default; length of the effect; both int (`"duration": 60`), and duration string (`"duration": "1 m"`) works; `PERMANENT` can be used to give a permanent effect | 
| "target_part" | optional | string or [variable object](#variable-object) | default is "whole body"; if used, only specified body part would be used. `RANDOM` can be used to pick a random body part | 
| "intensity" | optional | int, float or [variable object](#variable-object) | default 0; intensity of the effect | 
| "force_bool" | optional | boolean | default false; if true, all immunities would be ignored | 

##### Valid talkers:

| Avatar | Character | NPC | Monster |  Furniture | Item |
| ------ | --------- | --------- | ---- | ------- | --- | 
| ✔️ | ✔️ | ✔️ | ✔️ | ❌ | ❌ |

##### Examples
Apply effect `drunk` for 4.5 hours:
```json
{ "u_add_effect": "drunk", "duration": "270 minutes" }
```

Apply effect `fungus` of intensity 1, permanently, on random body part:
```json
{ "u_add_effect": "fungus", "intensity": 1, "duration": "PERMANENT", "target_part": "RANDOM" }
```

Apply effect `poison`, of [your strength value] intensity, for [random number between 0 and 10, multiplied on player's pain value] seconds, onto body part, stored in `body_part_to_poison` context value, ignoring player's immunity:
```json
{
  "u_add_effect": "poison",
  "intensity": { "math": [ "u_val(strength)" ] },
  "duration": { "math": [ "rand(10) * u_pain()" ] },
  "target_part": { "context_val": "body_part_to_poison" },
  "force_bool": true
}
```


#### `u_add_bionic`, `npc_add_bionic`
You or NPC would have some bionic installed

| Syntax | Optionality | Value  | Info |
| --- | --- | --- | --- | 
| "u_add_bionic"/ "npc_add_bionic" | **mandatory** | string or [variable object](#variable-object) | Your character or the NPC will gain the bionic; Only one bionic per effect | 

##### Valid talkers:

| Avatar | Character | NPC | Monster |  Furniture | Item |
| ------ | --------- | --------- | ---- | ------- | --- | 
| ✔️ | ✔️ | ✔️ | ❌ | ❌ | ❌ |

##### Examples
Install 1 `bio_power_storage` onto your character:
```json
{ "u_add_bionic": "bio_power_storage" }
```

Install 1 bionic, delivered from `bionic_id` context value, onto your character:
```json
{ "u_add_bionic": { "context_val": "bionic_id" } }
```


#### `u_lose_bionic`, `npc_lose_bionic`
You or NPC would have some bionic uninstalled from your body

| Syntax | Optionality | Value  | Info |
| --- | --- | --- | --- | 
| "u_lose_bionic" / "npc_lose_bionic"  | **mandatory** | string or [variable object](#variable-object) | Your character or the NPC will lose the bionic | 

##### Valid talkers:

| Avatar | Character | NPC | Monster |  Furniture | Item |
| ------ | --------- | --------- | ---- | ------- | --- | 
| ✔️ | ✔️ | ✔️ | ❌ | ❌ | ❌ |

##### Examples
Uninstall 1 `bio_power_storage` from your character:
```json
{ "u_lose_bionic": "bio_power_storage" }
```

Uninstall 1 bionic, delivered from `bionic_id` context value, onto your character:
```json
{ "u_lose_bionic": { "context_val": "bionic_id" } }
```


#### `u_add_trait`, `npc_add_trait`
Give character or NPC some mutation/trait 

| Syntax | Optionality | Value  | Info |
| --- | --- | --- | --- | 
| "u_add_trait" / "npc_add_trait" | **mandatory** | string or [variable object](#variable-object) | id of trait that should be given |

##### Valid talkers:

| Avatar | Character | NPC | Monster |  Furniture | Item |
| ------ | --------- | --------- | ---- | ------- | --- | 
| ✔️ | ✔️ | ✔️ | ❌ | ❌ | ❌ |

##### Examples
Adds `TELEPATH` trait to the character:
```json
{ "u_add_trait": "TELEPATH" }
```

Adds trait, stored in `trait_id` context value, to the character:
```json
{ "u_add_trait": { "context_val": "trait_id" } }
```


#### `u_lose_effect`, `npc_effect`
Remove effect from character or NPC, if it has one

| Syntax | Optionality | Value  | Info |
| --- | --- | --- | --- | 
| "u_lose_effect" / "npc_lose_effect" | **mandatory** | string or [variable object](#variable-object) | id of effect to be removed; if character or NPC has no such effect, nothing happens |

##### Valid talkers:

| Avatar | Character | NPC | Monster |  Furniture | Item |
| ------ | --------- | --------- | ---- | ------- | --- | 
| ✔️ | ✔️ | ✔️ | ✔️ | ❌ | ❌ |

##### Examples
Removes `infection` effect from player:
```json
{ "u_lose_effect": "infection" }
```

Removes effect, stored in `effect_id` context value, from the player:
```json
{ "u_lose_effect": { "context_val": "effect_id" } }
```


#### `u_lose_trait`, `npc_lose_trait`
Character or NPC got trait or mutation removed, if it has one

| Syntax | Optionality | Value  | Info |
| --- | --- | --- | --- | 
| "u_lose_trait" / "npc_lose_trait" | **mandatory** | string or [variable object](#variable-object) | id of mutation to be removed; if character or NPC has no such mutation, nothing happens |

##### Valid talkers:

| Avatar | Character | NPC | Monster |  Furniture | Item |
| ------ | --------- | --------- | ---- | ------- | --- | 
| ✔️ | ✔️ | ✔️ | ❌ | ❌ | ❌ |

##### Examples

`CHITIN` mutation is removed from character:
```json
{ "u_lose_trait": "CHITIN" }
```

mutation, stored in `mutation_id`  context value, is removed from character:
```json
{ "u_lose_trait": { "context_val": "mutation_id" } }
```


#### `u_activate_trait`, `npc_activate_trait`
Your character or the NPC will activate the trait.

| Syntax | Optionality | Value  | Info |
| --- | --- | --- | --- | 
| "u_activate_trait" / "npc_activate_trait" | **mandatory** | string or [variable object](#variable-object) | id of trait/mutation to be activated |

##### Valid talkers:

| Avatar | Character | NPC | Monster |  Furniture | Item |
| ------ | --------- | --------- | ---- | ------- | --- | 
| ✔️ | ✔️ | ✔️ | ❌ | ❌ | ❌ |

##### Examples
`process_mutation` mutation would be activated, which trigger all effect it can cause, including `activated_eocs` inside the mutation
```json
{ "u_activate_trait": "process_mutation" }
```

Deactivate trait, which contained in `this` context value:
```json
{ "u_deactivate_trait": { "context_val": "this" } }
```


#### `u_deactivate_trait`, `npc_deactivate_trait`
Your character or the NPC will deactivate the trait.

| Syntax | Optionality | Value  | Info |
| --- | --- | --- | --- | 
| "u_deactivate_trait" / "u_deactivate_trait" | **mandatory** | string or [variable object](#variable-object) | id of trait/mutation to be deactivated |

##### Valid talkers:

| Avatar | Character | NPC | Monster |  Furniture | Item |
| ------ | --------- | --------- | ---- | ------- | --- | 
| ✔️ | ✔️ | ✔️ | ❌ | ❌ | ❌ |

##### Examples
Deactivate `BIOLUM1_active` trait:
```json
{ "u_deactivate_trait": "BIOLUM1_active" }
```

Deactivate trait, which contained in `that` context value:
```json
{ "u_deactivate_trait": { "context_val": "that" } }
```


#### `u_learn_martial_art`, `npc_learn_martial_art`
Your character or the NPC will learn the martial art style.

| Syntax | Optionality | Value  | Info |
| --- | --- | --- | --- | 
| "u_learn_martial_art" / "npc_learn_martial_art" | **mandatory** | string or [variable object](#variable-object) | martial art, that would be learned |

##### Valid talkers:

| Avatar | Character | NPC | Monster |  Furniture | Item |
| ------ | --------- | --------- | ---- | ------- | --- | 
| ✔️ | ✔️ | ✔️ | ❌ | ❌ | ❌ |

##### Examples
The character learn Eskrima 
```json
{ "u_learn_martial_art": "style_eskrima" }
```

Character learn martial art, stored in `ma_id` context value
```json
{ "u_learn_martial_art": { "context_val": "ma_id" }
```


#### `u_forget_martial_art`, `npc_forget_martial_art`
Your character or the NPC will forget the martial art style.

| Syntax | Optionality | Value  | Info |
| --- | --- | --- | --- | 
| "u_forget_martial_art" / "npc_forget_martial_art" | **mandatory** | string or [variable object](#variable-object) | id of martial art to forget |

##### Valid talkers:

| Avatar | Character | NPC | Monster |  Furniture | Item |
| ------ | --------- | --------- | ---- | ------- | --- | 
| ✔️ | ✔️ | ✔️ | ❌ | ❌ | ❌ |

##### Examples
Character forget Eskrima 
```json
{ "u_forget_martial_art": "style_eskrima" }
```

Character forget martial art, stored in `ma_id` context value
```json
{ "u_forget_martial_art": { "context_val": "ma_id" }
```


#### `u_add_var`, `npc_add_var`
Save a personal variable, that you can check later using `u_has_var`, `npc_has_var` or `math` (see [Player or NPC conditions]( #Player_or_NPC_conditions) )

| Syntax | Optionality | Value  | Info |
| --- | --- | --- | --- | 
| "u_add_var" / "npc_add_var" | **mandatory** | string | name of variable, where the value would be stored |
| "value" | **mandatory** | string | value, that would be stored in variable; **incompatible with "possible_values" and "time"** | 
| "possible_values" | **mandatory** | string array | array of values, that could be picked to be stored in variable; **incompatible with "value" and "time"** | 
| "time" | **mandatory** | boolean | default false; if true, the current time would be saved in variable; **incompatible with "value" and "possible_values"** | 
| "type", "context" | optional | string | additional text to describe your variable, can be used in `u_lose_var` or in `math` syntax, as `type`\_`context`\_`variable_name` |  

##### Valid talkers:

| Avatar | Character | NPC | Monster |  Furniture | Item |
| ------ | --------- | --------- | ---- | ------- | --- | 
| ✔️ | ✔️ | ✔️ | ❌ | ❌ | ✔️ |

##### Examples
Saves personal variable `u_met_godco_jeremiah` with `general` type, `meeting` context, and value `yes
```json
{ "u_add_var": "u_met_godco_jeremiah", "type": "general", "context": "meeting", "value": "yes" }
```

Saves personal variable `time_of_last_succession` with value of current time:
```json
{ "u_add_var": "time_of_last_succession", "type": "timer", "time": true }
```

NPC (in this case it's actually item, see Beta Talkers) saves a personal variable `function` with one of four values: `morale`, `focus`, `pain`, or `fatigue` (used in mi-go bio tech to create four different versions of the same item, with different effects, that would be revealed upon activation)
```json
{
  "npc_add_var": "function",
  "type": "mbt",
  "context": "f",
  "possible_values": [ "morale", "focus", "pain", "fatigue" ]
}
```

Old variables, that was created in this way, could be migrated into `math`, using `u_`/`npc_`+`type`+`_`+`context`+`_`+`var`, for the sake of save compatibility between stable releases
For example:
```json
{ "u_add_var": "gunsmith_ammo_ammount", "type": "number", "context": "artisans", "value": "800" }
```
could be moved to:
```json  
[ "u_number_artisans_gunsmith_ammo_amount", "=", "800" ]
```


#### `u_lose_var`, `npc_lose_var`
Your character or the NPC will clear any stored variable that has the same name, `type` and `context`

| Syntax | Optionality | Value  | Info |
| --- | --- | --- | --- | 
| "u_lose_var", "npc_lose_var" | **mandatory** | string | variable to be removed |
| "type", "context" | optional | string | additional text to describe your variable; not mandatory, but required to remove correct variable |

##### Valid talkers:

| Avatar | Character | NPC | Monster |  Furniture | Item |
| ------ | --------- | --------- | ---- | ------- | --- | 
| ✔️ | ✔️ | ✔️ | ❌ | ❌ | ✔️ |

##### Examples

Character remove variable `time_of_last_succession`
```json
{ "u_lose_var": "time_of_last_succession" }
```

Character remove variable `on` of type `bio` and context `blade_electric`
```json
{ "u_lose_var": "on", "type": "bio", "context": "blade_electric" }
```


#### `u_adjust_var`, `npc_adjust_var`
Your character or the NPC will adjust the stored variable by `adjustment`. **Slowly removed from usage in favor of `math`**


| Syntax | Optionality | Value  | Info |
| --- | --- | --- | --- | 
| "u_adjust_var", "npc_adjust_var" | **mandatory** | string | variable to adjust |
| "adjustment" | **mandatory** | int, float or [variable object](#variable-object) | size of adjustment | 
| "type", "context" | optional | string | additional text to describe your variable; not mandatory, but required to adjust correct variable |

##### Valid talkers:

| Avatar | Character | NPC | Monster |  Furniture | Item |
| ------ | --------- | --------- | ---- | ------- | --- | 
| ✔️ | ✔️ | ✔️ | ❌ | ❌ | ❌ |

##### Examples
Increases the variable `mission_1` for 1
```json
{ "u_adjust_var": "mission_1", "type": "mission", "context": "test", "adjustment": 1 }
```

Decreases the variable `mission_2` for 5
```json
{ "u_adjust_var": "mission_2", "type": "mission", "context": "test", "adjustment": -5 }
```

Increases the variable `mission_3` for random amount from 0 to 50
```json
{ "u_adjust_var": "mission_3", "type": "mission", "context": "test", "adjustment": { "math": [ "rand(50)" ] } }
```


#### `set_string_var`
Store string from `set_string_var` in the variable object `target_var`


| Syntax | Optionality | Value  | Info |
| --- | --- | --- | --- | 
| "set_string_var" | **mandatory** | string, [variable object](#variable-object), or array of both | value, that would be put into `target_var` |
| "target_var" | **mandatory** | [variable object](#variable-object) | variable, that accept the value; usually `context_val` | 


##### Valid talkers:

| Avatar | Character | NPC | Monster |  Furniture | Item |
| ------ | --------- | --------- | ---- | ------- | --- | 
| ✔️ | ✔️ | ✔️ | ❌ | ❌ | ❌ |

##### Examples
Replace value of variable `foo` with value `bar`
```json
{ "set_string_var": "bar", "target_var": "foo" }
```

set `trait_id` context value as `perk_holdout_pocket`; further down, `{ "u_add_trait": { "context_val": "trait_id" } }` is used to give this trait
```json
{ "set_string_var": "perk_holdout_pocket", "target_var": { "context_val": "trait_id" } }
```

Replace text in `place_name` variable with one of 5 string, picked randomly; further down, a `"place_override": "place_name"` is used to override the location name with one of this five
```json
{
  "set_string_var": [ "Somewhere", "Nowhere", "Everywhere", "Yesterday", "Tomorrow" ],
  "target_var": { "global_val": "place_name" }
}
```


#### `set_condition`
Create a context value with condition, that you can pass down the next topic or EoC, using `get_condition`. Used, if you need to have dozens of EoCs, and you don't want to copy-paste it's conditions every time (also it's easier to maintain or edit one condition, comparing to two dozens)

| Syntax | Optionality | Value  | Info |
| --- | --- | --- | --- | 
| "set_condition" | **mandatory** | string or [variable object](#variable-object) | id of condition |
| "condition" | **mandatory** | [dialogue condition](#dialogue-conditions) | condition itself | 

##### Valid talkers:

| Avatar | Character | NPC | Monster |  Furniture | Item |
| ------ | --------- | --------- | ---- | ------- | --- | 
| ✔️ | ✔️ | ✔️ | ❌ | ❌ | ❌ |

##### Examples
Save the condition  `season is not winter, and it is a daytime` into `random_enc_condition` variable, then call the EoC `second_test`. Second EoC uses `random_enc_condition` to check and print message
```json
{
  "type": "effect_on_condition",
  "id": "test",
  "effect": [
    { "set_condition": "random_enc_condition", "condition": { "and": [ { "not": { "is_season": "winter" } }, "is_day" ] } },
    { "run_eocs": "second_test" }
  ]
},
{
  "type": "effect_on_condition",
  "id": "second_test",
  "condition": { "get_condition": "random_enc_condition" },
  "effect": [ { "u_message": "Yay, our condition works!" } ]
}
```


#### `u_location_variable`, `npc_location_variable`
Search a specific coordinates of map around `u_`, `npc_` or `target_params` and save them in [variable](#variable-object)

| Syntax | Optionality | Value | Info |
| --- | --- | --- | --- | 
| "u_location_variable" / "npc_location_variable" | **mandatory** | [variable object](#variable-object) | variable, where the location would be saved | 
| "min_radius", "max_radius" | optional | int, float or [variable object](#variable-object) | default 0; radius around the player or NPC, where the location would be searched | 
| "outdoor_only" | optional | boolean | default false; if true, only outdoor values would be picked | 
| "target_params" | optional | assign_mission_target | if used, the search would be performed not from `u_` or `npc_` location, but from `mission_target`. it uses an [assign_mission_target](MISSIONS_JSON.md) syntax | 
| "x_adjust", "y_adjust", "z_adjust" | optional | int, float or [variable object](#variable-object) | add this amount to `x`, `y` or `z` coordinate in the end; `"x_adjust": 2` would save the coordinate with 2 tile shift to the right from targeted | 
| "z_override" | optional | boolean | default is false; if true, instead of adding up to `z` level, override it with absolute value; `"z_adjust": 3` with `"z_override": true` turn the value of `z` to `3` | 
| "terrain" / "furniture" / "trap" / "monster" / "zone" / "npc" | optional | string or [variable object](#variable-object) | if used, search the entity with corresponding id between `target_min_radius` and `target_max_radius`; if empty string is used (e.g. `"monster": ""`), return any entity from the same radius  | 
| "target_min_radius", "target_max_radius" | optional | int, float or [variable object](#variable-object) | default 0, min and max radius for search, if previous field was used | 

##### Valid talkers:

| Avatar | Character | NPC | Monster |  Furniture | Item |
| ------ | --------- | --------- | ---- | ------- | --- | 
| ✔️ | ✔️ | ✔️ | ❌ | ❌ | ❌ |

##### Examples
Saves the current location into `i_am_here` variable:
```json
{ "u_location_variable": { "u_val": "i_am_here" } },
```

Search overmap terrain `afs_crashed_escape_pod` on z-level 0, range 500 overmap tiles, search `t_metal_floor` terrain in this overmap, and save its coordinates into `new_map` variable (`target_params` uses [assign_mission_target](MISSIONS_JSON.md) syntax):
```json
{
  "u_location_variable": { "global_val": "new_map" },
  "target_params": { "om_terrain": "afs_crashed_escape_pod", "search_range": 500, "z": 0 },
  "terrain": "t_metal_floor",
  "target_max_radius": 30,
  "min_radius": 0,
  "max_radius": 0
}
```

Search the map, that contain `house` in it's id on a range 200-1200 overmap tiles, picks random from them, and save its coordinates  into `OM_missionspot` variable:
```json
{
  "u_location_variable": { "global_val": "OM_missionspot" },
  "target_params": {
    "om_terrain": "house",
    "om_terrain_match_type": "CONTAINS",
    "search_range": 1200,
    "min_distance": 200,
    "random": true
  }
}
```


#### `location_variable_adjust`
Allow adjust location value, obtained by `u_location_variable`, and share the same syntax and rules

| Syntax | Optionality | Value | Info |
| --- | --- | --- | --- | 
| "location_variable_adjust" | **mandatory** | [variable object](#variable-object) | variable, where the location would be saved | 
| "x_adjust", "y_adjust", "z_adjust" | optional | int, float or [variable object](#variable-object) | add this amount to `x`, `y` or `z` coordinate in the end; `"x_adjust": 2` would save the coordinate with 2 tile shift to the right from targeted | 
| "z_override" | optional | boolean | default is false; if true, instead of adding up to `z` level, override it with absolute value; `"z_adjust": 3` with `"z_override": true` turn the value of `z` to `3` | 
| "overmap_tile" | optional | boolean | default is false; if true, the adjustments will be made in overmap tiles rather than map tiles | 


##### Valid talkers:

| Avatar | Character | NPC | Monster |  Furniture | Item |
| ------ | --------- | --------- | ---- | ------- | --- | 
| ✔️ | ✔️ | ✔️ | ❌ | ❌ | ❌ |

##### Examples
Move coordinates in `location_var` value one tile to the right
```json
{ "location_variable_adjust": "location_var", "x_adjust":  1 }
```

Move `portal_storm_center` coordinates randomly at 1 overmap tile in any direction, except Z
```json
{
  "location_variable_adjust": { "global_val": "portal_storm_center" },
  "overmap_tile": true,
  "x_adjust": [ -1, 1 ],
  "y_adjust": [ -1, 1 ]
}
```


#### `barber_hair`
Opens a menu allowing the player to choose a new hair style

##### Valid talkers:

| Avatar | Character | NPC | Monster |  Furniture | Item |
| ------ | --------- | --------- | ---- | ------- | --- | 
| ✔️ | ❌ | ❌ | ❌ | ❌ | ❌ |

##### Examples

```json
"barber_hair"
```

```json
{
  "type": "effect_on_condition",
  "id": "test",
  "effect": "barber_hair"
}
```


#### `barber_beard`
Opens a menu allowing the player to choose a new beard style.

##### Valid talkers:

| Avatar | Character | NPC | Monster |  Furniture | Item |
| ------ | --------- | --------- | ---- | ------- | --- | 
| ✔️ | ❌ | ❌ | ❌ | ❌ | ❌ |

##### Examples
```json
"barber_beard"
```

```json
{
  "type": "effect_on_condition",
  "id": "test",
  "effect": "barber_beard"
}
```

```json
{
  "type": "effect_on_condition",
  "id": "test",
  "effect": [ "barber_hair", "barber_beard" ] 
}
```


#### `u_learn_recipe`, `npc_learn_recipe`
Your character or the npc will learn and memorize the recipe

| Syntax | Optionality | Value  | Info |
| --- | --- | --- | --- | 
| "u_learn_recipe" / "npc_learn_recipe" | **mandatory** | string or [variable object](#variable-object) | Recipe, that would be learned |

##### Valid talkers:

| Avatar | Character | NPC | Monster |  Furniture | Item |
| ------ | --------- | --------- | ---- | ------- | --- | 
| ✔️ | ✔️ | ✔️ | ❌ | ❌ | ❌ |

##### Examples
You learn a recipe of cattail_jelly
```json
{ "u_learn_recipe": "cattail_jelly" }
```

You learn a recipe, that was passes by `recipe_id` context value
```json
{ "u_learn_recipe": { "context_val": "recipe_id" } }
```


#### `u_forget_recipe`, `npc_forget_recipe`
Your character or the npc will forget the recipe

| Syntax | Optionality | Value  | Info |
| --- | --- | --- | --- | 
| "u_forget_recipe" / "npc_forget_recipe" | **mandatory** | string or [variable object](#variable-object) | recipe, that would be forgotten |

##### Valid talkers:

| Avatar | Character | NPC | Monster |  Furniture | Item |
| ------ | --------- | --------- | ---- | ------- | --- | 
| ✔️ | ✔️ | ✔️ | ❌ | ❌ | ❌ |

##### Examples
You forget the recipe `inventor_research_base_1`
```json
{ "u_forget_recipe": "inventor_research_base_1" }
```

You forget a recipe, that was passes by `recipe_id` context value
```json
{ "u_forget_recipe": { "context_val": "recipe_id" } }
```


#### `npc_first_topic`
Changes the initial talk_topic of the NPC in all future dialogues.

| Syntax | Optionality | Value  | Info |
| --- | --- | --- | --- | 
| "npc_first_topic" | **mandatory** | string or [variable object](#variable-object)  | topic, that would be used |

##### Valid talkers:

| Avatar | Character | NPC | Monster |  Furniture | Item |
| ------ | --------- | --------- | ---- | ------- | --- | 
| ❌ | ❌ | ✔️ | ❌ | ❌ | ❌ |

##### Examples
Override the initial lighthouse girl topic `TALK_lighthouse_girl_start` with `TALK_lighthouse_girl_safe`
```json
{ "npc_first_topic": "TALK_lighthouse_girl_safe" }
```


#### `u_add_wet`, `npc_add_wet`
Your character or the NPC will be wet as if they were in the rain.


| Syntax | Optionality | Value  | Info |
| --- | --- | --- | --- | 
| "u_add_wet" / "npc_add_wet" | **mandatory** | int, float or [variable object](#variable-object) | How much wetness would be added (in percent) |

##### Valid talkers:

| Avatar | Character | NPC | Monster |  Furniture | Item |
| ------ | --------- | --------- | ---- | ------- | --- | 
| ✔️ | ✔️ | ✔️ | ❌ | ❌ | ❌ |

##### Examples
Makes you 10% wet (whatever that means)
```json
"effect": [ { "u_add_wet": 10 } ]
```


#### `u_make_sound`, `npc_make_sound`
Emit a sound

| Syntax | Optionality | Value  | Info |
| --- | --- | --- | --- | 
| "u_make_sound" / "npc_make_sound" | **mandatory** | string or [variable object](#variable-object) | description of the sound |
| "volume" | **mandatory** | int, float or [variable object](#variable-object) | how loud the sound is (1 unit = 1 tile around the character) | 
| "type" | **mandatory** | string or [variable object](#variable-object) | Type of the sound; Could be one of `background`, `weather`, `music`, `movement`, `speech`, `electronic_speech`, `activity`, `destructive_activity`, `alarm`, `combat`, `alert`, or `order` | 
| "target_var" | optional | [variable object](#variable-object) | if set, the center of the sound would be centered in this variable's coordinates instead of you or NPC | 
| "snippet" | optional | boolean | dafault false; if true, `_make_sound` would use a snippet of provided id instead of a message | 
| "same_snippet" | optional | boolean | default false; if true, it will connect the talker and snippet, and will always provide the same snippet, if used by this talker; require snippets to have id's set | 

##### Valid talkers:

| Avatar | Character | NPC | Monster |  Furniture | Item |
| ------ | --------- | --------- | ---- | ------- | --- | 
| ✔️ | ✔️ | ✔️ | ✔️ | ✔️ | ✔️ |

##### Examples
Generate a sound `Hi there!` 15 tiles around the NPC
```json
{ "npc_make_sound": "Hi there!", "volume": 15, "type": "speech" },
```

Generate a `a high-pitched squeal.` 60 tiles around `grass_transform`
```json
{
  "u_make_sound": "a high-pitched squeal.",
  "target_var": { "global_val": "grass_transform" },
  "volume": 60,
  "type": "alert"
},
```

Would pick a random swear from `<swear>` snippet, and always would be the same (if item call this EoC, it would always have the same swear)
```json
{ "u_make_sound": "<swear>", "snippet": true, "same_snippet": true }
```


#### `u_mod_healthy`, `npc_mod_healthy`
Increases or decreases your healthiness (respond for disease immunity and regeneration).

| Syntax | Optionality | Value  | Info |
| --- | --- | --- | --- | 
| "u_mod_healthy" / "npc_mod_healthy" | **mandatory** | int, float or [variable object](#variable-object) | Amount of health to be added |
| "cap" | optional | int, float or [variable object](#variable-object) | cap for healthiness, beyond which it can't go further | 

##### Valid talkers:

| Avatar | Character | NPC | Monster |  Furniture | Item |
| ------ | --------- | --------- | ---- | ------- | --- | 
| ✔️ | ✔️ | ✔️ | ❌ | ❌ | ❌ |

##### Examples
Your health is decreased by 1, but not smaller than -200
```json
{ "u_mod_healthy": -1, "cap": -200 }
```


#### `u_add_morale`, `npc_add_morale`
Your character or the NPC will gain a morale bonus


| Syntax | Optionality | Value  | Info |
| --- | --- | --- | --- | 
| "u_add_morale" / "npc_add_morale" | **mandatory** | string or [variable object](#variable-object) | `morale_type`, that would be given by effect |
| "bonus" | optional | int, float or [variable object](#variable-object) | default 1; mood bonus or penalty, that would be given by effect; can be stacked up to `max_bonus` cap, but each bonus is lower than previous (e.g. `bonus` of 100 gives mood bonus as 100, 141, 172, 198, 221 and so on) | 
| "max_bonus" | optional | int, float or [variable object](#variable-object) | default false; cap, beyond which mood won't increase or decrease | 
| "duration" | optional | int, duration or [variable object](#variable-object) | default 1 hour; how long the morale effect would last | 
| "decay_start" | optional | int, duration or [variable object](#variable-object) | default 30 min; when the morale effect would start to decay | 
| "capped" | optional | boolean | default false; if true, `bonus` is not decreased when stacked (e.g. `bonus` of 100 gives mood bonus as 100, 200, 300 and so on) |  

##### Valid talkers:

| Avatar | Character | NPC | Monster |  Furniture | Item |
| ------ | --------- | --------- | ---- | ------- | --- | 
| ✔️ | ✔️ | ✔️ | ❌ | ❌ | ❌ |

##### Examples
Gives `morale_off_drugs` thought with +1 mood bonus
```json
{
  "u_add_morale": "morale_off_drugs",
}
```

gives +20 mood `morale_feeling_good` bonus, that can be stacked up to +50, for 4 hours without decay start in 2 hours
```json
{
  "u_add_morale": "morale_feeling_good",
  "bonus": 20,
  "max_bonus": 50,
  "duration": "240 minutes",
  "decay_start": "120 minutes"
}
```


#### `u_lose_morale`, `npc_lose_morale`
Your character or the NPC will lose picked `morale_type`

| Syntax | Optionality | Value  | Info |
| --- | --- | --- | --- | 
| "u_lose_morale" / "npc_lose_morale" | **mandatory** | string or [variable object](#variable-object) | thought to remove |

##### Valid talkers:

| Avatar | Character | NPC | Monster |  Furniture | Item |
| ------ | --------- | --------- | ---- | ------- | --- | 
| ✔️ | ✔️ | ✔️ | ❌ | ❌ | ❌ |

##### Examples
removes `bad_mood` morale from you
```json
{ "u_lose_morale": "bad_mood" }
```

removes morale type, delivered by `morale_id` 
```json
{ "u_lose_morale": { "context_val": "morale_id" } }
```


#### `u_add_faction_trust`
 Your character gains trust with the speaking NPC's faction, which affects which items become available for trading from shopkeepers of that faction. Can be used only in `talk_topic`, as the code relies on the NPC you talk with to obtain info about it's faction

| Syntax | Optionality | Value  | Info |
| --- | --- | --- | --- | 
| "u_add_faction_trust" | **mandatory** | int, float or [variable object](#variable-object) | amount of trust to give or remove |

##### Valid talkers:

| Avatar | Character | NPC | Monster |  Furniture | Item |
| ------ | --------- | --------- | ---- | ------- | --- | 
| ❌ | ❌ | ✔️ | ❌ | ❌ | ❌ |

##### Examples
adds 5 points to faction trust
```json
{ "u_add_faction_trust": 5 }
```

adds [ your strength stat ] amount of faction trust
```json
{ "u_add_faction_trust": { "math": [ "u_val('strength')" ] } }
```


#### `u_lose_faction_trust`
same as `u_add_faction_trust`, not used in favor of `u_add_faction_trust` with negative number

#### `u_message`, `npc_message`
Display a text message in the log

| Syntax | Optionality | Value  | Info |
| --- | --- | --- | --- | 
| "u_message" / "npc_message" | **mandatory** | string or [variable object](#variable-object) | default true |
| "type" | optional | string or [variable object](#variable-object) | default neutral; how the message would be displayed in log (usually means the color); could be any of good (green), neutral (white), bad (red), mixed (purple), warning (yellow), info (blue), debug (appear only if debug mode is on), headshot (purple), critical (yellow), grazing (blue) | 
| "sound" | optional | boolean | default false; if true, shows message only if player is not deaf | 
| "outdoor_only" | optional | boolean | default false; if true, and `sound` is true, the message is harder to hear if you are underground | 
| "snippet" | optional | boolean | default false; if true, the effect instead display a random snippet from `u_message` | 
| "same_snippet" | optional | boolean | default false; if true, and `snippet` is true, it will connect the talker and snippet, and will always provide the same snippet, if used by this talker; require snippets to have id's set | 
| "popup" | optional | boolean | default false; if true, the message would generate a popup with `u_message` | 
| "popup_w_interrupt_query" | optional | boolean | default false; if true, and `popup` is true, the popup will interrupt any activity to send a message | 
| "interrupt_type" | optional | boolean | default is "neutral"; `distraction_type`, that would be used to interrupt, one that used in distraction manager; full list exist inactivity_type.cpp | 

##### Valid talkers:

| Avatar | Character | NPC | Monster |  Furniture | Item |
| ------ | --------- | --------- | ---- | ------- | --- | 
| ✔️ | ✔️ | ✔️ | ✔️ | ✔️ | ✔️ |

##### Examples
Send a red-colored `Bad json! Bad!` message in the log 
```json
{ "u_message": "Bad json! Bad!", "type": "bad" }
```

Print a snippet from `local_files_simple`, and popup it. The snippet is always the same
```json
 { "u_message": "local_files_simple", "snippet": true, "same_snippet": true, "popup": true }
```


#### `u_cast_spell`, `npc_cast_spell`
You or NPC cast a spell. The spell uses fake spell data (ignore `energy_cost`, `energy_source`, `cast_time`, `components`, `difficulty` and `spell_class` fields), and uses additional fields 

| Syntax | Optionality | Value  | Info |
| --- | --- | --- | --- | 
| "u_cast_spell" / "npc_cast_spell" | **mandatory** | [variable object](#variable-object) | information of what spell and how it should be casted |
| "id" | **mandatory** | string or [variable object](#variable-object) | part of `_cast_spell`; define the id of spell to cast | 
| "hit_self" | optional | boolean | part of `_cast_spell`; default false; if true, the spell could affect the caster (either as self damage from AoE spell, or as applying effect for buff spell) | 
| "message" | optional | string or [variable object](#variable-object) | part of `_cast_spell`; message to send when spell is casted | 
| "npc_message" | optional | string or [variable object](#variable-object) | part of `_cast_spell`; message if npc uses | 
| "min_level", "max_level" | optional | int, float or [variable object](#variable-object) | part of `_cast_spell`; level of the spell that would be casted (min level define what the actual spell level would be casted, adding max_level make EoC pick a random level between min and max) | 
| "targeted" | optional | boolean | default false; if true, allow you to aim casted spell, otherwise cast it in random place, like `RANDOM_TARGET` spell flag was used | 
| "true_eocs" | optional | string, [variable object](#variable-object), `effect_on_condition` or range of all of them | if spell was casted successfully, all EoCs from this field would be triggered; | 
| "false_eocs" | optional | string, [variable object](#variable-object), `effect_on_condition` or range of all of them | if spell was not casted successfully, all EoCs from this field would be triggered | 

##### Valid talkers:

| Avatar | Character | NPC | Monster |  Furniture | Item |
| ------ | --------- | --------- | ---- | ------- | --- | 
| ✔️ | ✔️ | ✔️ | ✔️ | ❌ | ❌ |

##### Examples
You cast `spell_1` spell
```json
{ "u_cast_spell": { "id": "spell_1" } }
```

You cast a `spell_boom` spell, that can be aimed, and create message `BOOM!` in the log
```json
{
  "u_cast_spell": { "id": "spell_boom", "message": "BOOM!" },
  "targeted": true
}
```

You cast `spell_healing` spell of 1-6 level, that can hit you, with message
```json
{
  "u_cast_spell": {
    "id": "spell_healing",
    "hit_self": true
    "min_level": 1,
    "max_level": 6,
    "message": "Your flesh is healed!"
  }
}
```

You cast a `this_spell_can_target_only_robots` spell; if it success, `EOC_ROBOT_IS_DEAD` is triggered, otherwise `EOC_NOT_A_ROBOT` is triggered
```json
{
  "u_cast_spell": { "id": "this_spell_can_target_only_robots" },
  "true_eocs": [ "EOC_ROBOT_IS_DEAD" ],
  "false_eocs": [ "EOC_NOT_A_ROBOT" ]
}
```


#### `u_assign_activity`, `npc_assign_activity`

NPC or character will start an activity

| Syntax | Optionality | Value  | Info |
| --- | --- | --- | --- | 
| "u_assign_activity" / "npc_assign_activity" | **mandatory** | string or [variable object](#variable-object), | id of activity to start |
| "duration" | **mandatory** | int, duration or [variable object](#variable-object) | how long the activity would last | 

##### Valid talkers:

| Avatar | Character | NPC | Monster |  Furniture | Item |
| ------ | --------- | --------- | ---- | ------- | --- | 
| ✔️ | ✔️ | ✔️ | ❌ | ❌ | ❌ |

##### Examples

You assign activity `ACT_GAME` for 45 minutes
```json
{ "u_assign_activity": "ACT_GAME", "duration": "45 minutes" }
```


#### `u_teleport`, `npc_teleport`
You or NPC is teleported to `target_var` coordinates

| Syntax | Optionality | Value  | Info |
| --- | --- | --- | --- | 
| "u_teleport", / "npc_teleport" | **mandatory** | [variable object](#variable-object) | location to teleport; should use `target_var`, created previously |
| "success_message" | optional | string or [variable object](#variable-object) | message, that would be printed, if teleportation was sucessful | 
| "fail_message" | optional | string or [variable object](#variable-object) | message, that would be printed, if teleportation was failed, like if coordinates contained creature or impassable obstacle (like wall) | 
| "force" | optional | boolean | default false; if true, teleportation can't fail - any creature, that stand on target coordinates, would be brutally telefragged, and if impassable obstacle occur, the closest point would be picked instead |

##### Valid talkers:

| Avatar | Character | NPC | Monster |  Furniture | Item |
| ------ | --------- | --------- | ---- | ------- | --- | 
| ✔️ | ✔️ | ✔️ | ❌ | ❌ | ❌ |

##### Examples

You teleport to `winch_teleport` coordinates
```json
{ "u_teleport": { "u_val": "winch_teleport" } }
```

You teleport to `grass_place` with message `Yay!`; as `force` boolean is `true`, you can't fail it.
```json
{
  "u_teleport": { "global_val": "grass_place" },
  "success_message": "Yay!",
  "fail_message": "Something is very wrong!",
  "force": true
}
```


#### `u_set_hp`, `npc_set_hp`
HP of you or NPC would be set to some amount

| Syntax | Optionality | Value  | Info |
| --- | --- | --- | --- | 
| "u_set_hp" / "npc_set_hp" | **mandatory** | int, float or [variable object](#variable-object) | amount of HP to set |
| "target_part" | optional | string or [variable object](#variable-object) | default whole body; if used, the HP adjustment would be applied only to this body part | 
| "only_increase" | optional | boolean | default false; if true, the HP could be only increased | 
| "main_only" | optional | boolean | default false; if true, only main body parts would be affected - arms, legs, head, torso etc.; can't be used with `minor_only` | 
| "minor_only" | optional | boolean | default false; if true, only minor parts would be affected - eyes, mouth, hands, foots etc.; can't be used with `main_only` | 
| "max" | optional | boolean | default false; if true, `_set_hp` value would be ignored, and body part would be healed to it's max HP | 

##### Valid talkers:

| Avatar | Character | NPC | Monster |  Furniture | Item |
| ------ | --------- | --------- | ---- | ------- | --- | 
| ✔️ | ✔️ | ✔️ | ❌ | ❌ | ❌ |

##### Examples

HP of whole body would be set to 10
```json
{ "u_set_hp": 10 }
```

Random bodypart would be healed entirely
```json
{ "u_set_hp": 0, "max": true, "target_part": "RANDOM" }
```

You increase the HP of your minor parts to 50, if possible
```json
{ "u_set_hp": 50, "minor_only": true, "only_increase": true }
```

You heal your right leg for 10 HP; in detail, you set the HP of your right leg to be 10 HP bigger than it's current HP; what people could do to not add `u_adjust_hp` XD
```json
{ "u_set_hp": { "math": { "u_hp('leg_r') + 10" } }, "target_part": "leg_r" }
```

#### `u_die`, `npc_die`
You or NPC will instantly die

##### Valid talkers:

| Avatar | Character | NPC | Monster |  Furniture | Item |
| ------ | --------- | --------- | ---- | ------- | --- | 
| ✔️ | ✔️ | ✔️ | ❌ | ❌ | ❌ |

##### Examples

You die
```json
"u_die"
```

You and NPC both die
```json
{
  "type": "effect_on_condition",
  "id": "both_are_ded",
  "effect": [ "u_die", "npc_die" ]
}
```
