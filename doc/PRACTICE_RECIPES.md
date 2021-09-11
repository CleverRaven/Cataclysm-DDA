# practice

Practice recipes allow practicing skills and proficiencies by working on crafts that are "just for
practice", and do not yield a finished result (although they may have some byproducts). They may be
designed to focus on specific niche skills or proficiencies, which might be prohibitive to learn
from the more productive (and time-consuming) crafting recipes requiring many proficiencies.

The practice recipe JSON format is a form of the "recipe" type, with a few differences noted below.
See [Recipes section of JSON_INFO](JSON_INFO.md#recipes) for more on recipe fields.


## JSON fields

Practice recipes have type "practice", and they cannot define a "result", since they are not
intended to make a specific item. In most other ways, they are just like "recipe" definitions, are
loaded into the main `recipe_dictionary` used for crafting, and appear in the crafting UI.

They:

- must define `id`, `name`, `description` and `practice_data`
- must not define a `result` or `difficulty`
- may define `byproducts`

The `practice_data` field is required for practice recipes, and must be defined. It sets a minimum
and maximum difficulty level.  The difficulty of the recipe will match your current skill level as
long as it is within those bounds.  `practice_data` can also set a hard cap on the maximum level
that can be attained via the recipe, overriding (lowering) the normal limit based on difficulty.

The "category" and "subcategory" fields tell where the recipe should appear in the crafting menu.
Use category "CC_PRACTICE" for practice recipes, and set a subcategory like "CSC_TAILORING" to make
it appear in the corresponding skill sub-tab.

As with other recipes, the `skill_used` and `proficiencies` will affect what is learned during
practice crafting, the `components`, `tools`, and `using` fields for ingredients and requirements
work the same way.


## Example

For example, here is a recipe for practicing intermediate level computer skill:

```
[
  {
    "id": "prac_computer_int",
    "type": "practice",
    "activity_level": "NO_EXERCISE",
    "category": "CC_PRACTICE",
    "subcategory": "CSC_PRACTICE_COMPUTERS",
    "name": "computer (intermediate)",
    "description": "Practice using the command line, writing and running scripts making use of common algorithms and data structures.",
    "skill_used": "computer",
    "time": "1 h",
    "practice_data": { "min_difficulty": 2, "max_difficulty": 4, "skill_limit": 5 },
    "autolearn": [ [ "computer", 4 ] ],
    "book_learn": [ [ "manual_computers", 2 ], [ "computer_science", 3 ] ],
    "tools": [ [ [ "laptop", 50 ] ] ]
  }
]
```

## Notes

Practice recipes can be autolearned at a certain knowledge level, derived from books or taught by
skilled NPCs. They don't show up as recipes in books because they're not really recipes.

The minimum practical skill level is a hard requirement. If you don't have it you can't practice.

They require tools and consume materials and/or tool charges, but are supposed to be more efficient
than crafting+uncrafting something repeatedly.

Practice recipes train skills and proficiencies. They are organized into categories by skill.

Each recipe is typically named after the main skill or proficiency that it trains plus an optional
suffix, e.g. the difficulty tier or material used

Practice is something you do on your own. NPCs will not help you, but may watch and learn.

