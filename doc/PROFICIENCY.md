# Proficiencies

Proficiencies are specializations or fields of expertise a character may have. Having a proficiency
makes a character more time-efficient and successful at activities and crafts related to it.


## Skills and proficiencies

Proficiencies are entirely distinct from skills, and are not directly associated with them; they are
a separate system of tracking more specific subsets of knowledge.  Most proficiencies will have an
obvious general affiliation with one or more skills, but this is loose.

Most skills in CDDA (particularly the non-combat ones) represent broad sets of transferable
knowledge, while proficiencies are narrower and more focused.  By keeping them mechanically separate
from skills we are able to represent, for example, that being proficient in construction may have a
role in fabricating things out of wood, or constructing survival shelters along similar principles,
regardless of what level fabrication or survival skill happen to be.

You could think of this as a three tiered system:

1. Top tier - skill - very large and broad areas of knowledge
2. Mid tier - proficiency - more focused and specific areas of knowledge
3. Bottom tier - recipes - very specific single-focus areas of knowledge



## Proficiency trees or graphs

Many proficiencies naturally align into a tree, with basic proficiencies branching into detailed
specializations.  For example, part of a metalworking proficiency tree:

- Principles of Metalworking
    - Principles of Welding
        - Welding
    - Blacksmithing
        - Armorsmithing
        - Bladesmithing
        - Manual Tooling
    - Redsmithing
    - Wire Making

One must be fully proficient in Principles of Metalworking before learning any other proficiencies
within this tree.  Similarly, Blacksmithing proficiency must be acquired before Armorsmithing,
Bladesmithing, or Manual Tooling can be studied - but those sub-proficiencies may be learned in
parallel, once the required Blacksmithing proficiency is achieved.

However, while related proficiencies may tend to follow a tree structure, the system of
proficiencies is actually a dependency graph; each proficiency may require or depend on any number
of other proficiencies.

For example, Antique Gunsmithing requires Principles of Metalworking, and also Principles of
Gunsmithing.  Similarly, Gem Setting requires both Redsmithing and Fine Metalsmithing.  This graph
structure makes proficiencies a powerful way of expressing complex requirements for many kinds of
activities a survivor might do.


## Effects of gaining proficiency

Crafting recipes may involve any number of proficiencies, in addition to whatever skill requirements
and difficulty they may have.  When proficiencies are used in recipes, they can affect the crafting
time and chance of failure, depending on how many of those proficiencies the character has, and how
well they know them.

Proficiencies may apply bonuses to certain activities.  The effects of these bonuses must be
hardcoded to the activity in question, but the kind of bonus and the strength of its effect can be
described as part of the proficiency (see below).

While a character is learning a proficiency, a percentage is displayed next to it showing how much
of it has been learned.  When it reaches 100%, they become fully "proficient" at that ability, and
will do activities and crafting with that proficiency at maximum speed with minimal failures.

Before reaching 100%, there are some penalties to time and success.  The extra time and chance to
fail are computed using [separate formulas](https://github.com/CleverRaven/Cataclysm-DDA/pull/49198)
and their multipliers may be given separately in the proficiency JSON and any recipes using them.


## Proficiency categories

In order to better organize proficiencies in the in-game UI, each proficiency belongs to a specific
category pointing to a JSON defined `"proficiency_category"` object:

```JSON
{
  "type": "proficiency_category",
  "id": "prof_archery",
  "name": "Archery",
  "description": "Proficiencies for all things bow and arrows.  Includes knowledge and experience of making and modifying bows, as well as archery form and posture."
}
```


## Definition

Proficiencies are defined in JSON files in the `data/json/proficiencies` directory.  Files are named
according to a conceptual classification of proficiencies within; they may relate to the names of
skills, but don't have to.  For example some are named "metalwork", "wilderness", and "woodworking".

Within these are the standard list of JSON objects having "type": "proficiency".  For example:

```JSON
[
  {
    "type": "proficiency",
    "id": "prof_bow_master",
    "category": "prof_archery",
    "name": { "str": "Master Archer's Form" },
    "description": "You are a master at the art of Archery.",
    "can_learn": true,
    "teachable": true,
    "time_to_learn": "20 h",
    "default_time_multiplier": 1.5,
    "default_skill_penalty": 0.2,
    "required_proficiencies": [ "prof_bow_expert" ],
    "bonuses": { "archery": [ { "type": "strength", "value": 1 } ] }
  }
]
```

### JSON fields

| field                       | mandatory | type   | description                                                                          |
| ---                         | ---       | ---    | ---                                                                                  |
| `id`                        | Mandatory | String | Internal id of the proficiency, used for all JSON and code references to it.         |
| `type`                      | Mandatory | String | Must be `proficiency` for all proficiencies.                                         |
| `category`                  | Mandatory | String | Internal id of the associated `proficiency_category` object.                         |
| `name`                      | Mandatory | String | Name of the proficiency, used for all in-game display.                               |
| `description`               | Mandatory | String | Description of what abilities or special knowledge the proficiency entails.          |
| `can_learn`                 | Mandatory | Bool   | Whether or not this proficiency can be learned through normal means during the game. |
| `teachable`                 | Optional  | Bool   | Whether it's possible to teach this proficiency between characters. (Default = true) |
| `default_time_multiplier`   | Optional  | Float  | Time multiplier for crafting recipes (see below).                                    |
| `default_skill_penalty`     | Optional  | Float  | Effective skill penalty for crafting recipes (see below).                            |
| `default_weakpoint_bonus`   | Optional  | Float  | Flat bonus to the attacker's skill.                                                  |
| `default_weakpoint_penalty` | Optional  | Float  | Flat penalty to the attacker's skill if they lack the skill.                         |
| `time_to_learn`             | Optional  | time_duration, as a string | The (optimal) time required to learn this proficiency.           |
| `required_proficiencies`    | Optional  | Array of strings | The proficiencies that must be obtained before this one can.  You cannot gain experience in a proficiency without the necessary prerequisites. |
| `ignore_focus`              | Optional  | Bool   | Proficiency exp gain will be as if focus is `100` regardless of actual focus.        |
| `bonuses`                   | Optional  | Object, with an array of object as values | This member is used to apply bonuses to certain activities given the player has a particular proficiency. The bonuses applied must be hardcoded to the activity in question. (see below) |

### time multiplier and skill penalty

Regarding `default_time_multiplier` and `default_skill_penalty`, these specify the maximum penalty
for lacking the proficiency when crafting a recipe that involves it.

- For proficiencies that represent core basic knowledge and foundational principles, the `time` multiplier should usually be low (1.5 or so), and the `skill` penalty should be lower (0.4 or more).
- For "flavor" proficiencies that offer a small boost, these should be around 1.5 for time and 0 to 0.1 for skill.
- Most other proficiencies should be in the 2-3 range for time values and 0.2-0.3 for skill.
- In general skill penalties should range from 0.1 to 0.5, while time rates should range from 1.5 to 5.

### bonuses

The keys of the `bonuses` object correspond to what bonuses are - e.g. the `archery` key marks
bonuses used for `archery`.  The general format is:

```json
  "bonuses": {
    "key": [ { "type": "string", "value": float } ]
  }
```

Using the example from above:

```JSON
  "bonuses": {
    "archery": [ { "type": "strength", "value": 1 } ]
  }
```

The values of the keys are an array of objects constructed as so:

Field   | Mandatory | Type   | Description
------- | --------- | ------ | ---
`type`  | Mandatory | String | Where this bonus applies. Valid values are `"strength"`, `"dexterity"`, `"intelligence"`, `"perception"`.
`value` | Mandatory | Float  | What the bonus is. This can be any numeric value representable as a floating point number.  Values of the same type from all available proficiencies are summed together to produce the final bonus for a proficiency.

For the `melee_attack` key, only `"type": "stamina"` is valid, and when an attack is performed using a weapon category that points to this proficiency, the final stamina cost of the attack will be multiplied by 1 - (the sum of stamina bonuses). This does not stack across categories, and only the lowest resulting stamina value will be used.
