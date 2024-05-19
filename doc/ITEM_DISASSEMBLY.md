# JSON INFO

Use the `Home` key to return to the top.

<!-- START doctoc generated TOC please keep comment here to allow auto update -->
<!-- DON'T EDIT THIS SECTION, INSTEAD RE-RUN doctoc TO UPDATE -->
**Table of Contents**

- [Introduction](#introduction)
- [The three methods](#the-three-methods)
  - [Uncraft recipes](#uncraft-recipes)

<!-- END doctoc generated TOC please keep comment here to allow auto update -->

# Introduction
This document describes various methods of taking apart items in the game, as well as how these work and how the approach should be balanced. For furniture/terrain ***DECONSTRUCTION*** you're out of luck because we don't have a guide for it.

# The three methods
There are three* general methods of having items taken apart into other items:
- Uncraft recipes
- Reversible crafting recipes
- Salvaging (or Cutting Up)

The first two are able to be altered through JSON, while the other can only be enabled or disabled, with the type and amount of items it gives being calculated by the game from the item's JSON definition.

*Technically you could argue that butchery is a separate method as well, but given its highly unique nature, and the fact that it's not possible for it to violate the conservation of mass, it has been omitted for the purpose of this file.

## Uncraft recipes
They are the most common and well known way of defining an item disassembly. With a syntax not unlike that of regular crafting recipes, they're fairly self-explanatory and easy to grasp.

```json
  {
    "result": "bio_blood_filter",
    "type": "uncraft",
    "activity_level": "LIGHT_EXERCISE",
    "skill_used": "electronics",
    "difficulty": 7,
    "skills_required": [ "firstaid", 5 ],
    "time": "50 m",
    "using": [ [ "soldering_standard", 20 ] ],
    "tools": [ [ [ "boltcutters", -1 ], [ "toolset", -1 ] ] ],
    "qualities": [ { "id": "SCREW", "level": 1 } ],
    "components": [ [ [ "burnt_out_bionic", 1 ] ] ],
    "flags": [ "BLIND_HARD" ]
  },
```

| Field                         | Meaning
|---                            |---
| `result`                      | (Mandatory) The ID of the item being disassembled
| `type`                        | (Mandatory) The type of the recipe; if we want an uncraft recipe, it should always be ``uncraft``
| `activity_level`              | (Mandatory) How energy intensive of an activity this craft is. Options are ``NO_EXERCISE``, ``LIGHT_EXERCISE``, ``MODERATE_EXERCISE``, ``BRISK_EXERCISE``, ``ACTIVE_EXERCISE``, ``EXTRA_EXERCISE``
| `skill_used`                  | Skill trained and used for success checks
| `difficulty`                  | Difficulty of success check, connected to ``skill_used``
| `skills_required`             | Skills required to unlock recipe
| `time`                        | (Mandatory) Time to perform the recipe; can specify in minutes, hours etc.
| `using`                       | Requirement IDs and multipliers of tools and materials used
| `qualities`                   | Qualities of tools needed to perform the disassembly
| `tools`                       | Specific tools needed to perform the disassembly
| `components`                  | (Mandatory) Items produced from completing the disassembly recipe; Items cannot have ``UNRECOVERABLE`` flag
| `flags`                       | A set of strings describing boolean features of the recipe; Supports ``BLIND_EASY`` and ``BLIND_HARD``

Things to note:
- Simple disassemblies, such as smashing a skull or simply cutting apart metal with a hacksaw, should likely not use any skills.
- It is not possible to obtain items with ``UNRECOVERABLE`` flag through disassembly, either through uncraft recipes or reversible crafting recipes, however, defining them in the ``components`` field does not cause errors. They will simply be ignored.
- ``copy-from`` support for uncraft recipes is extremely limited and it is best to avoid it where possible.
