# proficiency

## definition

```JSON
{
  "type": "proficiency",
  "id": "prof_bow_master",
  "name": { "str": "Master Archer's Form" },
  "description": "You are a master at the art of Archery.",
  "can_learn": true,
  "time_to_learn": "20 h",
  "default_time_multiplier": 1.5,
  "default_fail_multiplier": 1.2,
  "required_proficiencies": [ "prof_bow_expert" ],
  "bonuses": { "archery": [ { "type": "strength", "value": 1 } ] }
}
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

### `bonuses`
Optional. Object, with an array of object as values
This member is used to apply bonuses to certain activities given the player has a particular proficiency. The bonuses applied must be hardcoded to the activity in question.
The keys of the object correspond to what bonuses are - e.g. the `archery` key marks bonuses used for `archery`.
The general format is: `"key": [ { "type": "string", "value": float } ]`.

The values of the keys are an array of objects constructed as so:

#### `type`
Mandatory. String
Where this bonus applies. Valid values are `"strength"`, `"dexterity"`, `"intelligence"`, `"perception"`.

#### `value`
Mandatory. Float
What the bonus is. This can be any numeric value representable as a floating point number.
Values of the same type from all available proficiencies are summed together to produce the final bonus for a proficiency.
