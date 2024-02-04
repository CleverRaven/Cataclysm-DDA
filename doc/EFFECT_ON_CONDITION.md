
# Effect On Condition

An effect_on_condition is an object allowing the combination of dialog conditions and effects with their usage outside of a dialog.  When invoked, they will test their condition; on a pass, they will cause their effect. They can be activated automatically with any given frequency.  (Note: effect_on_conditions use the npc dialog conditions and effects syntax, which allows checking related to, or targeting an effect at, an npc (for example: `npc_has_trait`).  Using these commands in an effect_on_condition is not supported.)

## Contents

- [Effect On Condition](#effect-on-condition)
  - [Fields](#fields)
  - [Alpha and Beta Talkers](#alpha-and-beta-talkers)
  - [Value types](#value-types)
  - [Variable Object](#variable-object)
  - [Examples:](#examples)
- [Condition:](#condition)
  - [Boolean logic](#boolean-logic)
  - [Possible conditions](#possible-conditions)
- [Reusable EOCs:](#reusable-eocs)
- [EVENT EOCs:](#event-eocs)
  - [Event Context Vars:](#event-context-vars)
  - [Event EOC Types:](#event-eoc-types)
  - [Context Variables For Other EOCs](#context-variables-for-other-eocs)
- [Effects](#effects)
  - [General](#general)
  - [Character effects](#character-effects)
  - [Item effects](#item-effects)
  - [Map effects](#map-effects)
  - [Map Updates](#map-updates)

## Fields

| Identifier            | Type      | Description |
|--------------------- | --------- | ----------- |
|`recurrence`          | int or variable object or array | The effect_on_condition is automatically invoked (activated) with this many seconds in-between. If it is an object it must have strings `name`, `type`, and `context`. `default` can be either an int or a string describing a time span. `global` is an optional bool (default false), if it is true the variable used will always be from the player character rather than the target of the dialog.  If it is an array it must have two values which are either ints or variable_objects.
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
* `NPC_DEATH` - EOCs can only be assigned to run on the death of an npc, in which case u will be the dying npc and npc will be the killer. If after it npc is no longer dead they will not die, if there are multiple they all be run until npc is not dead.
* `OM_MOVE` - EOCs trigger when the player moves overmap tiles
* `PREVENT_DEATH` - whenever the current avatar dies it will be run with the avatar as `u`, if after it the player is no longer dead they will not die, if there are multiple they all be run until the player is not dead.
* `EVENT` - EOCs trigger when a specific event given by "required_event" takes place. 

## Alpha and Beta Talkers

Talker, in context of effect on condition, is any entity, that can use specific effect or be checked against specific condition. Some effects and conditions don't have talker whatsoever, so any entity can use it (like `mapgen_update` effect can be used no matter is it used by player, NPC, monster or item), other has only one talker (aka alpha talker, aka `u_`) - the entity that call the effect or check (like `u_know_recipe` or `u_has_mission`), and the third has two talkers - alpha and beta (aka `u_` and `npc_`), which allow to check both you and your interlocutor (or enemy, depending on context)

For example, `{ "npc_has_effect": "Shadow_Reveal" }`, used by shadow lieutenant, check the player as beta talker, despite the id imply it should be `npc_`; This is a legacy of dialogue system, from which EoC was extended, and won't be fixed, since dialogues still use it fully

### Typical Alpha and Beta Talkers by cases

| EOC                                              | Alpha (possible types)      | Beta (possible types)       |
| ------------------------------------------------ | ----------------------      | --------------------------- |
| Talk with NPC                                    | player (Avatar)             | NPC (NPC)                   |
| Talk with monster                                | player (Avatar)             | monster (monster)           |
| Use computer                                     | player (Avatar)             | computer (Furniture)        |
| furniture: "examine_action"                      | player (Avatar)             | NONE                        |
| SPELL: "effect": "effect_on_condition"           | target (Character, Monster) | spell caster (Character, Monster) |
| use_action: "type": "effect_on_conditions"       | user (Character)            | item (item)                 |
| tick_action: "type": "effect_on_conditions"      | carrier (Character)         | item (item)                 |
| countdown_action: "type": "effect_on_conditions" | carrier (Character)         | item (item)                 |
| COMESTIBLE: "consumption_effect_on_conditions"   | user (Character)            | item (item)                 |
| activity_type: "completion_eoc"                  | character (Character)       | NONE                        |
| activity_type: "do_turn_eoc"                     | character (Character)       | NONE                        |
| addiction_type: "effect_on_condition"            | character (Character)       | NONE                        |
| bionics: "activated_eocs"                        | character (Character)       | NONE                        |
| bionics: "deactivated_eocs"                      | character (Character)       | NONE                        |
| bionics: "processed_eocs"                        | character (Character)       | NONE                        |
| mutation: "activated_eocs"                       | character (Character)       | NONE                        |
| mutation: "deactivated_eocs"                     | character (Character)       | NONE                        |
| mutation: "processed_eocs"                       | character (Character)       | NONE                        |
| recipe: "result_eocs"                            | crafter (Character)         | NONE                        |

Note, that using `use_action: "type": "effect_on_conditions"` automatically passes the context variable `id`, that stores the id of an item that was activated

## Value types

Effect on Condition uses a huge variety of different values for effects or for conditions to check against, so to standardize it, most of them are explained here

| name | description | example |
| --- | --- | --- |
| simple string | technically lack of any type - you just write a sting, and effect is called or check is performed: "barber_beard", "u_female", "u_is_deaf" | N/A |
| int | simple number. Decimals also allowed most of the time | `0`, `10`, `-55`, `987`, `0.1`, `0.5`, `-0.00001`, `9.87654321` |
| string | anything within of quotation marks `"`, usually id's or plain text:  | `"mon_zombie"`, `"fd_blood"`, `"just generic text i want to print in message"` |
| boolean | just boolean | `true`, `false` |
| duration | string, that contain number and unit of time, that the game code transform into seconds and put into the game. It is possible to use int instead of duration, but it is recommended to use duration for the readability sake. Possible values are `s`/`seconds`, `m`/`minutes`, `h`/`hours`, `d`/`days` | `1`, `"1 s"`, `"1 seconds"`, `"60 minutes"`, `3600`, `"99 minutes"`, `"2 d"`, `"365 days"`   |
| value between two | not a separate format, it means the field can accept two values simultaneously, and pick a random between this two; only values, that uses int can utilize it (int, duration or variable object usually) | `[ 1, 10 ]`,</br>`[ "30 seconds", "50 seconds" ]`,</br>`[ -1, 1 ]`,</br>`[ { "global_val": "small_variable" }, { "global_val": "big_variable" } ]`,</br>`[ { "global_val": "small_variable" }, 100 ]` |
| array | not a separate format, it means the field can accept multiple values, and the game either will pick one random between them, or apply all of them simultaneously | `[ "Somewhere", "Nowhere", "Everywhere", "Yesterday", "Tomorrow" ]`  |
| variable object | any [variable object](##variable-object); can be int, time or string, stored in said variable, or math syntax | `{ "global_val": "some_value" }`,</br>`{ "u_val": "some_personal_value" }`,</br>`{ "math": [ "some_value * 5 + 1" ] }` |
| location variable | not a separate format, just any [variable object](##variable-object) that store location coordinates, usually obtained by using `u_location_variable` / `npc_location_variable`  | `{ "global_val": "some_location" }`,</br>`{ "u_val": "some_location_i_know" }` |

There is some amount of another types, that are not explained here, like "search_data" or "fake_spell", but since this one are rarely reused, they are described in the effects that utilize them

## Variable Object

Variable object is a value, that changes due some conditions. Variable can be int, time, string, `math` expression or location variable. Types of variables are:

- `u_val` - variable, that is stored in this character, and, if player dies, the variable is lost also (or if you swap the avatar, for example; the secret one NPC told to character A would be lost for character B); 
- `npc_val` - variable, that is stored in beta talker
- `global_val` - variable, that is store in the world, and won't be lost until you delete said world
- `context_val` - variable, that was delivered from some another entity; For example, EVENT type EoCs can deliver specific variables contributor can use to perform specific checks:
`character_casts_spell` event, that is called every time, you guess it, character cast spell, it also store information about spell name, difficulty and damage, so contributor can create a specific negative effect for every spell casted, depending on this values; Generalized EoCs can also create and use context variables; math equivalent is variable name with `_`
- `var_val` - var_val is a unique variable object in the fact that it attempts to resolve the variable stored inside a context variable. So if you had

| Name | Type | Value |
| --- | --- | --- |
| ref | context_val | key1 |
| ref2 | context_val | u_key2 |
| key1 | global_val | SOME TEXT |
| key2 | u_val | SOME OTHER TEXT |

- If you access "ref" as a context val it will have the value of "key1", if you access it as a var_val it will have a value of "SOME TEXT". 
- If you access "ref2" as a context val it will have the value of "u_key2", if you access it as a var_val it will have a value of "SOME OTHER TEXT". 

For example, imagine you have context variable `{ "context_val": "my_best_spell" }`, and this `my_best_gun` variable contain text `any_random_gun`; also you have a `{ "global_val": "any_random_gun" }`, and this `any_random_gun` variable happened to contain text `ak47`
With both of this, you can use effect `"u_spawn_item": { "var_val": "my_best_gun" }`, and the game will spawn `ak47`, since it is what is stored inside `my_best_gun` global variable

The values for var_val use the same syntax for scope that math [variables](#variables) do.

Examples:

you add morale equal to `how_good` variable
```json
{ "u_add_morale": "morale_feeling_good", "bonus": { "u_val": "how_good" } }
```

you add morale random between u_`how_good` and u_`how_bad` variable
```json
{ "u_add_morale": "morale_feeling_good", "bonus": [ { "u_val": "how_good" }, { "u_val": "how_bad" } ] }
```

You make sound `Wow, your'e smart` equal to beta talker's intelligence
```json
{ "u_make_sound": "Wow, your'e smart", "volume": { "npc_val": "intelligence" } }
```

you add morale, equal to `ps_str` portal storm strength value
```json
{ "u_add_morale": "global_val", "bonus": { "global_val": "ps_str" } }
```

you add morale, equal to `ps_str` portal storm strength value plus 1
```json
{ "u_add_morale": "morale_feeling_good", "bonus": { "math": [ "ps_str + 1" ] } }
```

Effect on Condition, that is called every time player cast spell, and add thought `morale_from_spell_difficulty` with mood bonus equal to spell difficulty, and thought `morale_from_spell_damage` with mood bonus equal to damage difficulty
```json
{
  "type": "effect_on_condition",
  "id": "EOC_morale_from_spell",
  "eoc_type": "EVENT",
  "required_event": "character_casts_spell",
  "effect": [
    { "u_add_morale": "morale_from_spell_difficulty", "bonus": { "context_val": "difficulty" } }
    { "u_add_morale": "morale_from_spell_damage", "bonus": { "math": [ "_damage" ] } }
  ]
}
```

TODO: add example of usage `context_val` in generalized EoC, and example for `var_val`

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

# Condition:

Dialogue conditions is a way to check is some specific statements are true or false. Conditions can be used both in dialogue with NPC, and in Effect on Conditions. 
The following keys and strings are available:

## Boolean logic
Conditions can be combined into blocks using `"and"`, `"or"` and `"not"`

- `"and"` allow to check multiple conditions, and if each of them are `true`, condition return `true`, otherwise `false`
- `"or"` allow to check multiple conditions, and if at least one of them is `true`, condition return `true`, otherwise `false`
- `"not"` allow to check only one condition (but this condition could be `"and"` or `"or"`, that themselves can check multiple conditions), and swap the result of condition: if you get `true`, the condition return `false`

Examples:

Checks if weather is lightning, **and** you have effect `narcosis`
```json
"condition": { "and": [ { "is_weather": "lightning" }, { "u_has_effect": "narcosis" } ] }
```

Checks if weather is portal storm **or** distant portal storm **or** close portal storm
```json
"condition": { "or": [ { "is_weather": "portal_storm" }, { "is_weather": "distant_portal_storm" }, { "is_weather": "close_portal_storm" } ] }
```

Checks you are **not** close to refugee center (at least 4 overmap tiles afar)
```json
"condition": { "not": { "u_near_om_location": "evac_center_18", "range": 4 } }
```

Checks you don't have any traits from the list
```json
"condition": {
  "or": [
    { "not": { "u_has_trait": "HUMAN_ARMS" } },
    { "not": { "u_has_trait": "HUMAN_SKIN" } },
    { "not": { "u_has_trait": "HUMAN_EYES" } },
    { "not": { "u_has_trait": "HUMAN_HANDS" } },
    { "not": { "u_has_trait": "HUMAN_LEGS" } },
    { "not": { "u_has_trait": "HUMAN_MOUTH" } }
  ]
}
```

Same as previous, but with different syntax
```json
"condition": {
  "not": {
    "or": [
      { "u_has_trait": "HUMAN_ARMS" },
      { "u_has_trait": "HUMAN_SKIN" },
      { "u_has_trait": "HUMAN_EYES" },
      { "u_has_trait": "HUMAN_HANDS" },
      { "u_has_trait": "HUMAN_LEGS" },
      { "u_has_trait": "HUMAN_MOUTH" }
    ]
  }
}
```

Checks there is portal storm **and** you have `MAKAYLA_MUTATOR` mutation **and** you do **not** have item with `PORTAL_PROOF` flag **and** you are outside
```json
"condition": {
  "and": [
    { "is_weather": "portal_storm" },
    { "u_has_trait": "MAKAYLA_MUTATOR" },
    { "not": { "u_has_worn_with_flag": "PORTAL_PROOF" } },
    "u_is_outside"
  ]
}
```

## Possible conditions

### `u_male`, `u_female`, `npc_male`, `npc_female`
- type: simple string
- return true if alpha or beta talker is male or female

#### Valid talkers:

| Avatar | Character | NPC | Monster |  Furniture | Item |
| ------ | --------- | --------- | ---- | ------- | --- | 
| ✔️ | ✔️ | ✔️ | ❌ | ❌ | ❌ |

#### Examples
return true if alpha talker is female
```json
"condition": "npc_female",
```

### `u_is_avatar`, `u_is_npc`, `u_is_character`, `u_is_monster`, `u_is_item`, `u_is_furniture`, `npc_is_avatar`, `npc_is_npc`, `npc_is_character`, `npc_is_monster`, `npc_is_item`, `npc_is_furniture`
- type: simple string
- return true if alpha or beta talker is avatar / NPC / character / monster / item /furniture
- `avatar` is you, player, that control specific NPC (yes, your character is still NPC, you just can control it, as you can control another NPC using faction succession)
- `npc` is any NPC, except Avatar
- `character` is both NPC or Avatar

#### Valid talkers:

| Avatar | Character | NPC | Monster |  Furniture | Item |
| ------ | --------- | --------- | ---- | ------- | --- | 
| ✔️ | ✔️ | ✔️ | ✔️ | ✔️ | ✔️ |

#### Examples
return true if alpha talker is character (avatar or NPC)
```json
"condition": "u_is_character",
```

### `u_at_om_location`, `npc_at_om_location`
- type: string or [variable object](##variable-object)
- return true if alpha talker stands on specific overmap tile;
- `FACTION_CAMP_ANY` can be used, that return true if alpha or beta talker stand on faction camp tile; 
- `FACTION_CAMP_START` can be used, that return true if alpha or beta talker stand on tile, that can be turned into faction camp

#### Valid talkers:

| Avatar | Character | NPC | Monster |  Furniture | Item |
| ------ | --------- | --------- | ---- | ------- | --- | 
| ✔️ | ✔️ | ✔️ | ✔️ | ✔️ | ✔️ |

#### Examples
return true if alpha talker at the `field`
```json
{ "u_at_om_location": "field" }
```

return true if alpha talker at faction camp
```json
{ "u_at_om_location": "FACTION_CAMP_ANY" }
```

return true if alpha talker at location that can be transformed to faction camp
```json
{ "npc_at_om_location": "FACTION_CAMP_START" }
```

### `u_has_trait`, `npc_has_trait`, `u_has_any_trait`, `npc_has_any_trait`
- type: string or [variable object](##variable-object)
- check does alpha or beta talker have specific trait/mutation;
- `_has_trait` checks only 1 trait, when `_has_any_trait` check a range, and return true if at least one trait is presented;

#### Valid talkers:

| Avatar | Character | NPC | Monster |  Furniture | Item |
| ------ | --------- | --------- | ---- | ------- | --- | 
| ✔️ | ✔️ | ✔️ | ❌ | ❌ | ❌ |

#### Examples
checks do alpha talker have `EAGLEEYED` trait
```json
{ "u_has_trait": "EAGLEEYED" }
```

using `_has_any_trait` with single trait is also possible
```json
{ "u_has_any_trait": [ "EAGLEEYED" ] }
```

```json
{ "u_has_any_trait": [ "CANINE_EARS", "LUPINE_EARS", "FELINE_EARS", "URSINE_EARS", "ELFA_EARS" ] }
```

### `u_has_visible_trait`, `npc_has_visible_trait`
- type: string or [variable object](##variable-object)
- return true if the alpha or beta talker has a trait/mutation that has any visibility (defined in mutation `visibility` field); probably was designed if alpha or beta talker was able to hide it's mutations, but it's hard to say

#### Valid talkers:

| Avatar | Character | NPC | Monster |  Furniture | Item |
| ------ | --------- | --------- | ---- | ------- | --- | 
| ✔️ | ✔️ | ✔️ | ❌ | ❌ | ❌ |

#### Examples
Checks do alpha talker has `FEATHERS` mutation
```json
{ "u_has_trait": "FEATHERS" }
```

### `u_has_martial_art`, `npc_has_martial_art`
- type: string or [variable object](##variable-object)
- return true if alpha or beta talker has some martial art

#### Valid talkers:

| Avatar | Character | NPC | Monster |  Furniture | Item |
| ------ | --------- | --------- | ---- | ------- | --- | 
| ✔️ | ✔️ | ✔️ | ❌ | ❌ | ❌ |

#### Examples
```json
{ "u_has_martial_art": "style_aikido" }
```

### `u_using_martial_art`, `npc_using_martial_art`
- type: string or [variable object](##variable-object)
- return true if alpha or beta talker using the martial art

#### Valid talkers:

| Avatar | Character | NPC | Monster |  Furniture | Item |
| ------ | --------- | --------- | ---- | ------- | --- | 
| ✔️ | ✔️ | ✔️ | ❌ | ❌ | ❌ |

#### Examples
```json
{ "u_using_martial_art": "style_aikido" }
```

### `u_has_flag`, `npc_has_flag`
- type: string or [variable object](##variable-object)
- return true if alpha or beta talker has specific flag; special flag `MUTATION_THRESHOLD` can be used to check do alpha talker has any mutant threshold; for monsters both json flags (applied by effects) and monster flags can be checked

#### Valid talkers:

| Avatar | Character | NPC | Monster |  Furniture | Item |
| ------ | --------- | --------- | ---- | ------- | --- | 
| ✔️ | ✔️ | ✔️ | ✔️ | ❌ | ✔️ |

#### Examples
Alpha talker has `GRAB` flag, and beta talker has `GRAB_FILTER` flag; monster uses it to perform grabs - the game checks do monster (alpha talker, `u_`) has GRAB flag (i.e. able to grab at all), and check is target able to be grabbed using `GRAB_FILTER` flag
```json
{ "npc_has_flag": "GRAB_FILTER" }, { "u_has_flag": "GRAB" }
```

### `u_has_species`, `npc_has_species`
- type: string or [variable object](##variable-object)
- true if alpha or beta talker has the defined species

#### Valid talkers:

| Avatar | Character | NPC | Monster |  Furniture | Item |
| ------ | --------- | --------- | ---- | ------- | --- | 
| ❌ | ❌ | ❌ | ✔️ | ❌ | ❌ |

#### Examples
alpha talker is `SLIME`
```json
{ "u_has_species": "SLIME" }
```

### `u_bodytype`, `npc_bodytype`
- type: string or [variable object](##variable-object)
- true if alpha / beta talker monster has the defined bodytype
- for player / NPC return true if bodytype is `human`

#### Valid talkers:

| Avatar | Character | NPC | Monster |  Furniture | Item |
| ------ | --------- | --------- | ---- | ------- | --- | 
| ✔️ | ✔️ | ✔️ | ❌ | ❌ | ❌ |

#### Examples
alpha talker has bodytype `migo` , and beta has bodytype `human`
```json
{ "u_bodytype": "migo" }, { "npc_bodytype": "human" }
```

### `u_has_var`, `npc_has_var`
- type: string
- checks do alpha or beta talker has specific variables, that was added `u_add_var` or `npc_add_var`
- `type`, `context` and `value` of the variable is also required

#### Valid talkers:

| Avatar | Character | NPC | Monster |  Furniture | Item |
| ------ | --------- | --------- | ---- | ------- | --- | 
| ✔️ | ✔️ | ✔️ | ✔️ | ✔️ | ✔️ |

#### Examples
Checks do alpha talker has `u_met_sadie` variable
```json
{ "u_has_var": "u_met_sadie", "type": "general", "context": "meeting", "value": "yes" }
```

### `expects_vars`
- type: array of strings and/or [variable object](##variable-object)
- return true if each provided variable exist
- return false and create debug error message if check fails

#### Valid talkers:

| Avatar | Character | NPC | Monster |  Furniture | Item |
| ------ | --------- | --------- | ---- | ------- | --- | 
| ✔️ | ✔️ | ✔️ | ✔️ | ✔️ | ✔️ |

#### Examples
checks this var exists
```json
{ "expects_vars": [ "u_met_me", "u_met_you", "u_met_yourself" ] }
```

### `compare_string`
- type: pair of strings or [variable objects](##variable-object)
- Compare two strings, and return true if strings are equal

#### Examples
checks if `victim_type` is `mon_zombie_phase_shrike`
```json
{ "compare_string": [ "mon_zombie_phase_shrike", { "context_val": "victim_type" } ] }
```

checks is `victim_type` has `zombie` faction
```json
{ "compare_string": [ "zombie", { "mutator": "mon_faction", "mtype_id": { "context_val": "victim_type" } } ] }
```

### `u_profession`
- type: string or [variable object](##variable-object)
- Return true if player character has the given profession id or its "hobby" subtype

#### Valid talkers:

| Avatar | Character | NPC | Monster |  Furniture | Item |
| ------ | --------- | --------- | ---- | ------- | --- | 
| ✔️ | ✔️ | ❌ | ❌ | ❌ | ❌ |

#### Examples
True if the character has selected Heist Driver profession at the character creation
```json
{ "u_profession": "heist_driver" }
```

True if the character has selected Fishing background at the character creation
```json
{ "u_profession": "fishing" }
```

### `u_has_strength`, `npc_has_strength`, `u_has_dexterity`, `npc_has_dexterity`, `u_has_intelligence`, `npc_has_intelligence`, `u_has_perception`, `npc_has_perception`
- type: int or [variable object](##variable-object)
- Return true if alpha or beta talker stat is at least the value or higher

#### Valid talkers:

| Avatar | Character | NPC | Monster |  Furniture | Item |
| ------ | --------- | --------- | ---- | ------- | --- | 
| ✔️ | ✔️ | ✔️ | ❌ | ❌ | ❌ |

#### Examples
True, if alpha talker has str 7 or more
```json
{ "u_has_strength": 7 }
```

### `u_has_part_temp`, `npc_has_part_temp`
- type: int or [variable object](##variable-object)
- return true if alpha or beta talker's body part has temperature higher than value; additional parameter `bodypart` specifies the body part
- temperature is written in arbitrary units, described in `weather.h`: `Body temperature is measured on a scale of 0u to 10000u, where 10u = 0.02C and 5000u is 37C`

#### Valid talkers:

| Avatar | Character | NPC | Monster |  Furniture | Item |
| ------ | --------- | --------- | ---- | ------- | --- | 
| ✔️ | ✔️ | ✔️ | ❌ | ❌ | ❌ |

#### Examples
check is your torso is 37 centigrade
```json
{ "u_has_part_temp": 5000, "bodypart": "torso" }
```

### `u_has_item`, `npc_has_item`, `u_has_items`, `npc_has_items`
- type: string or [variable object](##variable-object)
- return true if item or items are presented in your or NPC inventory;
- `_has_items` require `count` or `charges` to define how much copies of items (for `count`) or charges inside item (for `charges`) alpha or beta talker has

#### Valid talkers:

| Avatar | Character | NPC | Monster |  Furniture | Item |
| ------ | --------- | --------- | ---- | ------- | --- | 
| ✔️ | ✔️ | ✔️ | ❌ | ❌ | ❌ |

#### Examples
check do you have a guitar
```json
{ "u_has_item": "guitar" }
```

check do you have 6 ropes
```json
{ "u_has_items": { "item": "rope_6", "count": 6 } }
```

### `u_has_item_category`, `npc_has_item_category`
- type: string or [variable object](##variable-object)
- return true if alpha or beta talker have items of specific category;
- `count` can be used to check amount of items bigger than 1

#### Valid talkers:

| Avatar | Character | NPC | Monster |  Furniture | Item |
| ------ | --------- | --------- | ---- | ------- | --- | 
| ✔️ | ✔️ | ✔️ | ❌ | ❌ | ❌ |

#### Examples
check do you have manual in inventory
```json
{ "u_has_item_category": "manuals" }
```

check do you have 3 manuals in inventory
```json
{ "u_has_item_category": "manuals", "count": 3 }
```

### `u_has_bionics`, `npc_has_bionics`
- type: string or [variable object](##variable-object)
- return true if alpha or beta talker has specific bionic; `ANY` can be used to return true if any bionic is presented

#### Valid talkers:

| Avatar | Character | NPC | Monster |  Furniture | Item |
| ------ | --------- | --------- | ---- | ------- | --- | 
| ✔️ | ✔️ | ✔️ | ❌ | ❌ | ❌ |

#### Examples
check do you have `bio_synlungs`
```json
{ "u_has_bionics": "bio_synlungs" }
```

check do you have any bionic presented
```json
{ "u_has_bionics": "ANY" }
```

### `u_has_effect`, `npc_has_effect`, `u_has_any_effect`, `npc_has_any_effect`
- type: string or [variable object](##variable-object)
- return true if alpha or beta talker has specific effect applied. `_has_effect` checks only one effect, when `_has_any_effect` check a range, and return true if at least one effect is applied;
- `intensity` can be used to check an effect of specific intensity;
- `bodypart` can be used to check effect applied on specific body part
- martial arts `static_buffs` can be checked in form `mabuff:buff_id`

#### Valid talkers:

| Avatar | Character | NPC | Monster |  Furniture | Item |
| ------ | --------- | --------- | ---- | ------- | --- | 
| ✔️ | ✔️ | ✔️ | ❌ | ❌ | ❌ |

#### Examples
checks are you infected
```json
{ "u_has_effect": "infected" }
```

checks are you head bleed heavily
```json
{ "u_has_effect": "bleed", "intensity": 10, "bodypart": "head" }
```

checks do you have aikido stance active
```json
{ "u_has_effect": "mabuff:buff_aikido_static1" }
```

checks are you hot or cold
```json
{ "u_has_any_effect": [ "hot", "cold" ], "bodypart": "torso" }
```

### `u_has_proficiency`, `npc_has_proficiency`
- type: string or [variable object](##variable-object)
- return true if alpha or beta talker has mastered a proficiency (to 100%).

#### Valid talkers:

| Avatar | Character | NPC | Monster |  Furniture | Item |
| ------ | --------- | --------- | ---- | ------- | --- | 
| ✔️ | ✔️ | ✔️ | ❌ | ❌ | ❌ |

#### Examples
check do you have `Principles of Chemistry`
```json
{ "u_has_proficiency": "prof_intro_chemistry" }
```

### `u_can_stow_weapon`, `npc_can_stow_weapon`
- type: simple string
- return true if alpha or beta talker wield an item, and have enough space to put it away

#### Valid talkers:

| Avatar | Character | NPC | Monster |  Furniture | Item |
| ------ | --------- | --------- | ---- | ------- | --- | 
| ✔️ | ✔️ | ✔️ | ❌ | ❌ | ❌ |

#### Examples
You have equipped an item that you can stow
```json
"u_can_stow_weapon"
```

You have equipped an item that you can not stow, either because it's bionic pseudoitem, you have no space to store it, or by any another reason
```json
{ "not": "u_can_stow_weapon" }
```

### `u_can_drop_weapon`, `npc_can_drop_weapon`
- type: simple string
- return true if alpha or beta talker wield an item, and can drop it on the ground (i.e. weapon has no `NO_UNWIELD` flag like retracted bionic claws or monomolecular blade bionics)

#### Valid talkers:

| Avatar | Character | NPC | Monster |  Furniture | Item |
| ------ | --------- | --------- | ---- | ------- | --- | 
| ✔️ | ✔️ | ✔️ | ❌ | ❌ | ❌ |

#### Examples

```json
"u_can_drop_weapon"
```


```json
{ "not": "u_can_drop_weapon" }
```

`u_has_wielded_with_flag` may be used to replicate the effect
```json
{ "u_has_wielded_with_flag": "NO_UNWIELD" }
```


### `u_has_weapon`, `npc_has_weapon`
- type: simple string
- return true if alpha or beta talker wield any item

#### Valid talkers:

| Avatar | Character | NPC | Monster |  Furniture | Item |
| ------ | --------- | --------- | ---- | ------- | --- | 
| ✔️ | ✔️ | ✔️ | ❌ | ❌ | ❌ |

#### Examples

```json
"u_has_weapon"
```

You don't wield anything
```json
{ "not": "u_has_weapon" }
```

### `u_driving`, `npc_driving`
- type: simple string
- return true if alpha or beta talker operate a vehicles; Nota bene: NPCs cannot currently operate vehicles

#### Valid talkers:

| Avatar | Character | NPC | Monster |  Furniture | Item |
| ------ | --------- | --------- | ---- | ------- | --- | 
| ✔️ | ✔️ | ✔️ | ❌ | ❌ | ❌ |

#### Examples

```json
"u_driving"
```

true if you do not drive
```json
{ "not": "u_driving" }
```

### `u_know_recipe`
- type: string or [variable object](##variable-object)
- return true if character know specific recipe;
- only memorized recipes count, not recipes written in the book

#### Valid talkers:

| Avatar | Character | NPC | Monster |  Furniture | Item |
| ------ | --------- | --------- | ---- | ------- | --- | 
| ✔️ | ✔️ | ❌ | ❌ | ❌ | ❌ |

#### Examples
check do you memorize `meat_hunk` recipe
```json
{ "u_know_recipe": "meat_hunk" }
```

### `u_has_worn_with_flag`, `npc_has_worn_with_flag`
- type: string or [variable object](##variable-object)
- return true if alpha or beta talker wear some item with specific flag

#### Valid talkers:

| Avatar | Character | NPC | Monster |  Furniture | Item |
| ------ | --------- | --------- | ---- | ------- | --- | 
| ✔️ | ✔️ | ✔️ | ❌ | ❌ | ❌ |

#### Examples
check do you wear something with `RAD_DETECT` flag
```json
{ "u_has_worn_with_flag": "RAD_DETECT" }
```

### `u_has_wielded_with_flag`, `npc_has_wielded_with_flag`
- type: string or [variable object](##variable-object)
- return true if alpha or beta talker wield something with specific flag

#### Valid talkers:

| Avatar | Character | NPC | Monster |  Furniture | Item |
| ------ | --------- | --------- | ---- | ------- | --- | 
| ✔️ | ✔️ | ✔️ | ❌ | ❌ | ❌ |

#### Examples
check do you wield something with `WHIP` flag
```json
{ "u_has_wielded_with_flag": "WHIP" }
```

### `u_has_wielded_with_weapon_category`, `npc_has_wielded_with_weapon_category`
- type: string or [variable object](##variable-object)
- return true if alpha or beta talker wield something with specific weapon category

#### Valid talkers:

| Avatar | Character | NPC | Monster |  Furniture | Item |
| ------ | --------- | --------- | ---- | ------- | --- | 
| ✔️ | ✔️ | ✔️ | ❌ | ❌ | ❌ |

#### Examples
check do you wield something with `LONG_SWORDS` weapon category
```json
{ "u_has_wielded_with_weapon_category": "LONG_SWORDS" }
```

### `u_can_see`, `npc_can_see`
- type: simple string
- return true if alpha or beta talker can see (not blind)

#### Valid talkers:

| Avatar | Character | NPC | Monster |  Furniture | Item |
| ------ | --------- | --------- | ---- | ------- | --- | 
| ✔️ | ✔️ | ✔️ | ✔️ | ❌ | ❌ |

#### Examples

```json
"u_can_see"
```

You can't see
```json
{ "not": "u_can_see" }
```

### `u_is_deaf`, `npc_is_deaf`
- type: simple string
- return true if alpha or beta talker can't hear

#### Valid talkers:

| Avatar | Character | NPC | Monster |  Furniture | Item |
| ------ | --------- | --------- | ---- | ------- | --- | 
| ✔️ | ✔️ | ✔️ | ✔️ | ❌ | ❌ |

#### Examples

```json
"u_is_deaf"
```

You can hear
```json
{ "not": "u_is_deaf" }
```

### `u_is_alive`, `npc_is_alive`
- type: simple string
- return true if alpha or beta talker is not dead

#### Valid talkers:

| Avatar | Character | NPC | Monster |  Furniture | Item |
| ------ | --------- | --------- | ---- | ------- | --- | 
| ✔️ | ✔️ | ✔️ | ✔️ | ❌ | ❌ |

#### Examples

```json
"u_is_alive"
```

NPC is dead
```json
{ "not": "npc_is_alive" }
```

### `u_is_on_terrain`, `npc_is_on_terrain`
- type: string or [variable object](##variable-object)
- return true if alpha or beta talker stand on specific terrain

#### Valid talkers:

| Avatar | Character | NPC | Monster |  Furniture | Item |
| ------ | --------- | --------- | ---- | ------- | --- | 
| ✔️ | ✔️ | ✔️ | ❌ | ❌ | ❌ |

#### Examples
check do you stand on grass
```json
{ "u_is_on_terrain": "t_grass" }
```

### `u_is_on_terrain_with_flag`, `npc_is_on_terrain_with_flag`
- type: string or [variable object](##variable-object)
- return true if alpha or beta talker stand on terrain with specific flag

#### Valid talkers:

| Avatar | Character | NPC | Monster |  Furniture | Item |
| ------ | --------- | --------- | ---- | ------- | --- | 
| ✔️ | ✔️ | ✔️ | ✔️ | ❌ | ❌ |

#### Examples
check do you stand on terrain with `SHRUB` flag
```json
{ "u_is_on_terrain_with_flag": "SHRUB" }
```

### `u_is_in_field`, `npc_is_in_field`
- type: string or [variable object](##variable-object)
- return true if alpha or beta talker stand on specific field

#### Valid talkers:

| Avatar | Character | NPC | Monster |  Furniture | Item |
| ------ | --------- | --------- | ---- | ------- | --- | 
| ✔️ | ✔️ | ✔️ | ❌ | ❌ | ❌ |

#### Examples
check do you stand in a cloud of smoke
```json
{ "u_is_in_field": "fd_smoke" }
```

### `u_query`, `npc_query`
- type: string or [variable object](##variable-object)
- For avatar (player), create a popup with message, that could be answered as "yes" and "no". if "yes" is picked, true is returned, otherwise false;
- `default` field should be used to specify default output for NPC, that player do not control;
- popup is created only if the rest of conditions are true

#### Valid talkers:

| Avatar | Character | NPC | Monster |  Furniture | Item |
| ------ | --------- | --------- | ---- | ------- | --- | 
| ✔️ | ✔️ | ✔️ | ❌ | ❌ | ❌ |
#### Examples
Create a popup with message `You have died.  Continue as one of your followers?`
```json
{ "u_query": "You have died.  Continue as one of your followers?", "default": false }
```


### `u_query_tile`, `npc_query_tile`
- type: string
- Ask the player to select a tile. If tile is selected, true is returned, otherwise false;
- `anywhere`, `line_of_sight`, `around` are possible
  - `anywhere` is the same as the "look around" UI
  - `line_of_sight` only tiles that are visible at this moment (`range` is mandatory)
  - `around` is the same as starting a fire, you can only choose the 9 tiles you're immediately adjacent to
- `target_var` is [variable object](##variable-object) to contain coordinates of selected tile (**mandatory**)
- `range` defines the selectable range for `line_of_sight` (**mandatory** for `line_of_sight`, otherwise not required)
- `z_level` defines allow if select other z-level  for `anywhere`
- `message` is displayed while selecting

#### Valid talkers:

| Avatar | Character | NPC | Monster |  Furniture | Item |
| ------ | --------- | --------- | ---- | ------- | --- | 
| ✔️ | ❌ | ❌ | ❌ | ❌ | ❌ |

#### Examples
Display coordinates of selected tile.
```json
{
  "if": { "u_query_tile": "line_of_sight", "target_var": { "context_val": "pos" }, "message": "Select point", "range": 10 },
  "then": { "u_message": "<context_val:pos>" },
  "else": { "u_message": "Canceled" }
}
```

### `map_terrain_with_flag`, `map_furniture_with_flag`
- type: string or [variable object](##variable-object)
- return true if the terrain or furniture has specific flag
- `loc` will specify location of terrain or furniture (**mandatory**)

#### Valid talkers:

No talker is needed.

#### Examples
Check the north terrain or furniture has `TRANSPARENT` flag.
```json
{
  "type": "effect_on_condition",
  "id": "EOC_ter_furn_check",
  "effect": [
      { "set_string_var": { "mutator": "loc_relative_u", "target": "(0,-1,0)" }, "target_var": { "context_val": "loc" } },
      {
        "if": { "map_terrain_with_flag": "TRANSPARENT", "loc": { "context_val": "loc" } },
        "then": { "u_message": "North terrain: TRANSPARENT" },
        "else": { "u_message": "North terrain: Not TRANSPARENT" }
      },
      {
        "if": { "map_furniture_with_flag": "TRANSPARENT", "loc": { "context_val": "loc" } },
        "then": { "u_message": "North furniture: TRANSPARENT" },
        "else": { "u_message": "North furniture: Not TRANSPARENT" }
      }
  ]
},
```

### `map_in_city`
- type: location string or [variable object](##variable-object)
- return true if the location is in a city

#### Valid talkers:

No talker is needed.

#### Examples
Check the location is in a city.
```json
{ "u_location_variable": { "context_val": "loc" } },
{
  "if": { "map_in_city": { "context_val": "loc" } },
  "then": { "u_message": "Inside city" },
  "else": { "u_message": "Outside city" }
},
```

### `player_see_u`, `player_see_npc`
- type: simple string
- return true if player can see alpha or beta talker

#### Valid talkers:

| Avatar | Character | NPC | Monster |  Furniture | Item |
| ------ | --------- | --------- | ---- | ------- | --- | 
| ✔️ | ✔️ | ✔️ | ✔️ | ✔️ | ✔️ |

#### Examples
return true if player can see NPC.
```json
"condition": "player_see_npc",
```

### `u_can_see_location`, `npc_can_see_location`
- type: location string or [variable object](##variable-object)
- return true if alpha or beta talker can see the location

#### Valid talkers:

| Avatar | Character | NPC | Monster |  Furniture | Item |
| ------ | --------- | --------- | ---- | ------- | --- | 
| ✔️ | ✔️ | ✔️ | ✔️ | ❌ | ❌ |

#### Examples

You can see selected location.
```json
{
  "if": { "u_query_tile": "anywhere", "target_var": { "context_val": "pos" }, "message": "Select point" },
  "then": {
    "if": { "u_can_see_location": { "context_val": "pos" } },
    "then": { "u_message": "You can see <context_val:pos>." },
    "else": { "u_message": "You cant see <context_val:pos>." }
  }
}
```

### `has_ammo`
- type: simple string
- return true if beta talker is an item and has enough ammo for at least one "shot".

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

| EVENT              | Description | Context Variables | Talker(alpha/beta) |
| ------------------ | ----------- | ----------------- | ------------------ |
| activates_artifact | Triggers when the player activates an artifact | { "character", `character_id` },<br/> { "item_name", `string` }, | character / NONE |
| activates_mininuke | Triggers when any character arms a mininuke | { "character", `character_id` } | character / NONE |
| administers_mutagen |  | { "character", `character_id` },<br/> { "technique", `mutagen_technique` }, | character / NONE |
| angers_amigara_horrors | Triggers when amigara horrors are spawned as part of a mine finale | NONE | avatar / NONE |
| avatar_enters_omt |  | { "pos", `tripoint` },<br/> { "oter_id", `oter_id` }, | avatar / NONE |
| avatar_moves |  | { "mount", `mtype_id` },<br/> { "terrain", `ter_id` },<br/> { "movement_mode", `move_mode_id` },<br/> { "underwater", `bool` },<br/> { "z", `int` }, | avatar / NONE |
| avatar_dies |  | NONE | avatar / NONE |
| awakes_dark_wyrms | Triggers when `pedestal_wyrm` examine action is used | NONE | avatar / NONE |
| becomes_wanted | Triggered when copbots/riot bots are spawned as part of a timed event after mon/char is photo'd by eyebot | { "character", `character_id` } | character / NONE |
| broken_bone | Triggered when any body part reaches 0 hp | { "character", `character_id` },<br/> { "part", `body_part` }, | character / NONE |
| broken_bone_mends | Triggered when `mending` effect is removed by expiry (Character::mend) | { "character", `character_id` },<br/> { "part", `body_part` }, | character / NONE |
| buries_corpse | Triggers when item with flag CORPSE is located on same tile as construction with post-special `done_grave` is completed | { "character", `character_id` },<br/> { "corpse_type", `mtype_id` },<br/> { "corpse_name", `string` }, | character / NONE |
| causes_resonance_cascade | Triggers when resonance cascade option is activated via "old lab" finale's computer | NONE | avatar / NONE |
| character_casts_spell | Triggers when a character casts spells. When a spell with multiple effects is cast, the number of effects will be triggered | { "character", `character_id` },<br/> { "spell", `spell_id` },<br/> { "school", `trait_id` },<br/> { "difficulty", `int` },<br/> { "cost", `int` },<br/> { "cast_time", `int` },<br/> { "damage", `int` }, | character / NONE |
| character_consumes_item |  | { "character", `character_id` },<br/> { "itype", `itype_id` }, | character / NONE |
| character_dies |  | { "character", `character_id` }, | character / NONE |
| character_eats_item |  | { "character", `character_id` },<br/> { "itype", `itype_id` }, | character / NONE |
| character_finished_activity | Triggered when character finished or canceled activity | { "character", `character_id` },<br/> { "activity", `activity_id` },<br/> { "canceled", `bool` } | character / NONE |
| character_forgets_spell |  | { "character", `character_id` },<br/> { "spell", `spell_id` } | character / NONE |
| character_gains_effect |  | { "character", `character_id` },<br/> { "effect", `efftype_id` },<br/> { "bodypart", `bodypart_id` } | character / NONE |
| character_gets_headshot |  | { "character", `character_id` } | character / NONE |
| character_heals_damage |  | { "character", `character_id` },<br/> { "damage", `int` }, | character / NONE |
| character_kills_character |  | { "killer", `character_id` },<br/> { "victim", `character_id` },<br/> { "victim_name", `string` }, | character / NONE |
| character_kills_monster |  | { "killer", `character_id` },<br/> { "victim_type", `mtype_id` },<br/> { "exp", `int` }, | character / monster |
| character_learns_spell |  | { "character", `character_id` },<br/> { "spell", `spell_id` } | character / NONE |
| character_loses_effect |  | { "character", `character_id` },<br/> { "effect", `efftype_id` },<br/> { "bodypart", `bodypart_id` } | character / NONE |
| character_melee_attacks_character |  | { "attacker", `character_id` },<br/> { "weapon", `itype_id` },<br/> { "hits", `bool` },<br/> { "victim", `character_id` },<br/> { "victim_name", `string` }, | character (attacker) / character (victim) |
| character_melee_attacks_monster | | { "attacker", `character_id` },<br/> { "weapon", `itype_id` },<br/> { "hits", `bool` },<br/> { "victim_type", `mtype_id` },| character / monster |
| character_ranged_attacks_character | |  { "attacker", `character_id` },<br/> { "weapon", `itype_id` },<br/> { "victim", `character_id` },<br/> { "victim_name", `string` }, | character (attacker) / character (victim) |
| character_ranged_attacks_monster | | { "attacker", `character_id` },<br/> { "weapon", `itype_id` },<br/> { "victim_type", `mtype_id` }, | character / monster |
| character_smashes_tile | | { "character", `character_id` },<br/> { "terrain", `ter_str_id` },  { "furniture", `furn_str_id` }, | character / NONE |
| character_starts_activity | Triggered when character starts or resumes activity | { "character", `character_id` },<br/> { "activity", `activity_id` },<br/> { "resume", `bool` } | character / NONE |
| character_takes_damage | | { "character", `character_id` },<br/> { "damage", `int` }, | character / NONE |
| character_triggers_trap | | { "character", `character_id` },<br/> { "trap", `trap_str_id` }, | character / NONE |
| character_wakes_up | triggers in the moment player lost it's sleep effect and wakes up | { "character", `character_id` }, | character / NONE |
| character_attempt_to_fall_asleep | triggers in the moment character tries to fall asleep, after confirming and setting an alarm, but before "you lie down" | { "character", `character_id` }, | character / NONE |
| character_falls_asleep | triggers in the moment character actually falls asleep; trigger includes cases where character sleep for a short time because of fatigue or drugn; duration of the sleep can be changed mid sleep because of hurt/noise/light/pain thresholds and another factors | { "character", `character_id` }, { "duration", `int_` (in seconds) } | character / NONE |
| character_wields_item | | { "character", `character_id` },<br/> { "itype", `itype_id` }, | character / item to wield |
| character_wears_item | | { "character", `character_id` },<br/> { "itype", `itype_id` }, | character / item to wear |
| consumes_marloss_item | | { "character", `character_id` },<br/> { "itype", `itype_id` }, | character / NONE |
| crosses_marloss_threshold | | { "character", `character_id` } | character / NONE |
| crosses_mutation_threshold | | { "character", `character_id` },<br/> { "category", `mutation_category_id` }, | character / NONE |
| crosses_mycus_threshold | | { "character", `character_id` } | character / NONE |
| cuts_tree | | { "character", `character_id` } | character / NONE |
| dermatik_eggs_hatch | | { "character", `character_id` } | character / NONE |
| dermatik_eggs_injected | | { "character", `character_id` } | character / NONE |
| destroys_triffid_grove | Triggered *only* via spell with effect_str `ROOTS_DIE` (currently via death spell of triffid heart) | NONE | avatar / NONE |
| dies_from_asthma_attack | | { "character", `character_id` } | character / NONE |
| dies_from_drug_overdose | | { "character", `character_id` },<br/> { "effect", `efftype_id` }, | character / NONE |
| dies_from_bleeding | | { "character", `character_id` }  | character / NONE |
| dies_from_hypovolemia | | { "character", `character_id` }  | character / NONE |
| dies_from_redcells_loss | | { "character", `character_id` }  | character / NONE |
| dies_of_infection | | { "character", `character_id` }  | character / NONE |
| dies_of_starvation | | { "character", `character_id` }  | character / NONE |
| dies_of_thirst | | { "character", `character_id` }  | character / NONE |
| digs_into_lava | | NONE  | avatar / NONE |
| disarms_nuke | Triggered via disarm missile computer action in missile silo special | NONE  | avatar / NONE |
| eats_sewage | Triggered via use action `SEWAGE` | NONE  | avatar / NONE |
| evolves_mutation | | { "character", `character_id` },<br/> { "from_trait", `trait_id` },<br/> { "to_trait", `trait_id` }, | character / NONE |
| exhumes_grave | Triggers when construction with post-special `done_dig_grave` or `done_dig_grave_nospawn` is completed | { "character", `character_id` } | character / NONE |
| fails_to_install_cbm | | { "character", `character_id` },<br/> { "bionic", `bionic_id` }, | character / NONE |
| fails_to_remove_cbm | | { "character", `character_id` },<br/> { "bionic", `bionic_id` }, | character / NONE |
| falls_asleep_from_exhaustion | | { "character", `character_id` } | character / NONE |
| fuel_tank_explodes | Triggers when vehicle part (that is watertight container/magazine with magazine pocket/or is a reactor) is sufficiently damaged | { "vehicle_name", `string` }, | avatar / NONE |
| gains_addiction | |  { "character", `character_id` },<br/> { "add_type", `addiction_id` }, | character / NONE |
| gains_mutation | |  { "character", `character_id` },<br/> { "trait", `trait_id` }, | character / NONE |
| gains_proficiency | | { "character", `character_id` },<br/> { "proficiency", `proficiency_id` }, | character / NONE |
| gains_skill_level | | { "character", `character_id` },<br/> { "skill", `skill_id` },<br/> { "new_level", `int` }, | character / NONE |
| game_avatar_death | Triggers during bury screen with ASCII grave art is displayed (when avatar dies, obviously) | { "avatar_id", `character_id` },<br/> { "avatar_name", `string` },<br/> { "avatar_is_male", `bool` },<br/> { "is_suicide", `bool` },<br/> { "last_words", `string` }, | avatar / NONE |
| game_avatar_new | Triggers when new character is controlled and during new game character initialization  | { "is_new_game", `bool` },<br/> { "is_debug", `bool` },<br/> { "avatar_id", `character_id` },<br/> { "avatar_name", `string` },<br/> { "avatar_is_male", `bool` },<br/> { "avatar_profession", `profession_id` },<br/> { "avatar_custom_profession", `string` }, | avatar / NONE |
| game_load | Triggers only when loading a saved game (not a new game!) | { "cdda_version", `string` }, | avatar / NONE |
| game_begin | Triggered during game load and new game start | { "cdda_version", `string` }, | avatar / NONE |
| game_over | Triggers after fully accepting death, epilogues etc. have played (probably not useable for eoc purposes?) | { "total_time_played", `chrono_seconds` }, | avatar / NONE |
| game_save | | { "time_since_load", `chrono_seconds` },<br/> { "total_time_played", `chrono_seconds` }, | avatar / NONE |
| game_start | Triggered only during new game character initialization | { "game_version", `string` }, | avatar / NONE |
| installs_cbm | | { "character", `character_id` },<br/> { "bionic", `bionic_id` }, | character / NONE |
| installs_faulty_cbm | | { "character", `character_id` },<br/> { "bionic", `bionic_id` }, | character / NONE |
| learns_martial_art | |  { "character", `character_id` },<br/> { "martial_art", `matype_id` }, | character / NONE |
| loses_addiction | | { "character", `character_id` },<br/> { "add_type", `addiction_id` }, | character / NONE |
| loses_mutation | |  { "character", `character_id` },<br/> { "trait", `trait_id` }, | character / NONE |
| npc_becomes_hostile | Triggers when NPC's attitude is set to `NPCATT_KILL` via dialogue effect `hostile` | { "npc", `character_id` },<br/> { "npc_name", `string` }, | NPC / NONE |
| opens_portal | Triggers when TOGGLE PORTAL option is activated via ("old lab" finale's?) computer | NONE | avatar / NONE |
| opens_spellbook | Triggers when player opens the spell menu OR when NPC evaluates spell as best weapon(in preparation to use it) | { "character", `character_id` } | character / NONE |
| opens_temple | Triggers when `pedestal_temple` examine action is used to consume a petrified eye | NONE | avatar / NONE |
| player_fails_conduct | | { "conduct", `achievement_id` },<br/> { "achievements_enabled", `bool` }, | avatar / NONE |
| player_gets_achievement | | { "achievement", `achievement_id` },<br/> { "achievements_enabled", `bool` }, | avatar / NONE |
| player_levels_spell | triggers when player changes it's spell level, either by casting a spell, reading spell book, or using EoC. Spawning a new character with spells defined by using `spells` in chargen option will also run an event | { "character", `character_id` },<br/>{ "spell", `spell_id` },<br/>{ "new_level", `int` },{ "spell_class", `trait_id` } | character / NONE |
| reads_book | | { "character", `character_id` } | character / NONE |
| releases_subspace_specimens | Triggers when Release Specimens option is activated via ("old lab" finale's?) computer | NONE | avatar / NONE |
| removes_cbm | |  { "character", `character_id` },<br/> { "bionic", `bionic_id` }, | character / NONE |
| seals_hazardous_material_sarcophagus | Triggers via `srcf_seal_order` computer action | NONE | avatar / NONE |
| spellcasting_finish | Triggers only once when a character finishes casting a spell | { "character", `character_id` },<br/> { "success", `bool` },<br/>  { "spell", `spell_id` },<br/> { "school", `trait_id` },<br/> { "difficulty", `int` },<br/> { "cost", `int` },<br/> { "cast_time", `int` },<br/> { "damage", `int` }, | character / NONE |
| telefrags_creature | (Unimplemented) | { "character", `character_id` },<br/> { "victim_name", `string` }, | character / NONE |
| teleglow_teleports | Triggers when character(only avatar is actually eligible) is teleported due to teleglow effect | { "character", `character_id` } | character / NONE |
| teleports_into_wall | Triggers when character(only avatar is actually eligible) is teleported into wall | { "character", `character_id` },<br/> { "obstacle_name", `string` }, | character / NONE |
| terminates_subspace_specimens | Triggers when Terminate Specimens option is activated via ("old lab" finale's?) computer | NONE | avatar / NONE |
| throws_up | | { "character", `character_id` } | character / NONE |
| triggers_alarm | Triggers when alarm is sounded as a failure state of hacking, prying, using a computer, or (if player is sufficiently close)damaged terrain with ALARMED flag | { "character", `character_id` } | character / NONE |
| uses_debug_menu | | { "debug_menu_option", `debug_menu_index` }, | avatar / NONE |
| u_var_changed | | { "var", `string` },<br/> { "value", `string` }, | avatar / NONE |
| vehicle_moves | | { "avatar_on_board", `bool` },<br/> { "avatar_is_driving", `bool` },<br/> { "avatar_remote_control", `bool` },<br/> { "is_flying_aircraft", `bool` },<br/> { "is_floating_watercraft", `bool` },<br/> { "is_on_rails", `bool` },<br/> { "is_falling", `bool` },<br/> { "is_sinking", `bool` },<br/> { "is_skidding", `bool` },<br/> { "velocity", `int` }, // vehicle current velocity, mph * 100<br/> { "z", `int` }, | avatar / NONE |

## Context Variables For Other EOCs
Other EOCs have some variables as well that they have access to, they are as follows:

| EOC            | Context Variables |
| --------------------- | ----------- |
| mutation: "activated_eocs" | { "this", `mutation_id` } |
| mutation: "processed_eocs" | { "this", `mutation_id` } |
| mutation: "deactivated_eocs" | { "this", `mutation_id` } |
| damage_type: "ondamage_eocs" | { "bp", `bodypart_id` }, { "damage_taken", `double` damage the character will take post mitigation }, { "total_damage", `double` damage pre mitigation } |
| furniture: "examine_action" | { "this", `furniture_id` }, { "pos", `tripoint` } |


# Effects

## General

#### `sound_effect`
Play a sound effect from sound pack `"type": "sound_effect"`


| Syntax | Optionality | Value  | Info |
| --- | --- | --- | --- | 
| "sound_effect" | **mandatory** | string or [variable object](#variable-object) | sound effect, that would be used, respond to `variant` field in `"type": "sound_effect"` |
| "id" | optional | string or [variable object](#variable-object) | `id`, that would be used to play, respond to `id` field in `"type": "sound_effect"`  | 
| "outdoor_event" | optional | boolean | default false; if true, and player is underground, the player is less likely to hear the sound | 
| "volume" | optional | int or [variable object](#variable-object)  | default 80; volume at which the sound would be played; affected by hearing modifier | 

##### Valid talkers:

| Avatar | Character | NPC | Monster |  Furniture | Item |
| ------ | --------- | --------- | ---- | ------- | --- | 
| ✔️ | ✔️ | ✔️ | ✔️ | ✔️ | ✔️ |

##### Examples

Plays sound `bionics`, variant `pixelated` with volume 50
```json
{ "sound_effect": "pixelated", "id": "bionics", "volume": 50 },
```


#### `open_dialogue`
Opens up a dialog between the participants; this should only be used in effect_on_conditions, not in actual npc dialogue

| Syntax | Optionality | Value  | Info |
| --- | --- | --- | --- | 
| "topic" | optional | string or [variable object](#variable-object) | if used, instead of the dialogue with the participant, this topic would be used with an empty talker |
| "true_eocs", "false_eocs" | optional | string, [variable object](##variable-object), inline EoC, or range of all of them | if was successful, all EoCs from `true_eocs` are run, otherwise all EoCs from `false_eocs` are run | 

##### Valid talkers:

| Avatar | Character | NPC | Monster |  Furniture | Item |
| ------ | --------- | --------- | ---- | ------- | --- | 
| ✔️ | ✔️ | ✔️ | ✔️ | ✔️ | ✔️ |

##### Examples
Opens dialogue with topic `TALK_PERK_MENU_MAIN`
```json
{ "open_dialogue": { "topic": "TALK_PERK_MENU_MAIN" } }
```

Opens a dialogue with computer; computer has defined `"chat_topics": [ "COMP_REFUGEE_CENTER_MAIN" ],` on the map side, which makes it valid participant
```json
  {
    "id": "EOC_REFUGEE_CENTER_COMPUTER",
    "type": "effect_on_condition",
    "effect": [ "open_dialogue" ]
  },
```

#### `take_control`
If beta talker is NPC, take control of it


| Syntax | Optionality | Value  | Info |
| --- | --- | --- | --- | 
| "take_control" | **mandatory** | simple string | makes you control the NPC; works only if avatar (you) is alpha talker, and beta talker is NPC |
| "true_eocs", "false_eocs" | optional | string, [variable object](##variable-object), inline EoC, or range of all of them | if `take_control` was successful, all `true_eocs` are run, otherwise all `false_eocs` run | 

##### Valid talkers:

| Avatar | Character | NPC | Monster |  Furniture | Item |
| ------ | --------- | --------- | ---- | ------- | ---  | 
| ❌ | ❌ | ✔️ | ❌ | ❌ | ❌ |

##### Examples
Takes control of NPC
```json
"effect": [ "take_control" ]
```

Takes control of NPC; If successful; `EOC_GOOD` is run, if not, `EOC_BAD` is run
```json
{ "take_control": { "true_eocs": [ "EOC_GOOD" ], "false_eocs": [ "EOC_BAD" ] } }
```

#### `take_control_menu`
Opens up a menu to choose a follower to take control of.
Works only with your followers

##### Valid talkers:

| Avatar | Character | NPC | Monster |  Furniture | Item |
| ------ | --------- | --------- | ---- | ------- | ---  | 
| ✔️ | ❌ | ❌ | ❌ | ❌ | ❌ |

##### Examples
Opens the menu to swap the avatar
```json
"effect": [ "take_control_menu" ]
```


#### `give_achievement`
Marks the given achievement as complete

| Syntax | Optionality | Value  | Info |
| --- | --- | --- | --- | 
| "give_achievement" | **mandatory** | string or [variable object](#variable-object) | the achievement that would be given |

##### Valid talkers:

| Avatar | Character | NPC | Monster |  Furniture | Item |
| ------ | --------- | --------- | ---- | ------- | ---  | 
| ✔️ | ❌ | ❌ | ❌ | ❌ | ❌ |

##### Examples
Gives achievement `escaped_the_cataclysm`
```json
{ "give_achievement": "escaped_the_cataclysm" }
```

`assign_mission: `string or [variable object](#variable-object)   Will assign mission to the player.

#### `assign_mission`
Will assign mission to the player

| Syntax | Optionality | Value  | Info |
| --- | --- | --- | --- | 
| "assign_mission" | **mandatory** | string or [variable object](#variable-object) | Mission that would be assigned to the player |

##### Valid talkers:

| Avatar | Character | NPC | Monster |  Furniture | Item |
| ------ | --------- | --------- | ---- | ------- | --- | 
| ✔️ | ❌ | ❌ | ❌ | ❌ | ❌ |

##### Examples
Assign you a `MISSION_REACH_FAKE_DAVE` mission
```json
{ "assign_mission": "MISSION_REACH_FAKE_DAVE" }
```

#### `remove_active_mission`
Will remove mission from the player's active mission list without failing it.

| Syntax | Optionality | Value  | Info |
| --- | --- | --- | --- | 
| "remove_active_mission" | **mandatory** | string or [variable object](#variable-object) | mission that would be removed |

##### Valid talkers:

| Avatar | Character | NPC | Monster |  Furniture | Item |
| ------ | --------- | --------- | ---- | ------- | --- | 
| ✔️ | ✔️ | ✔️ | ✔️ | ✔️ | ✔️ |

##### Examples
removes `MISSION_BONUS_KILL_BOSS` mission from your list
```json
{ "remove_active_mission": "MISSION_BONUS_KILL_BOSS" }
```


#### `finish_mission`
Will complete mission the player has, in one way or another
// todo - test how optional `success` and `step` actually are

| Syntax | Optionality | Value  | Info |
| --- | --- | --- | --- | 
| "finish_mission" | **mandatory** | string or [variable object](#variable-object) | id of the mission that would be finished |
| "success" | optional | boolean | default false; if true, complete the mission as successful | 
| "step" | optional | int | if used, complete mission up to this step | 

##### Valid talkers:

| Avatar | Character | NPC | Monster |  Furniture | Item |
| ------ | --------- | --------- | ---- | ------- | --- | 
| ✔️ | ✔️ | ✔️ | ✔️ | ✔️ | ✔️ |

##### Examples
Complete the mission `DID_I_WIN` as failed
```json
{ "finish_mission": "DID_I_WIN" }
```

Complete the mission `DID_I_WIN` as successful
```json
{ "finish_mission": "DID_I_WIN", "success": true }
```

Complete the first step of a `DID_I_WIN` mission
```json
{ "finish_mission": "DID_I_WIN", "step": 1 }
```

#### `offer_mission`
Adds this mission on a list NPC can offer

| Syntax | Optionality | Value  | Info |
| --- | --- | --- | --- | 
| "offer_mission" | **mandatory** | string, [variable object](#variable-object) or array | id of a mission to offer |


##### Valid talkers:

| Avatar | Character | NPC | Monster |  Furniture | Item |
| ------ | --------- | --------- | ---- | ------- | --- | 
| ❌ | ❌ | ✔️ | ❌ | ❌ | ❌ |

##### Examples
NPC can offer mission `MISSION_GET_RELIC` now
```json
{ "offer_mission": "MISSION_GET_RELIC" }
```

Same as before
```json
{ "offer_mission": [ "MISSION_GET_RELIC" ] }
```

NPC can offer missions `MISSION_A`, `B` and `C` now
```json
{ "offer_mission": [ "MISSION_A", "MISSION_B", "MISSION_C" ] }
```


#### `run_eocs`
Runs another EoC. It can be a separate EoC, or an inline EoC inside `run_eocs` effect

| Syntax | Optionality | Value  | Info |
| --- | --- | --- | --- | 
| "run_eocs" | **mandatory** | string (eoc id or inline eoc) or [variable object](#variable-object)) or array of eocs | EoC or EoCS that would be run |

##### Valid talkers:

| Avatar | Character | NPC | Monster |  Furniture | Item |
| ------ | --------- | --------- | ---- | ------- | --- | 
| ✔️ | ✔️ | ✔️ | ✔️ | ✔️ | ✔️ |

##### Examples
Run `EOC_DO_GOOD_THING` EoC
```json
{ "run_eocs": [ "EOC_DO_GOOD_THING" ] }
```

Run inline `are_you_strong` EoC
```json
"run_eocs": {
  "id": "are_you_strong",
  "condition": { "math": [ "u_val('strength')", ">", "8" ] },
  "effect": [ { "u_message": "You are strong" } ],
  "false_effect": [ { "u_message": "You are normal" } ]
}
```

Inline EoCs could have their own inline EoCS
This EoC checks your str stat, and if it's less than 4, write `You are weak`; 
if it's bigger, `are_you_strong` EoC is run, that checks is your str is bigger than 8; if it's less, `You are normal` is written
if it's bigger, `are_you_super_strong` effect is run, that checks is your str is bigger than 12; If it's less, `You are strong` is written; if it's more, `You are super strong` is written
```json
{
  "type": "effect_on_condition",
  "id": "are_you_weak",
  "//": "there is a variety of ways you can do the exact same effect that would work better",
  "//2": "but for the sake of example, let's ignore it",
  "condition": { "math": [ "u_val('strength')", ">", "4" ] },
  "false_effect": [ { "u_message": "You are weak" } ],
  "effect": {
    "run_eocs": {
      "id": "are_you_strong",
      "condition": { "math": [ "u_val('strength')", ">", "8" ] },
      "false_effect": [ { "u_message": "You are normal" } ],
      "effect": {
        "run_eocs": {
          "id": "are_you_super_strong",
          "condition": { "math": [ "u_val('strength')", ">", "12" ] },
          "effect": [ { "u_message": "You are super strong" } ],
          "false_effect": [ { "u_message": "You are strong" } ]
        }
      }
    }
  }
}
```

Use Context Variable as a eoc (A trick for loop)
```json
  {
    "type": "effect_on_condition",
    "id": "debug_eoc_for_loop",
    "effect": [ { "run_eoc_with": "eoc_for_loop", "variables": { "i": "0", "length": "10", "eoc": "eoc_msg_hello_world" } } ]
  },
  {
    "type": "effect_on_condition",
    "id": "eoc_msg_hello_world",
    "effect": [ { "u_message": "hello world" } ]
  },
  {
    "type": "effect_on_condition",
    "id": "eoc_for_loop",
    "condition": { "and": [ { "expects_vars": [ "i", "length", "eoc" ] }, { "math": [ "_i", "<", "_length" ] } ] },
    "effect": [ { "run_eocs": [ { "context_val": "eoc" } ] }, { "math": [ "_i", "++" ] }, { "run_eocs": "eoc_for_loop" } ],
    "//": "Checks i is lesser than length, and if true, runs EoC, stored in context val 'eoc', increment i, and runs itself again; all context variables would be saved and passed to the next iteration"
  }
```
#### `u_set_talker`, `npc_set_talker`
Store the character_id of You or NPC into a variable object

| Syntax | Optionality | Value  | Info |
| --- | --- | --- | --- | 
| "u_set_talker" / "npc_set_talker" | **mandatory** | [variable object](#variable-object) | the variable object to store the character_id |

##### Examples

```json
{
  "effect": [ 
    { "u_set_talker": { "global_val": "u_character_id" } }, 
    { "u_message": "Your character id is <global_val:u_character_id>" }
  ]
}
```

##### Valid talkers:

| Avatar | Character | NPC | Monster |  Furniture | Item |
| ------ | --------- | --------- | ---- | ------- | --- | 
| ✔️ | ✔️ | ✔️ | ❌ | ❌ | ❌ |

#### `run_eoc_with`
Same as `run_eocs`, but runs the specific EoC with provided variables as context variables

| Syntax | Optionality | Value  | Info |
| --- | --- | --- | --- | 
| "run_eoc_with" | **mandatory** | string (eoc id or inline eoc) | EoC or EoCS that would be run |
| "alpha_loc","beta_loc" | optional | string, [variable object](#variable-object) | `u_location_variable`, where the EoC should be run. Set the alpha/beta talker to the creature at the location. |
| "alpha_talker","beta_talker" | optional (If you use both "alpha_loc" and "alpha_talker", "alpha_talker" will be ignored.) | string, [variable object](#variable-object) | Set alpha/beta talker. This can be either a `character_id` (you can get from [EOC event](#event-eocs) or [u_set_talker](#u_set_talkernpc_set_talker) ) or some hard-coded values: <br> `""`: null talker <br> `"u"/"npc": the alpha/beta talker of the EOC`(Should be Avatar/Character/NPC/Monster) <br> `"avatar"`: your avatar|
| "false_eocs" | optional | string, [variable object](#variable-object), inline EoC, or range of all of them | false EOCs will run if<br>1. there is no creature at "alpha_loc"/"beta_loc",or<br>2. "alpha_talker" or "beta_talker" doesn't exist in the game (eg. dead NPC),or<br>3. alpha and beta talker are both null |
| "variables" | optional | pair of `"variable_name": "varialbe"` | variables, that would be passed to the EoC;  all variables should be strings, even if it's int; `expects_vars` condition can be used to ensure every variable exist before the EoC is run | 

##### Valid talkers:

| Avatar | Character | NPC | Monster |  Furniture | Item |
| ------ | --------- | --------- | ---- | ------- | --- | 
| ✔️ | ✔️ | ✔️ | ✔️ | ✔️ | ✔️ |

##### Examples
Run `EOC_BOOM` EoC; completely same as `run_eocs`
```json
{ "run_eoc_with": "EOC_BOOM" }
```

Run `EOC_BOOM` EoC at `where_my_enemy_is` location variable
```json
{ "run_eoc_with": "EOC_BOOM", "beta_loc": { "global_val": "where_my_enemy_is" } },
```

The first EoC `EOC_I_NEED_AN_AR15` run another `EOC_GIVE_A_GUN` EoC, and give it two variables: variable `gun_name` with value `ar15_223medium` and variable `amount_of_guns` with value `5`;
Second EoC `EOC_I_NEED_AN_AK47` aslo run `EOC_GIVE_A_GUN` with the same variables, but now the values are `ak47` and `3`
`EOC_GIVE_A_GUN`, once called, will spawn a gun, depending on variables it got
```json
{
  "type": "effect_on_condition",
  "id": "EOC_I_NEED_AN_AR15",
  "effect": [
    {
      "run_eoc_with": "EOC_GIVE_A_GUN",
      "variables": {
        "gun_name": "ar15_223medium",
        "amount_of_guns": "5"
      }
    }
  ]
},
{
  "type": "effect_on_condition",
  "id": "EOC_I_NEED_AN_AK47",
  "effect": [
    {
      "run_eoc_with": "EOC_GIVE_A_GUN",
      "variables": {
        "gun_name": "ak47",
        "amount_of_guns": "3"
      }
    }
  ]
},
{
  "type": "effect_on_condition",
  "id": "EOC_GIVE_A_GUN",
  "condition": { "expects_vars": [ "gun_name", "amount_of_guns" ] },
  "effect": [
    {
      "u_spawn_item": { "context_val": "gun_name" },
      "count": { "context_val": "amount_of_guns" }
    }
  ]
}
```
Control a NPC and return to your original body.
By using `EOC_control_npc`, you can gain control of an NPC, and your original body's character_id will be stored in the global variable `"player_id"`.
Then, by using `EOC_return_to_player`, you can return to your original body.
```json

{
  "type": "effect_on_condition",
  "id": "EOC_control_npc",
  "effect": [
    { "u_set_talker": { "global_val": "player_id" } },
    {
      "if": { "u_query_tile": "anywhere", "target_var": { "context_val": "loc" }, "message": "Select point" },
      "then": {
        "run_eoc_with": {
          "id": "_EOC_control_npc_do",
          "effect": [
            {
              "if": "npc_is_npc",
              "then": [ "follow", "take_control" ],
              "else": { "message": "Please select a NPC." }
            }
          ]
        },
        "beta_loc": { "context_val": "loc" },
        "false_eocs": { "id": "_EOC_control_npc_fail_msg", "effect": { "message": "Please select a NPC." } }
      },
      "else": { "u_message": "Canceled" }
    }
  ]
},
{
  "type": "effect_on_condition",
  "id": "EOC_return_to_player",
  "effect": [
    {
      "run_eoc_with": { "id": "_EOC_return_to_player_do", "effect": [ "follow", "take_control" ] },
      "alpha_talker": "avatar",
      "beta_talker": { "global_val": "player_id" },
      "false_eocs": { "id": "_EOC_return_to_player_fail_msg", "effect": { "message": "Unable to locate your original body." } }
    }
  ]
}
```

#### `queue_eocs`
  Same as `run_eocs`, but instead of running EoCs right now, put it in queue and run it in some future


| Syntax | Optionality | Value  | Info |
| --- | --- | --- | --- | 
| "queue_eocs" | **mandatory** | string (eoc id or inline eoc) or [variable object](#variable-object) or array of eocs | EoCs, that would be added into queue; Could be an inline EoC |
| "time_in_future" | optional | int, duration, [variable object](#variable-object) or value between two | When in the future EoC would be run; default 0 | 

##### Valid talkers:
If the eoc is global the avatar will be u and npc will be invalid. Otherwise it will be queued for the current alpha if they are a character and not be queued otherwise.

| Avatar | Character | NPC | Monster |  Furniture | Item |
| ------ | --------- | --------- | ---- | ------- | --- | 
| ✔️ | ✔️ | ✔️ | ❌ | ❌ | ❌ |

##### Examples
run `EOC_BOOM_RIGHT_NOW` instantly
```json
{ "queue_eocs": "EOC_BOOM_RIGHT_NOW" }
```

run `EOC_BOOM_RANDOM` randomly in 20-30 seconds
```json
{ "queue_eocs": "EOC_BOOM_RANDOM", "time_in_future": [ "20 seconds", "30 seconds" ] },
```

#### `queue_eoc_with`
Combination of `run_eoc_with` and `queue_eocs` - Put EoC into queue and run into some future, with provided variables as context variables

| Syntax | Optionality | Value  | Info |
| --- | --- | --- | --- | 
| "queue_eoc_with" | **mandatory** | string (eoc id or inline eoc) | EoC, that would be added into queue; Could be an inline EoC |
| "time_in_future" | optional | int, duration, [variable object](#variable-object) or value between two | When in the future EoC would be run; default 0 |
| "variables" | optional | pair of `"variable_name": "varialbe"` | variables, that would be passed to the EoC; `expects_vars` condition can be used to ensure every variable exist before the EoC is run | 

##### Valid talkers:

| Avatar | Character | NPC | Monster |  Furniture | Item |
| ------ | --------- | --------- | ---- | ------- | --- | 
| ✔️ | ✔️ | ✔️ | ❌ | ❌ | ❌ |

##### Examples
see a detailed example in `run_eoc_with`
In three hours, you will be given five AR-15
```json
{
  "type": "effect_on_condition",
  "id": "EOC_I_NEED_AN_AR15_BUT_NOT_NOW",
  "effect": [
    {
      "queue_eoc_with": "EOC_GIVE_A_GUN",
      "time_in_future": "3 h",
      "variables": {
        "gun_name": "ar15_223medium",
        "amount_of_guns": 5
      }
    }
  ]
},
{
  "type": "effect_on_condition",
  "id": "EOC_GIVE_A_GUN",
  "condition": { "expects_vars": [ "gun_name", "amount_of_guns" ] },
  "effect": [
    {
      "u_spawn_item": { "context_val": "gun_name" },
      "count": { "context_val": "amount_of_guns" }
    }
  ]
}
```

#### `u_roll_remainder`, `npc_roll_remainder`
If you or NPC does not have all of the listed bionics, mutations, spells or recipes, gives one randomly

| Syntax | Optionality | Value  | Info |
| --- | --- | --- | --- | 
| "u_roll_remainder" / "npc_roll_remainder" | **mandatory** | string, [variable](#variable-object) or array of both | thing, that would be rolled and given |
| "type" | **mandatory** | string or [variable object](#variable-object) | type of thing that would be given; can be one of `bionic`, `mutation`, `spell` or `recipe` | 
| "message" | optional | string or [variable object](#variable-object) | message, that would be displayed in log, once remainder is used; `%s` symbol can be used in this message to write the name of a thing, that would be given; message would be printed only if roll was successful | 
| "true_eocs", "false_eocs" | optional | string, [variable object](##variable-object), inline EoC, or range of all of them | If reminder was positive, all EoCs in `true_eocs` run, otherwise `false_eocs` run | 


##### Valid talkers:

| Avatar | Character | NPC | Monster |  Furniture | Item |
| ------ | --------- | --------- | ---- | ------- | --- | 
| ✔️ | ✔️ | ✔️ | ❌ | ❌ | ❌ |

##### Examples
Tries to give you a mutation `A`, `B` or `C`, if you don't have one, with message `You got %s!`; If roll is successful, `EOC_SUCCESS` is run, otherwise `EOC_FAIL` is run
```json
{
  "u_roll_remainder": [ "mutationA", "mutationB", "mutationC" ],
  "type": "mutation",
  "message": "You got %s!",
  "true_eocs": [ "EOC_SUCCESS" ],
  "false_eocs": [ "EOC_FAIL" ]
}
```


#### `if`
Set effects to be executed when conditions are met and when conditions are not met.

| Syntax | Optionality | Value  | Info |
| --- | --- | --- | --- | 
| "if" | **mandatory** | [dialogue condition](#dialogue-conditions) | condition itself | 
| "then" | **mandatory** | effect | Effect(s) executed when conditions are met. | 
| "else" | optional | effect | Effect(s) executed when conditions are not met. | 

##### Valid talkers:

| Avatar | Character | NPC | Monster |  Furniture | Item |
| ------ | --------- | --------- | ---- | ------- | --- | 
| ✔️ | ✔️ | ✔️ | ✔️ | ✔️ | ✔️ |

##### Examples
Displays a different message the first time it is run and the second time onwards
```json
{
  "if": { "u_has_var": "test", "type": "eoc_sample", "context": "if_else", "value": "yes" },
  "then": { "u_message": "You have variable." },
  "else": [
    { "u_message": "You don't have variable." },
    {
      "if": { "not": { "u_has_var": "test", "type": "eoc_sample", "context": "if_else", "value": "yes" } },
      "then": [
        { "u_add_var": "test", "type": "eoc_sample", "context": "if_else", "value": "yes" },
        { "u_message": "Vriable added." }
      ]
    }
  ]
}
```


#### `switch`
Check the value, and, depending on it, pick the case that would be run

| Syntax | Optionality | Value  | Info |
| --- | --- | --- | --- | 
| "switch" | **mandatory** | variable/math_expression | the value, that would be read; only numerical values can be used |
| "cases" | **mandatory** | `case` and `effect` | effects, that would be run, if the value of switch is higher or equal to this case | 

##### Valid talkers:

| Avatar | Character | NPC | Monster |  Furniture | Item |
| ------ | --------- | --------- | ---- | ------- | --- | 
| ✔️ | ✔️ | ✔️ | ❌ | ❌ | ❌ |

##### Examples
Checks the level of `some_spell` spell, and, related to this, do something: for spell level 0 it casts another_spell, for spell level 3 it adds effect "drunk", and so on.
```json
{
  "switch": { "math": [ "u_spell_level('some_spell')" ] },
  "cases": [
    { "case": 0, "effect": { "u_cast_spell": { "id": "another_spell" } } },
    { "case": 3, "effect": { "u_add_effect": "drunk", "duration": "270 minutes" } },
    { "case": 6, "effect": { "u_lose_bionic": "bio_power_storage" } },
    { "case": 9, "effect": { "run_eocs": [ "EOC_DO_GOOD_THING" ] } },
    {
      "case": 12,
      "effect": [
        { "u_forget_martial_art": "style_eskrima" },
        { "u_forget_martial_art": "style_crane" },
        { "u_forget_martial_art": "style_judo" }
      ]
    }
  ]
}
```


#### `foreach`
Executes the effect repeatedly while storing the values ​​of a specific list one by one in the variable.

| Syntax | Optionality | Value  | Info |
| --- | --- | --- | --- |
| "foreach" | **mandatory** | string | Type of list. `"ids"`, `"item_group"`, `"monstergroup"`, `"array"` are available. |
| "var" | **mandatory** | [variable objects](#variable-object) | Variable to store value in the list. |
| "effect" | **mandatory** | effect | Effect(s) executed. |
| "target" | **mandatory** | See below | Changes depending on the value of "foreach". See below. |

The correspondence between "foreach" and "target" is as follows.

| "foreach" | Value | Info |
| --- | --- | --- |
| "ids" | string | List the IDs of objects that appear in the game. `"bodypart"`, `"flag"`, `"trait"`, `"vitamin"` are available. |
| "item_group" | string | List the IDs of items in the item group. |
| "monstergroup" | string | List the IDs of monsters in the monster group. |
| "array" | array of strings or [variable objects](#variable-object) | List simple strings. |

##### Valid talkers:

| Avatar | Character | NPC | Monster |  Furniture | Item |
| ------ | --------- | --------- | ---- | ------- | --- |
| ✔️ | ✔️ | ✔️ | ✔️ | ✔️ | ✔️ |

##### Examples
Resets all of your vitamins.
```json
{
  "foreach": "ids",
  "var": { "context_val": "id" },
  "target": "vitamin",
  "effect": [ { "math": [ "u_vitamin(_id)", "=", "0" ] } ]
}
```


#### `u_run_npc_eocs`, `npc_run_npc_eocs`
NPC run EoCs, provided by this effect; can work outside of reality bubble

| Syntax | Optionality | Value  | Info |
| --- | --- | --- | --- | 
| "u_run_npc_eocs"/ "npc_run_npc_eocs" | **mandatory** | array of eocs | EoCs that would be run by NPCs |
| "unique_ids" | optional | string, [variable objects](#variable-object) or array | id of NPCs that would be affected; lack of ids make effect run EoC on every NPC in your reality bubble, if `"local": true`, and to every NPC in the world, if `"local": false`; unique ID of every npc is specified in mapgen, using `npcs` or `place_npcs` | 
| "local" | optional | boolean | default false; if true, the effect is run for every NPC in the world; if false, effect is run only to NPC in your reality bubble |
| "npc_range" | optional | int or [variable object](#variable-object) | if used, only NPC in this range are affected |
| "npc_must_see" | optional | boolean | default false; if true, only NPC you can see are affected | 
 
example of specifying `unique_id` in mapgen using `npcs`:
```json
"npcs": { "T": { "class": "guard", "unique_id": "GUARD7" } },
```

and using `place_npcs`:
```json
"place_npcs": [ { "class": "arsonist", "x": 9, "y": 1, "unique_id": "GUARD7" } ],
```

##### Valid talkers:

| Avatar | Character | NPC | Monster |  Furniture | Item |
| ------ | --------- | --------- | ---- | ------- | --- | 
| ❌ | ❌ | ✔️ | ❌ | ❌ | ❌ |

##### Examples

All NPC in range 30, that you can see, run `EOC_DEATH` and `EOC_TAXES`
```json
{
  "type": "effect_on_condition",
  "id": "EOC_KILL_ALL_NPCS_YOU_SEE_30_TILES",
  "effect": [
    {
      "u_run_npc_eocs": [ "EOC_DEATH", "EOC_TAXES" ],
      "npc_range": 30,
      "npc_must_see": true
    }
  ]
}
```

Move refugee center guards `GUARD1` - `GUARD7` to the `_First` position - EoC for effect is inlined
```json
{
  "type": "effect_on_condition",
  "id": "EOC_REFUGEE_CENTER_GUARD_FIRST_POSITION",
  "effect": [
    {
      "u_run_npc_eocs": [
        {
          "id": "EOC_REFUGEE_CENTER_GUARD_FIRST_SHIFT",
          "effect": {
            "u_set_guard_pos": { "global_val": "_First" },
            "unique_id": true
          }
        }
      ],
      "unique_ids": [ "GUARD1", "GUARD2", "GUARD3", "GUARD4", "GUARD5", "GUARD6", "GUARD7" ]
    }
  ]
}
```


#### `u_run_inv_eocs`, `npc_run_inv_eocs`
Run EOCs on items in your or NPC's inventory

| Syntax | Optionality | Value  | Info |
| --- | --- | --- | --- | 
| "u_run_inv_eocs" / "npc_run_inv_eocs" | **mandatory** | string or [variable object](#variable-object) | way the item would be picked; <br/>values can be:<br/>`all` - all items that match the conditions are picked;<br/> `random` - from all items that match the conditions, one picked;<br/>`manual` - menu is open with all items that can be picked, and you can choose one;<br/>`manual_mult` - same as `manual`, but multiple items can be picked |
| "search_data" | optional | `search_data` | sets the condition for the target item; lack of search_data means any item can be picked; conditions can be:<br/>`id` - id of a specific item;<br/>`category` - category of an item (case sensitive, should always be in lower case);<br/>`flags`- flag or flags the item has<br/>`excluded_flags`- flag or flags the item doesn't have<br/>`material` - material of an item;<br/>`worn_only` - if true, return only items, that are worn;<br/>`wielded_only` - if true, return only wielded items | 
| "title" | optional | string or [variable object](#variable-object) | name of the menu, that would be shown, if `manual` or `manual_mult` is used | 
| "true_eocs" / "false_eocs" | optional | string, [variable object](##variable-object), inline EoC, or range of all of them | if item was picked successfully, all EoCs from `true_eocs` are run, otherwise all EoCs from `false_eocs` are run; picked item is returned as npc; for example, `n_hp()` return hp of an item | 

##### Valid talkers:

| Avatar | Character | NPC | Monster |  Furniture | Item |
| ------ | --------- | --------- | ---- | ------- | --- | 
| ❌ | ❌ | ❌ | ❌ | ❌ | ✔️ |

##### Examples
Picks an item in character's hands, and run `EOC_DESTROY_ITEM` EoC on it
```json
{
  "type": "effect_on_condition",
  "id": "EOC_PICK_ITEM_IN_HANDS",
  "effect": [
    {
      "u_run_inv_eocs": "random",
      "search_data": [ { "wielded_only": true } ],
      "true_eocs": [ "EOC_DESTROY_ITEM" ]
    }
  ]
}
```
Pick a wooden item with `DURABLE_MELEE` and `ALWAYS_TWOHAND` flags, and run `EOC_DO_SOMETHING_WITH_ITEM` on it; if there is no such item, `EOC_NO_SUCH_ITEM` is run
```json
{
  "type": "effect_on_condition",
  "id": "EOC_PICK_WOODEN_ITEM",
  "effect": [
    {
      "u_run_inv_eocs": "manual",
      "search_data": [ { "material": "wood", "flags": [ "DURABLE_MELEE", "ALWAYS_TWOHAND" ] } ],
      "true_eocs": [ "EOC_DO_SOMETHING_WITH_ITEM" ]
      "false_eocs": [ { "id": "EOC_NO_SUCH_ITEM", "effect": [ { "u_message": "You don't have an item i need" } ] } ]
    }
  ]
}
```


#### `u_map_run_item_eocs`, `npc_map_run_item_eocs`
Search items around you on the map, and run EoC on them

| Syntax | Optionality | Value  | Info |
| --- | --- | --- | --- | 
| "u_map_run_item_eocs", "npc_map_run_item_eocs" | **mandatory** | string or [variable object](#variable-object) | way the item would be picked; <br/>values can be:<br/>`all` - all items that match the conditions are picked;<br/> `random` - from all items that match the conditions, one picked;<br/>`manual` - menu is open with all items that can be picked, and you can choose one;<br/>`manual_mult` - same as `manual`, but multiple items can be picked |
| "loc" | optional | location variable | location, where items would be scanned; lack of it would scan only tile the talker stands on | 
| "min_radius", "max_radius" | optional | int or [variable object](#variable-object) | radius around the location/talker that would be searched | 
| "title" | optional | string or [variable object](#variable-object) | name of the menu that would be shown, if `manual` or `manual_mult` values are used | 
| "search_data" | optional | `search_data` | sets the condition for the target item; lack of search_data means any item can be picked; conditions can be:<br/>`id` - id of a specific item;<br/>`category` - category of an item (case sensitive, should always be in lower case);<br/>`flags`- flag or flags the item has<br/>`excluded_flags`- flag or flags the item doesn't have<br/>`material` - material of an item;<br/>`worn_only` - if true, return only items, that are worn;<br/>`wielded_only` - if true, return only wielded items | 
| "true_eocs", "false_eocs" | optional | string, [variable object](##variable-object), inline EoC, or range of all of them | if item was picked successfully, all EoCs from `true_eocs` are run, otherwise all EoCs from `false_eocs` are run; picked item is returned as npc | 

##### Valid talkers:

| Avatar | Character | NPC | Monster |  Furniture | Item |
| ------ | --------- | --------- | ---- | ------- | --- | 
| ✔️ | ✔️ | ✔️ | ✔️ | ✔️ | ✔️ |

##### Examples

Run `EOC_GOOD` on all items 5-10 tiles around you (but not tiles 1-4 tiles around you)
```json
  {
    "type": "effect_on_condition",
    "id": "EOC_TEST",
    "effect": [
      {
        "u_map_run_item_eocs": "all",
        "min_radius": 5,
        "max_radius": 10,
        "title": "Something good?",
        "true_eocs": [ "EOC_GOOD" ]
      }
    ]
  }
```

Create context variable `loc` where player stands
Scan all items 1-10 tiles around the `loc` (actually `loc` can be omitted here, since it's the same location as player); open the `Test: Item collection` menu, where player can pick items from said radius; after confirmation, all picked items are run in `EOC_map_item_test_run`
`EOC_map_item_test_run` teleport all items to `loc`, and print `Items rolled at your feet`
```json
{
  "type": "effect_on_condition",
  "id": "EOC_map_item_test1",
  "effect": [
    { "u_location_variable": { "context_val": "loc" } },
    {
      "u_map_run_item_eocs": "manual_mult",
      "loc": { "context_val": "loc" },
      "min_radius": 1,
      "max_radius": 10,
      "title": "Test: Item collection",
      "true_eocs": [
        {
          "id": "EOC_map_item_test_run",
          "effect": [ { "npc_teleport": { "context_val": "loc" } }, { "u_message": "Items rolled at your feet." } ]
        }
      ]
    }
  ]
}
```

Search `crashing_ship_4`  overmap terrain 10 overmap tiles around the player, on z-level -10 (where the ship is places), then in this overmap search `t_escape_pod_floor` terrain, and if found, put it's coordinates to `escape_pod_crate` context value
Search all items on `escape_pod_crate` tile, and run `EOC_AFS_ESCAPE_POD_CARGO_TP` (it will teleport items to `new_map` location)
Print a message with popup
Teleport player to `new_map`
```json
{
  "type": "effect_on_condition",
  "id": "EOC_ESCAPE_POD_CHAIR",
  "effect": [
    {
      "u_location_variable": { "context_val": "escape_pod_crate" },
      "target_params": { "om_terrain": "crashing_ship_4", "search_range": 10, "z": -10 },
      "terrain": "t_escape_pod_floor",
      "target_max_radius": 24
    },
    {
      "u_map_run_item_eocs": "all",
      "loc": { "context_val": "escape_pod_crate" },
      "min_radius": 0,
      "max_radius": 0,
      "true_eocs": [ { "id": "EOC_AFS_ESCAPE_POD_CARGO_TP", "effect": [ { "npc_teleport": { "global_val": "new_map" } } ] } ]
    },
    {
      "u_message": "You make sure that all equipment is strapped down and loose items are put away to avoid projectiles during launch before you strap yourself into the escape pod seat and press the launch button.  With an incredibly loud roar and a massive acceleration that presses you into the seat, the escape pod fires from the ship, plummeting towards the planet below.",
      "popup": true
    },
    { "u_teleport": { "global_val": "new_map" }, "force": true }
  ]
}
```

#### `weighted_list_eocs`
Will choose one of a list of eocs to activate based on it's weight

| Syntax | Optionality | Value  | Info |
| --- | --- | --- | --- | 
| "weighted_list_eocs" | **mandatory** | n/a | EoC that would be run, and it's weight; EoC can be either id or inline EoC, and weight can be int or variable object |

##### Examples

Run one EoC from the list
```json
{
  "type": "effect_on_condition",
  "id": "EOC_WEIGHT",
  "effect": {
    "weighted_list_eocs": [
      [ "EOC_THING_1", 1 ],
      [ "EOC_THING_2", 1 ],
      [ "EOC_THING_3", 3 ],
      { "id": "EOC_THING_4", "effect": [ { "u_message": "A test message appears!", "type": "bad" } ] },
      [ "EOC_THING_5", { "math": [ "super_important_variable + 4" ] } ],
      [ "EOC_THING_6", { "math": [ "_super_important_context_variable + 4" ] } ],
      [ "EOC_THING_7", { "math": [ "33 + 77" ] } ]
    ]
  }
}
```

#### `run_eoc_selector`
Open a menu, that allow to select one of multiple options

| Syntax | Optionality | Value  | Info |
| --- | --- | --- | --- | 
| "run_eoc_selector" | **mandatory** | array of strings or [variable objects](#variable-object) | list of EoCs, that could be picked; conditions of the listed EoCs would be checked, and one that do not pass would be grayed out |
| "names" | optional | array of strings or [variable objects](#variable-object) | name of the option, that would be shown on the list; amount of names should be equal amount of EoCs | 
| "descriptions" | optional | array of strings or [variable objects](#variable-object) | description of the options, that would be shown on the list; amount of descriptions should be equal amount of EoCs | 
| "keys" | optional | single character | a character, that would be used as a shortcut to pick each EoC; amount of keys should be equal amount of EoCs | 
| "title" | optional | string | Text, that would be shown as the name of the list; Default `Select an option.` | 
| "hide_failing" | optional | boolean | if true, the options, that fail their check, would be completely removed from the list, instead of being grayed out | 
| "allow_cancel" | optional | boolean | if true, you can quit the menu without selecting an option, no effect will occur | 
| "variables" | optional | pair of `"variable_name": "varialbe"` | variables, that would be passed to the EoCs; `expects_vars` condition can be used to ensure every variable exist before the EoC is run | 
##### Valid talkers:

| Avatar | Character | NPC | Monster |  Furniture | Item |
| ------ | --------- | --------- | ---- | ------- | --- | 
| ✔️ | ❌ | ❌ | ❌ | ❌ | ❌ |

##### Examples
you can pick one of four options from `Choose your destiny` list; 
```json
{
  "run_eoc_selector": [ "EOC_OPTION_1", "EOC_OPTION_2", "EOC_OPTION_3", "EOC_OPTION_4" ],
  "names": [ "Option 1", "Option 2", "Option 3", "Option 4" ],
  "keys": [ "a", "b", "1", "2" ],
  "title": "Choose your destiny",
  "descriptions": [
    "Gives you something good",
    "Gives you something bad",
    "Gives you twice as good",
    "Gives you twice as bad, but condition is not met, so you can't pick it up"
  ]
}
```

#### `run_eoc_until`
Run EoC multiple times, until specific condition would be met

| Syntax | Optionality | Value  | Info |
| --- | --- | --- | --- | 
| "run_eoc_until" | **mandatory** | string or [variable object](#variable-object) | EoC that would be run multiple times |
| "condition" | **mandatory** | string or [variable object](#variable-object) | name of condition, that would be checked; doesn't support inline condition, so it should be specified in `set_condition` somewhere before the effect; **condition should return "false" to terminate the loop** | 
| "iteration" | optional | int or [variable object](#variable-object) | default 100; amount of iteration, that is allowed to run; if amount of iteration exceed this number, EoC is stopped, and game sends the error message | 

##### Valid talkers:

| Avatar | Character | NPC | Monster |  Furniture | Item |
| ------ | --------- | --------- | ---- | ------- | --- | 
| ✔️ | ✔️ | ✔️ | ✔️ | ✔️ | ✔️ |

##### Examples
`EOC_until_nested` is run until `my_variable` hit 10; in this case 10 times
```json
  {
    "type": "effect_on_condition",
    "id": "EOC_run_until",
    "effect": [
      { "set_condition": "to_test", "condition": { "math": [ "my_variable", "<", "10" ] } },
      { "run_eoc_until": "EOC_until_nested", "condition": "to_test" }
    ]
  },
  {
    "type": "effect_on_condition",
    "id": "EOC_until_nested",
    "effect": [ { "u_spawn_item": "knife_combat" }, { "math": [ "my_variable", "++" ] } ]
  },
```

## Character effects

#### `u_mutate`, `npc_mutate`

Your character or the NPC will attempt to mutate; used in mutation system, for other purposes it's better to use [`u_add_trait`](#`u_add_trait`, `npc_add_trait`)

| Syntax | Optionality | Value  | Info |
| --- | --- | --- | --- | 
| "u_mutate" / "npc_mutate" | **mandatory** | int, float or [variable object](##variable-object) | one in `int` chance of using the highest category, with 0 never using the highest category |
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
| "u_mutate_category" / "npc_mutate_category" | **mandatory** | string or [variable object](##variable-object) | mutation category |
| "use_vitamins" | optional | boolean | same as in `u_mutate` | 

##### Examples

```json
{ "u_mutate_category": "PLANT" }
```

```
{ "u_mutate_category": { "global_val": "next_mutation" }
```

#### `u_mutate_towards`, `npc_mutate_towards`

Similar to the above, but designates a desired end-point of mutation and uses the normal mutate_towards steps to get there, respecting base traits and `changes_to/cancels/types` restrictions.

| Syntax | Optionality | Value  | Info |
| --- | --- | --- | --- | 
| "u_mutate_towards" / "npc_mutate_towards" | **mandatory** | string or [variable object](##variable-object) | Trait ID |
| "category"     | optional | string or [variable object](#variable-object) | default ANY, defines which category to use for the mutation steps - necessary for vitamin usage
| "use_vitamins" | optional | boolean | same as in `u_mutate`, requires a defined `category` | 

##### Examples
Mutate towards Tail Stub (removing any incompatibilities) using the category set in the variable, deprecating that vitamin and using the category's base trait removal chance/multiplier.
```json
      {
        "u_mutate_towards": "TAIL_STUB",
        "category": { "u_val": "mutation_category", "type": "upcoming", "context": "mutation" },
        "use_vitamins": true
      },
```

#### `u_add_effect`, `npc_add_effect`

Some effect would be applied on you or NPC

| Syntax | Optionality | Value  | Info |
| --- | --- | --- | --- | 
| "u_add_effect" / "npc_add_effect" | **mandatory** | string or [variable object](##variable-object) | id of effect to give |
| "duration" | optional | int, duration or [variable object](##variable-object) | 0 by default; length of the effect; both int (`"duration": 60`), and duration string (`"duration": "1 m"`) works; `PERMANENT` can be used to give a permanent effect | 
| "target_part" | optional | string or [variable object](##variable-object) | default is "whole body"; if used, only specified body part would be used. `RANDOM` can be used to pick a random body part | 
| "intensity" | optional | int, float or [variable object](##variable-object) | default 0; intensity of the effect | 
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
| "u_add_bionic"/ "npc_add_bionic" | **mandatory** | string or [variable object](##variable-object) | Your character or the NPC will gain the bionic; Only one bionic per effect | 

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
| "u_lose_bionic" / "npc_lose_bionic"  | **mandatory** | string or [variable object](##variable-object) | Your character or the NPC will lose the bionic | 

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
| "u_add_trait" / "npc_add_trait" | **mandatory** | string or [variable object](##variable-object) | id of trait that should be given |
| "variant" | optional | string or [variable object](##variable-object) | id of the trait's variant |

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

Adds `hair_mohawk` trait with the `purple` variant to the character:
```json
{ "u_add_trait": "hair_mohawk", "variant": "purple" }
```

#### `u_lose_effect`, `npc_lose_effect`
Remove effect from character or NPC, if it has one

| Syntax | Optionality | Value  | Info |
| --- | --- | --- | --- | 
| "u_lose_effect" / "npc_lose_effect" | **mandatory** | string or [variable object](##variable-object) | id of effect to be removed; if character or NPC has no such effect, nothing happens |
| "target_part" | optional | string or [variable object](##variable-object) | default is "whole body"; if used, only specified body part would be used. `RANDOM` can be used to pick a random body part | 

##### Valid talkers:

| Avatar | Character | NPC | Monster |  Furniture | Item |
| ------ | --------- | --------- | ---- | ------- | --- | 
| ✔️ | ✔️ | ✔️ | ✔️ | ❌ | ❌ |

##### Examples
Removes `infection` effect from player:
```json
{ "u_lose_effect": "infection" }
```

Removes `bleed` effect from player's head:
```json
{ "u_lose_effect": "bleed", "target_part": "head" }
```

Removes effect, stored in `effect_id` context value, from the player:
```json
{ "u_lose_effect": { "context_val": "effect_id" } }
```


#### `u_lose_trait`, `npc_lose_trait`
Character or NPC got trait or mutation removed, if it has one

| Syntax | Optionality | Value  | Info |
| --- | --- | --- | --- | 
| "u_lose_trait" / "npc_lose_trait" | **mandatory** | string or [variable object](##variable-object) | id of mutation to be removed; if character or NPC has no such mutation, nothing happens |

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
| "u_activate_trait" / "npc_activate_trait" | **mandatory** | string or [variable object](##variable-object) | id of trait/mutation to be activated |

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
| "u_deactivate_trait" / "u_deactivate_trait" | **mandatory** | string or [variable object](##variable-object) | id of trait/mutation to be deactivated |

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
| "u_learn_martial_art" / "npc_learn_martial_art" | **mandatory** | string or [variable object](##variable-object) | martial art, that would be learned |

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
{ "u_learn_martial_art": { "context_val": "ma_id" } }
```


#### `u_forget_martial_art`, `npc_forget_martial_art`
Your character or the NPC will forget the martial art style.

| Syntax | Optionality | Value  | Info |
| --- | --- | --- | --- | 
| "u_forget_martial_art" / "npc_forget_martial_art" | **mandatory** | string or [variable object](##variable-object) | id of martial art to forget |

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
{ "u_forget_martial_art": { "context_val": "ma_id" } }
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

#### `set_string_var`
Store string from `set_string_var` in the variable object `target_var`


| Syntax | Optionality | Value  | Info |
| --- | --- | --- | --- | 
| "set_string_var" | **mandatory** | string, [variable object](##variable-object), or array of both | value, that would be put into `target_var` |
| "target_var" | **mandatory** | [variable object](##variable-object) | variable, that accept the value; usually `context_val` | 
| "parse_tags" | optional | boolean | Allo if parse [custom entries](NPCs.md#customizing-npc-speech) in string before storing | 


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

Concatenate string of variable `foo` and `bar`
```json
{ "set_string_var": "<global_val:foo><global_val:bar>", "target_var": { "global_val": "new" }, "parse_tags": true }
```


#### `set_condition`
Create a context value with condition, that you can pass down the next topic or EoC, using `get_condition`. Used, if you need to have dozens of EoCs, and you don't want to copy-paste it's conditions every time (also it's easier to maintain or edit one condition, comparing to two dozens)

| Syntax | Optionality | Value  | Info |
| --- | --- | --- | --- | 
| "set_condition" | **mandatory** | string or [variable object](##variable-object) | id of condition |
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
Search a specific coordinates of map around `u_`, `npc_` or `target_params` and save them in [variable](##variable-object)

| Syntax | Optionality | Value | Info |
| --- | --- | --- | --- | 
| "u_location_variable" / "npc_location_variable" | **mandatory** | [variable object](##variable-object) | variable, where the location would be saved | 
| "min_radius", "max_radius" | optional | int, float or [variable object](##variable-object) | default 0; radius around the player or NPC, where the location would be searched | 
| "outdoor_only" | optional | boolean | default false; if true, only outdoor values would be picked | 
| "passable_only" | optional | boolean | default false; if true, only passable values would be picked | 
| "target_params" | optional | assign_mission_target | if used, the search would be performed not from `u_` or `npc_` location, but from `mission_target`. it uses an [assign_mission_target](MISSIONS_JSON.md) syntax | 
| "x_adjust", "y_adjust", "z_adjust" | optional | int, float or [variable object](##variable-object) | add this amount to `x`, `y` or `z` coordinate in the end; `"x_adjust": 2` would save the coordinate with 2 tile shift to the right from targeted | 
| "z_override" | optional | boolean | default is false; if true, instead of adding up to `z` level, override it with absolute value; `"z_adjust": 3` with `"z_override": true` turn the value of `z` to `3` | 
| "terrain" / "furniture" / "trap" / "monster" / "zone" / "npc" | optional | string or [variable object](##variable-object) | if used, search the entity with corresponding id between `target_min_radius` and `target_max_radius`; if empty string is used (e.g. `"monster": ""`), return any entity from the same radius  | 
| "target_min_radius", "target_max_radius" | optional | int, float or [variable object](##variable-object) | default 0, min and max radius for search, if previous field was used | 
| "true_eocs", "false_eocs" | optional | string, [variable object](##variable-object), inline EoC, or range of all of them | if the location was found, all EoCs from `true_eocs` are run, otherwise all EoCs from `false_eocs` are run | 

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
| "location_variable_adjust" | **mandatory** | [variable object](##variable-object) | variable, where the location would be saved | 
| "x_adjust", "y_adjust", "z_adjust" | optional | int, float or [variable object](##variable-object) | add this amount to `x`, `y` or `z` coordinate in the end; `"x_adjust": 2` would save the coordinate with 2 tile shift to the right from targeted | 
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
| "u_learn_recipe" / "npc_learn_recipe" | **mandatory** | string or [variable object](##variable-object) | Recipe, that would be learned |

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
| "u_forget_recipe" / "npc_forget_recipe" | **mandatory** | string or [variable object](##variable-object) | recipe, that would be forgotten |

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
| "npc_first_topic" | **mandatory** | string or [variable object](##variable-object)  | topic, that would be used |

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
| "u_add_wet" / "npc_add_wet" | **mandatory** | int, float or [variable object](##variable-object) | How much wetness would be added (in percent) |

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
| "u_make_sound" / "npc_make_sound" | **mandatory** | string or [variable object](##variable-object) | description of the sound |
| "volume" | **mandatory** | int, float or [variable object](##variable-object) | how loud the sound is (1 unit = 1 tile around the character) | 
| "type" | **mandatory** | string or [variable object](##variable-object) | Type of the sound; Could be one of `background`, `weather`, `music`, `movement`, `speech`, `electronic_speech`, `activity`, `destructive_activity`, `alarm`, `combat`, `alert`, or `order` | 
| "target_var" | optional | [variable object](##variable-object) | if set, the center of the sound would be centered in this variable's coordinates instead of you or NPC | 
| "snippet" | optional | boolean | default false; if true, `_make_sound` would use a snippet of provided id instead of a message | 
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
| "u_mod_healthy" / "npc_mod_healthy" | **mandatory** | int, float or [variable object](##variable-object) | Amount of health to be added |
| "cap" | optional | int, float or [variable object](##variable-object) | cap for healthiness, beyond which it can't go further | 

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
| "u_add_morale" / "npc_add_morale" | **mandatory** | string or [variable object](##variable-object) | `morale_type`, that would be given by effect |
| "bonus" | **mandatory** | int, [variable object](##variable-object) or array of both | default 1; mood bonus or penalty, that would be given by effect; can be stacked up to `max_bonus` cap, but each bonus is lower than previous (e.g. `bonus` of 100 gives mood bonus as 100, 141, 172, 198, 221 and so on) | 
| "max_bonus" | **mandatory** | int, [variable object](##variable-object) or array of both | default false; cap, beyond which mood won't increase or decrease | 
| "duration" | optional | int, duration or [variable object](##variable-object) | default 1 hour; how long the morale effect would last | 
| "decay_start" | optional | int, duration or [variable object](##variable-object) | default 30 min; when the morale effect would start to decay | 
| "capped" | optional | boolean | default false; if true, `bonus` is not decreased when stacked (e.g. `bonus` of 100 gives mood bonus as 100, 200, 300 and so on) |  

##### Valid talkers:

| Avatar | Character | NPC | Monster |  Furniture | Item |
| ------ | --------- | --------- | ---- | ------- | --- | 
| ✔️ | ✔️ | ✔️ | ❌ | ❌ | ❌ |

##### Examples
Gives `morale_afs_drugs` thought with +1 mood bonus
```json
{
  "u_add_morale": "morale_afs_drugs",
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
| "u_lose_morale" / "npc_lose_morale" | **mandatory** | string or [variable object](##variable-object) | thought to remove |

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
| "u_add_faction_trust" | **mandatory** | int, float or [variable object](##variable-object) | amount of trust to give or remove |

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

#### `u_message`, `npc_message`, `message`
Display a text message in the log. `u_message` and `npc_message` display a message only if you or NPC is avatar. `message` always displays a message.

| Syntax | Optionality | Value  | Info |
| --- | --- | --- | --- | 
| "u_message" / "npc_message" / "message" | **mandatory** | string or [variable object](##variable-object) | message, that would be printed; If `snippet` is true, id of a snippet that would be printed |
| "type" | optional | string or [variable object](##variable-object) | default neutral; how the message would be displayed in log (usually means the color); could be any of good (green), neutral (white), bad (red), mixed (purple), warning (yellow), info (blue), debug (appear only if debug mode is on), headshot (purple), critical (yellow), grazing (blue) | 
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

Print a text with a context variable
```json
  { "u_message": "Test event with trait_id FIRE! <context_val:trait_id>", "type": "good" } 
```

#### `u_cast_spell`, `npc_cast_spell`
You or NPC cast a spell. The spell uses fake spell data (ignore `energy_cost`, `energy_source`, `cast_time`, `components`, `difficulty` and `spell_class` fields), and uses additional fields 

| Syntax | Optionality | Value  | Info |
| --- | --- | --- | --- | 
| "u_cast_spell" / "npc_cast_spell" | **mandatory** | `fake_spell` | information of what spell and how it should be casted; next field can be used: |
| "id" | **mandatory** | string or [variable object](##variable-object) | part of `_cast_spell`; define the id of spell to cast | 
| "hit_self" | optional | boolean | part of `_cast_spell`; default false; if true, the spell could affect the caster (either as self damage from AoE spell, or as applying effect for buff spell) | 
| "message" | optional | string or [variable object](##variable-object) | part of `_cast_spell`; message to send when spell is casted | 
| "npc_message" | optional | string or [variable object](##variable-object) | part of `_cast_spell`; message if npc uses | 
| "min_level", "max_level" | optional | int, float or [variable object](##variable-object) | part of `_cast_spell`; level of the spell that would be casted (min level define what the actual spell level would be casted, adding max_level make EoC pick a random level between min and max) | 
| "targeted" | optional | boolean | default false; if true, allow you to aim casted spell, otherwise cast it in the location set by "loc" | 
| "loc" | optional | [variable object](##variable-object) | Set target location of the spell. If not used, target to caster's location |
| "true_eocs", "false_eocs" | optional | string, [variable object](##variable-object), inline EoC, or range of all of them | if spell was casted successfully, all EoCs from `true_eocs` are run, otherwise all EoCs from `false_eocs` are run | 

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
| "u_assign_activity" / "npc_assign_activity" | **mandatory** | string or [variable object](##variable-object), | id of activity to start |
| "duration" | **mandatory** | int, duration or [variable object](##variable-object) | how long the activity would last | 

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
| "u_teleport", / "npc_teleport" | **mandatory** | [variable object](##variable-object) | location to teleport; should use `target_var`, created previously |
| "success_message" | optional | string or [variable object](##variable-object) | message, that would be printed, if teleportation was successful | 
| "fail_message" | optional | string or [variable object](##variable-object) | message, that would be printed, if teleportation was failed, like if coordinates contained creature or impassable obstacle (like wall) | 
| "force" | optional | boolean | default false; if true, teleportation can't fail - any creature, that stand on target coordinates, would be brutally telefragged, and if impassable obstacle occur, the closest point would be picked instead |

##### Valid talkers:

| Avatar | Character | NPC | Monster |  Furniture | Item |
| ------ | --------- | --------- | ---- | ------- | --- | 
| ✔️ | ✔️ | ✔️ | ❌ | ❌ | ✔️ |

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

#### `u_die`, `npc_die`
You or NPC will instantly die

##### Valid talkers:

| Avatar | Character | NPC | Monster |  Furniture | Item |
| ------ | --------- | --------- | ---- | ------- | --- | 
| ✔️ | ✔️ | ✔️ | ✔️ | ❌ | ✔️ |

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

#### `u_prevent_death`, `npc_prevent_death`
You or NPC will be prevented from death. Intended for use in EoCs has `NPC_DEATH` or `EVENT(character_dies)` type (Take care that u will be the dying npc in these events).

##### Valid talkers:

| Avatar | Character | NPC | Monster |  Furniture | Item |
| ------ | --------- | --------- | ---- | ------- | --- | 
| ✔️ | ✔️ | ✔️ | ❌ | ❌ | ❌ |

##### Examples

NPC is prevented from death.

`NPC_DEATH`
```json
{
  "type": "effect_on_condition",
  "id": "EOC_event_NPC_DEATH_test",
  "eoc_type": "NPC_DEATH",
  "effect": [ "u_prevent_death" ]
}
```

`EVENT`
```json
{
  "type": "effect_on_condition",
  "id": "EOC_event_character_dies_test",
  "eoc_type": "EVENT",
  "required_event": "character_dies",
  "condition": { "u_has_trait": "DEBUG_PREVENT_DEATH" },
  "effect": [ "u_prevent_death" ]
}
```

#### `u_attack`, `npc_attack`
Alpha or beta talker forced to use a technique or special attack

| Syntax | Optionality | Value  | Info |
| --- | --- | --- | --- | 
| "u_attack" / "npc_attack" | **mandatory** | string, boolean or [variable object](#variable-object) | technique, that would be used; `"tec_none"` can be used, in this case a default autoattack would be used |
| "allow_special" | optional | boolean | default true; if true, special attacks should be selected (`special_attack` that monsters can use, like `monster_attack` or `spell`) | 
| "allow_unarmed" | optional | boolean | default true; if true, unarmed techniques can be considered | 
| "forced_movecost" | optional | int or [variable object](#variable-object) | default -1; If used, attack will consume this amount of moves (100 moves = 1 second); negative value make it use the default movecost of attack | 

##### Valid talkers:

| Avatar | Character | NPC | Monster |  Furniture | Item |
| ------ | --------- | --------- | ---- | ------- | --- | 
| ✔️ | ✔️ | ✔️ | ❌ | ❌ | ❌ |

##### Examples
you use autoattack
```json
{
  "type": "effect_on_condition",
  "id": "EOC_attack_test",
  "effect": [ { "u_attack": "tec_none" } ]
},
```

mutator `valid_technique` return random technique, that alpha talker can use; this technique is set into `random_attack` global variable; then you attack using this technique
```json
{
  "type": "effect_on_condition",
  "id": "EOC_attack_mutator",
  "effect": [
    { "set_string_var": { "mutator": "valid_technique" }, "target_var": { "global_val": "random_attack" } },
    { "u_attack": { "global_val": "random_attack" } }
  ]
}
```

Picks random pankration technique, assign it to `pankration_random_attack`, and use it in attack
```json
{
  "type": "effect_on_condition",
  "id": "EOC_attack_random_tech",
  "effect": [
    {
      "set_string_var": [
        "tec_pankration_cross",
        "tec_pankration_kick",
        "tec_pankration_grabknee",
        "tec_pankration_grabdisarm",
        "tec_pankration_grabthrow"
      ],
      "target_var": { "context_val": "pankration_random_attack" }
    },
    { "u_attack": { "context_val": "pankration_random_attack" } }
  ]
}
```

## Item effects

#### `u_set_flag`, `npc_set_flag`
Give item a flag

| Syntax | Optionality | Value  | Info |
| ------ | ----------- | ------ | ---- | 
| "u_set_flag" / "npc_set_flag" | **mandatory** | string or [variable object](##variable-object) | id of flag that should be given |

##### Valid talkers:

| Avatar | Character | NPC | Monster |  Furniture | Item |
| ------ | --------- | --------- | ---- | ------- | --- | 
| ❌ | ❌ | ❌ | ❌ | ❌ | ✔️ |

##### Examples
Make item filthy
```json
{ "npc_set_flag": "FILTHY" }
```

#### `u_unset_flag`, `npc_unset_flag`
Remove a flag from item

| Syntax | Optionality | Value  | Info |
| ------ | ----------- | ------ | ---- | 
| "u_unset_flag" / "npc_unset_flag" | **mandatory** | string or [variable object](##variable-object) | id of flag that should be remove |

##### Valid talkers:

| Avatar | Character | NPC | Monster |  Furniture | Item |
| ------ | --------- | --------- | ---- | ------- | --- | 
| ❌ | ❌ | ❌ | ❌ | ❌ | ✔️ |

##### Examples
Make item clean
```json
{ "npc_unset_flag": "FILTHY" }
```

#### `u_activate`, `npc_activate`
You activate beta talker / NPC activates alpha talker. One must be a Character and the other an item.

| Syntax | Optionality | Value  | Info |
| ------ | ----------- | ------ | ---- | 
| "u_activate" / "npc_activate" | **mandatory** | string or [variable object](##variable-object) |  use action id of item that activate |
| "target_var" | optional | [variable object](##variable-object) | if set, target location is forced this variable's coordinates | 

##### Valid talkers:

| Avatar | Character | NPC | Monster |  Furniture | Item |
| ------ | --------- | --------- | ---- | ------- | --- | 
| ✔️ | ✔️ | ✔️ | ❌ | ❌ | ❌ |

##### Examples
Force you consume drug item
```json
{ "u_activate": "consume_drug" }
```

## Map effects

#### `map_spawn_item`
Spawn and place the item

| Syntax | Optionality | Value  | Info |
| ------ | ----------- | ------ | ---- | 
| "map_spawn_item" | **mandatory** | string or [variable object](##variable-object) | id of item or item group that should spawn |
| "loc" | optional | [variable object](##variable-object) | Location that the item spawns. If not used, spawns from player's location |
| "count" | optional | int or [variable object](##variable-object) | default 1; Number of item copies |
| "container" | optional | string or [variable object](##variable-object) | id of container. Item is contained in container if specified |
| "use_item_group" | optional | bool | default false; If true, it will instead create items from the item group given. ("count" and "containter" will be ignored since they are defined in the item group.) |
| "flags" | optional | array of string or [variable object](#variable-object) | The item will have all the flags from the array `flags` |

##### Examples
Spawn a plastic bottle on ground
```json
{
  "type": "effect_on_condition",
  "id": "EOC_map_spawn_item",
  "effect": [
    { "set_string_var": { "mutator": "loc_relative_u", "target": "(0,1,0)" }, "target_var": { "context_val": "loc" } },
    { "map_spawn_item": "bottle_plastic", "loc": { "mutator": "loc_relative_u", "target": "(0,1,0)" } }
  ]
},
```

## Map Updates

Map updates are related to  any change in the map, weather, or coordinates, and any talker can use them

#### `mapgen_update`
Update the map with changes, described in `mapgen_update`

| Syntax | Optionality | Value  | Info |
| --- | --- | --- | --- | 
| "mapgen_update" | **mandatory** | string, [variable objects](##variable-object) or array | With no other parameters, update the overmap tile at player's current location with changes in `update_mapgen_id` id.  If array is used, the map would be updated for each id |
| "time_in_future" | optional | int, duration, [variable object](##variable-object)) or value between two  | If used, the map change would be delayed for this amount of time.  "infinity" could be used, to make location not update until `key` event happen | 
| "key" | optional | string or [variable object](##variable-object) | id of the event, that you can call outside of EoC to trigger map update.  Key should be [alter_timed_events](#`alter_timed_events`) | 
| "target_var" | optional | [variable objects](##variable-object) | if used, the target from variable would be used instead of player's current location.  It uses [assign_mission_target](MISSIONS_JSON.md) syntax | 

##### Examples
Update the map with `map_spawn_seller` map

```json
{ "mapgen_update": "map_spawn_seller" }
```

Update the map with `map_spawn_terrain`, then `map_spawn_furniture`, then `map_spawn_trap`, then `map_spawn_field`
```json
{ "mapgen_update": [ "map_spawn_terrain", "map_spawn_furniture", "map_spawn_trap", "map_spawn_field" ] }
```

Update the `small_pond` with `map_bridge` when `as_soon_as_this_event_trigger` event occur
```json
{ "mapgen_update": "map_bridge", "om_terrain": "small_pond", "key": "as_soon_as_this_event_trigger" }
```

Update the `robofachq_subcc_a2` in `ancilla_bar_loc` coordinates, with `nest_ancilla_bar_place_BEMs` map
```json
{
  "mapgen_update": "nest_ancilla_bar_place_BEMs",
  "om_terrain": "robofachq_subcc_a2",
  "target_var": { "global_val": "ancilla_bar_loc" }
}
```

#### `revert_location`
Save picked location, and then restore it to this state
Usually used as `revert_location` with `"time_in_future": "infinity"`, to save mold of location, and some `key`. Then `mapgen_update` is used to alter location. In the end,  `alter_timed_events` with `key`  is called to actually revert location.

| Syntax | Optionality | Value  | Info |
| --- | --- | --- | --- | 
| "revert_location" | **mandatory** | [variable object](##variable-object) | id of variable, where the location would be stored |
| "time_in_future" | **mandatory** | int, duration, [variable object](##variable-object)) or value between two  | when the location should be reverted; "infinity" could be used, to make location not update until `key` event happen | 
| "key" | optional | string or [variable objects](##variable-object) | id of the event, that you can call outside of EoC to trigger location reverse.  Key should be [alter_timed_events](#`alter_timed_events`) | 

##### Examples
Store `vitrified_farm_ground`. When `vitrified_farm_escape_key` is called, the location is reverted
```json
{
  "revert_location": { "global_val": "vitrified_farm_ground" },
  "time_in_future": "infinite",
  "key": "vitrified_farm_escape_key"
},
```

#### `alter_timed_events`
All effects, that has this event as a `key`, would be triggered, if they did not yet.  Usually used with `mapgen_update` or `revert_location` with `"time_in_future": "infinite"` 

| Syntax | Optionality | Value  | Info |
| --- | --- | --- | --- | 
| "alter_timed_events" | **mandatory** | string or [variable object](##variable-object) | id of the event, that you can call outside of EoC to trigger map update.  Key should be [alter_timed_events](#`alter_timed_events`) |
| "time_in_future" | optional | int, duration, [variable object](##variable-object)) or value between two  | If used, all related effects would be triggered not instantly, but this amount of time after the trigger | 

##### Examples
Trigger every effect, that has `portal_dungeon` as a key
```json
{ "alter_timed_events": "portal_dungeon" }
```

for example, if this effect would exist, and `alter_timed_events` occur, the location would be reverted 
```json
{
  "revert_location": { "global_val": "portal_dungeon" },
  "time_in_future": "infinite",
  "key": "portal_dungeon_entrance"
}
```

#### `lightning`
Allows supercharging monster in electrical fields, legacy command for lightning weather

#### `next_weather`
Forces a check for what weather it should be. Doesn't force the weather change itself, so if conditions is not met, or custom weather has lower priority, the weather won't change

#### `custom_light_level`
Sets the ambient light of the world for some amount of time, ignoring time or sun/moon light

| Syntax | Optionality | Value  | Info |
| --- | --- | --- | --- | 
| "custom_light_level" | **mandatory** | int, [variable object](##variable-object) or value between two | level of light from 0 to 125, where 0 is complete darkness, where 125 is daylight |
| "length" | **mandatory** | int, duration, [variable object](##variable-object) or value between two | how long the effect would last | 
| "key" | optional | string or [variable object](##variable-object) | id of the event, that you can call outside of EoC to trigger map update.  Key should be [alter_timed_events](#`alter_timed_events`) | 

##### Examples
Highlight the world for 1-10 seconds
```json
{ "custom_light_level": 100, "length": [ "1 seconds", "10 seconds" ] }
```

Darken the world for 1 day or until `who_turn_off_the_light` would be triggered
```json
{ "custom_light_level": 0, "length": "1 day", "key": "who_turn_off_the_light" }
```

#### `u_transform_radius`, `npc_transform_radius`
transform the territory around you, npc or target using `ter_furn_transform`

| Syntax | Optionality | Value  | Info |
| --- | --- | --- | --- | 
| "u_transform_radius" / "npc_transform_radius" | **mandatory** | int or [variable object](##variable-object) | range, where transformation occur |
| "ter_furn_transform" | **mandatory** | string or [variable object](##variable-object) | [`ter_furn_transform`](#TER_FURN_TRANSFORM.md), that would be used to transform territory around | 
| "target_var" | optional | [variable object](##variable-object) | if used, the target from variable would be used instead of player's current location.  It uses [assign_mission_target](MISSIONS_JSON.md) syntax | 
| "time_in_future" | optional | int, duration, [variable object](##variable-object) or value between two | delay when the location should be transformed; "infinity" could be used, to make location not update until `key` event happen  | 
| "key" | optional | string or [variable object](##variable-object)) | id of the event, that you can call outside of EoC to trigger map update.  Key should be [alter_timed_events](#`alter_timed_events`) | 

##### Examples
transform everything 5 tiles around player according to `merc_spike_transform`
```json
{ "u_transform_radius": 5, "ter_furn_transform": "merc_spike_transform" }
```

transform the `door_transform` 2 tiles around player, according `detonate_the_door`, in 2-10 seconds, or if `detonator` event happens
```json
{
  "u_transform_radius": 2,
  "ter_furn_transform": "detonate_the_door",
  "target_var": { "global_val": "door_transform" },
  "time_in_future": [ "2 seconds", "10 seconds" ],
  "key": "detonator"
}
```

#### `transform_line`
Transform terrain, furniture, fields or traps on a line between two coordinates

| Syntax | Optionality | Value  | Info |
| --- | --- | --- | --- | 
| "transform_line" | **mandatory** | string or [variable object](##variable-object) | [ter_furn_transform](TER_FURN_TRANSFORM.md), that would be used to transform terrain |
| "first", "second" | **mandatory** | [variable object](##variable-object) | coordinates, created by `u_location_variable`, between which the line would be drawn | 

##### Examples
change the terrain between `point_0` and `point_1` according to `blood_trail` ter_furn_transform
```json
{
  "transform_line": "blood_trail",
  "first": { "global_val": "point_0" },
  "second": { "global_val": "point_1" }
}
```

#### `place_override`
Override the current player location for some amount of time or until event would be called

| Syntax | Optionality | Value  | Info |
| --- | --- | --- | --- | 
| "place_override" | **mandatory** | string or [variable object](##variable-object) | new name of the location;  |
| "length" | **mandatory** | int, duration, [variable object](##variable-object) or value between two | how long changed name would last; "infinity" could be used, to make location not revert until `key` event happen | 
| "key" | optional | string or [variable object](##variable-object) | id of the event, that you can call outside of EoC to trigger map update.  Key should be [alter_timed_events](#`alter_timed_events`) | 

##### Examples
change the name of your current location to `devilish place` to 11 minutes 6 seconds (666 seconds)
```json
{
  "place_override": "devilish place",
  "length": 666
}
```

```json
{
  "place_override": "devilish place",
  "length": "666 s"
}
```

Set `place_name` to be one of five from a range randomly, then set it for `cell_time` time
```json
{
  "set_string_var": [ "Somewhere", "Nowhere", "Everywhere", "Yesterday", "Tomorrow" ],
  "target_var": { "global_val": "place_name" }
},
{
  "place_override": { "global_val": "place_name" },
  "length": { "u_val": "cell_time" }
}
```

#### `u_spawn_monster`, `npc_spawn_monster`
Spawn some monsters around you, NPC or `target_var`

| Syntax | Optionality | Value  | Info |
| --- | --- | --- | --- | 
| "u_spawn_monster", "npc_spawn_monster" | **mandatory** | string or [variable object](##variable-object) | monster or monstergroup that would be spawned, using "" picks randomly from nearby monsters |
| "real_count" | optional | int, [variable object](##variable-object) or value between two | default 0; amount of monsters, that would be spawned | 
| "hallucination_count" | optional | int, [variable object](##variable-object) or value between two | default 0; amount of hallucination versions of the monster that would be spawned |
| "group" | optional | boolean | default false; if true, `_spawn_monster` will spawn a monster from `monstergroup` |
| "single_target" | optional | boolean | default false; if true, `_spawn_monster` the game pick only one monster from the provided `monstergroup` or from nearby monsters | 
| "min_radius", "max_radius" | optional | int, [variable object](##variable-object) or value between two | default 1 and 10 respectively; range around the target, where the monster would spawn | 
| "outdoor_only"/ "indoor_only" | optional | boolean | default false; if used, monsters would be able to spawn only outside or only inside buildings | 
| "open_air_allowed" | optional | boolean | default false; if true, monsters can spawn in the open air | 
| "target_range" | optional | int, [variable object](##variable-object) or value between two | if `_spawn_monster` is empty, pick a random hostile critter from this amount of tiles from target | 
| "lifespan" | optional | int, duration, [variable object](##variable-object) or value between two | if used, critters would live that amount of time, and disappear in the end | 
| "target_var" | optional | [variable object](##variable-object) | if used, the monster would spawn from this location instead of you or NPC | 
| "spawn_message", "spawn_message_plural" | optional | string or [variable object](##variable-object) | if you see monster or monsters that was spawned, related message would be printed | 
| "true_eocs", "false_eocs" | optional | string, [variable object](##variable-object), inline EoC, or range of all of them | if at least 1 monster was spawned, all EoCs from `true_eocs` are run, otherwise all EoCs from `false_eocs` are run | 

##### Examples
Spawn 2-5 zombies in range 3-24 around player with lifespan 40-120 seconds, with messages if player see spawn
```json
{
  "u_spawn_monster": "mon_zombie",
  "real_count": [ 2, 5 ],
  "min_radius": [ 3, 5 ],
  "max_radius": [ 11, 24 ],
  "lifespan": [ "40 seconds", "2 minutes" ],
  "spawn_message": "Zombie!",
  "spawn_message_plural": "Zombies!"
}
```

Pick a random monster 50 tiles around the player, and spawn it's hallucination copy near the player
```json
{
  "u_spawn_monster": "",
  "hallucination_count": 1,
  "target_range": 50
}
```

#### `u_spawn_npc`, `npc_spawn_npc`
Spawn some NPC near you or another NPC

| Syntax | Optionality | Value  | Info |
| --- | --- | --- | --- | 
| "u_spawn_npc", "npc_spawn_npc" | **mandatory** | string or [variable object](##variable-object) | class of NPC, that would be spawned |
| "unique_id" | optional | string or [variable object](##variable-object) | --- | 
| "traits" | optional | string, [variable object](##variable-object) or array | additional traits/mutations that NPC would have upon spawn | 
| "real_count" | optional | int, [variable object](##variable-object) or value between two | default 0; amount of NPCs, that would be spawned | 
| "hallucination_count" | optional | int, [variable object](##variable-object) or value between two | default 0; amount of hallucination versions of NPC that would be spawned |
| "min_radius", "max_radius" | optional | int, [variable object](##variable-object) or value between two | default 1 and 10 respectively; range around the target, where the monster would spawn | 
| "outdoor_only"/ "indoor_only" | optional | boolean | default false; if used, NPC would be able to spawn only outside or only inside buildings | 
| "open_air_allowed" | optional | boolean | default false; if true, NPC can spawn in the open air | 
| "lifespan" | optional | int, duration, [variable object](##variable-object) or value between two | if used, NPC would live that amount of time, and disappear in the end | 
| "target_var" | optional | [variable object](##variable-object) | if used, the NPC would spawn from this location instead of you or NPC | 
| "spawn_message", "spawn_message_plural" | optional | string or [variable object](##variable-object) | if you see NPC or NPCs that was spawned, related message would be printed | 
| "true_eocs", "false_eocs" | optional | string, [variable object](##variable-object), inline EoC, or range of all of them | if at least 1 monster was spawned, all EoCs from `true_eocs` are run, otherwise all EoCs from `false_eocs` are run | 

##### Examples
Spawn 2 hallucination `portal_person`s, outdoor, 3-5 tiles around the player, for 1-3 minutes and with messages 
```json
{
  "u_spawn_npc": "portal_person",
  "hallucination_count": 2,
  "outdoor_only": true,
  "min_radius": 3,
  "max_radius": 5,
  "lifespan": [ "1 minutes", "3 minutes" ],
  "spawn_message": "A person steps nearby from somewhere else.",
  "spawn_message_plural": "Several identical people walk nearby from nowhere."
}
```

#### `u_set_field`, `npc_set_field`
spawn a field around player. it is recommended to not use it in favor of `u_transform_radius`, if possible

| Syntax | Optionality | Value  | Info |
| --- | --- | --- | --- | 
| "u_set_field", "npc_set_field" | **mandatory** | string or [variable object](##variable-object) | id of field to spawn around the player |
| "intensity" | optional | int, [variable object](##variable-object) or value between two | default 1; intensity of field to spawn | 
| "radius" | optional | int, [variable object](##variable-object) or value between two | default 10000000; radius of a field to spawn | 
| "age" | optional | int, duration, [variable object](##variable-object) or value between two | how long the field would last | 
| "outdoor_only"/ "indoor_only" | optional | boolean | default false; if used, field would be spawned only outside or only inside buildings | 
| "hit_player" | optional | boolean | default true; if field spawn where the player is, process like player stepped on this field | 
| "target_var" | optional | [variable object](##variable-object) | if used, the field would spawn from this location instead of you or NPC | 

##### Examples
Spawn blood 10 tiles around the player outdoor
```json
{ "u_set_field": "fd_blood", "radius": 10, "outdoor_only": true, "intensity": 3 }
```

#### `turn_cost`
Subtract this many turns from the alpha talker's moves.

| Syntax | Optionality | Value  | Info |
| --- | --- | --- | --- |
| "turn_cost" | **mandatory** | number, duration, [variable object](##variable-object) or value between two | how long the action takes (can be specified in number of turns (as decimal), or as a duration) |

##### Examples

```json
{
  "effect": [
    { "turn_cost": "1 sec" }
  ]
}
```

```json
{
  "effect": [
    { "turn_cost": 0.6 }
  ]
}
```

##### Valid talkers:

| Avatar | Character | NPC | Monster |  Furniture | Item |
| ------ | --------- | --------- | ---- | ------- | --- |
| ✔️ | ✔️ | ❌ | ❌ | ❌ | ❌ |

#### `transform_item`
Convert the beta talker (which must be an item) into a different item, optionally activating it. Works similarly to the "transform" use_action.

| Syntax | Optionality | Value  | Info |
| --- | --- | --- | --- |
| "transform_item" | **mandatory** | string or [variable object](##variable-object) | item ID to transform to |
| "active" | optional | boolean | if true, activate the item |

##### Examples

```json
{
  "condition": "has_ammo",
  "effect": [
    { "transform_item": "chainsaw_on", "active": true }
  ],
  "false_effect": {
    "u_message": "You yank the cord, but nothing happens."
  }
}
```

##### Valid talkers:

| Avatar | Character | NPC | Monster |  Furniture | Item |
| ------ | --------- | --------- | ---- | ------- | --- |
| ✔️ | ✔️ | ❌ | ❌ | ❌ | ❌ |
