# Item disassembly

Use the `Home` key to return to the top.

<!-- START doctoc generated TOC please keep comment here to allow auto update -->
<!-- DON'T EDIT THIS SECTION, INSTEAD RE-RUN doctoc TO UPDATE -->
**Table of Contents**

- [Introduction](#introduction)
- [The three methods](#the-three-methods)
  - [Uncraft recipes](#uncraft-recipes)
  - [Reversible crafting recipes](#reversible-crafting-recipes)
  - [Salvaging / Cutting Up](#salvaging--cutting-up)
- [Choosing the method](#choosing-the-method)
- [Closing words (Or what you should remember when working with item disassembly in general)](#closing-words-or-what-you-should-remember-when-working-with-item-disassembly-in-general)

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
| `time`                        | (Mandatory) Time to perform the recipe; can specify in minutes, hours etc.
| `using`                       | Requirement IDs and multipliers of tools and materials used
| `qualities`                   | Qualities of tools needed to perform the disassembly
| `tools`                       | Specific tools needed to perform the disassembly
| `components`                  | (Mandatory) Items produced from completing the disassembly recipe; Items cannot have ``UNRECOVERABLE`` flag
| `flags`                       | A set of strings describing boolean features of the recipe; Supports ``BLIND_EASY`` and ``BLIND_HARD``

Things to note:
- Simple disassemblies, such as smashing a skull or simply cutting apart metal with a hacksaw, should likely not use any skills
- It is not possible to obtain items with ``UNRECOVERABLE`` flag through disassembly, either through uncraft recipes or reversible crafting recipes, however, defining them in the ``components`` field does not cause errors. They will simply be ignored
- ``copy-from`` support for uncraft recipes is extremely limited and it is best to avoid it where possible
- for the purposes of keeping things easy to find, future uncraft recipes should be included inside the ``uncraft`` folder inside of ``data\json``
- uncraft recipes do not support component lists, the syntax shown below does **NOT** work - only the first item read by the game has any effect
```json
"components": [ [ [ "burnt_out_bionic", 1 ], [ "scrap", 1 ] ] ],
```

- due to not supporting component lists, and not remembering what items were used to craft the item that is being disassembled, uncraft recipes can be used to transmute resources by the players **if not used alongside reversible crafting recipes - more info on that in [Reversible crafting recipes](#reversible-crafting-recipes)**
- it is technically possible to define proficiencies for uncraft recipes, but they currently have no effect
- similarly, it is possible to define a ``skills_required`` field for uncraft recipes, but it has no effect either

## Reversible crafting recipes
A reversible recipe and an uncraft recipe are almost indistinguishable in game, with the only potential way to tell them apart being items crafted by the player through a reversible crafting recipe may yield different items upon disassembly than items of the same ID found spawned in the world. Having said that, they are quite different from the JSON side.

The first thing that comes to mind is - reversible crafting recipes are created through a singular field. Adding ``"reversible": "true`` to the recipe definition automatically creates a disassembly for the item the recipe is for. It is worth noting that unlike uncraft recipes, reversible crafting recipes support ingredient lists, **but only in regards to items crafted by the player**. If the item in question was crafted by the player, disassembling it will yield items used to craft it. If the item was spawned in the world, however, the disassembly will instead yield the first component combination the game reads off the recipe definition.
Reversible crafting recipes also have their time, skills used, difficulty, and tools taken from the same crafting recipe they're created as a part of. **Out of all those, time is the only one that can be overwritten directly in the crafting recipe.** ``"reversible": { "time": "3 m" },`` will make the disassembly take 3 minutes, regardless of how long the craft takes.

Things to note:
- **Reversible crafting recipes cannot have byproducts!** Trying to make a recipe with byproducts reversible will not work.
- On the other hand, it is possible to make recipes using ``result_mult`` reversible, but this will inadvertently cause infinite resource generation, as full recipe ingredients will be obtained from disassembling a single result item
- All items used to craft the item will be obtained through the disassembly, with the exception of items with the ``UNRECOVERABLE`` flag
- While unlike with uncraft recipes it is impossible to transmute materials through those, it is very easy to make nonsensical disassemblies through this method when it comes to required tools. Consider using the two methods alongside one another.
- Making a recipe that crafts a specific item variant reversible will result in all variants of this item using the same disassembly

**Using uncrafts and reversible crafting together:**
- Either of those methods will work alone, but they also interact if both are defined
- If a crafting recipe has ``reversible: true``, *and* the item has a manually defined uncraft recipe, what will happen is the uncraft recipe will be able to return components used to craft the specific item. In a case like this, the uncraft will supply all information exception for the components used to craft that item - this means that this combination can remember components (overcoming uncraft's weakness) **AND** avoid nonsensical tool requirements or difficulty levels (overcoming reversible crafting's weakness)

## Salvaging / Cutting Up
This process is largely hardcoded, with the JSON side only consisting of defining whether a specific material is salvageable, and possible per-item salvaging disabling through flags. The only way to make an item that's normally salvageable not-salvageable is by either editing its material or adding the ``NO_SALVAGE`` flag.

To salvage an item with **at least one salvageable material** you must have a tool with the ``CUTTING`` quality. This process can only grant a singular type of resource (per material), with the amount calculated from the item's weight. How long the process takes is calculated from the item's size as well.

Any material with a ``salvaged_into`` field defined is considered salvageable.

Things to note:
- A tool with 1 ``CUTTING`` quality is enough to salvage any item which is at least partially the listed material. As such, items that should require more powerful tools should be given the ``NO_SALVAGE`` flag
- If an item has more than one material that can be salvaged, it will give resources corresponding to all of its materials. However, **if the item is partially a material that cannot be salvaged, that material will be ignored, and no resources from it will be given**
- It is the only listed here method of taking an item apart which cannot be done through the ``disassemble`` option or menu
- It does not work with charges well. Multiple charge-based item will be treated as a singular bigger item for the purpose of salvaging. **100 charge-based items weighing 1g equals to 1 normal item weighing 100g**

# Choosing the method

So you want to make a new disassembly recipe? First of all, I'm proud of you, but you probably should have some vague idea of which of the methods above you should pick, because they all have their pros and cons. Except salvaging, that's all cons - we're ignoring it for this part of the documentation.

You should use reversible crafting recipes if:
- The item has a crafting recipe in the first place, which should be fairly self-explanatory
- The recipe for this item does not produce byproducts
- **The tools needed for the craft make sense to be required for its disassembly as well.** This disqualifies most of blacksmithing and metalworking as a whole, because needing a crucible to take apart tongs is ridiculous

If the following three are **NOT** true, you likely want a manually defined uncraft recipe, as you can omit skills and define tools required as you please.

# Closing words (Or what you should remember when working with item disassembly in general)
1. Conservation of mass is pretty damn important. You won't always be able to make sure there is no mass loss or generation - it is just not possible in more complex crafts due to our generic nature of resource items - but you should still try to minimize the amount of mass lost or generated whenever you're working on a recipe. After getting your recipe done, calculate the mass of the ingredients and compare it to the mass of the item to make sure you're not violating physics.
2. Double check the syntax when it comes to uncraft recipes. They do not support lists, but the game doesn't realize that. You will not get an error, it is only on **YOU** to catch your missing brackets.
3. It is not possible to have both an uncraft and a reversible recipe defined for the same item. The same goes for more than one uncraft. If this occurs, the game will only read one and ignore everything else altogether.
