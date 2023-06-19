
# effect_on_condition
An effect_on_condition is an object allowing the combination of dialog conditions and effects with their usage outside of a dialog.  When invoked, they will test their condition; on a pass, they will cause their effect. They can be activated automatically with any given frequency.  (Note: effect_on_conditions use the npc dialog conditions and effects syntax, which allows checking related to, or targeting an effect at, an npc (for example: `npc_has_trait`).  Using these commands in an effect_on_condition is not supported.)

## Fields

Identifier            | Type      | Description |
--------------------- | --------- | ----------- |
`recurrence`          | int or variable object or array | The effect_on_condition is automatically invoked (activated) with this many seconds in-between. If it is an object it must have strings `name`, `type`, and `context`. `default` can be either an int or a string describing a time span. `global` is an optional bool (default false), if it is true the variable used will always be from the player character rather than the target of the dialog.  If it is an array it must have two values which are either ints or varible_objects.
`condition`           | condition  | The condition(s) under which this effect_on_condition, upon activation, will cause its effect.  See the "Dialogue conditions" section of [NPCs](NPCs.md) for the full syntax.
`deactivate_condition`| condition  | *optional* When an effect_on_condition is automatically activated (invoked) and fails its condition(s), `deactivate_condition` will be tested if it exists and there is no `false_effect` entry.  If it returns true, this effect_on_condition will no longer be invoked automatically every `recurrence` seconds.  Whenever the player/npc gains/loses a trait or bionic all deactivated effect_on_conditions will have `deactivate_condition` run; on a return of false, the effect_on_condition will start being run again.  This is to allow adding effect_on_conditions for specific traits or bionics that don't waste time running when you don't have the target bionic/trait.  See the "Dialogue conditions" section of [NPCs](NPCs.md) for the full syntax.
`required_event`      | cata_event | The event that when it triggers, this EOC does as well. Only relevant for an EVENT type EOC.
`effect`              | effect     | The effect(s) caused if `condition` returns true upon activation.  See the "Dialogue Effects" section of [NPCs](NPCs.md) for the full syntax.
`false_effect`        | effect     | The effect(s) caused if `condition` returns false upon activation.  See the "Dialogue Effects" section of [NPCs](NPCs.md) for the full syntax.
`global`              | bool       | If this is true, this recurring eoc will be run on the player and every npc from a global queue.  Deactivate conditions will work based on the avatar. If it is false the avatar and every character will have their own copy and their own deactivated list. Defaults to false.
`run_for_npcs`        | bool       | Can only be true if global is true. If false the eoc will only be run against the avatar. If true the eoc will be run against the avatar and all npcs.  Defaults to false.
`EOC_TYPE`            | string     | The effect_on_condition is automatically invoked once on scenario start.

 Can be any of:ACTIVATION, RECURRING, SCENARIO_SPECIFIC, AVATAR_DEATH, NPC_DEATH, OM_MOVE, PREVENT_DEATH, EVENT. It defaults to ACTIVATION unless `recurrence` is provided in which case it defaults to RECURRING.  If it is SCENARIO_SPECIFIC it is automatically invoked once on scenario start. If it is PREVENT_DEATH whenever the current avatar dies it will be run with the avatar as u, if after it the player is no longer dead they will not die, if there are multiple they all be run until the player is not dead. If it is AVATAR_DEATH whenever the current avatar dies it will be run with the avatar as u and the killer as npc. NPC_DEATH eocs can only be assigned to run on the death of an npc, in which case u will be the dying npc and npc will be the killer. OM_MOVE EOCs trigger when the player moves overmap tiles. EVENT EOCs trigger when a specific event given by "required_event" takes place.

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
EVENT            | Description | Context Variables |
--------------------- | --------- | ----------- |
activates_artifact | Triggers when the player activates an artifact | { "character", `character_id` }, { "item_name", `string` }, |
activates_mininuke |  | { "character", `character_id` } |
administers_mutagen |  | { "character", `character_id` }, { "technique", `mutagen_technique` }, |
angers_amigara_horrors |  | NONE |
avatar_enters_omt |  | { "pos", `tripoint` }, { "oter_id", `oter_id` }, |
avatar_moves |  | { "mount", `mtype_id` }, { "terrain", `ter_id` }, { "movement_mode", `move_mode_id` }, { "underwater", `bool` }, { "z", `int` }, |
avatar_dies |  | NONE |
awakes_dark_wyrms |  | NONE |
becomes_wanted |  | { "character", `character_id` } |
broken_bone |  | { "character", `character_id` }, { "part", `body_part` }, |
broken_bone_mends |  | { "character", `character_id` }, { "part", `body_part` }, |
buries_corpse |  | { "character", `character_id` }, { "corpse_type", `mtype_id` }, { "corpse_name", `string` }, |
causes_resonance_cascade |  | NONE |
character_consumes_item |  | { "character", `character_id` }, { "itype", `itype_id` }, |
character_eats_item |  | { "character", `character_id` }, { "itype", `itype_id` }, |
character_forgets_spell |  | { "character", `character_id` }, { "spell", `spell_id` } |
character_gains_effect |  | { "character", `character_id` }, { "effect", `efftype_id` }, |
character_gets_headshot |  | { "character", `character_id` } |
character_heals_damage |  | { "character", `character_id` }, { "damage", `int` }, |
character_kills_character |  | { "killer", `character_id` }, { "victim", `character_id` }, { "victim_name", `string` }, |
character_kills_monster |  | { "killer", `character_id` }, { "victim_type", `mtype_id` }, |
character_learns_spell |  | { "character", `character_id` }, { "spell", `spell_id` } |
character_loses_effect |  | { "character", `character_id` }, { "effect", `efftype_id` }, |
character_melee_attacks_character |  | { "attacker", `character_id` }, { "weapon", `itype_id` }, { "hits", `bool` }, { "victim", `character_id` }, { "victim_name", `string` }, |
character_melee_attacks_monster | | { "attacker", `character_id` }, { "weapon", `itype_id` }, { "hits", `bool` }, { "victim_type", `mtype_id` },|
character_ranged_attacks_character | |  { "attacker", `character_id` },  { "weapon", `itype_id` }, { "victim", `character_id` }, { "victim_name", `string` }, |
character_ranged_attacks_monster | | { "attacker", `character_id` }, { "weapon", `itype_id` }, { "victim_type", `mtype_id` }, |
character_smashes_tile | | { "character", `character_id` },  { "terrain", `ter_str_id` },  { "furniture", `furn_str_id` }, |
character_takes_damage | | { "character", `character_id` }, { "damage", `int` }, |
character_triggers_trap | | { "character", `character_id` }, { "trap", `trap_str_id` }, |
character_wakes_up | | { "character", `character_id` }, |
character_wields_item | | { "character", `character_id` }, { "itype", `itype_id` }, |
character_wears_item | | { "character", `character_id` }, { "itype", `itype_id` }, |
consumes_marloss_item | | { "character", `character_id` }, { "itype", `itype_id` }, |
crosses_marloss_threshold | | { "character", `character_id` } |
crosses_mutation_threshold | | { "character", `character_id` }, { "category", `mutation_category_id` }, |
crosses_mycus_threshold | | { "character", `character_id` } |
cuts_tree | | { "character", `character_id` } |
dermatik_eggs_hatch | | { "character", `character_id` } |
dermatik_eggs_injected | | { "character", `character_id` } |
destroys_triffid_grove | | NONE |
dies_from_asthma_attack | | { "character", `character_id` } |
dies_from_drug_overdose | | { "character", `character_id` }, { "effect", `efftype_id` }, |
dies_from_bleeding | | { "character", `character_id` }  |
dies_from_hypovolemia | | { "character", `character_id` }  |
dies_from_redcells_loss | | { "character", `character_id` }  |
dies_of_infection | | { "character", `character_id` }  |
dies_of_starvation | | { "character", `character_id` }  |
dies_of_thirst | | { "character", `character_id` }  |
digs_into_lava | | NONE  |
disarms_nuke | | NONE  |
eats_sewage | | NONE  |
evolves_mutation | | { "character", `character_id` }, { "from_trait", `trait_id` }, { "to_trait", `trait_id` }, |
exhumes_grave | | { "character", `character_id` } |
fails_to_install_cbm | | { "character", `character_id` }, { "bionic", `bionic_id` }, |
fails_to_remove_cbm | | { "character", `character_id` }, { "bionic", `bionic_id` }, |
falls_asleep_from_exhaustion | | { "character", `character_id` } |
fuel_tank_explodes | | { "vehicle_name", `string` }, |
gains_addiction | |  { "character", `character_id` }, { "add_type", `addiction_id` }, |
gains_mutation | |  { "character", `character_id` }, { "trait", `trait_id` }, |
gains_skill_level | | { "character", `character_id` }, { "skill", `skill_id` }, { "new_level", `int` }, |
game_avatar_death | | { "avatar_id", `character_id` }, { "avatar_name", `string` }, { "avatar_is_male", `bool` }, { "is_suicide", `bool` }, { "last_words", `string` }, |
game_avatar_new | | { "is_new_game", `bool` }, { "is_debug", `bool` }, { "avatar_id", `character_id` }, { "avatar_name", `string` }, { "avatar_is_male", `bool` }, { "avatar_profession", `profession_id` }, { "avatar_custom_profession", `string` }, |
game_load | | { "cdda_version", `string` }, |
game_begin | | { "cdda_version", `string` }, |
game_over | | { "total_time_played", `chrono_seconds` }, |
game_save | | { "time_since_load", `chrono_seconds` }, { "total_time_played", `chrono_seconds` }, |
game_start | | { "game_version", `string` }, |
installs_cbm | | { "character", `character_id` }, { "bionic", `bionic_id` }, |
installs_faulty_cbm | | { "character", `character_id` }, { "bionic", `bionic_id` }, |
learns_martial_art | |  { "character", `character_id` }, { "martial_art", `matype_id` }, |
loses_addiction | | { "character", `character_id` }, { "add_type", `addiction_id` }, |
npc_becomes_hostile | | { "npc", `character_id` }, { "npc_name", `string` }, |
opens_portal | | NONE |
opens_spellbook | | { "character", `character_id` } |
opens_temple | | NONE |
player_fails_conduct | | { "conduct", `achievement_id` }, { "achievements_enabled", `bool` }, |
player_gets_achievement | | { "achievement", `achievement_id` }, { "achievements_enabled", `bool` }, |
player_levels_spell | | { "character", `character_id` }, { "spell", `spell_id` }, { "new_level", `int` }, |
reads_book | | { "character", `character_id` } |
releases_subspace_specimens | | NONE |
removes_cbm | |  { "character", `character_id` }, { "bionic", `bionic_id` }, |
seals_hazardous_material_sarcophagus | | NONE |
spellcasting_finish | | { "character", `character_id` }, { "spell", `spell_id` }, { "school", `trait_id` }  |
telefrags_creature | | { "character", `character_id` }, { "victim_name", `string` }, |
teleglow_teleports | | { "character", `character_id` } |
teleports_into_wall | | { "character", `character_id` }, { "obstacle_name", `string` }, |
terminates_subspace_specimens | | NONE |
throws_up | | { "character", `character_id` } |
triggers_alarm | | { "character", `character_id` } | 
uses_debug_menu | | { "debug_menu_option", `debug_menu_index` }, | 
u_var_changed | | { "var", `string` }, { "value", `string` }, |
vehicle_moves | | { "avatar_on_board", `bool` }, { "avatar_is_driving", `bool` }, { "avatar_remote_control", `bool` }, { "is_flying_aircraft", `bool` }, { "is_floating_watercraft", `bool` }, { "is_on_rails", `bool` }, { "is_falling", `bool` }, { "is_sinking", `bool` }, { "is_skidding", `bool` }, { "velocity", `int` }, // vehicle current velocity, mph * 100 { "z", `int` }, |

## Context Variables For Other EOCs
Other EOCs have some variables as well that they have access to, they are as follows:

EOC            | Context Variables |
--------------------- | ----------- |
mutation: "activated_eocs" | { "this", `mutation_id` }
mutation: "processed_eocs" | { "this", `mutation_id` }
mutation: "deactivated_eocs" | { "this", `mutation_id` }
