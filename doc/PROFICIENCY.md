# proficiency

## definition

```JSON
{
  "id": "prof_knapping",
  "type": "proficiency",
  "name": { "str": "Knapping" },
  "description": "The ability to turn stones into usable tools.",
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

### `default_time_multiplier` and `default_fail_multiplier`
Optional. Float  
When used in recipes these values are the amount the time to complete the recipe, and the chance to fail the recipe, will be multiplied by.

- For proficiencies that represent core basic knowledge and foundational principles, the `time` should usually be low (1.5 or so), and the `fail` should be high (4 or more).
- For "flavor" proficiencies that offer a small boost, these should be around 1.5 each.
- Most other proficiencies should be in the 2-3 range for both values.

### `time_to_learn`
Optional. time_duration, as a string  
The (optimal) time required to learn this proficiency.

### `required_proficiencies`
Optional. Array of strings  
The proficiencies that must be obtained before this one can.  You cannot gain experience in a proficiency without the necessary prerequisites.
