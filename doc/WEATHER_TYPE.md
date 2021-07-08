# WEATHER TYPES

Each weather type is a type of weather that occurs, and what causes it. The only required entries are null and clear.

## `weather_type` properties

|     Identifier                 |                              Description                              |
| ------------------------------ | --------------------------------------------------------------------- |
| `name`                         | UI name of weather type.                                              |
| `color`                        | UI color of weather type.                                             |
| `map_color`                    | Map color of weather type.                                            |
| `sym`                          | Map glyph of weather type.                                            |
| `ranged_penalty`               | Penalty to ranged attacks.                                            |
| `sight_penalty`                | Penalty to per-square visibility, applied in transparency map.        |
| `light_modifier`               | modification to ambient light.                                        |
| `sound_attn`                   | Sound attenuation of a given weather type.                            |
| `dangerous`                    | If true, our activity gets interrupted.                               |
| `precip`                       | Amount of associated precipitation. Valid values are: none, very_light, light and heavy |
| `rains`                        | Whether said precipitation falls as rain.                             |
| `acidic`                       | Whether said precipitation is acidic.                                 |
| `tiles_animation`              | Optional, name of the tiles animation to use                          |
| `sound_category`               | Optional, what sound effect to play. Valid values are: silent, drizzle, rainy, thunder, flurries, snowstorm and snow. |
| `sun_intensity`                | Strength of the sun. Valid values are: none, light, normal, and high  |
| `duration_min`                 | Optional, the lower bound on the amount of time this weather can last. Defaults to 5 minutes. Unless time_between_min and time_between_max are set the weather can happen again as soon as it ends. |
| `duration_max`                 | Optional, the upper bound on the amount of time this weather can last. Defaults to 5 minutes. Unless time_between_min and time_between_max are set the weather can happen again soon as it ends. |
| `time_between_min`             | Optional: the lower bound on the amount of time that will be guaranteed to pass before this weather happens again. Defaults to 0. |
| `time_between_max`             | Optional: the upper bound on the amount of time that will be guaranteed to pass before this weather happens again. Defaults to 0. |
| `weather_animation`            | Optional, Information controlling weather animations.  Members: factor, color and glyph |
|	`required_weathers`          | a string array of possible weathers it is at this point in the loop. i.e. rain can only happen if the conditions for clouds light drizzle or drizzle are present |
| `condition`                  | A dialog condition to determine if this weather is happening.  See Dialogue conditions section of [NPCs](NPCs.md)

#### `weather_type` example

```json
[
  {
    "id": "lightning",
    "type": "weather_type",
    "name": "Lightning Storm",
    "color": "c_yellow",
    "map_color": "h_yellow",
    "glyph": "%",
    "ranged_penalty": 4,
    "sight_penalty": 1.25,
    "light_modifier": -45,
    "sound_attn": 8,
    "dangerous": false,
    "precip": "heavy",
    "rains": true,
    "acidic": false,
    "required_weathers": [ "thunder" ],
    "condition": { "not": { "is_pressure": 990 } }
  }
]
```


### Weather effect properties

|     Identifier                 |                              Description                              |
| ------------------------------ | --------------------------------------------------------------------- |
| `message`                      | Optional: Message displayed when this effect happens.                 |
| `sound_message`                | Optional: Message describing what you hear, will not display if deaf  |
| `sound_effect`                 | Optional: Name of sound effect to play                                |
| `sound_message`                | Optional: Message describing what you hear for this, will not display if deaf. |
| `must_be_outside`              | Whether the effect only happens while you are outside.                |
| `one_in_chance`                | Optional: The chance of the event occurring is 1 in this value, if blank will always happen. |
| `time_between`                 | Optional: The time between instances of this effect occurring.  If both this and one_in_chance are set will only happen when both are true. |
| `lightning`                    | Optional: Causes the world be bright at night and supercharge monster electric fields. |
| `rain_proof`                   | Optional: If rainproof, resistant gear will help against this         |
| `pain_max`                     | Optional: If there is a threshold of pain at which this will stop happening. |
| `pain`                         | Optional: How much pain this causes.                                  |
| `wet`                          | Optional: How much wet this causes.                                   |
| `radiation`                    | Optional: How much radiation this causes.                             |
| `healthy`                      | Optional: How much healthy this adds or removes.                      |
| `effect_id`                    | Optional: String id of an effect to add.                              |
| `effect_duration`              | Optional: How long the above effect will be added for, defaults to 1 second. |
| `target_part`                  | Optional: Bodypart that above effect or damage are applied to, if blank affects whole body. |
| `damage`                       | Optional: List of damage instances applied                            |
| `spawns`                       | Optional: Array of spawns to cause.  If spawns are selected but are unable to spawn the effect is cancelled. |
| `fields`                       | Optional: Array of fields to cause.  Elements are discussed below     |

#### `effects` example

```json
{
      "must_be_outside": true,
      "radiation": 10,
      "healthy" :1,
      "message": "Suddenly a something",
      "add_effect": "bite",
      "effect_duration": "10 minutes",
      "target_part": "arm_l",
      "damage": [
        {
          "damage_type": "electric",
          "amount": 4.0,
          "armor_penetration": 1,
          "armor_multiplier": 1.2,
          "damage_multiplier": 1.4
        }
      ],
      "spawns":
      [{
        "max_radius": 10,
        "min_radius": 10,
        "target": "mon_zombie_survivor_elite",
        "hallucination_count": 1,
        "real_count": 0
      }]
}
```
