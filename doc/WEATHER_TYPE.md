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
| `duration_min`                 | Optional, the lower bound on the amount of time this weather can last. Defaults to 5 minutes. Unless time_between_min and time_between_max are set the weather can happen again as soon as it ends. |
| `duration_max`                 | Optional, the upper bound on the amount of time this weather can last. Defaults to 5 minutes. Unless time_between_min and time_between_max are set the weather can happen again soon as it ends. |
| `time_between_min`             | Optional, the lower bounds of the amount of time that will be guranteed to pass before this weather happens again. Defaults to 0. |
| `time_between_max`             | Optional, the upper bounds of the amount of time that will be guranteed to pass before this weather happens again. Defaults to 0. |
| `weather_animation`            | Optional, Information controlling weather animations.  Members: factor, color and glyph |
| `effects`                      | Array for the effects the weather has. Descibed in detail below
| `requirements`                 | Optional, is what determines what weather it is.  All members are optional. 
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
	`time_passed_min`            | Optional: Until this much time post cataclysm this weather won't happen.|
	`time_passed_max`            | Optional: After this much time post cataclysm this weather stops.|
	`one_in_chance`              | Optional: This has a 1 in this value chance of happening.  This will usually be called every 5 minutes|
	
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
    "effects": [ 
    { 
      "one_in_chance": 50,
      "must_be_outside":false,
      "sound_message": "You hear a distant rumble of thunder.",
      "sound_effect": "thunder_far" 
    },
    { 
      "one_in_chance": 600,
      "must_be_outside":false,
      "message": "A flash of lightning illuminates your surroundings!.",
      "sound_effect": "thunder_near",
      "lightning":true
    }
  ],
    "tiles_animation": "weather_rain_drop",
    "weather_animation": { "factor": 0.04, "color": "c_light_blue", "glyph": "," },
    "sound_category": "thunder",
    "sun_intensity": "none",
    "requirements": { "pressure_max": 990, "required_weathers": [ "thunder" ] }
  },
```
### Weather_effects

Things that weather can cause to happen.

##Fields

|     Identifier                 |                              Description                              |
| ------------------------------ | --------------------------------------------------------------------- |
| `message`                      | Optional: Message displayed when this effect happens.                 |
| `sound_message`                | Optional: Message describing what you hear, will not display if deaf  |
| `sound_effect`                 | Optional: Name of sound effect to play                                |
| `sound_message`                | Optional: Message describing what you hear for this, will not display if deaf. |
| `must_be_outside`              | Whether the effect only happens while you are outside.                |
| `one_in_chance`                | Optional: The chance of the event occuring is 1 in this value, if blank will always happen. |
| `time_between`                 | Optional: The time between instances of this effect occuring.  If both this and one_in_chance are set will only happen when both are true. |
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
      
        optional( weather_effect_jo, was_loaded, "", effect.lightning, false );
        ### Example

```json
 {
      "must_be_outside":true,
      "radiation":10,
      "healthy":1,
      "message":"Suddenly a something",
      "add_effect":"bite",
      "effect_duration":"10 minutes",
      "target_part": "arm_l",
      "damage":[
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
        "max_radius":10,
        "min_radius":10,
        "target":"mon_zombie_survivor_elite",
        "hallucination_count":1,
        "real_count":0
}]
```  
   ### spawn_type

How many and what spawns

##Fields     
        
|     Identifier                 |                              Description                              |
| ------------------------------ | --------------------------------------------------------------------- |
| `max_radius`                   | Optional: The furthest away a spawn will happen.                      |
| `min_radius`                   | Optional: The closest a spawn will happen.                            |
| `hallucination_count`          | Optional: Number of hallucinations of the target to spawn.            |
| `real_count`                   | Optional: Number of real copies to spawn.                             |
| `target`                       | Optional: Monster id of target to spawn.  If left blank a nearby monster will be used. |
| `target_range`                 | Optional: If target is left blank how far away to look for something to copy. |

 ### fields

Fields to create what kind and where

##Fields     
        
|     Identifier                 |                              Description                              |
| ------------------------------ | --------------------------------------------------------------------- |
| `type`                         | The string id of the field.                                           |
| `intensity`                    | Intensity of the field.                                               |
| `age`                          | Age of the field.                                                     |
| `outdoor_only`                 | Optional: Defaults to true. If true field will only spawn outdoors.   |
| `radius`                       | Optional: Radius around player the effect will spread, defaults to everywhere.  |



