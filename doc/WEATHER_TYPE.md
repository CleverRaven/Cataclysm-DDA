## Weather_type

Each weather type is a type of weather that occurs, and what causes it. The only required entries are null and clear.


##Fields

|     Identifier                 |                              Description                              |
| ------------------------------ | --------------------------------------------------------------------- |
| `name`                         | UI name of weather type.                                              |
| `color`                        | UI color of weather type.                                             |
| `map_color`                    | Map color of weather type.                                            |
| `glyph`                        | Map glyph of weather type.                                            |
| `ranged_penalty`               | Penalty to ranged attacks.                                            |
| `sight_penalty`                | Penalty to per-square visibility, applied in transparency map.        |
| `light_modifier`               | modification to ambient light.                                        |
| `sound_attn`                   | Sound attenuation of a given weather type.                            |
| `dangerous`                    | If true, our activity gets interrupted.                               |
| `precip`                       | Amount of associated precipitation. Valid values are: none, very_light, light and heavy |
| `rains`                        | Whether said precipitation falls as rain.                             |
| `acidic`                       | Whether said precipitation is acidic.                                 |
| `tiles_animation`              | Optional, name of the tiles animation to use                          |
| `sound_category`               | Optional, if playing sound effects what to use. Valid values are: silent, drizzle, rainy, thunder, flurries,
    snowstorm and snow. |
| `sun_intensity`                | Strength of the sun. Valid values are: none, light, normal, and high  |
| `duration_min`                 | Optional, the lower bound on the amount of time this weather can last. Defaults to 5 minutes. Unless time_between_min and time_between_max are set the weather can happen again as soon as it ends. |
| `duration_max`                 | Optional, the upper bound on the amount of time this weather can last. Defaults to 5 minutes. Unless time_between_min and time_between_max are set the weather can happen again soon as it ends. |
| `time_between_min`             | Optional, the lower bounds of the amount of time that will be guaranteed to pass before this weather happens again. Defaults to 0. |
| `time_between_max`             | Optional, the upper bounds of the amount of time that will be guaranteed to pass before this weather happens again. Defaults to 0. |
| `weather_animation`            | Optional, Information controlling weather animations.  Members: factor, color and glyph |
|	`required_weathers`          | a string array of possible weathers it is at this point in the loop. i.e. rain can only happen if the conditions for clouds light drizzle or drizzle are present |
| `condition`                  | A dialog condition to determine if this weather is happening.  See Dialogue conditions section of [NPCs](NPCs.md)
	
### Example

```json
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
},
```