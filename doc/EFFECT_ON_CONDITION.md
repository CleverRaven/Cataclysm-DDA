### effect_on_condition
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

# EVENT EOCs:
EVENT EOCs trigger on in game events specified in the event_type enum in `event.h`. When an EVENT EOC triggers it tries to perform the EOC on the NPC that is the focus of the event and if it cannot determine one, triggers on the avatar. So any cata_event that has a field for "avatar_id", "character", "attacker", "killer", "npc" will potentially resolve to another npc rather than the avatar, based on who the event triggers for.