### effect_on_condition
An effect_on_condition is an object allowing the combination of dialog conditions and effects and their usage outside of a dialog.  When invoked they will test their condition and on a pass then cause their effect. They can be activated automatically with any given frequency or caused by using items, monster special attacks, or on monster death.  effect_on_conditions use the npc dialog conditions and effects syntax and those allow checking or effecting an npc, which an effect_on_condition does not have.  Doing so is nonsensical, unsupported, and should not be used in an effect_on_condition.


## Fields

|Identifier|Type|Description|
|-|-|-|
| `recurrence_min`| int | The effect_on_condition automatically activates with at least this many seconds in between.
| `recurrence_max`| int | The effect_on_condition automatically activates at least once this many seconds.
| `condition`| condition | The conditions to cause this to occur on activation.  See "Dialogue conditions" section of [NPCs](NPCs.md) for full syntax.
| `deactivate_condition`| condition | *optional* When an effect_on_condition is automaticly activated and fails `deactivate_condition` will be tested if it exists and there is no `false_effect`.  If it exists and returns true this effect_on_condition will no longer be run automaticly every `once_every` seconds.  When ever the player gains/loses a trait or bionic or the weather changes all deactivated effect_on_conditions will have `deactivate_condition` run and on a return of false will reactivate.  This is for performance reasons only and should not be used for logic purposes as saving and exiting will reactivate all effect_on_conditions. See "Dialogue conditions" section of [NPCs](NPCs.md) for full syntax.
| `effect`| effect | The effects caused on conditon returning true upon activation.  See "Dialogue Effects" section of [NPCs](NPCs.md) for full syntax.
| `false_effect`| effect | The effects caused on conditon returning false upon activation.  See "Dialogue Effects" section of [NPCs](NPCs.md) for full syntax.

## Examples:
```JSON
  {
    "type": "effect_on_condition",
    "id": "thunder",
    "recurrence_min": 1,
    "recurrence_max": 1,
    "condition": {
      "and": [
        { "or": [ { "is_weather": "thunder" }, { "is_weather": "lightning" } ] },
        { "one_in_chance": 50 },
        { "u_is_height": 0 }
      ]
    },
    "effect": [
      { "message": "You hear a distant rumble of thunder.", "sound": true },
      { "sound_effect": "thunder_far", "outdoor_event": true }
    ]
  },
  {
    "type": "effect_on_condition",
    "id": "test_deactivate",    
    "recurrence_min": 1,
    "recurrence_max": 10,
    "condition": {"u_has_trait": "SPIRITUAL" },
    "deactivate_condition": {"not":{"u_has_trait": "SPIRITUAL" } },
    "effect": [ { "message": "Being spiritual rules." } ]
  },
```
