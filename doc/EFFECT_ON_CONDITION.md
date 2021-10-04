### effect_on_condition
An effect_on_condition is an object allowing the combination of dialog conditions and effects with their usage outside of a dialog.  When invoked, they will test their condition; on a pass, they will cause their effect. They can be activated automatically with any given frequency.  (Note: effect_on_conditions use the npc dialog conditions and effects syntax, which allows checking related to, or targeting an effect at, an npc (for example: `npc_has_trait`).  Using these commands in an effect_on_condition is not supported.)

## Fields

|Identifier|Type|Description|
|-|-|-|
| `recurrence_min`| int | The effect_on_condition is automatically invoked (activated) with at least this many seconds in-between.
| `recurrence_max`| int | The effect_on_condition is automatically invoked (activated) at least once this many seconds.
| `condition`| condition | The condition(s) under which this effect_on_condition, upon activation, will cause its effect.  See the "Dialogue conditions" section of [NPCs](NPCs.md) for the full syntax.
| `deactivate_condition`| condition | *optional* When an effect_on_condition is automatically activated (invoked) and fails its condition(s), `deactivate_condition` will be tested if it exists and there is no `false_effect` entry.  If it returns true, this effect_on_condition will no longer be invoked automatically every `recurrence_max` seconds.  Whenever the player/npc gains/loses a trait or bionic all deactivated effect_on_conditions will have `deactivate_condition` run; on a return of false, the effect_on_condition will start being run again.  This is to allow adding effect_on_conditions for specific traits or bionics that don't waste time running when you don't have the target bionic/trait.  See the "Dialogue conditions" section of [NPCs](NPCs.md) for the full syntax.
| `effect`| effect | The effect(s) caused if `condition` returns true upon activation.  See the "Dialogue Effects" section of [NPCs](NPCs.md) for the full syntax.
| `false_effect`| effect | The effect(s) caused if `condition` returns false upon activation.  See the "Dialogue Effects" section of [NPCs](NPCs.md) for the full syntax.
| `global`| bool | If this is true, this recurring eoc will be run on the player and every npc from a global queue.  Deactivate conditions will work based on the avatar. If it is false the avatar and every character will have their own copy and their own deactivated list. Defaults to false.
| `run_on_npcs`| bool | Can only be true if global is true. If false the eoc will only be run against the avatar. If true the eoc will be run against the avatar and all npcs.  Defaults to false.
| `EOC_TYPE`| string | The effect_on_condition is automatically invoked once on scenario start.
 Can be any of:ACTIVATION, RECURRING, SCENARIO_SPECIFIC, AVATAR_DEATH, NPC_DEATH. It defaults to ACTIVATION unless a `recurrence_min` and `recurrence_max` are provided in which case it defaults to RECURRING.  If it is SCENARIO_SPECIFIC it is automatically invoked once on scenario start. If it is AVATAR_DEATH whenever the current avatar dies it will be run with the avatar as u and the killer as npc. NPC_DEATH eocs can only be assigned to run on the death of an npc.

## Examples:
```JSON
  {
    "type": "effect_on_condition",
    "id": "test_deactivate",    
    "recurrence_min": 1,
    "recurrence_max": 1,
    "condition": { "u_has_trait": "SPIRITUAL" },
    "deactivate_condition": {"not":{ "u_has_trait": "SPIRITUAL" } },
    "effect": { "u_add_effect": "infection", "duration": 1 }
  },
  {
    "type": "effect_on_condition",
    "id": "test_stats",
    "recurrence_min": 1,
    "recurrence_max": 10,
    "condition": { "not": { "u_has_strength": 7 } },
    "effect": { "u_add_effect": "infection", "duration": 1 }
  }
