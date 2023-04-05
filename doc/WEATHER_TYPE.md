# WEATHER TYPES

Each weather type is a type of weather that occurs, and what causes it. The only required entries are null and clear.

## `weather_type` properties

|      Identifier      |                                           Description                                            |
| -------------------- | ------------------------------------------------------------------------------------------------ |
| `name`               | UI name of weather type.                                                                         |
| `color`              | UI color of weather type.                                                                        |
| `map_color`          | Map color of weather type.                                                                       |
| `sym`                | Map glyph of weather type.                                                                       |
| `ranged_penalty`     | Penalty to ranged attacks.                                                                       |
| `sight_penalty`      | Penalty to per-square visibility, applied in transparency map.                                   |
| `light_modifier`     | modification to ambient light.                                                                   |
| `sound_attn`         | Sound attenuation of a given weather type.                                                       |
| `dangerous`          | If true, our activity gets interrupted.                                                          |
| `precip`             | Amount of associated precipitation. Valid values are: none, very_light, light and heavy          |
| `rains`              | Whether said precipitation falls as rain.                                                        |
| `tiles_animation`    | Optional, name of the tiles animation to use                                                     |
| `debug_cause_eoc`    | Optional, id of effect_on_condition to be run when the debug menu selects this weather.          |
| `debug_leave_eoc`    | Optional, id of effect_on_condition to be run when the debug menu is used to leave this weather. |
| `sound_category`     | Optional, what sound effect to play. Valid values are: silent, drizzle, rainy, thunder, flurries, snowstorm and snow. |
| `duration_min`       | Optional, the lower bound on the amount of time this weather can last. Defaults to 5 minutes.    |
| `duration_max`       | Optional, the upper bound on the amount of time this weather can last. Defaults to 5 minutes.    |
| `weather_animation`  | Optional, Information controlling weather animations.  Members: factor, color and glyph          |
| `condition`          | A dialog condition to determine if this weather is happening.  See Dialogue conditions section of [NPCs](NPCs.md) |
| `priority`           | An integer.  If the condition of multiple weather types are true the one with higher priority wins. |
| `required_weathers`  | A string array of possible weathers, it is at this point in the loop. i.e. rain can only happen if the conditions for clouds light drizzle or drizzle are present.  Required weathers need to have lower load orders to be. |

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
    "required_weathers": [ "thunder" ],
    "priority": 80,
    "condition": { "not": { "is_pressure": 990 } }
  }
]
