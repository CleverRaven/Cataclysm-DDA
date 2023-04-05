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
- may define `byproducts` / `byproduct_group`

The `practice_data` field is required for practice recipes, and must be defined. It is an object
with its own fields, as follows:

- `min_difficulty` - Lowest skill level when practice recipe can be attempted.
- `max_difficulty` - Highest skill level difficulty you will get credit for when practicing.
- `skill_limit` - Hard cap on highest skill level that can be obtained. Optional, default max skill level (10). This should not be set higher than `max_difficulty` + 1.

The difficulty of the recipe will match your current *practical* skill level if it is between
`min_difficulty` and `max_difficulty`.

The "category" and "subcategory" fields tell where the recipe should appear in the crafting menu.
Use category "CC_PRACTICE" for practice recipes, and set a subcategory like "CSC_TAILORING" to make
it appear in the corresponding skill sub-tab.

As with other recipes, the `skill_used` and `proficiencies` will affect what is learned during
practice crafting, the `components`, `tools`, and `using` fields for ingredients and requirements
work the same way.


## Example

For example, here is a recipe for practicing intermediate level computer skill:

```json
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

The recipe `time` should be set to the standard `1 h` unless there is a compelling reason to do
otherwise.  Practice recipes will be easier to balance against one another for skill and proficiency
development if they all adhere to the same standard recipe length.

