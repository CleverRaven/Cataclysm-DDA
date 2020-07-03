## Weather_type

Each weather type is a type of weather that occurs, its effects and what causes it.  The only required entries are null and clear.


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
| `weather_animation`            | Optional, Information controlling weather animations.  Members: factor, color and glyph |
| `effects`                      | String, int pair array for the effects the weather has.  At present wet, thunder, lightning, light_acid, and acid are supported. |  
	`wet`                         | wets player by int amount
	`thunder`                     | thunder sound with chance 1 in int
	`lightening`                  | 1 in int chance of sound plus message and possible super charging electric fields
	`light_acid`                  | causes pain unless waterproof
	`acid_rain`                   | causes more pain unless waterproof

| `requirements`                  | Optional, is what determines what weather it is.  All members are optional.  
	When checking what weather it is it loops through the entries in order and uses the last one to succeed. |
	
	`pressure_min`
	`pressure_max`
	`humidity_min`
	`humidity_max`
	`temperature_min`
	`temperature_max`
	`windpower_min`
	`windpower_max`
	| These are all minimum and maximum values for which the weather will occur.  I.e it will only rain if its humid enough |
	
	`humidity_and_pressure`      | if there are pressure and humidity requirements are they both required or just one |
	`acidic`                     | does this require acidic precipitation                                |
	`time`                       | Valid values are: day, night, and both.                               |
	`required_weathers`          | a string array of possible weathers it is at this point in the loop. i.e. rain can only happen if the conditions for clouds light drizzle or drizzle are present |

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
    "effects": [ { "name": "thunder", "intensity": 50 }, { "name": "lightning", "intensity": 600 } ],
    "tiles_animation": "weather_rain_drop",
    "weather_animation": { "factor": 0.04, "color": "c_light_blue", "glyph": "," },
    "sound_category": "thunder",
    "sun_intensity": "none",
    "requirements": { "pressure_max": 990, "required_weathers": [ "thunder" ] }
}
```
