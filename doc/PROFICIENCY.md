# proficiency

## definition

```JSON
{
  "id": "prof_knapping",
  "type": "proficiency",
  "name": { "str": "Knapping" },
  "desription": "The ability to turn stones into usable tools.",
  "can_learn": true,
  "time_to_learn": "10 h",
  "required_proficiencies": [ "prof_foo" ]
},
```
### `id`
Mandatory. String
The id of the proficiency.

### `type`
Mandatory. String
Must be `proficiency`.

### `name`
Mandatory. String
The name of the proficiency.

### `description`
Mandatory. String
The description of the proficiency.

### `can_learn`
Mandatory. Bool
Whether or not this proficiency can be learned through normal means during the game.

### `time_to_learn`
Optional. time_duration, as a string
The (optimal) time required to learn this proficiency.

### `required_proficiencies`
Optional. Array of strings
The proficiencies that must be obtained before this one can.
