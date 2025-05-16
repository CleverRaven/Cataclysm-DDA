# JSON INFO

Use the `Home` key to return to the top.

<!-- START doctoc generated TOC please keep comment here to allow auto update -->
<!-- DON'T EDIT THIS SECTION, INSTEAD RE-RUN doctoc TO UPDATE -->
**Table of Contents**

- [Introduction](#introduction)
  - [Overall structure](#overall-structure)
  - [Common properties](#common-properties)
    - [`"copy-from"` and `"abstract"`](#copy-from-and-abstract)
- [Navigating the JSON](#navigating-the-json)
- [Common field types](#common-field-types)
  - [Units](#units)
    - [Time duration](#time-duration)
  - [Translatable strings](#translatable-strings)
  - [Comments](#comments)
- [File descriptions](#file-descriptions)
  - [`data/json/`](#datajson)
  - [`data/json/items/`](#datajsonitems)
    - [`data/json/items/comestibles/`](#datajsonitemscomestibles)
  - [`data/json/requirements/`](#datajsonrequirements)
  - [`data/json/vehicles/`](#datajsonvehicles)
- [Description and content of each JSON file](#description-and-content-of-each-json-file)
  - [`data/json/` JSONs](#datajson-jsons)
    - [Ascii_arts](#ascii_arts)
    - [Snippets](#snippets)
    - [Addiction types](#addiction-types)
    - [Body Graphs](#body-graphs)
      - [Graph Parts](#graph-parts)
    - [Body_parts](#body_parts)
- [On-hit Effects](#on-hit-effects)
    - [Limb scores](#limb-scores)
    - [Character Modifiers](#character-modifiers)
      - [Character Modifiers - Value](#character-modifiers---value)
    - [Bionics](#bionics)
    - [Damage Types](#damage-types)
    - [Damage Info Ordering](#damage-info-ordering)
    - [Dreams](#dreams)
    - [Disease](#disease)
    - [End Screen](#end-screen)
    - [Emitters](#emitters)
    - [Item Groups](#item-groups)
    - [Item Category](#item-category)
    - [Item faults](#item-faults)
    - [Item fault fixes](#item-fault-fixes)
    - [Item fault groups](#item-fault-groups)
    - [Materials](#materials)
      - [Fuel data](#fuel-data)
      - [Burn data](#burn-data)
    - [Monster Groups](#monster-groups)
      - [Group definition](#group-definition)
      - [Monster/Subgroup definition](#monstersubgroup-definition)
    - [Monster Factions](#monster-factions)
    - [Monsters](#monsters)
    - [Mutation Categories](#mutation-categories)
    - [Names](#names)
    - [Profession item substitution](#profession-item-substitution)
    - [Professions](#professions)
      - [`items`](#items)
      - [`hobbies`](#hobbies)
      - [`whitelist_hobbies`](#whitelist_hobbies)
    - [Hobbies](#hobbies)
    - [Profession groups](#profession-groups)
    - [Constructions](#constructions)
    - [Scent_types](#scent_types)
    - [Scores, Achievements, and Conducts](#scores-achievements-and-conducts)
      - [`event_transformation`](#event_transformation)
      - [`event_statistic`](#event_statistic)
      - [`score`](#score)
      - [`achievement`](#achievement)
      - [`conduct`](#conduct)
    - [Skills](#skills)
    - [Speed Description](#speed-description)
    - [Mood Face](#mood-face)
    - [Tool Qualities](#tool-qualities)
    - [Traits/Mutations](#traitsmutations)
    - [Trait Migrations](#trait-migrations)
    - [Traps](#traps)
    - [Vehicle Groups](#vehicle-groups)
    - [Vehicle Parts](#vehicle-parts)
      - [Symbols and Variants](#symbols-and-variants)
      - [The following optional fields are specific to CARGO parts.](#the-following-optional-fields-are-specific-to-cargo-parts)
      - [The following optional fields are specific to ENGINEs.](#the-following-optional-fields-are-specific-to-engines)
      - [The following optional fields are specific to WHEELs.](#the-following-optional-fields-are-specific-to-wheels)
      - [The following optional fields are specific to ROTORs.](#the-following-optional-fields-are-specific-to-rotors)
      - [The following optional fields are specific to WORKBENCHes.](#the-following-optional-fields-are-specific-to-workbenches)
      - [The following optional fields are specific to SEATs.](#the-following-optional-fields-are-specific-to-seats)
      - [The following optional field describes pseudo tools for any part.](#the-following-optional-field-describes-pseudo-tools-for-any-part)
    - [Part Resistance](#part-resistance)
    - [Vehicle Placement](#vehicle-placement)
    - [Vehicle Spawn](#vehicle-spawn)
    - [Vehicles](#vehicles)
    - [Weakpoint Sets](#weakpoint-sets)
- [`json/` JSONs](#json-jsons)
    - [Harvest](#harvest)
      - [`id`](#id)
      - [`type`](#type)
      - [`message`](#message)
      - [`entries`](#entries)
      - [`leftovers`](#leftovers)
    - [Harvest Drop Type](#harvest-drop-type)
    - [Weapon Category](#weapon-category)
    - [Connect group definitions](#connect-group-definitions)
    - [Furniture](#furniture)
      - [`type`](#type-1)
      - [`keg_capacity`](#keg_capacity)
      - [`deployed_item`](#deployed_item)
      - [`lockpick_result`](#lockpick_result)
      - [`lockpick_message`](#lockpick_message)
      - [`light_emitted`](#light_emitted)
      - [`boltcut`](#boltcut)
      - [`hacksaw`](#hacksaw)
      - [`oxytorch`](#oxytorch)
      - [`prying`](#prying)
      - [`required_str`](#required_str)
      - [`crafting_pseudo_item`](#crafting_pseudo_item)
      - [`workbench`](#workbench)
      - [`plant_data`](#plant_data)
      - [`surgery_skill_multiplier`](#surgery_skill_multiplier)
    - [Terrain](#terrain)
      - [`type`](#type-2)
      - [`heat_radiation`](#heat_radiation)
      - [`light_emitted`](#light_emitted-1)
      - [`lockpick_result`](#lockpick_result-1)
      - [`lockpick_message`](#lockpick_message-1)
      - [`trap`](#trap)
      - [`boltcut`](#boltcut-1)
      - [`hacksaw`](#hacksaw-1)
      - [`oxytorch`](#oxytorch-1)
      - [`prying`](#prying-1)
      - [`transforms_into`](#transforms_into)
      - [`allowed_template_ids`](#allowed_template_ids)
      - [`curtain_transform`](#curtain_transform)
      - [`shoot`](#shoot)
      - [`harvest_by_season`](#harvest_by_season)
      - [`liquid_source`](#liquid_source)
      - [`roof`](#roof)
    - [Common To Furniture And Terrain](#common-to-furniture-and-terrain)
      - [`id`](#id-1)
      - [`name`](#name)
      - [`flags`](#flags)
      - [`connect_groups`](#connect_groups)
        - [Connection groups](#connection-groups)
      - [`connects_to`](#connects_to)
      - [`rotates_to`](#rotates_to)
      - [`symbol`](#symbol)
      - [`move_cost_mod`](#move_cost_mod)
      - [`comfort`](#comfort)
      - [`floor_bedding_warmth`](#floor_bedding_warmth)
      - [`fall_damage_reduction`](#fall_damage_reduction)
      - [`bonus_fire_warmth_feet`](#bonus_fire_warmth_feet)
      - [`looks_like`](#looks_like)
      - [`color` or `bgcolor`](#color-or-bgcolor)
      - [`coverage`](#coverage)
      - [`max_volume`](#max_volume)
      - [`examine_action`](#examine_action)
      - [`close` and `open`](#close-and-open)
      - [`bash`](#bash)
      - [`deconstruct`](#deconstruct)
      - [`map_bash_info`](#map_bash_info)
        - [`str_min`, `str_max`](#str_min-str_max)
        - [`str_min_blocked`, `str_max_blocked`](#str_min_blocked-str_max_blocked)
        - [`str_min_supported`, `str_max_supported`](#str_min_supported-str_max_supported)
        - [`sound`, `sound_fail`, `sound_vol`, `sound_fail_vol`](#sound-sound_fail-sound_vol-sound_fail_vol)
        - [`furn_set`, `ter_set`](#furn_set-ter_set)
        - [`ter_set_bashed_from_above`](#ter_set_bashed_from_above)
        - [`explosive`](#explosive)
        - [`destroy_only`](#destroy_only)
        - [`bash_below`](#bash_below)
        - [`tent_centers`, `collapse_radius`](#tent_centers-collapse_radius)
        - [`items`](#items-1)
      - [`map_deconstruct_info`](#map_deconstruct_info)
        - [`furn_set`, `ter_set`](#furn_set-ter_set-1)
        - [`skill`](#skill)
        - [`items`](#items-2)
      - [`plant_data`](#plant_data-1)
        - [`transform`](#transform)
        - [`emissions`](#emissions)
        - [`base`](#base)
        - [`growth_multiplier`](#growth_multiplier)
        - [`harvest_multiplier`](#harvest_multiplier)
    - [clothing_mod](#clothing_mod)
    - [Flags](#flags)
- [Scenarios](#scenarios)
  - [`description`](#description)
  - [`name`](#name-1)
  - [`points`](#points)
  - [`items`](#items-3)
  - [`flags`](#flags-1)
  - [`cbms`](#cbms)
  - [`traits`, `forced_traits`, `forbidden_traits`](#traits-forced_traits-forbidden_traits)
  - [`allowed_locs`](#allowed_locs)
  - [`start_name`](#start_name)
  - [`professions`](#professions)
  - [`hobbies`](#hobbies-1)
  - [`whitelist_hobbies`](#whitelist_hobbies-1)
  - [`map_special`](#map_special)
  - [`requirement`](#requirement)
  - [`reveal_locale`](#reveal_locale)
  - [`distance_initial_visibility`](#distance_initial_visibility)
  - [`eocs`](#eocs)
  - [`missions`](#missions)
  - [`start_of_cataclysm`](#start_of_cataclysm)
  - [`start_of_game`](#start_of_game)
- [Starting locations](#starting-locations)
  - [`name`](#name-2)
  - [`terrain`](#terrain)
    - [Examples](#examples)
  - [`city_sizes`](#city_sizes)
  - [`city_distance`](#city_distance)
  - [`allowed_z_levels`](#allowed_z_levels)
  - [`flags`](#flags-2)
- [Mutation overlay ordering](#mutation-overlay-ordering)
  - [`id`](#id-2)
  - [`order`](#order)
- [MOD tileset](#mod-tileset)
  - [`compatibility`](#compatibility)
  - [`tiles-new`](#tiles-new)
- [Obsoletion and migration](#obsoletion-and-migration)
- [Field types](#field-types)
  - [Emits](#emits)
  - [Immunity data](#immunity-data)
- [Option sliders](#option-sliders)
  - [Option sliders - Fields](#option-sliders---fields)
  - [Option sliders - Levels](#option-sliders---levels)

<!-- END doctoc generated TOC please keep comment here to allow auto update -->

# Introduction
This document describes the contents of the json files used in Cataclysm: Dark Days Ahead. You are probably reading this if you want to add or change content of Cataclysm: Dark Days Ahead and need to learn more about what to find where and what each file and property does.

## Overall structure
The game data is distributed amongst many JSON files in `data`.  Most of the
core game data is in `data/json`, with mod data in `data/mods`.  There is also
some in other subdirectories of `data`, but you are less likely to be interested
in those.

Each JSON file is a list of JSON objects
```jsonc
[
  {
    "…": "…"
  },
  {
    "…": "…"
  }
]
```

Each object must have a `"type"` member that tells the game how to interpret
that object.  For example, crafting recipes have `"type": "recipe"`, vehicle
parts have `"type": "vehicle_part"`, and so on.  Note that items are a little
unusual; there are multiple types which can be used to define an item.  See
[the item documentation](#datajsonitems-jsons) for more details.

Each of these types is documented separately, either below or in other
documentation which should be linked from below (doubtless a few have been
missed; feel free to file bugs for missing documentation).

The documentation is organized by file, because objects of the same type tend
to be defined together in one file or a collection of co-located files.
However, the game does not enforce this convention and in practice you could
define a JSON object of any type in any file.  If you were writing a small mod
it might be reasonable to simply put all your JSON in a single file and that
would be fine.

There are a few cases where certain objects must be loaded before other objects
and the names of the files defining those objects will affect the [loading
order](JSON_LOADING_ORDER.md); sticking to established convention should avoid
that issue.

There are a few features that most types of JSON object have in common.  Those
common features are documented in the next section.

## Common properties

For most types, every object of that type must have a unique id.  That id is
typically defined by the `"id"` field.  For example:

```jsonc
  {
    "type": "skill",
    "id": "barter",
    "name": { "str": "bartering" },
    "description": "…",
    "display_category": "display_social"
  }
```

This defines a skill with id `barter`.

### `"copy-from"` and `"abstract"`

Sometimes you want to define an object which is similar to another object,  or
a collection of similar objects.  In most cases you can achieve this with
`"copy-from"`, specifying the id of the object you wish to copy.  For example,
the definition of a harvested pine tree copies `t_tree_pine` (the unharvested
pine tree) and then specifies only a few properties.  Other properties (such as
the fact that it's impassable, flammable, etc.) are inherited from
`t_tree_pine`.

```jsonc
  {
    "type": "terrain",
    "id": "t_tree_pine_harvested",
    "copy-from": "t_tree_pine",
    "name": "pine tree",
    "description": "A towering coniferous tree that belongs to the 'Pinus' genus, with the New England species varying from 'P. strobus', 'P. resinosa' and 'P. rigida'.  Some of the branches have been stripped away and many of the pinecones aren't developed fully yet, but given a season, it could be harvestable again.",
    "symbol": "4",
    "color": "brown",
    "looks_like": "t_tree_deadpine",
    "transforms_into": "t_tree_pine",
    "examine_action": "harvested_plant"
  },
```

Sometimes you might want define a collection of objects which are similar, but
there is no obvious single object that the others should copy.  In this case,
you can create a special, *abstract* object and have all the others copy it.

An abstract object specifies its id via the `"abstract"` field rather than
`"id"`.  For example, here is the abstract vehicle alternator:

```jsonc
  {
    "abstract": "vehicle_alternator",
    "type": "vehicle_part",
    "fuel_type": "battery",
    "symbol": "*",
    "color": "yellow",
    "broken_color": "red",
    "flags": [ "ALTERNATOR" ]
  },
```

All vehicle alternator definitions use `"copy-from": "vehicle_alternator"` to
inherit these common properties, but because `vehicle_alternator` is an
abstract object, it does not appear in the game as a real vehicle part you can
install.

When using `"copy-from"`, you can define fields that were also defined in the
object you are copying, and the new value will override the old.  However,
sometimes you want to change the value in the copied object without overriding
it entirely; there is support for that.  See the [JSON
inheritance](JSON_INHERITANCE.md) documentation for details.

`"copy-from"` also implies `"looks_like"` connection, so you don't have
to specify the id multiple times.


# Navigating the JSON
A lot of the JSON involves cross-references to other JSON entities.  To make it easier to navigate, we provide a script `tools/json_tools/cddatags.py` that can build a `tags` file for you.  This enables you to jump to the definition of an object given its id.

To run the script you'll need Python 3.  On Windows you'll probably need to install that, and associate `.py` files with Python.  Then open a command prompt, navigate to your CDDA folder, and run `tools\json_tools\cddatags.py`.

To use this feature your editor will need [ctags support](http://ctags.sourceforge.net/).  When that's working you should be able to easily jump to the definition of any entity.  For example, by positioning your cursor over an id and hitting the appropriate key combination.

* In Vim, this feature exists by default, and you can jump to a definition using [`^]`](http://vimdoc.sourceforge.net/htmldoc/tagsrch.html#tagsrch.txt).
* In Notepad++ go to "Plugins" -> "Plugins Admin" and enable the "TagLEET" plugin.  Then select any id and press Alt+Space to open the references window.

# Common field types
This section describes some common features of formatting values in CDDA JSON files.

## Units

Most values which represent physical quantities (length, volume, time, etc.)
are given as a string with a numerical value and an abbreviation of the unit,
separated with a space.  Generally we use SI units and try to stick to the
conventional SI abbreviations.  For example, a volume of 3 liters would be
defined as `"3 L"`.

### Time duration

A string containing one or more pairs of number and time duration unit. Number and unit, as well as each pair, can be separated by an arbitrary amount of spaces.
Available units:
- "hours", "hour", "h" - one hour
- "days", "day", "d" - one day
- "minutes", "minute", "m" - one minute
- "turns", "turn", "t" - one turn,

Examples:
- " +1 day -23 hours 50m " `(1*24*60 - 23*60 + 50 == 110 minutes)`
- "1 turn 1 minutes 9 turns" (1 minute and 10 seconds because 1 turn is 1 second)

## Translatable strings

Some json strings are extracted for translation, for example item names, descriptions, etc. The exact extraction is handled in `lang/extract_json_strings.py`. Apart from the obvious way of writing a string without translation context, the string can also have an optional translation context (and sometimes a plural form), by writing it like:

```jsonc
"name": { "ctxt": "foo", "str": "bar", "str_pl": "baz" }
```

or, if the plural form is the same as the singular form:

```jsonc
"name": { "ctxt": "foo", "str_sp": "foo" }
```

You can also add comments for translators by adding a "//~" entry like below. The
order of the entries does not matter.

```jsonc
"name": {
    "//~": "as in 'foobar'",
    "str": "bar"
}
```

Currently, only some JSON values support this syntax (see [here](/doc/TRANSLATING.md#translation) for a list of supported values and more detailed explanation).

The string extractor will extract all encountered strings from JSON for translation. But if some string should not be translated, such as text that is not normally visible to the player (names and descriptions of monster-only effects and spells), then you can write `"NO_I18N"` in the comment for translators:

```jsonc
"name": {
    "//~": "NO_I18N",
    "str": "Fake Monster-Only Spell"
},
"description": {
    "//~": "NO_I18N",
    "str": "Fake Monster-Only Spell Description"
}
```

The extractor will skip these two specified strings and only these, extracting the remaining unmarked strings from the same JSON object.

## Comments

JSON has no intrinsic support for comments.  However, by convention in CDDA
JSON, any field starting with `//` is a comment.

```jsonc
{
  "//" : "comment"
}
```

If you want multiple comments in a single object then append a number to `//`.
For example:

```jsonc
{
  "//" : "comment",
  "//1" : "another comment",
  "//2" : "yet another comment"
}
```

# File descriptions
Here's a quick summary of what each of the JSON files contain, broken down by folder. This list is not comprehensive, but covers the broad strokes.

## `data/json/`

| Filename                      | Description
|---                            |---
| `achievements.json`           | achievements
| `anatomy.json`                | a listing of player body parts - do not edit
| `ascii_arts.json`             | ascii arts for item descriptions
| `bionics.json`                | bionics, does NOT include bionic effects
| `body_parts.json`             | an expansion of anatomy.json - do not edit
| `clothing_mods.json`          | definition of clothing mods
| `conducts.json`               | conducts
| `connect_groups.json`         | definition of terrain and furniture connect groups
| `construction.json`           | definition of construction menu tasks
| `default_blacklist.json`      | a standard blacklist of joke monsters
| `doll_speech.json`            | talking doll speech messages
| `dreams.json`                 | dream text and linked mutation categories
| `disease.json`                | disease definitions
| `effects.json`                | common effects and their effects
| `emit.json`                   | smoke and gas emissions
| `flags.json`                  | common flags and their descriptions
| `furniture.json`              | furniture, and features treated like furniture
| `game_balance.json`           | various options to tweak game balance
| `gates.json`                  | gate terrain definitions
| `harvest.json`                | item drops for butchering corpses
| `health_msgs.json`            | messages displayed when the player wakes
| `item_actions.json`           | descriptions of standard item actions
| `item_category.json`          | item categories and their default sort
| `item_groups.json`            | item spawn groups
| `lab_notes.json`              | lab computer messages
| `martialarts.json`            | martial arts styles and buffs
| `materials.json`              | material types
| `monster_attacks.json`        | monster attacks
| `monster_drops.json`          | monster item drops on death
| `monster_factions.json`       | monster factions
| `monster_flags.json`          | monster flags and their descriptions
| `monstergroups.json`          | monster spawn groups
| `monstergroups_egg.json`      | monster spawn groups from eggs
| `monsters.json`               | monster descriptions, mostly zombies
| `morale_types.json`           | morale modifier messages
| `mutation_category.json`      | messages for mutation categories
| `mutation_ordering.json`      | draw order for mutation and CBM overlays in tiles mode
| `mutations.json`              | traits/mutations
| `names.json`                  | names used for NPC/player name generation
| `overmap_connections.json`    | connections for roads and tunnels in the overmap
| `overmap_terrain.json`        | overmap terrain
| `player_activities.json`      | player activities
| `professions.json`            | profession definitions
| `recipes.json`                | crafting/disassembly recipes
| `regional_map_settings.json`  | settings for the entire map generation
| `road_vehicles.json`          | vehicle spawn information for roads
| `rotatable_symbols.json`      | rotatable symbols - do not edit
| `scent_types.json`            | type of scent available
| `scores.json`                 | scores
| `skills.json`                 | skill descriptions and ID's
| `snippets.json`               | flier/poster/monster speech/dream/etc descriptions
| `species.json`                | monster species
| `speed_descripton.json`       | monster speed description
| `speech.json`                 | monster vocalizations
| `statistics.json`             | statistics and transformations used to define scores and achievements
| `start_locations.json`        | starting locations for scenarios
| `techniques.json`             | generic for items and martial arts
| `terrain.json`                | terrain types and definitions
| `test_regions.json`           | test regions
| `tips.json`                   | tips of the day
| `tool_qualities.json`         | standard tool qualities and their actions
| `traps.json`                  | standard traps
| `tutorial.json`               | messages for the tutorial (that is out of date)
| `vehicle_groups.json`         | vehicle spawn groups
| `vehicle_parts.json`          | vehicle parts, does NOT affect flag effects
| `vitamin.json`                | vitamins and their deficiencies

selected subfolders

## `data/json/items/`

See below for specifics on the various items

| Filename                       | Description
|---                             |---
| `ammo.json`                    | common base components like batteries and marbles
| `ammo_types.json`              | standard ammo types by gun
| `archery.json`                 | bows and arrows
| `armor.json`                   | armor and clothing
| `bionics.json`                 | Compact Bionic Modules (CBMs)
| `biosignatures.json`           | animal waste
| `books.json`                   | books
| `chemicals_and_resources.json` | chemical precursors
| `comestibles.json`             | food/drinks
| `containers.json`              | containers
| `crossbows.json`               | crossbows and bolts
| `fake.json`                    | fake items for bionics or mutations
| `fuel.json`                    | liquid fuels
| `grenades.json`                | grenades and throwable explosives
| `handloaded_bullets.json`      | random ammo
| `melee.json`                   | melee weapons
| `newspaper.json`               | flyers, newspapers, and survivor notes. `snippets.json` for messages
| `ranged.json`                  | guns
| `software.json`                | software for SD-cards and USB sticks
| `tool_armor.json`              | clothes and armor that can be (a)ctivated
| `toolmod.json`                 | modifications of tools
| `tools.json`                   | tools and items that can be (a)ctivated
| `vehicle_parts.json`           | components of vehicles when they aren't on the vehicle

### `data/json/items/comestibles/`

## `data/json/requirements/`

Standard components and tools for crafting (See [Recipe requirements](#recipe-requirements))

| Filename                     | Description
|---                           |---
| `ammo.json`                  | ammo components
| `cooking_components.json`    | common ingredient sets
| `cooking_requirements.json`  | cooking tools and heat sources
| `materials.json`             | thread, fabric, and other basic materials
| `toolsets.json`              | sets of tools commonly used together
| `uncraft.json`               | common results of taking stuff apart
| `vehicle.json`               | tools to work on vehicles

## `data/json/vehicles/`

Groups of vehicle definitions with self-explanatory names of files:

| Filename
|---
| `bikes.json`
| `boats.json`
| `cars.json`
| `carts.json`
| `custom_vehicles.json`
| `emergency.json`
| `farm.json`
| `helicopters.json`
| `military.json`
| `trains.json`
| `trucks.json`
| `utility.json`
| `vans_busses.json`
| `vehicles.json`

# Description and content of each JSON file
This section describes each json file and their contents. Each json has their own unique properties that are not shared with other Json files (for example 'chapters' property used in books does not apply to armor). This will make sure properties are only described and used within the context of the appropriate JSON file.


## `data/json/` JSONs

### Ascii_arts

| Identifier | Description
|---         |---
| `id`       | Unique ID. Must be one continuous word, use underscores if necessary.
| `picture`  | Array of string, each entry is a line of an ascii picture and must be at most 41 columns long. \ have to be replaced by \\\ in order to be visible.

```jsonc
  {
    "type": "ascii_art",
    "id": "cashcard",
    "picture": [
      "",
      "",
      "",
      "       <color_white>╔═══════════════════╗",
      "       <color_white>║                   ║",
      "       <color_white>║</color> <color_yellow>╔═   ╔═╔═╗╔═║ ║</color>   <color_white>║",
      "       <color_white>║</color> <color_yellow>║═ ┼ ║ ║═║╚╗║═║</color>   <color_white>║",
      "       <color_white>║</color> <color_yellow>╚═   ╚═║ ║═╝║ ║</color>   <color_white>║",
      "       <color_white>║                   ║",
      "       <color_white>║   RIVTECH TRUST   ║",
      "       <color_white>║                   ║",
      "       <color_white>║                   ║",
      "       <color_white>║ 555 993 55221 066 ║",
      "       <color_white>╚═══════════════════╝"
    ]
  }
```
For information about tools with option to export ASCII art in format ready to be pasted into `ascii_arts.json`, see [ASCII_ARTS.md](ASCII_ARTS.md).

### Snippets 

Snippets are the way for the game to store multiple instances of text, and use it on demand for different purposes: in item descriptions, NPC dialogues or in Effect on conditions

------

**Snippets may be made in two ways:**

First is when snippet contain multiple fields, mainly `text` and `id` - in this case the game would be able to save it, and call only specific one - for example, if used in item description or in lab report file

```jsonc
{
  "type": "snippet",
  "category": "test_breads",  // Category is the id of a snippet
  "text": [
    {
      "id": "bread1",                                 // Id of this exact text, in this case "flatbread"
      "name": "flatbread because i love flatbread",   // Name of a snippet, not actually used anywhere except to describe the snippet
      "text": "flatbread",                            // Text, that would be used if this snippet category is called
      "effect_on_examine": [ "effect_on_condition" ], // Examining of this snippet will call effect_on_condition
      "weight": 10                                    // Weight of this specific snippet, in case of this group the probability to get this one would be 10/13 ~= 76%
    },
    { "id": "bread2", "text": "yeast bread" },
    { "id": "bread3", "text": "cornbread" },
    { "id": "bread4", "text": "fruit bread" }
  ]
}
```

Second is when snippet contain plain text with no ids - it is used, when the snippet can be generated on the fly, and the game don't need to memorize which one it should be - like in dialogue, for example (we don't need the game to remember what <swear> character used when talked to you)

```jsonc
{
  "type": "snippet",
  "category": "test_breads", 
  "text": [
    "flatbread",
    "yeast bread",
    "cornbread",
    "fruit bread"
  ]
}
```

------

**There is also a multiple ways to use said snippet:**

Items can utilize it using `snippet_category`, in this case the whole description of an item would be replaced with randomly picked snipped out of category:

```jsonc
"snippet_category": "test_breads",
```

Alternatively, the `snippet_category` may itself contain a range of descriptions, avoiding making a new category:

```jsonc
"snippet_category": [
  { "id": "bread1", "text": "flatbread" },
  { "id": "bread2", "text": "yeast bread" },
  { "id": "bread3", "text": "cornbread" },
  { "id": "bread4", "text": "fruit bread" }
]
```
note, that using `id` is mandatory in every way, if you don't want to make the game change the description of an item every time they do anything in the game

------

Both dialogues and snippets may reference the snippets right inside themselves - to differentiate the snippets, that are used in this way, their id contain `<>` in the name, like `<test_breads>`

```jsonc
"dynamic_line": "I don't even <swear> know anymore.  I have no <swear> idea what is going on.  I'm just doing what I can to stay alive.  The world ended and I bungled along not dying, until I met you."
```

```jsonc
{
  "type": "snippet",
  "category": "<music_description>",
  "text": [ "some <musicgenre>.", "some <musicgenre>. The <musicdesc_part> is <musicdesc_evaluation>." ]
},
```

------

Item descriptions also capable to use snippet system, but to use them, `expand_snippets` should be `true`, otherwise snippet won't be used

```jsonc
{
  "id": "nice_mug",
  "type": "GENERIC",
  "name": { "str": "complimentary mug" },
  "description": "A ceramic mug.  It says \"Nice job, <name_g>!\"",
  "expand_snippets": true,
  // ...
}
```

Same works with variants 


```jsonc
{
  "id": "mean_mug",
  "type": "GENERIC",
  "name": { "str": "insulting mug" },
  "description": "A ceramic mug.",
  "variant_type": "generic",
  "variants": [
    {
      "id": "fuck_you",
      "name": { "str": "insulting mug" },
      "description": "It says \"<fuck_you>, <name_b>!\"",
      "append": true,
      "expand_snippets": true,
      "weight": 1
    },
    {
      "id": "worst_dad",
      "name": { "str": "bad dad mug" },
      "description": "It says \"Worlds Worst Dad\"",
      "append": true,
      "weight": 1
    }
  ],
  "material": [ "ceramic" ],
  "weight": "375 g",
  "volume": "375 ml"
}
```

Using `expand_snippets` required only where snippets are used - if item do not uses snippet, but variant does, then only variant require to have `"expand_snippets":true`
Using `expand_snippets` on item itself will work as all variants have `"expand_snippets":true`, but variants without any snippet would be effectively removed

```jsonc
{
  "id": "mean_mug",
  "type": "GENERIC",
  "name": { "str": "insulting mug" },
  "description": "A ceramic mug.",
  "expand_snippets": true,
  "variant_type": "generic",
  "variants": [
    {
      "id": "fuck_you",
      "name": { "str": "insulting mug" },
      "description": "It says \"<fuck_you>, <name_b>!\"",
      "append": true,
      "weight": 1
    },
    {
      "id": "worst_dad",
      "name": { "str": "bad mug" },
      "description": "This mug never appears, because it doesnn't have any snippets",
      "append": true,
      "weight": 1
    }
  ],
  "material": [ "ceramic" ],
  "weight": "375 g",
  "volume": "375 ml"
}
```

------

Item groups can specify the description of the item that is spawned:

```jsonc
{
  "type": "item_group",
  "id": "test_itemgroup",
  "//": "it spawns `child's drawing` item with `mutant_kid_boss_5` description",
  "entries": [
    { "item": "note_mutant_alpha_boss", "snippets": "mutant_kid_boss_5" },
  ]
}
```

Without specifying, the random snippet would be used

------

Snippets can also be used in EoC, see [EFFECT_ON_CONDITION.md#u_message](EFFECT_ON_CONDITION.md#u_messagenpc_message)

------

Items, that uses effect on condition action to reveal a snippet, may utilize `conditional_names` syntax to change the name of an item, depending on it's description

`log_psych` is the category of snippet, `dream_1` is the id of a snippet, and `name` is the name of new item

```jsonc
{
  "type": "GENERIC",
  "id": "psych_file",
  "name": { "str": "lab report (psychology)", "str_pl": "lab reports (psychology)" },
  "conditional_names": [
    { "type": "SNIPPET_ID", "condition": "log_psych", "value": "dream_1", "name": { "str_sp": "Session S-3397-5" } },
    { "type": "SNIPPET_ID", "condition": "log_psych", "value": "dream_2", "name": { "str_sp": "Session T-1215-4" } },
    { "type": "SNIPPET_ID", "condition": "log_psych", "value": "dream_3", "name": { "str_sp": "Scrawled note" } }
  ],
  "description": "A folder full of what appear to be transcripts of confidential psychotherapy sessions.  Most of it is rather trivial, but certain passages catch your eye…",
  "copy-from": "file",
  "use_action": {
    "type": "effect_on_conditions",
    "description": "Activate to read the file",
    "effect_on_conditions": [
      {
        "id": "EOC_LAB_FILE_PSY",
        "effect": [ { "u_message": "log_psych", "snippet": true, "same_snippet": true, "popup": true } ]
      }
    ]
  }
}
```

Once the item would be activated, the description would be replaced with one of `log_psych` texts, and the `lab report (psychology)` name would be replaced with one of `conditional_names`

------

Snippets also support the color codes

```jsonc
"<color_yellow_red>Biohazard</color>",
```

To use literal `<` and `>` characters in a snippet, escape them with `<lt>` and
`<gt>`. For example, `<lt>swear<gt>` expands into `<swear>` rather than any
snippet in the `<swear>` category.

### Addiction types

Addictions are defined in JSON using `"addiction_type"`:

```jsonc
{
  "type": "addiction_type",
  "id": "caffeine",
  "name": "Caffeine Withdrawal",
  "type_name": "caffeine",
  "description": "Strength - 1;   Slight sluggishness;   Occasional cravings",
  "craving_morale": "morale_craving_caffeine",
  "effect_on_condition": "EOC_CAFFEINE_ADDICTION"
}
```

| Field                   | Description
|---                      |---
| `"name"`                | The name of the addiction's effect as it appears in the player's status
| `"type_name"`           | The name of the addiction's source
| `"description"`         | Description of the addiction's effects as it appears in the player's status
| `"craving_morale"`      | ID of the `morale_type` penalty
| `"effect_on_condition"` | ID of the `effect_on_condition` (can also be an inline EOC) which activates on each `update_body` (aka every turn)
| `"builtin"`             | *(for legacy addiction code)* Name of a hardcoded function to process the addiction's effect. For new addictions, use `"effect_on_condition"` instead.

Each turn, the player's addictions are processed using either the given `effect_on_condition` or `builtin`. These effects usually have a rng condition so that the effect isn't applied constantly every turn. Ex:

```jsonc
  {
    "type": "effect_on_condition", 
    "id": "EOC_MARLOSS_R_ADDICTION",
    "condition": { "math": [ "rand(800) <= addiction_rational(800, 20, u_addiction_intensity('marloss_r'))" ] },
    "effect": [
      { "u_add_morale": "morale_craving_marloss", "bonus": -5, "max_bonus": -30 },
      { "u_message": "You daydream about luscious pink berries as big as your fist.", "type": "info" },
      { "if": { "math": [ "u_val('focus') > 40" ] }, "then": { "math": [ "u_val('focus')", "--" ] } }
    ]
  },
```

Current hardcoded builtins:
- `nicotine_effect`
- `alcohol_effect`
- `diazepam_effect`
- `opiate_effect`
- `amphetamine_effect`
- `cocaine_effect`
- `crack_effect`


### Body Graphs

Body graphs are displayed in the body status menu, accessible by pressing `s` on the player's @-screen.
These are interactive graphs that highlight different body parts or sub body parts.

```jsonc
{
  "type": "body_graph",
  "id": "head",
  "parent_bodypart": "head",
  "fill_sym": "#",
  "fill_color": "white",
  "rows": [
    "             7777777777777              ",
    "          7777777777777777777           ",
    "         777777777777777777777          ",
    "        ######66666666666######         ",
    "        ####666666666666666####         ",
    "        ####666666666666666####         ",
    "      9 #####6666666666666##### 0       ",
    "      99#####111###4###222#####00       ",
    "      99####11111#444#22222####00       ",
    "      99##5555555544455555555##00       ",
    "       9##5555555544455555555##0        ",
    "        ##5555555444445555555##         ",
    "         ###555533333335555###          ",
    "          #####333333333#####           ",
    "           #######333#######            ",
    "            ###############             ",
    "            8 ########### 8             ",
    "         8888888 ##### 8888888          ",
    "       88888888888   88888888888        ",
    "           88888888888888888            "
  ],
  "parts": {
    "1": { "sub_body_parts": [ "eyes_left" ], "select_color": "red", "nested_graph": "eyes" },
    "2": { "sub_body_parts": [ "eyes_right" ], "select_color": "red", "nested_graph": "eyes" },
    "3": { "sub_body_parts": [ "mouth_lips" ], "select_color": "red", "nested_graph": "mouth" },
    "4": { "sub_body_parts": [ "mouth_nose" ], "select_color": "red", "nested_graph": "mouth" },
    "5": { "sub_body_parts": [ "mouth_cheeks" ], "select_color": "red", "nested_graph": "mouth" },
    "6": { "sub_body_parts": [ "head_forehead" ], "select_color": "red" },
    "7": { "sub_body_parts": [ "head_crown" ], "select_color": "red" },
    "8": { "sub_body_parts": [ "head_throat", "head_nape" ], "select_color": "red" },
    "9": { "sub_body_parts": [ "head_ear_r" ], "select_color": "red" },
    "0": { "sub_body_parts": [ "head_ear_l" ], "select_color": "red" }
  }
}
```

| Field             | description
|---                |---
| `type`            | Always `body_graph`.
| `id`              | String uniquely identifying this graph.
| `parent_bodypart` | (_optional_) ID of the parent body part of this graph, if any. Only used to display the current body part as the window's subtitle.
| `fill_sym`        | (_optional_) Specifies a character to fill all sections of the graph when viewing in-game.
| `fill_color`      | (_optional_) Specifies a color to use for unselected sections of the graph when viewing in-game.
| `rows`            | Array of strings that form the graph. The symbols used for each fragment may correspond to an entry in `parts`, which form the sections of the graph. Empty spaces (` `) are ignored for the purposes of filling.
| `mirror`          | (_optional_) Can be specified instead of `rows`. This takes a string ID referring to a different body_graph, which will be flipped horizontally and used as the rows in this graph (ex: `hand_l` mirrors `hand_r`).
| `parts`           | A list of symbols present in the graph that correspond to specific body parts or sub body parts.

The resolution limit for the `rows` field is 40x20, in order to maintain compatibility with 80x24 terminals.

#### Graph Parts

The `parts` field can be used to define the interaction with different sections of the graph. Each part should
reference at least one body part or sub body part.

| Field            | description
|---               |---
| `body_parts`     | An array of `body_part` IDs that are represented by this graph section.
| `sub_body_parts` | An array of `sub_body_part` IDs that are represented by this graph section.
| `sym`            | (_optional_) A symbol to override fragments belonging to this section.
| `select_color`   | (_optional_) Color to use when selecting this section.
| `nested_graph`   | (_optional_) ID of another body_graph. When the player selects and confirms this section, the UI switches to the given nested graph.


### Body_parts

| `Identifier`           | Description
|---                     |---
| `id`                   | (_mandatory_) Unique ID. Must be one continuous word, use underscores if necessary.
| `name`                 | (_mandatory_) In-game name displayed.
| `limb_type`            | (_mandatory_) Type of limb, as defined by `bodypart.h`. Certain functions will check only a given bodypart type for their purposes. Currently implemented types are: `head, torso, sensor, mouth, arm, hand, leg, foot, wing, tail, other`.
| `limb_types`           | (_optional_) (Can be used instead of `limb_type`) Weighted list of limb types this body part can emulate. The weights are modifiers that determine how good this body part is at acting like the given limb type. (Ex: `[ [ "foot", 1.0 ], [ "hand", 0.15 ] ]`)
| `secondary_types`      | (_optional_) List of secondary limb types for the bodypart, to include it in relevant calculations.
| `accusative`           | (_mandatory_) Accusative form for this bodypart.
| `heading`              | (_mandatory_) How it's displayed in headings.
| `heading_multiple`     | (_mandatory_) Plural form of heading.  Gets used if opposite bodyparts have the same encumbrance data, health and temperature.
| `encumbrance_text`     | (_mandatory_) Message printed when the limb reaches 40 encumbrance.
| `encumbrance_threshold`| (_optional_) Encumbrance value where the limb's scores start scaling based on encumbrance. Default 0, meaning scaling from the first point of encumbrance.
| `encumbrance_limit`    | (_optional_) When encumbrance reaches or surpasses this value the limb stops contributing its scores. Default 100.
| `grabbing_effect`      | (_optional_) Effect id of the `GRAB_FILTER` effect to apply to a monster grabbing this limb, necessary for adequate grab removal (see `MONSTER_SPECIAL_ATTACKS.md` for the grab logic). 
| `hp_bar_ui_text`       | (_mandatory_) How it's displayed next to the hp bar in the panel.
| `main_part`            | (_mandatory_) What is the main part this one is attached to. (If this is a main part it's attached to itself)
| `connected_to`         | (_mandatory_ if main_part is itself) What is the next part this one is attached to towards the "root" bodypart (the root bodypart should be connected to itself).  Each anatomy should have a unique root bodypart, usually the head.
| `base_hp`              | (_mandatory_) The amount of hp this part has before any modification.
| `opposite_part`        | (_mandatory_) What is the opposite part of this one in case of a pair.
| `hit_size`             | (_mandatory_) Size of the body part for (melee) attack targeting.  Monster special attacks are capable of targeting set bodypart hitsizes (see `hitsize_min/max` in `MONSTERS.md`).  The character's whole `hitsize sum / base hitsize sum` acts as a denominator of dodge rolls, meaning extra limbs passively make it harder to dodge.
| `hit_difficulty`       | (_mandatory_) How hard is it to hit a given body part, assuming "owner" is hit. Higher number means good hits will veer towards this part, lower means this part is unlikely to be hit by inaccurate attacks. Formula is `chance *= pow(hit_roll, hit_difficulty)`
| `drench_capacity`      | (_mandatory_) How wet this part can get before being 100% drenched. 0 makes the limb waterproof, morale checks for absolute wetness while other effects for wetness percentage - making a high `drench_capacity` prevent the penalties longer.
| `drench_increment`     | (_optional_) Units of "wetness" applied each time the limb gets drenched. Default 2, ignored by diving underwater.
| `drying_rate`          | (_optional float_) Divisor on the time needed to dry a given amount of wetness from a bodypart, modified by clothing.  Final drying rate depends on breathability, weather, and `drying_capacity` of the bodypart.
| `wet_morale`           | (_optional_) Mood bonus/malus when the limb gets wet, representing the morale effect at 100% limb saturation. Modified by worn clothing and ambient temperature.
| `hot_morale_mod`       | (_optional_) Mood effect of being too hot on this part. (default: `0`)
| `cold_morale_mod`      | (_optional_) Mood effect of being too cold on this part. (default: `0`)
| `squeamish_penalty`    | (_optional_) Mood effect of wearing filthy clothing on this part. (default: `0`)
| `fire_warmth_bonus`    | (_optional_) How effectively you can warm yourself at a fire with this part. (default: `0`)
| `temp_mod`             | (_optional array_) Intrinsic temperature modifier of the bodypart.  The first value (in the same "temperature unit" as mutations' `bodytemp_modifier`) is always applied, the second value is applied on top when the bodypart isn't overheated.
| `env_protection`       | (_optional_) Innate environmental protection of this part. (default: `0`)
| `stat_hp_mods`         | (_optional_) Values modifying hp_max of this part following this formula: `hp_max += int_mod*int_max + dex_mod*dex_max + str_mod*str_max + per_mod*per_max + health_mod*get_healthy()` with X_max being the unmodified value of the X stat and get_healthy() being the hidden health stat of the character.
| `heal_bonus`           | (_optional_) Innate amount of HP the bodypart heals every successful healing roll. See the `ALWAYS_HEAL` and `HEAL_OVERRIDE` flags.
| `mend_rate`            | (_optional_) Innate mending rate of the limb, should it get broken. Default `1.0`, used as a multiplier on the healing factor after other factors are calculated.
| `health_limit`         | (_optional_) Amount of limb HP necessary for the limb to provide its melee `techniques` and `conditional_flags`.  Defaults to 1, meaning broken limbs don't contribute.
| `ugliness`             | (_optional_) Ugliness of the part that can be covered up, negatives confer beauty bonuses.
| `ugliness_mandatory`   | (_optional_) Inherent ugliness that can't be covered up by armor.
| `bionic_slots`         | (_optional_) How many bionic slots does this part have.
| `is_limb`              | (_optional_) Is this bodypart a limb and capable of breaking. (default: `false`)
| `smash_message`        | (_optional_) The message displayed when using that part to smash something.
| `smash_efficiency`     | (_optional_) Modifier applied to your smashing strength when using this part to smash terrain or furniture unarmed. (default: `0.5`)
| `flags`                | (_optional_) List of bodypart flags.  These are considered character flags, similar to bionic/trait/effect flags.
| `conditional_flags`    | (_optional_) List of character flags this limb provides as long as it's above `health_limit` HP.
| `techniques`           | (_optional_) List of melee techniques granted by this limb as long as it's above its `health_limit` HP.  The chance for the technique to be included in each attack's tech list is dependent on limb encumbrance. ( `!x_in_y(current encumbrance / technique_encumbrance_limit`)
| `technique_encumbrance_limit` | (_optional_) Level of encumbrance that disables the given techniques for this limb completely, lower encumbrance still reduces the chances of the technique being chosen (see above).
| `limb_scores`          | (_optional_) List of arrays defining limb scores. Each array contains 2 mandatory values and 1 optional value. Value 1 is a reference to a `limb_score` id. Value 2 is a float defining the limb score's value. (optional) Value 3 is a float defining the limb score's maximum value (mostly just used for manipulator score).
| `effects_on_hit`       | (_optional_) Array of effects that can apply whenever the limb is damaged.  For details see below.
| `unarmed_damage`       | (_optional_) An array of objects, each detailing the amount of unarmed damage the bodypart contributes to unarmed attacks and their armor penetration. The unarmed damages of each limb are summed and added to the base unarmed damage. Should be used for limbs the character is expected to *always* attack with, for special attacks use a dedicated technique.
| `similar_bodyparts`           | (_optional_) Array of (sub)bodypart ids.  Armor coverage is automatically extended to these bodyparts - Ex: any armor covering the bodypart `arm_l` will cover `arm_bear_l` with the same coverage in the below example.  Sublocations will need a similar definition as well to ensure correct function.  Currently bodyparts can only point at bodyparts and sub-bodyparts at sub-bodyparts. 
  Only one step of substitution occurs ( Ie. an armor covering `arm_l` will cover `arm_bear_l`, but not any similar bps defined in `arm_bear_l` ).
  Any coverage of a similar sbp will imply coverage of the substitute subpart's parent for the sub-part in question:  Armor covering the elbows will cover similar elbows on other limbs, but not any of the other locations.
| `armor`                | (_optional_) An object containing damage resistance values. Ex: `"armor": { "bash": 2, "cut": 1 }`. See [Part Resistance](#part-resistance) for details.

```jsonc
{
  "id": "arm_l",
  "type": "body_part",
  "//": "See comments in `body_part_struct::load` of bodypart.cpp about why xxx and xxx_multiple are not inside a single translation object.",
  "name": "left arm",
  "name_multiple": "arms",
  "accusative": { "ctxt": "bodypart_accusative", "str": "left arm" },
  "accusative_multiple": { "ctxt": "bodypart_accusative", "str": "arms" },
  "heading": "L. Arm",
  "heading_multiple": "Arms",
  "encumbrance_text": "Melee and ranged combat is hampered.",
  "hp_bar_ui_text": "L ARM",
  "main_part": "arm_l",
  "connected_to": "torso",
  "opposite_part": "arm_r",
  "hit_size": 9,
  "hit_difficulty": 0.95,
  "limb_type": "arm",
  "limb_scores": [ [ "manip", 0.1, 0.2 ], [ "lift", 0.5 ], [ "block", 1.0 ], [ "swim", 0.1 ] ],
  "armor": { "electric": 2, "stab": 1 },
  "side": "left",
  "legacy_id": "ARM_L",
  "hot_morale_mod": 0.5,
  "cold_morale_mod": 0.5,
  "fire_warmth_bonus": 600,
  "squeamish_penalty": 5,
  "is_limb": true,
  "base_hp": 60,
  "drench_capacity": 10,
  "smash_message": "You elbow-smash the %s.",
  "bionic_slots": 20,
  "similar_bodyparts": [ "arm_bear_l" ],
  "sub_parts": [ "arm_shoulder_l", "arm_upper_l", "arm_elbow_l", "arm_lower_l" ]
}
```

# On-hit Effects

An array of effects to add whenever the limb in question takes damage. Variables for each entry:

| `Identifier`           | Description
|---                     |---
| `id`                   | (_mandatory_) ID of the effect to apply.
| `global`               | (_optional_) Bool, if true the effect won't apply to the bodypart but to the whole character. Default false.
| `dmg_type`             | (_optional_) String id of the damage type eligible to apply the effect. Defaults to all damage.
| `dmg_threshold`        | (_optional_) Integer, amount of damage to trigger the effect. For main parts used as percent of limb max health, for minor parts as absolute damage amount. Default 1.
| `dmg_scale_increment`  | (_optional_) Float, steps of scaling based on damage above `damage_threshold`. Default 1.
| `chance`               | (_optional_) Integer, percent chance to trigger the effect. Default 100.
| `chance_dmg_scaling`   | (_optional_) Float, chance is increased by this value for every `dmg_scale_increment` above `dmg_threshold`. Default 0.
| `intensity`            | (_optional_) Integer, intensity of effect to apply. Default 1.
| `intensity_dmg_scaling`| (_optional_) Float, intensity is increased by this value for every `dmg_scale_increment` above `dmg_threshold`. Default 0.
| `max_intensity`        | (_optional_) Integer, max intensity the limb can gain as part of the onhit effect - other sources of effects like spells or explicit special attack effects can still apply higher intensities. Default INT_MAX.
| `duration`             | (_optional_) Integer, duration of effect to apply in seconds. Default 1.
| `duration_dmg_scaling` | (_optional_) Float, duration is increased by this value for every `dmg_scale_increment` above `dmg_threshold`. Default 0.
| `max_duration`         | (_optional_) Integer, max seconds duration the limb can gain as part of the onhit effect - see `max_intensity`. Default INT_MAX.


```jsonc
{
"effects_on_hit": [
    {
      "id": "staggered",
      "dmg_type": "bash",
      "dmg_threshold": 5,
      "dmg_scale_increment": 5,
      "chance": 10,
      "chance_dmg_scaling": 10,
      "duration": 5,
      "duration_dmg_scaling": 2,
      "max_duration": 15
    },
    {
      "id": "downed",
      "global": true,
      "dmg_threshold": 20,
      "dmg_scale_increment": 10,
      "chance": 5,
      "chance_dmg_scaling": 20,
      "duration": 2,
      "duration_dmg_scaling": 0.5
    }
  ]
}
```

### Limb scores
Limb scores act as the basis of calculating the effect of limb encumbrance and damage on the abilities of characters.  Most limb scores affect the character via `character_modifiers`, for further information see there. They are defined using the `"limb_score"` type:

```jsonc
{
  "type": "limb_score",
  "id": "lift",
  "name": "Lifting",
  "affected_by_wounds": true,
  "affected_by_encumb": false
}
```
- `"type"`: Always "limb_score".
- `"id"`: Identifies this limb score
- `"name"`: Mandatory. Defines a translatable name for this limb score that will be displayed in the UI.
- `"affected_by_wounds"`: Optional, defaults to true. Determines whether this limb score is affected by the character's limb health. Lower limb health => lower score.
- `"affected_by_encumb"`: Optional, defaults to true. Determines whether this limb score is affected by the character's limb encumbrance. Higher encumbrance => lower score.

Here are the currently defined limb scores:

| Limb score id          | Description
|------                  |------
| `consume_liquid`       | Speed modifier when consuming liquids.
| `consume_solid`        | Speed multiplier when consuming solids.
| `manipulator_score`    | Modifies aim speed, reload speed, thrown attack speed, ranged dispersion and crafting speed.  The manipulator scores of each limb type are aggregated and the best limb group is chosen for checks.
| `manipulator_max`      | The upper limit of manipulator score the limb can contribute to.
| `lifting_score`        | Modifies melee attack stamina and move cost, as well as a number of STR checks.  A sum above 0.5 qualifies for wielding two-handed weapons and similar checks.  Arms below 0.1 lift score don't count as working for the purposes of melee combat.
| `blocking_score`       | The blocking limb is chosen by a roll weighted by eligible limbs' block score, and blocking efficiency is multiplied by the target limb's score.
| `breathing_score`      | Modifies stamina recovery speed and shout volume.
| `vision_score`         | Modifies ranged dispersion, ranged and melee weakpoint hit chances.
| `nightvision_score`    | Modifies night vision range (multiplier on the calculated range).
| `reaction_score`       | Modifies dodge chance, block chance, melee weakpoint hit chances.
| `balance_score`        | Modifies thrown attack speed, movement cost and melee attack rolls.
| `footing_score`        | Modifies movement cost.
| `movement_speed_score` | Modifies movement cost.
| `swim_score`           | Modifies swim speed.

These limb scores are referenced in `"body_part"` within the `"limb_scores"` array. (See [body parts](#body_parts)).

### Character Modifiers

Character modifiers define how effective different behaviours are for actions the character takes. These are usually derived from a limb score.

```jsonc
{
  "type": "character_mod",
  "id": "ranged_dispersion_manip_mod",
  "description": "Hand dispersion when using ranged attacks",
  "mod_type": "+",
  "value": { "limb_score": "manip", "max": 1000.0, "nominator": 22.8, "subtract": 22.8 }
},
{
  "type": "character_mod",
  "id": "slip_prevent_mod",
  "description": "Slip prevention modifier",
  "mod_type": "x",
  "value": {
    "limb_score": [ [ "grip", 3.0 ], [ "lift", 2.0 ], "footing" ],
    "override_encumb": true,
    "limb_score_op": "+",
    "denominator": 6.0
  }
},
{
  "type": "character_mod",
  "id": "stamina_move_cost_mod",
  "description": "Stamina move cost modifier",
  "mod_type": "x",
  "value": { "builtin": "stamina_move_cost_modifier" }
}
```

| Field         | Description
|------         |------------
| `type`        | Always "character_mod".
| `id`          | Unique identifier for this character modifier.
| `description` | Translatable text that describes the function of this modifier, which will be displayed in the UI.
| `mod_type`    | Describes how this modifier is applied. Can be `"+"` (added), `"x"` (multiplied), or `""` (unspecified).
| `value`       | Object that describes how this modifier is calculated.

#### Character Modifiers - Value

| Field             | Description
|------             |------------
| `limb_score`      | Refers to a `limb_score` id, or an array of `limb_score` id's (can be a weighted list). These are the limb scores from which this modifier is derived.  For additive calculations ( `limb_score_op: "+"`) the score is multiplied by the weight, for multiplicative calculation (`limb_score_op: "x"`) it is raised to the weight's power.
| `limb_score_op`   | (_optional_) Operation (add `+` or multiply `x`) to apply when multiple limb scores are defined. Ex: `x` => `score1 x score2 x score3 ...`. (Defaults to `x`)
| `limb_type`       | (_optional_) Refers to a `limb_type` as defined in [`body_part`](#body_parts). If present, only limb scores from body parts with that `limb_type` are used.
| `override_encumb` | (_optional_) Boolean (true/false). If specified, this forces the limb score to be affected/unaffected by limb encumbrance if true/false. (Overrides `affected_by_encumb` in `limb_score`)
| `override_wounds` | (_optional_) Boolean (true/false). If specified, this forces the limb score to be affected/unaffected by limb health if true/false.(Overrides `affected_by_wounds` in `limb_score`)
| `min`             | (_optional_) Defines a minimum value for this modifier. Generally only used for "bonus" multipliers that provide a benefit. Should not be used together with `max`.
| `max`             | (_optional_) Defines a maximum value for this modifier. Generally used for "cost" multipliers that provide a malus. Should not be used together with `min`. This value can be defined as a decimal or as the special value `"max_move_cost"`.
| `nominator`       | (_optional_) Causes the limb score to divide the specified value, such that `nominator / ( limb_score * denominator )`.
| `denominator`     | (_optional_) Divides the limb score (or the nominator, if specified) by the specified value, such that `limb_score / denominator`.
| `subtract`        | (_optional_) Defines a value to subtract from the resulting modifier, such that `mod - subtract`.
| `builtin`         | Instead of a limb score, the `value` object can define a built-in function to handle the calculation of the modifier.

The modifier is normally derived from a limb score, which is modified in a sequence of operations. Here are some possible outcomes for different combinations of specified fields in `value`:
```cpp
// Only one "limb_score" specified:
mod = limb_score;
// 3 score id's in "limb_score" array (with "x" operation):
mod = limb_score1 * limb_score2 * limb_score3;
// "max" specified:
mod = min( max, limb_score );
// "min" specified:
mod = max( min, limb_score );
// Both "max" and "nominator" specified:
mod = min( max, nominator / limb_score );
// "max", "nominator", and "subtract" specified:
mod = min( max, ( nominator / limb_score ) - subtract );
// "max", "denominator", and "subtract" specified:
mod = min( max, ( limb_score / denominator ) - subtract );
```


### Bionics

| Identifier                   | Description
|---                           |---
| `id`                         | Unique ID. Must be one continuous word, use underscores if necessary.
| `name`                       | In-game name displayed.
| `description`                | In-game description.
| `act_cost`                   | (_optional_) How many kJ it costs to activate the bionic.  Strings can be used "1 kJ"/"1000 J"/"1000000 mJ" (default: `0`)
| `deact_cost`                 | (_optional_) How many kJ it costs to deactivate the bionic.  Strings can be used "1 kJ"/"1000 J"/"1000000 mJ" (default: `0`)
| `react_cost`                 | (_optional_) How many kJ it costs over time to keep this bionic active, does nothing without a non-zero "time".  Strings can be used "1 kJ"/"1000 J"/"1000000 mJ" (default: `0`)
| `trigger_cost`               | (_optional_) How many kJ it costs to trigger special effects for this bionic. This can be a reaction to specific conditions or an action taken while the bionic is active.  Strings can be used "1 kJ"/"1000 J"/"1000000 mJ" (default: `0`)
| `time`                       | (_optional_) How long, when activated, between drawing cost. If 0, it draws power once. (default: `0`)
| `upgraded_bionic`            | (_optional_) Bionic that can be upgraded by installing this one.
| `available_upgrades`         | (_optional_) Upgrades available for this bionic, i.e. the list of bionics having this one referenced by `upgraded_bionic`.
| `encumbrance`                | (_optional_) A list of body parts and how much this bionic encumber them.
| `known_ma_styles`            | (_optional_) A list of martial art styles that are known to the wearer when the bionic is activated
| `canceled_mutations`         | (_optional_) A list of mutations/traits that are removed when this bionic is installed (e.g. because it replaces the fault biological part).
| `mutation_conflicts`         | (_optional_) A list of mutations that prevent this bionic from being installed.
| `included_bionics`           | (_optional_) Additional bionics that are installed automatically when this bionic is installed. This can be used to install several bionics from one CBM item, which is useful as each of those can be activated independently.
| `included`                   | (_optional_) Whether this bionic is included with another. If true this bionic does not require a CBM item to be defined. (default: `false`)
| `env_protec`                 | (_optional_) How much environmental protection does this bionic provide on the specified body parts.
| `protec`                     | (_optional_) An array of resistance values that determines the types of protection this bionic provides on the specified body parts.
| `occupied_bodyparts`         | (_optional_) A list of body parts occupied by this bionic, and the number of bionic slots it take on those parts.
| `capacity`                   | (_optional_) Amount of power storage added by this bionic.  Strings can be used "1 kJ"/"1000 J"/"1000000 mJ" (default: `0`)
| `fuel_options`               | (_optional_) A list of materials that this bionic can use to produce bionic power.
| `is_remote_fueled`           | (_optional_) If true this bionic allows you to plug your power banks to an external power source (solar backpack, UPS, vehicle etc) via a cable. (default: `false`)
| `fuel_capacity`              | (_optional_) Volume of fuel this bionic can store.
| `fuel_efficiency`            | (_optional_) Fraction of fuel energy converted into power. (default: `0`)
| `passive_fuel_efficiency`    | (_optional_) Fraction of fuel energy passively converted into power. Useful for CBM using PERPETUAL fuel like `muscle`, `wind` or `sun_light`. (default: `0`)
| `exothermic_power_gen`       | (_optional_) If true this bionic emits heat when producing power. (default: `false`)
| `coverage_power_gen_penalty` | (_optional_) Fraction of coverage diminishing fuel_efficiency. Float between 0.0 and 1.0. (default: `nullopt`)
| `power_gen_emission`         | (_optional_) `emit_id` of the field emitted by this bionic when it produces energy. Emit_ids are defined in `emit.json`.
| `stat_bonus`                 | (_optional_) List of passive stat bonus. Stat are designated as follow: "DEX", "INT", "STR", "PER".
| `activated_eocs`             | (_optional_) List of effect_on_conditions that attempt to activate when this CBM is successfully activated.
| `processed_eocs`             | (_optional_) List of effect_on_conditions that attempt to activate each turn this CBM is active.
| `deactivated_eocs`           | (_optional_) List of effect_on_conditions that attempt to activate when this CBM is successfully deactivated.
| `enchantments`               | (_optional_) List of enchantments applied by this CBM (see MAGIC.md for instructions on enchantment. NB: enchantments are not necessarily magic.) Values can either be the enchantment's id or an inline definition of the enchantment.
| `learned_spells`             | (_optional_) Map of {spell:level} you gain when installing this CBM, and lose when you uninstall this CBM. Spell classes are automatically gained.
| `learned_proficiencies`      | (_optional_) Array of proficiency ids you gain when installing this CBM, and lose when uninstalling
| `installation_requirement`   | (_optional_) Requirement id pointing to a requirement defining the tools and components necessary to install this CBM.
| `dupes_allowed`              | (_optional_) Boolean to determine if multiple copies of this bionic can be installed.  Defaults to false.
| `cant_remove_reason`         | (_optional_) String message to be displayed as the reason it can't be uninstalled.  Having any value other than `""` as this will prevent unistalling the bionic. Formatting includes two `%s` for example: `The Telescopic Lenses are part of %1$s eyes now. Removing them would leave %2$s blind.`  (default: `""`)
| `social_modifiers`           | (_optional_) Json object with optional members: persuade, lie, and intimidate which add or subtract that amount from those types of social checks
| `activated_on_install`       | (_optional_) Auto-activates this bionic when installed.
| `required_bionic`            | (_optional_) Bionic which is required to install this bionic, and which cannot be uninstalled if this bionic is installed
| `give_mut_on_removal`        | (_optional_) A list of mutations/traits that are added when this bionic is uninstalled (for example a "blind" mutation if you removed bionic eyes after installation).
| `passive_pseudo_items`       | (_optional_) This fake item is added into player's inventory, when bionic is installed.
| `installable_weapon_flags`   | (_optional_) Items with this flag can be installed inside this bionic
| `fake_weapon`                | (_optional_) Activation of this bionic spawn an irremovable weapon in your hands. Require `BIONIC_TOGGLED` flag
| `active_flags`               | (_optional_) Activation of this bionic applies this character flag
| `auto_deactivates`           | (_optional_) Activation of this bionic automatically turn of another bionic, if character has one
| `toggled_pseudo_items`       | (_optional_) Activation of this bionic spawn an irremovable tool in your hands.  Require `BIONIC_TOGGLED` flag
| `spell_on_activation`        | (_optional_) Activation of this bionic allow you to cast a spell
| `activated_close_ui`         | (_optional_) Activation of this bionic closes the bionic menu

```jsonc
{
    "id"           : "bio_batteries",
    "name"         : "Battery System",
    "active"       : false,
    "act_cost"     : 0,
    "time"         : 1,
    "fuel_efficiency": 1,
    "stat_bonus": [ [ "INT", 2 ], [ "STR", 2 ] ],
    "fuel_options": [ "battery" ],
    "fuel_capacity": 500,
    "encumbrance"  : [ [ "torso", 10 ], [ "arm_l", 10 ], [ "arm_r", 10 ], [ "leg_l", 10 ], [ "leg_r", 10 ], [ "foot_l", 10 ], [ "foot_r", 10 ] ],
    "description"  : "You have a battery draining attachment, and thus can make use of the energy contained in normal, everyday batteries. Use 'E' to consume batteries.",
    "canceled_mutations": ["HYPEROPIC"],
    "mutation_conflicts": [ "HUGE" ],
    "installation_requirement": "sewing_standard",
    "included_bionics": ["bio_blindfold"]
},
{
    "id": "bio_purifier",
    "type": "bionic",
    "name": "Air Filtration System",
    "description": "Surgically implanted in your trachea is an advanced filtration system.  If toxins, or airborne diseases find their way into your windpipe, the filter will attempt to remove them.",
    "occupied_bodyparts": [ [ "torso", 4 ], [ "mouth", 2 ] ],
    "env_protec": [ [ "mouth", 7 ] ],
    "protec": [
      [ "arm_l", { "bash": 3, "cut": 3, "bullet": 3 } ],
      [ "arm_r", { "bash": 3, "cut": 3, "bullet": 3 } ],
      [ "hand_l", { "bash": 3, "cut": 3, "bullet": 3 } ],
      [ "hand_r", { "bash": 3, "cut": 3, "bullet": 3 } ]
    ],
    "flags": [ "BIONIC_NPC_USABLE" ]
},
  {
    "id": "bio_hydraulics",
    "type": "bionic",
    "name": { "str": "Hydraulic Muscles" },
    "description": "While activated, your muscles will be greatly enhanced, increasing your strength by 20.",
    "occupied_bodyparts": [ [ "torso", 10 ], [ "arm_l", 8 ], [ "arm_r", 8 ], [ "leg_l", 10 ], [ "leg_r", 10 ] ],
    "flags": [ "BIONIC_TOGGLED", "BIONIC_NPC_USABLE" ],
    "act_cost": "10 kJ",
    "react_cost": "10 kJ",
    "time": "1 s",
    "required_bionic": "bio_weight"
  },
  {
    "type": "bionic",
    "id": "afs_bio_skullgun",
    "name": { "str": "Skullgun" },
    "description": "Concealed in your head is a single shot 10mm pistol.  Activate the bionic to fire and reload the skullgun.",
    "occupied_bodyparts": [ [ "head", 5 ] ],
    "encumbrance": [ [ "head", 5 ] ],
    "fake_weapon": "bio_skullgun_gun",
    "flags": [ "BIONIC_GUN" ],
    "stat_bonus": [ [ "INT", -4 ], [ "PER", -2 ] ],
    "canceled_mutations": [ "INT_UP", "INT_UP_2", "INT_UP_3", "INT_UP_4", "INT_ALPHA", "SKULLGUN_STUPID" ],
    "give_mut_on_removal": [ "SKULLGUN_STUPID" ],
    "activated_close_ui": true
  }
```

Bionics effects are defined in the code and new effects cannot be created through JSON alone.
When adding a new bionic, if it's not included with another one, you must also add the corresponding CBM item in `data/json/items/bionics.json`. Even for a faulty bionic.


### Damage Types

| Field               | Description
| ---                 | ---
| `name`              | The name of the damage type as it appears in the protection values in the item info screen.
| `skill`             | _(optional)_ Determines the skill used when dealing this damage type. (defaults to none)
| `physical`          | _(optional)_ Identifies this damage type as originating from physical sources. (defaults to false)
| `melee_only`        | _(optional)_ Identifies this damage type as originating from melee weapons and attacks. (defaults to false)
| `edged`             | _(optional)_ Identifies this damage type as originating from a sharp or pointy weapon or implement. (defaults to false)
| `environmental`     | _(optional)_ This damage type corresponds to environmental sources. Currently influences whether an item or piece of armor includes environmental resistance against this damage type. (defaults to false)
| `material_required` | _(optional)_ Determines whether materials must defined a resistance for this damage type. (defaults to false)
| `mon_difficulty`    | _(optional)_ Determines whether this damage type should contribute to a monster's difficulty rating. (defaults to false)
| `no_resist`         | _(optional)_ Identifies this damage type as being impossible to resist against (ie. "pure" damage). (defaults to false)
| `immune_flags`      | _(optional)_ An object with two optional fields: `"character"` and `"monster"`. Both inner fields list an array of character flags and monster flags, respectively, that would make the character or monster immune to this damage type.
| `magic_color`       | _(optional)_ Determines which color identifies this damage type when used in spells. (defaults to "black")
| `derived_from`      | _(optional)_ An array that determines how this damage type should be calculated in terms of armor protection and monster resistance values. The first value is the source damage type and the second value is the modifier applied to source damage type calculations.
| `onhit_eocs`        | _(optional)_ An array of effect-on-conditions that activate when a monster or character hits another monster or character with this damage type. In this case, `u` refers to the damage source and `npc` refers to the damage target.
| `ondamage_eocs`        | _(optional)_ An array of effect-on-conditions that activate when a monster or character takes damage from another monster or character with this damage type. In this case, `u` refers to the damage source and `npc` refers to the damage target. Also have access to some [context vals](EFFECT_ON_CONDITION#context-variables-for-other-eocs)

```jsonc
  {
    "//": "stabbing/piercing damage",
    "id": "stab",
    "type": "damage_type",
    "melee_only": true,
    "physical": true,
    "edged": true,
    "magic_color": "light_red",
    "name": "pierce",
    "skill": "stabbing",
    "//2": "derived from cut only for monster defs",
    "derived_from": [ "cut", 0.8 ],
    "immune_flags": { "character": [ "STAB_IMMUNE" ] }
  },
  {
    "//": "e.g. electrical discharge",
    "id": "electric",
    "type": "damage_type",
    "physical": false,
    "magic_color": "light_blue",
    "name": "electric",
    "immune_flags": { "character": [ "ELECTRIC_IMMUNE" ], "monster": [ "ELECTRIC", "ELECTRIC_FIELD" ] },
    "onhit_eocs": [ "EOC_ELECTRIC_ONHIT" ]
  }
```


### Damage Info Ordering

Damage types are displayed in various parts of the item info UI, representing armor resistances, melee damage, etc.
Using `damage_info_order` we can reorder how these are shown, and even determine whether they can be displayed at all.

| Field          | Description
| ---            | ---
| `id`           | Unique identifier, must correspond to an existing `damage_type`
| `info_display` | _(optional)_ Determines the detail in which this damage type is displayed in protection values. Valid values are "detailed", "basic", and "none". (defaults to "none")
| `verb`         | _(optional)_ A verb describing how this damage type is applied (ex: "bashing"). Used in the melee section of an item's info.
| `*_info`       | _(optional)_ An object that determines the order and visibility of this damage type for the specified section of an item's info. `"order"` determines where in the list of damage types it will be displayed in this section, and `"show_type"` determines whether to show this damage type in this section. Possible sections include: `bionic_info`, `protection_info`, `pet_prot_info`, `melee_combat_info`, and `ablative_info`.

```jsonc
{
  "id": "acid",
  "type": "damage_info_order",
  "info_display": "basic",
  "verb": "corroding",
  "bionic_info": { "order": 500, "show_type": true },
  "protection_info": { "order": 800, "show_type": true },
  "pet_prot_info": { "order": 500, "show_type": true },
  "melee_combat_info": { "order": 500, "show_type": false },
  "ablative_info": { "order": 500, "show_type": false }
}
```


### Dreams

| Identifier | Description
|---         |---
| `messages` | List of potential dreams.
| `category` | Mutation category needed to dream.
| `strength` | Mutation category strength required (1 = 20-34, 2 = 35-49, 3 = 50+).

```jsonc
{
    "messages" : [
        "You have a strange dream about birds.",
        "Your dreams give you a strange feathered feeling."
    ],
    "category" : "MUTCAT_BIRD",
    "strength" : 1
}
```

### Disease

| Identifier           | Description
|---                   |---
| `id`                 | Unique ID. Must be one continuous word, use underscores if necessary.
| `min_duration`       | The minimum duration the disease can last. Uses strings "x m", "x s","x d".
| `max_duration`       | The maximum duration the disease can last.
| `min_intensity`      | The minimum intensity of the effect applied by the disease
| `max_intensity`      | The maximum intensity of the effect.
| `health_threshold`   | The amount of health above which one is immune to the disease. Must be between -200 and 200. (optional )
| `symptoms`           | The effect applied by the disease.
| `affected_bodyparts` | The list of bodyparts on which the effect is applied. (optional, default to bp_null)


```jsonc
  {
    "type": "disease_type",
    "id": "bad_food",
    "min_duration": "6 m",
    "max_duration": "1 h",
    "min_intensity": 1,
    "max_intensity": 1,
    "affected_bodyparts": [ "TORSO" ],
    "health_threshold": 100,
    "symptoms": "foodpoison"
  }
```

### End Screen

| Identifier           | Description
|---                   |---
| `id`                 | (_mandatory_) Unique ID. Must be one continuous word, use underscores if necessary.
| `priority`           | (_mandatory_) Int used to chose among several end sreens with valid conditions, higher value have higher priority. Priority 0 is the default tombstone end.
| `picture_id`         | (_mandatory_) ID of an ascii art, see #Ascii_arts.
| `condition`          | (_mandatory_) Conditions necessary to display this end screen.  See the "Dialogue conditions" section of [NPCs](NPCs.md) for the full syntax.
| `added_info`         | (_optional_) Vector of pairs of a pair of int character offset and Line number and a string to be written on the end screen. The string can use talk tags, see the "Special Custom Entries" section of [NPCs](NPCs.md) for the full syntax.
| `last_words_label`   | (_optional_) String used to label the last word input prompt. If left empty no prompt will be displayed.

```jsonc
  {
    "type": "end_screen",
    "id": "death_cross",
    "priority": 1,
    "picture_id": "ascii_rip_cross",
    "condition": {
      "and": [
        { "not": "u_is_alive" },
        {
          "or": [ { "u_has_item": "holybook_bible1" }, { "u_has_item": "holybook_bible2" }, { "u_has_item": "holybook_bible3" } ]
        },
        { "not": { "and": [ { "u_has_trait": "CANNIBAL" }, { "u_has_trait": "PSYCHOPATH" } ] } }
      ]
    },
    "added_info": [
      [ [ 8, 8 ], "In memory of: <u_name>" ],
      [ [ 15, 9 ], "Survived: <time_survived>" ],
      [ [ 17, 10 ], "Kills: <total_kills>" ]
    ],
    "last_words_label": "Last Words:"
  }
```

### Emitters

Emitters randomly place [fields](#field-types) around their positions - every turn for monster emissions, every ten seconds for furniture/terrain.

| Identifier  | Description
|---          |---
| `id`        | Unique ID
| `field`     | Field type emitted.  This can be a Variable Object, see the [doc](EFFECT_ON_CONDITION.md) for more info. 
| `intensity` | Initial intensity of the spawned fields (spawning multiple fields will still cause their intensity to increase). Default 1.  This can be a Variable Object, see the [doc](EFFECT_ON_CONDITION.md) for more info. 
| `chance`    | **Percent** chance of the emitter emitting, values above 100 will increase the quantity of fields placed via `roll_remainder` (ex: `chance: 150` will place one field 50% of the time and two fields the other 50% ). Failing the roll will disable the whole emission for the tick, not rolled for every `qty`! Default 100.  This can be a Variable Object, see the [doc](EFFECT_ON_CONDITION.md) for more info. 
| `qty`       | Number of fields placed. Fields are placed using the field propagation rules, allowing fields to spread. Default 1.  This can be a Variable Object, see the [doc](EFFECT_ON_CONDITION.md) for more info. 

```jsonc
  {
    "id": "emit_shock_burst",
    "type": "emit",
    "field": "fd_electricity",
    "intensity": 3,
    "chance": 1,
    "qty": 10
  },
```
### Item Groups

Item groups have been expanded, look at [the detailed docs](ITEM_SPAWN.md) to their new description.
The syntax listed here is still valid.

| Identifier | Description
|---         |---
| `id`       | Unique ID. Must be one continuous word, use underscores if necessary
| `items`    | List of potential item ID's. Chance of an item spawning is x/T, where X is the value linked to the specific item and T is the total of all item values in a group.
| `groups`   | ??

```jsonc
{
    "id":"forest",
    "items":[
        ["rock", 40],
        ["stick", 95],
        ["mushroom", 4],
        ["mushroom_poison", 3],
        ["mushroom_magic", 1],
        ["blueberries", 3]
    ],
    "groups":[]
}
```

### Item Category

When you sort your inventory by category, these are the categories that are displayed.

| Identifier       | Description
|---               |---
| `id`             | Unique ID. Must be one continuous word, use underscores if necessary
| `name_header`    | The name of the category used for headers. This is what shows up in-game when you open the inventory.
| `name_noun`      | The name of the category used for descriptive text, including singular and plural names.
| `zone`           | The corresponding loot_zone (see loot_zones.json)
| `sort_rank`      | Used to sort categories when displaying.  Lower values are shown first
| `priority_zones` | When set, items in this category will be sorted to the priority zone if the conditions are met. If the user does not have the priority zone in the zone manager, the items get sorted into zone set in the 'zone' property. It is a list of objects. Each object has 3 properties: ID: The id of a LOOT_ZONE (see LOOT_ZONES.json), filthy: boolean. setting this means filthy items of this category will be sorted to the priority zone, flags: array of flags
| `spawn_rate`      | Sets amount of items from item category that might spawn.  Checks for `spawn_rate` value for item category.  If `spawn_chance` is 0.0, the item will not spawn. If `spawn_chance` is greater than 0.0 and less than 1.0, it will make a random roll (0.0-1.0) to check if the item will have a chance to spawn.  If `spawn_chance` is more than or equal to 1.0, it will add a chance to spawn additional items from the same category.  Items will be taken from item group which original item was located in.  Therefore this parameter won't affect chance to spawn additional items for items set to spawn solitary in mapgen (e.g. through use of `item` or `place_item`).

```jsonc
{
    "id": "armor",
    "name": "ARMOR",
    "zone": "LOOT_ARMOR",
    "sort_rank": -21,
    "priority_zones": [ { "id": "LOOT_FARMOR", "filthy": true, "flags": [ "RAINPROOF" ] } ],
    "spawn_rate": 0.5
}
```

### Item faults

Faults can be defined for more specialized damage of an item.

```jsonc
{
  "type": "fault",
  "id": "fault_gun_chamber_spent", // unique id for the fault
  "name": { "str": "Spent casing in chamber" }, // fault name for display
  "description": "This gun currently...", // fault description
  "item_prefix": "jammed", // optional string, items with this fault will be prefixed with this
  "item_suffix": "no handle", // optional string, items with this fault will be suffixed with this. The string would be encased in parentheses, like `sword (no handle)`
  "message": "%s has it's handle broken!", // Message, that would be shown when such fault is applied, unless supressed
  "fault_type": "gun_mechanical_simple", // type of a fault, code may call for a random fault in a group instead of specific fault
  "affected_by_degradation": false, // default false. If true, the item degradation value would be added to fault weight on roll
  "degradation_mod": 50,  // default 0. Having this fault would add this amount of temporary degradation on the item, resulting in higher chance to trigger faults with "affected_by_degradation": true. Such degradation will be removed when fault is fixed
  "price_modifier": 0.4, // (Optional, double) Defaults to 1 if not specified. A multiplier on the price of an item when this fault is present. Values above 1.0 will increase the item's value.
  "melee_damage_mod": [ { "damage_id": "cut", "add": -5, "multiply": 0.8 } ], // (Optional) alters the melee damage of this type for item, if fault of this type is presented. `damage_id` is mandatory, `add` is 0 by default, `multiply` is 1 by default
  "armor_mod": [ { "damage_id": "cut", "add": -5, "multiply": 0.8 } ], // (Optional) Same as armor_mod, changes the protection value of damage type of the faulted item if it's presented
  "block_faults": [ "fault_handle_chipping", "fault_handle_cracked" ], // Faults, that cannot be applied if this fault is already presented on item. If there is already such a fault, it will be removed. Can't have chipped blade if the blade is gone
  "flags": [ "JAMMED_GUN" ] // optional flags, see below
}
```

`flags` trigger hardcoded C++ chunks that provide effects, see [JSON_FLAGS.md](JSON_FLAGS.md#faults) for a list of possible flags.

### Item fault fixes

Fault fixes are methods to fix faults, the fixes can optionally add other faults, modify damage, degradation and item variables.

```jsonc
{
  "type": "fault_fix",
  "id": "mend_gun_fouling_clean", // unique id for the fix
  "name": "Clean fouling", // name for display
  "success_msg": "You clean your %s.", // message printed when fix is applied
  "time": "50 m", // time to apply fix
  "faults_removed": [ "fault_gun_dirt", "fault_gun_blackpowder" ], // faults removed when fix is applied
  "faults_added": [ "fault_gun_unlubricated" ], // faults added when fix is applied
  "skills": { "mechanics": 1 }, // skills required to apply fix
  "set_variables": { "dirt": 0, "pos": { "tripoint": [ 0, 1, 0 ] }, "name": { "str": "blorg" } }, // sets the variables on the item when fix is applied
  "adjust_variables_multiply": { "dirt": 0.8 }, // adjusts the variables on the item when fix is applied using MULTIPLICATION
  "requirements": [ [ "gun_cleaning", 1 ] ], // requirements array, see below
  "mod_damage": 1000, // damage to modify on item when fix is applied, can be negative to repair
  "mod_degradation": 50, // degradation to modify on item when fix is applied, can be negative to reduce degradation
  "time_save_profs": { "prof_gun_cleaning": 0.5 }, // this prof change how fast you fix the item
  "time_save_flags": { "EASY_CLEAN": 0.5 } // This flag on the item change how fast you fix this item
}
```

`requirements` is an array of requirements, they can be specified in 2 ways:
* An array specifying an already defined requirement by it's id and a multiplier, `[ "gun_lubrication", 2 ]` will add `gun_lubrication` requirement and multiply the components and tools ammo required by 2.
* Inline object specifying the requirement in the same way [recipes define it](#recipe-requirements)

### Item fault groups

Fault group is a combination of a fault and corresponding weight, made so multiples of similar fault groups (handles, blades, cotton pieces of clothes etc) can be combined and reused

```jsonc
{
  "type": "fault_group",
  "id": "handles",
  "group": [ 
    { "fault": "fault_broken_handle", "weight": 100 }, // `fault` should be a fault id
    { "fault": "fault_cracked_handle" }, // default weight is 100, if omitted
    { "fault": "fault_broken_heart", "weight": 10 } 
  ]
}
```

The list of possible faults, their weight and actual chances can be checked in item info with a debug mode on

### Materials

| Identifier             | Description
|---                     |---
| `id`                   | Unique ID. Lowercase snake_case. Must be one continuous word, use underscores if necessary.
| `name`                 | In-game name displayed.
| `resist`               | An object that determines resistance values for this material.
| `chip_resist`          | Returns resistance to being damaged by attacks against the item itself.
| `bash_dmg_verb`        | Verb used when material takes bashing damage.
| `cut_dmg_verb`         | Verb used when material takes cutting damage.
| `dmg_adj`              | Description added to damaged item in ascending severity.
| `dmg_adj`              | Adjectives used to describe damage states of a material.
| `density`              | Affects vehicle collision damage, with denser parts having the advantage over less-dense parts.
| `wind_resist`          | Percentage 0-100. How effective this material is at stopping wind from getting through. Higher values are better. If none of the materials an item is made of specify a value, a default of 99 is assumed.
| `vitamins`             | Vitamins in a material. Usually overridden by item specific values.  An integer percentage of ideal daily value.
| `specific_heat_liquid` | Specific heat of a material when not frozen (J/(g K)). Default 4.186 - water.
| `specific_heat_solid`  | Specific heat of a material when frozen (J/(g K)). Default 2.108 - water.
| `latent_heat`          | Latent heat of fusion for a material (J/g). Default 334.
| `freezing_point`       | Freezing point of this material (C). Default 0 C ( 32 F ).
| `edible`               | Optional boolean. Default is false.
| `rotting`              | Optional boolean. Default is false.
| `breathability`        | What breathability the clothes, made out of this material, would have; can be `IMPERMEABLE` (0%), `POOR` (30%), `AVERAGE` (50%), `GOOD` (80%), `MOISTURE_WICKING` (110%), `SECOND_SKIN` (140%)
| `burn_products`        | Burning this material drop this items; array, first in array is the id of an item, and another is the number, respond for effeciency of burning - the bigger the burnable item is (by weight), and the more items there is, the bigger output; Multiple items could be returned simultaneously, like `[ [ "corpse_ash", 0.035 ], [ "glass_shard", 0.5 ] ]`,
| `repair_difficulty`    | Skill level that would be used to repair this item by default; if item has multiple materials, the most difficult would be used
| `repaired_with`        | Material, that would be used to repair item, made out of this material
| `salvaged_into`        | Item, into which this material could be salvaged
| `sheet_thickness`      | Clothes, made out of this material, has this thickness, meaning clothes thickness should be multiple of this value; layered kevlar has `"sheet_thickness": 4.4,`, meaning all clothes that uses layered kevlar should be either 4.4, 8.8, 13.2 etc milimeters thick; unless `"ignore_sheet_thickness": true` is used for this clothes
| `uncomfortable`        | Clothes made out of this material is always uncomfortable, no matter of it's properties
| `soft`                 | True for pliable materials, whose length doesn't prevent fitting into a container, or through the opening of a container. Default is false.
| `conductive`           | True if the material conducts electricity, defaults to false
| `reinforces`           | Optional boolean. Default is false.

There are seven -resist parameters: acid, bash, chip, cut, elec, fire, and bullet. These are integer values; the default is 0 and they can be negative to take more damage.

```jsonc
{
    "type": "material",
    "id": "hflesh",
    "name": "Human Flesh",
    "density": 5,
    "specific_heat_liquid": 3.7,
    "specific_heat_solid": 2.15,
    "latent_heat": 260,
    "edible": true,
    "rotting": true,
    "resist": { "bash": 1, "cut": 1, "acid": 1, "heat": 1, "bullet": 1 },
    "chip_resist": 2,
    "dmg_adj": [ "bruised", "mutilated", "badly mutilated", "thoroughly mutilated" ],
    "bash_dmg_verb": "bruised",
    "cut_dmg_verb": "sliced",
    "vitamins": [ [ "calcium", 0.1 ], [ "vitB", 1 ], [ "iron", 1.3 ] ],
    "burn_data": [
      { "fuel": 1, "smoke": 1, "burn": 1, "volume_per_turn": "2500_ml" },
      { "fuel": 2, "smoke": 3, "burn": 2, "volume_per_turn": "10000_ml" },
      { "fuel": 3, "smoke": 10, "burn": 3 }
    ]
}
```

Note that the above example gives floats, not integers, for the vitamins values.  This is likely incorrect; they should be replaced with integers.


#### Fuel data

Every material can have fuel data that determines how much horse power it produces per unit consumed. Currently, gases and plasmas cannot really be fuels.

If a fuel has the PERPETUAL flag, engines powered by it never use any fuel.  This is primarily intended for the muscle pseudo-fuel, but mods may take advantage of it to make perpetual motion machines.

```jsonc
"fuel_data" : {
    "energy": "34200_kJ",        // Energy per litre of fuel.
                                 // https://en.wikipedia.org/wiki/Energy_density
   "perpetual": true,            // this material is a perpetual fuel like `wind`, `sunlight`, `muscle`, `animal` and `metabolism`.
   "pump_terrain": "t_gas_pump", // optional. terrain id for the fuel's pump, if any.
   "explosion_data": {           // optional for fuels that can cause explosions
        "chance_hot": 2,         // 1 in chance_hot of explosion when attacked by HEAT weapons
        "chance_cold": 5,        // 1 in chance_cold of explosion when attacked by other weapons
        "factor": 1.0,           // explosion factor - larger numbers create more powerful explosions
        "fiery": true,           // true for fiery explosions
        "size_factor": 0.1       // size factor - larger numbers make the remaining fuel increase explosion power more
    }
}
```

#### Burn data

Every material can have burn data that determines how it interacts with fire. Fundamentally, the intensity, smoke production, and longevity of fires depends on the volume of consumed items. However, these values allow for certain items to burn more for a given volume, or even put out or inhibit the growth of fires.

Note that burn_data is defined per material, but items may be made of multiple materials. For such cases, each material of the item will be calculated separately, as if it was multiple items each corresponding to a single material.

```jsonc
"burn_data": [
    { "immune": true,                    // Defaults to false, optional boolean. If true, makes the resulting material immune to fire. As such it can neither provide fuel nor be burned or damaged.
	"fuel": 300,                     // Float value that determines how much time and intensity this material adds to a fire. Negative values will subtract fuel from the fire, smothering it. 
	                                 // Items with a phase ID of liquid should be made of materials with a value of >= 200 if they are intended to be flammable.
	"smoke": 0,                      // Float value, determines how much smoke this material produces when burning.
	"volume_per_turn": "750 ml",     // If non-zero and lower than item's volume, scale burning by volume_per_turn / volume
	"burn": 1 }                      // Float value, determines how quickly a fire will convert items made of this material to fuel. Does not affect the total fuel provided by a given
                                         // volume of a given material.
    ],
```

### Monster Groups

#### Group definition

| Identifier              | Description
|---                      |---
| `name`                  | Unique ID. Must be one continuous word, use underscores if necessary.
| `default`               | (_optional_) Default monster, used to represent the monster group. (default: The monster with the highest `weight` in the group)
| `monsters`              | To choose a monster for spawning, the game creates entries equal to the sum of all `weight` and picks one. Each monster will have a number of entries equal to its `weight`. See the table below for how to build the single monster definitions.
| `is_safe`               | (_optional_) (bool) Check to not trigger safe-mode warning, currently inconsequential.
| `is_animal`             | (_optional_) (bool) Check if that group has only normal animals, currently inconsequential.
| `replace_monster_group` | (_optional_) (bool) Check if the group should be replaced completely by another monster group as game time progresses - doesn't affect already spawned monsters, as such mostly superseded by monster evolution.
| `new_monster_group_id`  | (_optional_) (string) The id of the monster group that should replace this one.
| `replacement_time`      | (_optional_) (int) The amount of time before the group should be replaced by the new one, in days. Final replacement date is calculated by `replacement_time * evolution factor`.

#### Monster/Subgroup definition

In monster groups, within the `"monsters"` array, you can define `"group"` objects as well as `"monster"` objects. Groups use the same fields as monsters, but they are processed differently. When the game looks for possible spawns from a monster group, it will recursively check subgroups if they exist. The weight of the subgroup is defined just like monster objects, so spawn chances only matter for top-level objects.

| Identifier        | Description
|---                |---
| `monster`         | The monster's unique ID, eg. `"mon_zombie"`. Indicates that this entry is a "monster".
| `group`           | The sub-group's unique ID eg. `"GROUP_ZOMBIE"`. Indicates that this entry is a "monstergroup".
| `weight`          | (_optional_) Chance of occurrence (`weight` / total `weight` in group) (default: 1)
| `freq`            | (_optional_) Not used anymore, works exactly like weight
| `cost_multiplier` | (_optional_) How many monsters each monster in this definition should count as, if spawning a limited number of monsters.  (default: 1)
| `pack_size`       | (_optional_) The minimum and maximum number of monsters in this group that should spawn together.  (default: `[1,1]`)
| `conditions`      | (_optional_) Conditions limit when monsters spawn. Valid options: `SUMMER`, `WINTER`, `AUTUMN`, `SPRING`, `DAY`, `NIGHT`, `DUSK`, `DAWN`. Multiple Time-of-day conditions (`DAY`, `NIGHT`, `DUSK`, `DAWN`) will be combined together so that any of those conditions makes the spawn valid. Multiple Season conditions (`SUMMER`, `WINTER`, `AUTUMN`, `SPRING`) will be combined together so that any of those conditions makes the spawn valid.
| `starts`          | (_optional_) This entry becomes active after this much time multiplied by the evolution scaling factor.  Specified using time units (See [Time duration](#time-duration)).
| `ends`            | (_optional_) This entry becomes inactive after this much time multiplied by the evolution scaling factor.  Specified using time units (See [Time duration](#time-duration)).
| `spawn_data`      | (_optional_) Any properties that the monster only has when spawned in this group. `ammo` defines how much of which ammo types the monster spawns with. Only applies to "monster" type entries.
| `event`           | (_optional_) If present, this entry can only spawn during the specified event. See the `holiday` enum for possible values. Defaults to `none`. (Ex: `"event": "halloween"`)

```jsonc
// Example of a monstergroup containing only "monster" entries:
{
  "name" : "GROUP_ANT",
  "default" : "mon_ant",
  "monsters" : [
    { "monster" : "mon_ant", "weight" : 870, "cost_multiplier" : 0 },
    { "monster" : "mon_ant_larva", "weight" : 40, "cost_multiplier" : 0 },
    { "monster" : "mon_ant_soldier", "weight" : 90, "cost_multiplier" : 5 },
    { "monster" : "mon_ant_queen", "weight" : 0, "cost_multiplier" : 0 },
    { "monster" : "mon_thing", "weight" : 100, "cost_multiplier" : 0, "pack_size" : [3,5], "conditions" : ["DUSK","DAWN","SUMMER"] },
    { "monster" : "mon_santa", "weight" : 500, "event" : "christmas" }
  ]
},
// Example of a monstergroup containing subgroups:
{
  "type": "monstergroup",
  "name": "GROUP_MIGO_RAID",
  "//": "Meta-group for mi-gos on-the-go.",
  "monsters": [
    { "group": "GROUP_MI-GO_BASE_CAPTORS", "weight": 150, "cost_multiplier": 6, "pack_size": [ 1, 2 ] },
    { "group": "GROUP_MI-GO_SCOUT_TOWER", "weight": 100, "cost_multiplier": 4, "pack_size": [ 0, 2 ] },
    { "monster": "mon_mi_go_guard", "weight": 200, "cost_multiplier": 4 },
    { "monster": "mon_mi_go", "weight": 500, "cost_multiplier": 2, "pack_size": [ 3, 4 ] }
  ]
}
```

### Monster Factions

| Identifier      | Description
|---              |---
| `name`          | Unique ID. Must be one continuous word, use underscores when necessary.
| `base_faction`  | Optional base faction. Relations to other factions are inherited from it and relations of other factions to this one check this.
| `by_mood`       | Be hostile towards this faction when angry, neutral otherwise. Default attitude to all other factions.
| `neutral`       | Always be neutral towards this faction.
| `friendly`      | Always be friendly towards this faction. By default a faction is friendly towards itself.
| `hate`          | Always be hostile towards this faction. Will change target to monsters of this faction if available.

```jsonc
{
    "name"         : "cult",
    "base_faction" : "zombie",
    "by_mood"      : ["slime"],
    "neutral"      : ["nether"],
    "friendly"     : ["slime"],
    "hate"         : ["fungus"]
}
```

### Monsters

See [MONSTERS.md](MONSTERS.md)

### Mutation Categories

See [MUTATIONS.md](MUTATIONS.md)

### Names

```jsonc
{ "name" : "Aaliyah", "gender" : "female", "usage" : "given" }, // Name, gender, "given"/"family"/"city" (first/last/city name).
```

### Profession item substitution

Defines item replacements that are applied to the starting items based upon the starting traits. This allows for example to replace wool items with non-wool items when the characters starts with the wool allergy trait.

If the JSON objects contains a "item" member, it defines a replacement for the given item, like this:

```jsonc
{
  "type": "profession_item_substitutions",
  "item": "sunglasses",
  "sub": [
    { "present": [ "HYPEROPIC" ], "new": [ "fitover_sunglasses" ] },
    { "present": [ "MYOPIC" ], "new": [ { "fitover_sunglasses", "ratio": 2 } ] }
  ]
}
```
This defines each item of type "sunglasses" shall be replaced with:
- an item "fitover_sunglasses" if the character has the "HYPEROPIC" trait,
- two items "fitover_sunglasses" if the character has the "MYOPIC" trait.

If the JSON objects contains a "trait" member, it defines a replacement for multiple items that applies when the character has the given trait:
```jsonc
{
  "type": "profession_item_substitutions",
  "trait": "WOOLALLERGY",
  "sub": [
    { "item": "blazer", "new": [ "jacket_leather_red" ] },
    { "item": "hat_hunting", "new": [ { "item": "hat_cotton", "ratio": 2 } ] }
  ]
}
```
This defines characters with the WOOLALLERGY trait get some items replaced:
- "blazer" is converted into "jacket_leather_red",
- each "hat_hunting" is converted into *two* "hat_cotton" items.

If the JSON objects contains a "bonus" member, it defines which items will be received, like this:
```jsonc
{
  "type": "profession_item_substitutions",
  "group": {
    "items": [ "winter_pants_army", "undershirt", "socks", "sweatshirt", "boots_hiking", "knife_folding", "wristwatch" ],
    "entries": [
      { "group": "charged_two_way_radio" },
      { "group": "charged_matches" },
      { "item": "ear_plugs" },
      { "item": "water_clean", "container-item": "canteen" },
      { "item": "m1911", "ammo-item": "45_acp", "charges": 7, "container-item": "holster" },
      { "item": "45_acp", "charges": 23 },
      { "item": "garand", "ammo-item": "3006", "charges": 8, "contents-item": "shoulder_strap" },
      { "item": "3006", "charges": 8, "container-item": "garandclip" },
      { "item": "3006", "charges": 4 }
    ]
  },
  "bonus": {
    "present": [ "ALBINO" ],
    "absent": [ "HYPEROPIC" ]
  }
}
```

### Professions

Professions are specified as JSON objects with the "type" member set to "profession".
The following properties (mandatory, except if noted otherwise) are supported:

```jsonc
  {
    "type": "profession",
    "id": "profession_example",                                // Unique ID for the profession
    "name": { "male": "Groom", "female": "Bride" },            // String, either a single gender neutral (i.e. "Survivor") or object with members "male" and "female"
    "description": "This is an example profession.",           // In-game description
    "points": 0,                                               // Point cost of profession. Positive values cost points and negative values grant points. Has no effect as of 0.G
    "starting_cash": 500000,                                   // (optional) Int value for the starting bank balance.
    "npc_background": "BG_survival_story_LAB",                 // (optional) BG_trait_group ID, provides list of background stories. (see BG_trait_groups.json)
    "addictions": [ { "intensity": 10, "type": "nicotine" } ], // (optional) Array of addictions. Requires "type" as the string ID of the addiction (see JSON_FLAGS.md) and "intensity"
    "skills": [ { "name": "archery", "level": 2 } ],           // (optional) Array of starting skills. Requires "name" as the string ID of the skill (see skills.json) and "level", which is a value added to the skill level after character creation
    "missions": [ "MISSION_LAST_DELIVERY" ],                   // (optional) Array of starting mission IDs
    "proficiencies": [ "prof_knapping" ],                      // (optional) Array of starting proficiencies
    "items": {                                                 // (optional) Object of items the character starts with (see below for further explanation)
      "both": {
        "entries": [
          { "item": "jeans" },
          { "item": "tank_top", "variant": "tank_top_camo" },
          { "item": "ear_plugs", "custom-flags": [ "no_auto_equip" ] },
          { "item": "socks" },
          { "item": "sneakers" },
          { "item": "water_clean", "container-item": "canteen" },
          { "item": "m1911", "ammo-item": "45_acp", "charges": 7, "container-item": "holster" },
          { "item": "45_acp", "charges": 23 },
          { "item": "legpouch_large", "contents-group": "army_mags_m4" }
        ]
      },
      "male": { "entries": [ { "item": "boxer_shorts" } ] },
      "female": { "entries": [ { "item": "bra" }, { "item": "panties" } ] }
    },
    "age_lower": 18,                                           // (optional) Int. The lowest age that a character with this profession can generate with. This places no limits on manual input, only on random generation (i.e. Play Now!). Defaults to 21
    "age_upper": 25,                                           // (optional) Int. Similar as above
    "pets": [ { "name": "mon_black_rat", "amount": 13 } ],     // (optional) Array of starting monster IDs, tamed as pets
    "vehicle": "car_sports",                                   // (optional) String of a starting vehicle ID. The vehicle will be spawned at the closest road, with the character "remembering" its location in the overmap
    "flags": [ "SCEN_ONLY", "NO_BONUS_ITEMS" ],                // (optional) Array of flags applied to the character, for character creation purposes
    "CBMs": [ "bio_fuel_cell_blood" ],                         // (optional) Array of starting implanted CMBs
    "traits": [ "PROF_CHURL", "ILLITERATE" ],                  // (optional) Array of starting traits/mutations. For further information, see mutations.json and MUTATIONS.md. Note: "trait" is also supported, used for a single trait/mutation ID (legacy!)
    "requirement": "achievement_survive_28_days",              // (optional) String of an achievement ID required to unlock this profession
    "effect_on_conditions": [ "scenario_assassin_conv" ],      // (optional) eoc id, inline eoc, or multiple of them, that would run when scenario starts
    "spells": [                                                // (optional) Array of starting spell IDs the character knows upon creation. For further information, see MAGIC.md
      { "id": "magic_missile", "level": 4 },
      { "id": "summon_undead", "level": 5 },
      { "id": "necrotic_gaze", "level": 1 }
    ],
    "recipes": [ "beartrap" ]                                  // (optional) Array of starting recipe IDs the character knows upon creation
  },
```

The following fields are further described

#### `items`

Items the player starts with when selecting this profession.  One can specify different items based on the gender of the character.  Each lists of items should be an array of items IDs, or pairs of item IDs and snippet IDs.  Item IDs may appear multiple times, in which case multiple times are spawned.  The syntax for each of the three lists is identical.  The old and new formats can be combined, with the old format shown here for legacy purposes:

```jsonc
  "items": {
    "both": [
      { "item": "jeans" },
      "rock",
      [ "tshirt_text", "allyourbase" ],
      { "item": "tank_top", "variant": "tank_top_camo" }
    ],
    "male": { "entries": [ { "item": "boxer_shorts" } ] },
    "female": [ "panties" ]
  }
```


#### `hobbies`

(optional) Array of string profession IDs.  A list of hobbies that will be the only hobbies this profession can choose from.  If empty, all hobbies will be allowed.

#### `whitelist_hobbies`

(optional) Boolean.  If false, `hobbies` will instead be a list of hobbies that this profession _cannot_ choose from.  Defaults to true.


### Hobbies

Hobbies consist of backgrounds for the character upon creation, designed in the code as subtype professions.
These can be combined with the "primary" profession, adding minor bonuses and/or demerits on top of the starting parameters, allowing more flexibility and character identity.
Note: hobby fields derive from those used in professions.  The following is a non-exhaustive list:

```jsonc
  {
    "type": "profession",
    "subtype": "hobby",
    "id": "hobby_profession",                                  // Unique ID of the hobby
    "name": "Driving License",                                 // String, in-game name
    "description": "Description goes here",                    // In-game description
    "points": 0,                                               // Point cost of profession. Positive values cost points and negative values grant points. Has no effect as of 0.G
    "skills": [                                                // (optional) Array of starting skills, added on top of the profession starting skills
      { "level": 2, "name": "driving" }
    ],
    "proficiencies": [ "prof_driver" ],                        // (optional) Array of starting proficiencies
    "addictions": [ { "intensity": 40, "type": "opiate" } ],   // (optional) Array of addictions, added on top of the profession starting addictions
    "traits": [ "LOVES_BOOKS" ],                               // (optional) Array of starting traits/mutations
    "CBMs": [ "bio_adrenaline" ],                              // (optional) Array of starting implanted CMBs
    "spells": [ { "id": "create_atomic_light", "level": 5 } ]  // (optional) Array of starting spell IDs the character knows upon creation
  },
```

### Profession groups

Profession groups are a list of hobbies added to the character upon creation.
While the list is automatically added by hardcode to each character upon creation (adding a minimum set of skills to all characters, regardless of their professions), the list can be modified via JSON.
Thus, it is listed here for reference and modding purposes only.

```jsonc
  {
    "type": "profession_group",
    "id": "adult_basic_background",
    "professions": [
      "driving_license",
      "simple_home_cooking",
      "computer_literate",
      "social_skills",
      "high_school_graduate",
      "mundane_survival"
    ]
  }
```

The array of hobbies (listed as professions) is whitelisted to all characters.  Thus, if one wants to start with no hobbies, the list has to be set as:

```jsonc
    "professions": [  ]
```

### Constructions
```jsonc
"group": "spike_pit",                                               // Construction group, used to group related constructions in UI
"category": "DIG",                                                  // Construction category
"skill": "fabrication",                                             // Primary skill, that would be used in the recipe
"difficulty": 1,                                                    // Difficulty of primary skill
"required_skills": [ [ "survival", 1 ] ],                           // Skill levels required to undertake construction
"qualities": [ [ { "id": "SCREW", "level": 1 } ] ],                 // Tool qualities, required to construct
"tools": [ [ [ "oxy_torch", 10 ], [ "welder", 50 ] ] ],             // Tools and amount of charges, that would be used in construction
"using": [ [ "welding_standard", 64 ] ],                            // Requirements that would be used in construction
"activity_level": "EXTRA_EXERCISE",                                 // Activity level of the activity, harder activities consume more calories over time. Valid values are, from easiest to most demanding of the body: `NO_EXERCISE`, `LIGHT_EXERCISE`, `MODERATE_EXERCISE`, `BRISK_EXERCISE`, `ACTIVE_EXERCISE`, `EXTRA_EXERCISE`.
"do_turn_special": "do_turn_shovel",                                // Special effect, that occur, when you perform a construction. Can be either `do_turn_shovel` (cause "hsh!" message every minute, may trigger a buried trap, if there is one) or `do_turn_exhume` (applied mood effect for gravedigging related to your traits)
"vehicle_start": true,                                              // Hardcoded check for construction recipe, that result into vehicle frame; Can be used only with `done_vehicle`
"time": "30 m",                                                     // Time required to complete construction. Integers will be read as minutes or a time string can be used.
"components": [ [ [ "spear_wood", 4 ], [ "pointy_stick", 4 ] ] ],   // Items used in construction
"pre_special": [ "check_empty", "check_up_OK" ],                    // Required something that isn't terrain. The syntax also allows for a square bracket enclosed list of specials which all have to be fulfilled
"pre_terrain": "t_pit",                                             // Alternative to pre_special; Required terrain to build on
"pre_flags": [ "WALL", { "flag": "DIGGABLE", "force_terrain": true } ], // Flags beginning furniture/terrain must have. force_ter forces the flag to apply to the underlying terrain
"post_terrain": "t_pit_spiked",                                     // Terrain type after construction is complete
"post_special": "done_mine_upstairs",                               // Required to do something beyond setting the post terrain. The syntax also allows for a square bracket enclosed list of specials which all have to be fulfilled
"pre_note": "Build a spikes on a diggable terrain",                 // Create an annotation to this recipe
"dark_craftable": true,                                             // If true, you can construct it with lack of light
"byproducts": [ { "item": "material_soil", "count": [ 2, 5 ] } ],   // Items, that would be left after construction
"strict": false,                                                    // If true, the build activity for this construction will only look for prerequisites in the same group
"on_display": false                                                 // This is a hidden construction item, used by faction camps to calculate construction times but not available to the player
```

| pre_special            | Description
|---                     |---
| `check_channel`        | Must be empty and have a current in at least one orthogonal tile
| `check_empty_lite`     | Tile is empty (no furniture, trap, item, or vehicle)
| `check_empty`          | Tile is empty (no furniture, trap, item, or vehicle) and flat terrain
| `check_unblocked`      | Tile is empty (no furniture, trap, item, or vehicle), and either flat terrain or empty space
| `check_support`        | Must have at least two solid walls/obstructions nearby on orthogonals (non-diagonal directions only) or solid support directly below to support the tile
| `check_support_below`  | Must have at least two solid walls/obstructions at the Z level below on orthogonals (non-diagonal directions only) or solid support directly below to support the tile and be empty lite but with a ledge trap acceptable, as well as open air
| `check_single_support` | Must have solid support directly below to support the tile
| `check_stable`         | Tile on level below has a flag `SUPPORTS_ROOF`
| `check_empty_stable`   | Tile is empty and stable
| `check_nofloor_above`  | Tile on level above has a flag `NO_FLOOR`
| `check_deconstruction` | The furniture (or tile, if no furniture) in the target tile must have a "deconstruct" entry
| `check_up_OK`          | Tile is below the maximum possible elevation (can build up here)
| `check_down_OK`        | Tile is above the lowest possible elevation (can dig down here)
| `check_no_trap`        | There is no trap object in this tile
| `check_ramp_low`       | Both this and the next level above can be built up one additional Z level
| `check_ramp_high`      | There is a complete downramp on the next higher level, and both this and next level above can be built up one additional Z level
| `check_no_wiring`      | The tile must either be free of a vehicle, or at least a vehicle that doesn't have the WIRING flag
| `check_matching_down_above`| The tile directly above must have the same base identifier but with a suffix of 'down'

| post_special            | Description
|---                     |---
| `done_trunk_plank`     | Generate logs and then planks here (under the assumption the tile was a trunk or stump)
| `done_grave`           | Finish grave digging by performing burial activities
| `done_vehicle`         | Create a new vehicle and name it
| `done_appliance`       | Finish the placement of a partially placed appliance
| `done_wiring`          | Place a wiring "appliance" at the location
| `done_deconstruct`     | Finish deconstruction of furniture or, if not present, terrain
| `done_dig_grave`       | Finish digging up a grave and find what's inside of it
| `done_dig_grave_nospawn`| Finish digging up a grave and retrieve a coffin (which is not an option above)
| `done_dig_stair`       | Finish diggins stairs downwards (with handling of what's beneath and how to get up)
| `done_mine_downstair`  | Same as the previous one, but mining rather than digging
| `done_mine_upstair`    | Finish mining stairs from below
| `done_wood_stairs`     | Finish building stairs from below
| `done_window_curtains` | Finish boarding up window and get materials from curtains
| `done_extract_maybe_revert_to_dirt`| Finish sand/clay extraction, which may exhaust the resource
| `done_mark_firewood`   | Sets a firewood source trap at the location
| `done_mark_practice_target`| Sets a target practice trap at the location
| `done_ramp_low`        | Sets a t_ramp_down_low at the tile above the target
| `done_ramp_high`       | Sets a t_ramp_down_high at the tile above the target
| `done_matching_down_above`| The terrain on the Z level above is set to the corresponding terrain at this level, but with the "down" suffix instead
| `remove_above`         | Remove the terrain at the Z level above and replace it with t_open_air
| `add_roof`             | Add the roof specified for the terrain at the site to the tile above


### Scent_types

| Identifier               | Description
|---                       |---
| `id`                     | Unique ID. Must be one continuous word, use underscores if necessary.
| `receptive_species`      | Species able to track this scent. Must use valid ids defined in `species.json`

```jsonc
  {
    "type": "scent_type",
    "id": "sc_flower",
    "receptive_species": [ "MAMMAL", "INSECT", "MOLLUSK", "BIRD" ]
  }
```

### Scores, Achievements, and Conducts

Scores are defined in two or three steps based on *events*.  To see what events
exist and what data they contain, read [`event.h`](../src/event.h).

Each event contains a certain set of fields.  Each field has a string key and a
`cata_variant` value.  The fields should provide all the relevant information
about the event.

For example, consider the `gains_skill_level` event.  You can see this
specification for it in `event.h`:

<!-- {% raw %} -->
```cpp
template<>
struct event_spec<event_type::gains_skill_level> {
    static constexpr std::array<std::pair<const char *, cata_variant_type>, 3> fields = {{
            { "character", cata_variant_type::character_id },
            { "skill", cata_variant_type::skill_id },
            { "new_level", cata_variant_type::int_ },
        }
    };
};
```
<!-- {% endraw %} -->
From this, you can see that this event type has three fields:
* `character`, with the id of the character gaining the level.
* `skill`, with the id of the skill gained.
* `new_level`, with the integer level newly acquired in that skill.

Events are generated by the game when in-game circumstances dictate.  These
events can be transformed and summarized in various ways.  There are three
concepts involved: event streams, event statistics, and scores.

* Each `event_type` defined by the game generates an event stream.
* Further event streams can be defined in json by applying an
  `event_transformation` to an existing event stream.
* An `event_statistic` summarizes an event stream into a single value (usually
  a number, but other types of value are possible).
* A `score` uses such a statistic to define an in-game score which players can
  see.

#### `event_transformation`

An `event_transformation` can modify an event stream, producing another event
stream.

The input stream to be transformed is specified either as an `"event_type"`, to
use one of the built-in event type streams, or an `"event_transformation"`,
to use another json-defined transformed event stream.

Any or all of the following alterations can be made to the event stream:

* Add new fields to each event based on event field transformations.  The event
  field transformations can be found in
  [`event_field_transformations.cpp`](/src/event_field_transformations.cpp).
* Filter events based on the values they contain to produce a stream containing
  some subset of the input stream.
* Drop some fields which are not of interest in the output stream.

Here are examples of each modification:

```jsonc
"id": "avatar_kills_with_species",
"type": "event_transformation",
"event_type": "character_kills_monster", // Transformation acts upon events of this type
"new_fields": { // A dictionary of new fields to add to the event
    // The key is the new field name; the value should be a dictionary of one element
    "species": {
        // The key specifies the event_field_transformation to apply; the value specifies
        // the input field whose value should be provided to that transformation.
        // So, in this case, we are adding a new field 'species' which will
        // contain the species of the victim of this kill event.
        "species_of_monster": "victim_type"
    }
}
```

```jsonc
"id": "moves_on_horse",
"type": "event_transformation",
"event_type" : "avatar_moves", // An event type.  The transformation will act on events of this type
"value_constraints" : { // A dictionary of constraints
    // Each key is the field to which the constraint applies
    // The value specifies the constraint.
    // "equals" can be used to specify a constant cata_variant value the field must take.
    // "lt", "lteq", "gteq" and "gt" can be used with int type to compare against a constant cata_variant value.
    // "equals_any" can be used to check for a value in a set of values
    // "equals_statistic" specifies that the value must match the value of some statistic (see below)
    "mount" : { "equals": [ "mtype_id", "mon_horse" ] }
}
// Since we are filtering to only those events where 'mount' is 'mon_horse', we
// might as well drop the 'mount' field, since it provides no useful information.
"drop_fields" : [ "mount" ]
```

The parameter to `"equals"` (and other single-value comparators) is normally a
length-two array specifying a `cata_variant_type` and a value.  As a short cut,
you can simply specify an `int` or `bool` (e.g. `"equals": 7` or `"equals": true`)
for fields which have those types.

The parameter to `"equals_any"` will be a pair where the first element is a
string `cata_variant_type` and the second is an array of values.  For example:
```
"value_constraints": {
  "oter_type_id": { "equals_any": [ "oter_type_str_id", [ "central_lab_finale", "lab_finale" ] ] }
}
```

Value constraints are type-checked, so you should see an error message at game
data verification time if the variant type you have specified doesn't match the
type of the field you're matching.

#### `event_statistic`

As with `event_transformation`, an `event_statistic` requires an input event
stream.  That input stream can be specified in the same was as for
`event_transformation`, via one of the following two entries:

```jsonc
"event_type" : "avatar_moves" // Events of this built-in type
"event_transformation" : "moves_on_horse" // Events resulting from this json-defined transformation
```

Then it specifies a particular `stat_type` and potentially additional details
as follows:

The number of events:
```jsonc
"stat_type" : "count"
```

The sum of the numeric value in the specified field across all events:
```jsonc
"stat_type" : "total"
"field" : "damage"
```

The maximum of the numeric value in the specified field across all events:
```jsonc
"stat_type" : "maximum"
"field" : "damage"
```

The minimum of the numeric value in the specified field across all events:
```jsonc
"stat_type" : "minimum"
"field" : "damage"
```

Assume there is only a single event to consider, and take the value of the
given field for that unique event:
```jsonc
"stat_type": "unique_value",
"field": "avatar_id"
```

The value of the given field for the first event in the input stream:
```jsonc
"stat_type": "first_value",
"field": "avatar_id"
```

The value of the given field for the last event in the input stream:
```jsonc
"stat_type": "last_value",
"field": "avatar_id"
```

Regardless of `stat_type`, each `event_statistic` can also have:
```jsonc
// Intended for use in describing scores and achievement requirements.
"description": "Number of things"
```

#### `score`

Scores simply associate a description to an event for formatting in tabulations
of scores.  The `description` specifies a string which is expected to contain a
`%s` format specifier where the value of the statistic will be inserted.

Note that even though most statistics yield an integer, you should still use
`%s`.

If the underlying statistic has a description, then the score description is
optional.  It defaults to "<statistic description>: <value>".

```jsonc
"id": "score_headshots",
"type": "score",
"description": "Headshots: %s",
"statistic": "avatar_num_headshots"
```

#### `achievement`

Achievements are goals for the player to aspire to, in the usual sense of the
term as popularized in other games.

An achievement is specified via requirements, each of which is a constraint on
an `event_statistic`.  For example:

```jsonc
{
  "id": "achievement_kill_zombie",
  "type": "achievement",
  // The achievement name and description are used for the UI.
  // Description is optional and can provide extra details if you wish.
  "name": "One down, billions to go\u2026",
  "description": "Kill a zombie",
  // if you don't specify requirements because the achievement is given by an EOC
  // you should set manually_given to true, this will avoid errors.
  // this value defaults to false
  "manually_given": false,
  "requirements": [
    // Each requirement must specify the statistic being constrained, and the
    // constraint in terms of a comparison against some target value.
    { "event_statistic": "num_avatar_zombie_kills", "is": ">=", "target": 1 }
  ]
},
```

The `"is"` field must be `">="`, `"<="` or `"anything"`.  When it is not
`"anything"` the `"target"` must be present, and must be an integer.

Additional optional fields for each entry in `requirements` are:

* `"visible"`, which can take the values `"always"`,
  `"when_requirement_completed"`, `"when_achievement_completed"`, or `"never"`
  to dictate when a requirement is visible.  Non-visible requirements will be
  hidden in the UI.
* `"description"` will override the default description of the requirement, for
  cases where the default is not suitable.  The default takes the form `x/y
  foo` where `x` is the current statistic value, `y` is the target value, and
  `foo` is the statistic description (if any).

There are further optional fields for the `achievement`:

```jsonc
"hidden_by": [ "other_achievement_id" ]
```

Give a list of other achievement ids.  This achievement will be hidden (i.e.
not appear in the achievements UI) until all of the achievements listed have
been completed.

Use this to prevent spoilers or to reduce clutter in the list of achievements.

If you want an achievement to be hidden until completed, then mark it as
`hidden_by` its own id.

```jsonc
"time_constraint": { "since": "game_start", "is": "<=", "target": "1 minute" }
```

This allows putting a time limit (either a lower or upper bound) on when the
achievement can be claimed.  The `"since"` field can be either `"game_start"`
or `"cataclysm"`.  The `"target"` describes an amount of time since that
reference point.

Note that achievements can only be captured when a statistic listed in their
requirements changes.  So, if you want an achievement which would normally be
triggered by reaching some time threshold (such as "survived a certain amount
of time") then you must place some requirement alongside it to trigger it after
that time has passed.  Pick some statistic which is likely to change often, and
add an `"anything"` constraint on it.  For example:

```jsonc
{
  "id": "achievement_survive_one_day",
  "type": "achievement",
  "description": "The first day of the rest of their unlives",
  "time_constraint": { "since": "game_start", "is": ">=", "target": "1 day" },
  "requirements": [ { "event_statistic": "num_avatar_wake_ups", "is": "anything" } ]
},
```

This is a simple "survive a day" but is triggered by waking up, so it will be
completed when you wake up for the first time after 24 hours into the game.

#### `conduct`

A conduct is a self-imposed constraint that players can choose to aspire to
maintain.  In some ways a conduct is the opposite of an achievement: it
specifies a set of conditions which can be true at the start of a game, but
might cease to be true at some point.

The implementation of conducts shares a lot with achievements, and their
specification in JSON uses all the same fields.  Simply change the `"type"`
from `"achievement"` to `"conduct"`.

The game enforces that any requirements you specify for a conduct must "become
false" in the sense that once they are false, they can never become true again.
So, for example, an upper bound on some monotonically increasing statistic is
acceptable, but you cannot use a constraint on a statistic which might go down
and up arbitrarily.

With a good motivating example, this constraint might be weakened, but for now
it is present to help catch errors.

### Skills

```jsonc
{
  "type": "skill",
  "id": "smg",
  "name": { "str": "submachine guns" },
  "description": "Comprised of an automatic rifle carbine designed to fire a pistol cartridge, submachine guns can reload and fire quickly, sometimes in bursts, but they are relatively inaccurate and may be prone to mechanical failures.",
  "tags": [ "combat_skill" ],
  "time_to_attack": { "min_time": 20, "base_time": 30, "time_reduction_per_level": 1 },
  "display_category": "display_ranged",
  "sort_rank": 11000,
  "teachable": true,
  "level_descriptions_theory": [
    { "level": 0, "description": "You know absolutely nothing about computers, even in theory." },
    { "level": 1, "description": "You know how to open browser and search images." },
    { "level": 2, "description": "foo" },
    { "level": 3, "description": "bar" },
    { "level": 4, "description": "xyz" }
],
"level_descriptions_practice": [
    { "level": 0, "description": "You don't know how to operate the computer whatsoever" },
    { "level": 1, "description": "You know how to open browser and search images.  Were able to, at least." },
    { "level": 2, "description": "foo" },
    { "level": 3, "description": "bar" },
    { "level": 4, "description": "xyz" }
],
  "companion_skill_practice": [ { "skill": "hunting", "weight": 25 } ]
}
```

| Field                      | Purpose |
| ---                        | ---     |
| `name`                     | Name of the skill as displayed in the the character info screen. |
| `description`              | Description of the skill as displayed in the the character info screen. |
| `tags`                     | Identifies special cases. Currently valid tags are: "combat_skill" and "contextual_skill". |
| `time_to_attack`           | Object used to calculate the movecost for firing a gun. |
| `display_category`         | Category in the character info screen where this skill is displayed. |
| `sort_rank`                | Order in which the skill is shown. |
| `teachable`                | Whether it's possible to teach this skill between characters. (Default = true) |
| `companion_skill_practice` | Determines the priority of this skill within a mision skill category when an NPC gains experience from a companion mission. |
| `level_descriptions_theory`|  Description of your current theoretical level. The smallest one is picked, if you define only level 0 and 2, skill level 1 would show description for level 0 
| `level_descriptions_practice`| Same as level_descriptions_theory, but for practical skill level.
| `companion_combat_rank_factor`   | _(int)_ Affects an NPC's rank when determining the success rate for combat missions. |
| `companion_survival_rank_factor` | _(int)_ Affects an NPC's rank when determining the success rate for survival missions. |
| `companion_industry_rank_factor` | _(int)_ Affects an NPC's rank when determining the success rate for industry missions. |

### Speed Description

```jsonc
{
    "type": "speed_description",
    "id": "mon_speed_centipede",
    "values": [ // (optional)
        {
            // value is mandatory
            "value": 1.40,
            // description is optional
            "descriptions": "Absurdly faster than you", // single description
        },
        {
            "value": 1.00,
            "descriptions": [ // array of descriptions, chosen randomly when called
                "Roughly around the same speed",
                "At a similar pace as you"
            ]
        },
        {
            "value": 0.01,
            "descriptions": [ // array of descriptions, chosen randomly when called
                "Barely moving",
                "Is it even alive?"
            ]
        },
        {
            "value": 0.00, // immobile monsters have it set to zero
            "descriptions": [ "It's immobile" ] // array of descriptions with a single description
        }
    ]
}
```

There won't be any errors on two `values` with the same `value` but avoid it as one of them won't get called.

Currently the ratio for values is `player_tiles_per_turn / monster_speed_rating`. The monster speed rating is their `effective_speed / 100`, their effective speed is equal to the monster speed, but the leap ability increases it by `50`.

Values are checked from highest first, the order they're defined in doesn't matter since they get sorted, but keep them organized anyway.

**Having a value of `0.00`** is important but not necessary, as it's used in case the ratio turns zero for whatever reason ( like monster has the flag `MF_IMMOBILE` ). If the ratio is zero and this value doesn't exist, the returned string will be empty.

### Mood Face
```jsonc
{
    "type": "mood_face",
    "id": "DEFAULT_HORIZONTAL",
    "values": [ // mandatory
        {
            "value": 200, // mandatory
            "face": "<color_green>@w@</color>" // mandatory
        },
        {
            "value": -200,
            "face": "<color_red>XvX</color>" // adding a color is also mandatory
        },
        {
            "value": -201, // morale is clamped [200, -200] on regular gameplay, not necessary unless debugging
            "face": "<color_yellow>@^@</color>"
        }
    ]
}
```

Color is mandatory, or else it won't appear on the sidebar.

`DEFAULT` and `DEFAULT_HORIZONTAL` for the default value, must not be deleted ( modifying is fine ).

`THRESH_TRAIT` and `THRESH_TRAIT_HORIZONTAL` for traits.
Examples:
For `THRESH_BIRD`: `THRESH_BIRD` and `THRESH_BIRD_HORIZONTAL`
For `THRESH_SPIDER`: `THRESH_SPIDER` and `THRESH_SPIDER_HORIZONTAL`

The `id` must be exact as it is hardcoded to look for that.

`HORIZONTAL` means 3 characters width.

### Tool Qualities

Defined in tool_qualities.json.

Format and syntax:
```jsonc
{
  "type": "tool_quality",
  "id": "SAW_W",                      // Unique ID
  "name": { "str": "wood sawing" },   // Description used in tabs in-game when looking at entries with the id
  "usages": [ [ 2, [ "LUMBER" ] ] ]   // Not mandatory.  The special actions that may be performed with the item.
},
```

Examples of various usages syntax:
```jsonc
"usages": [ [ 1, [ "PICK_LOCK" ] ] ]
"usages": [ [ 2, [ "LUMBER" ] ] ]
"usages": [ [ 1, [ "salvage", "inscribe"] ] ]
"usages": [ [ 2, [ "HACKSAW", "saw_barrel" ] ] ]
"usages": [ [ 1, [ "CHOP_TREE", "CHOP_LOGS" ] ], [ 2, [ "LUMBER" ] ] ]
```

The usages line is only required for items that have qualities that allow
special actions on activation. See [Use Actions](#use-actions) for specific
actions and documentation.

IDs of actions and the plaintext action description for the player are defined
in item_actions.json.

Each usage must be defined first by the minimum level of the tool quality that
is required for that action to be possible, then the ID of the action or array
of actions that is possible with that tool quality level or greater.

As shown in the examples, one or more actions for multiple tool levels may be
defined and if multiple levels are defined, those must be defined in a
higher order array.

Comment lines using the normal `"//"` (or `"//1"`, or higher numbers) format are
allowed (see [Comments](#comments)).

Qualities are (non-exclusively) associated with items in the various item
definitions in the json files by adding a `"qualities":` line.
For example: `"qualities": [ [ "ANVIL", 2 ] ],` associates the `ANVIL` quality
at level `2` to the item.

### Traits/Mutations

See [MUTATIONS.md](MUTATIONS.md)

### Trait Migrations

See [MUTATIONS.md](MUTATIONS.md)

### Traps

```jsonc
    "type": "trap",
    "id": "tr_beartrap", // Unique ID
    "name": "bear trap", // In-game name displayed
    "color": "blue",
    "symbol": "^",
    "visibility": 2, // 0 to infinity, 0 means a blatantly obvious trap, the higher, the harder to spot.
    "avoidance": 7, // 0 to infinity, affects how easy it is to dodge a triggered trap. 0 means dead easy, the higher the harder.
    "difficulty": 3, // 0 to 99, 0 means disarming is always successful (e.g funnels or other benign traps), 99 means disarming is impossible.
    "trap_radius": 1, // 0 to infinity, radius of space the trap needs when being deployed.
    "action": "blade", // C++ function that gets run when trap is triggered, usually in trapfunc.cpp
    "map_regen": "microlab_shifting_hall", // a valid overmap id, for map_regen action traps
    "benign": true, // For things such as rollmats, funnels etc. They can not be triggered.
    "always_invisible": true, // Super well hidden traps the player can never detect
    "funnel_radius": 200, // millimeters. The higher the more rain it will capture.
    "comfort": 0, // Same property affecting furniture and terrain
    "floor_bedding_warmth": -500, // Same property affecting furniture and terrain. Also affects how comfortable a resting place this is(affects healing). Vanilla values should not exceed 1000.
    "eocs": [ "EOC_COOL_EOC_TRAP" ], // array of eocs to trigger, only usable with "action": "eocs". The alpha talker is the creature that triggered the trap and the trap's location is passed as a context variable trap_location for use with ranged traps. Items can't currently trigger eoc traps.
    "spell_data": { "id": "bear_trap" }, // data required for trapfunc::spell(), only usable with "action": "spell"
    "trigger_weight": "200 g", // If an item with this weight or more is thrown onto the trap, it triggers. Defaults to 500 grams.
    "drops": [ "beartrap" ], // ID of item spawned when disassembled
    "flags": [ "UNDODGEABLE", "AVATAR_ONLY" ], // UNDODGEABLE means that it can not be dodged, no roll required. AVATAR_ONLY means only the player can trigger this trap.
    "vehicle_data": {
      "damage": 300,
      "sound_volume": 8,
      "sound": "SNAP!",
      "sound_type": "trap",
      "sound_variant": "bear_trap",
      "remove_trap": true,
      "spawn_items": [ "beartrap" ]
    },
    "trigger_message_u": "A bear trap closes on your foot!", // This message will be printed when player steps on a trap
    "trigger_message_npc": "A bear trap closes on <npcname>'s foot!", // This message will be printed when NPC or monster steps on a trap
    "sound_threshold": [5,10] // Optional.  Minimum volume of sound that will trigger this trap.  Defaults to [0,0] (Will not trigger from sound).  If two values [min,max] are provided, trap triggers on a linearly increasing chance depending on volume, from 25% (min) to 100%(max).  To always trigger at some noise, say noise level N, specify as [N,N].  IMPORTANT: Not all traps work with this system.  Make sure to double check and test.
```

### Vehicle Groups


```jsonc
"id":"city_parked", // Unique ID. Must be one continuous word, use underscores if necessary
"vehicles":[        // List of potential vehicle ID's. Chance of a vehicle spawning is X/T, where
  ["suv", 600],     //    X is the value linked to the specific vehicle and T is the total of all
  ["pickup", 400],  //    vehicle values in a group
  ["car", 4700],
  ["road_roller", 300]
]
```

### Vehicle Parts

Vehicle components when installed on a vehicle.

```jsonc
"id": "wheel",                // Unique identifier, must not contain a # symbol
"name": "wheel",              // Displayed name
"looks_like": "small_wheel",  // (Optional) hint to tilesets if this part has no tile,
                              // use the looks_like tile.
"bonus": 100,                 // Function depends on part type:
                              // seatbelt part is in "str" (non-functional #30239)
                              // muffler part is % noise reduction
                              // horn part volume
                              // light part intensity
                              // recharger part charging speed in watts
                              // funnel part water collection area in mm^2
"color": "dark_gray",         // Color used when part is working
"broken_color": "light_gray", // Color used when part is broken
"location": "fuel_source",    // Optional. One of the checks used when determining if a part 
                              // can be installed on a given tile. A part cannot be installed
                              // if any existing part occupies the same location.
"damage_modifier": 50,        // (Optional, default = 100) Dealt damage multiplier when this
                              // part hits something, as a percentage. Higher = more damage to
                              // creature struck
"durability": 200,            // How much damage the part can take before breaking
"description": "A wheel.",    // A description of this vehicle part when installing it
"fuel_type": "diesel",        // (Optional, default = "NULL") Type of fuel/ammo the part consumes,
                              // as an item id
"epower": -10                 // The electrical power use of the part, in watts.
                              // Negative values mean power is consumed, positive values mean power
                              // is generated.  Power consumption usually also requires the
                              // ENABLED_DRAINS_EPOWER flag and for the item to be turned on.
                              // Solar panel power gneration is modified by sun angle.
                              // When sun is at 90 degrees the panel produces the full epower.
"item": "wheel",              // The item used to install this part, and the item obtained when
                              // removing this part.
"remove_as": "solar_panel",   // Overrides "item", item returned when removing this part.
"difficulty": 4,              // Your mechanics skill must be at least this level to install this part
"breaks_into" : [             // When the vehicle part is destroyed, items from this item group
                              // (see ITEM_SPAWN.md) will be spawned around the part on the ground.
  {"item": "scrap", "count": [0,5]} // instead of an array, this can be an inline item group,
],
"breaks_into" : "some_item_group", // or just the id of an item group.
"flags": [                    // Flags associated with the part
     "EXTERNAL", "MOUNT_OVER", "WHEEL", "MOUNT_POINT", "VARIABLE_SIZE"
],
"requirements": {             // (Optional) Special installation, removal, or repair requirements
                              // for the part.  Each field consists of an object, with fields
                              // "skills", "time", and "using".
  "install": {
    "skills": [ [ "mechanics", 1 ] ], // "skills" is a list of lists, with each list being a skill
                              // name and skill level.
    "time": "200 s",          // "time" is a string specifying the time to perform the action.
    "using": [ [ "vehicle_screw", 1 ] ] // "using" is a list of list, with each list being a
                              // crafting requirement.
  },
  "removal": { "skills": [ [ "mechanics", 1 ] ], "time": "200 s", "using": [ [ "vehicle_screw", 1 ] ] },
  "repair": { "skills": [ [ "mechanics", 1 ] ], "time": "20 s", "using": [ [ "adhesive", 1 ] ] }
},
"control_requirements": {     // (Optional) Control requirements of the vehicle this part is installed.
  "air": {                    // Requirements of flying in air.
    "proficiencies": [ "prof_helicopter_pilot" ], // "proficiencies" is a list of proficiency names.
  },
  "land": {                    // Requirements of running on ground.
    "skills": [ [ "driving", 1 ] ], // "skills" is a list of lists, with each list being a skill
                              // name and skill level.
  }
},
"pseudo_tools" : [            // Crafting tools provided by this part
  { "id": "hotplate", "hotkey": "h" },
  { "id": "pot" }
],
"folded_volume": "750 ml", // volume this vpart takes in folded form, undefined or null disables folding
"folding_tools": [ "needle_curved" ], // tool itype_ids required for folding
"folding_time": "100 seconds", // time to fold this part
"unfolding_tools": [ "hand_pump" ], // tool itype_ids required for unfolding
"unfolding_time": "150 seconds", // time to unfold this part
"damage_reduction" : {        // Flat reduction of damage; see "Part Resistance". If not specified, set to zero
    "all" : 10,
    "physical" : 5
},
"qualities": [ [ "SELF_JACK", 17 ] ], // (Optional) A list of lists, with each list being a tool
                              // quality and the quality level, that the vehicle part provides.
                              // Only the "LIFT", "JACK", and "SELF_JACK" qualities are valid.
"transform_terrain": {        // (Optional) This part can transform terrain, like a plow.
  "pre_flags": [ "PLOWABLE" ], // (Optional) List of flags for the terrain that can be transformed.
  "post_terrain": "t_dirtmound", // (Optional*) The resulting terrain.
  "post_furniture": "f_boulder", // (Optional*) The resulting furniture.
  "post_field": "fd_fire",    // (Optional*) The resulting field.
  "post_field_intensity": 10, // (Mandatory if post_field is specified) The field's intensity.
  "post_field_age": "20 s"    // (Mandatory if post_field is specified) The field's time to live.
                              // *One of "post_terrain", "post_furniture", or "post_field" is required.
},
"variants_bases": [ // variant bases to generate (see below)
  { "id": "scooter", "label": "Scooter" },
  { "id": "bike", "label": "bike" }
],
"variants": [
    {
        "id": "front",         // variant id (must be unique in this part)
        "label": "Front",      // label to display for ui
        "symbols": "oooooooo", // symbols when part isn't broken
        "symbols_broken": "x"  // symbols when part is broken
    },
    { "id": "rear", "label": "Rear", "symbols": "o", "symbols_broken": "x" }
]
```

#### Symbols and Variants
Vehicle parts can have cosmetic variants that use different symbols and tileset sprites.  They are declared by the "variants" object.  Variants are used in the vehicle prototype as a suffix following the part id (ie `id#variant`), for example `"frame#nw"` or `"halfboard#cover"`.

`symbols` and `symbols_broken` can be either a string of 1 character (A 1 character string is effectively 8 of that characters) or 8 characters long. The length is measured in console characters. An 8 character string represents the 8 symbols used for parts which can rotate; `abcdefgh` will put `a` when part is rotated north, `b` for NW, `c` for west, `d` for SW etc.

A subset of unicode box drawing characters is supported as symbols: `│ ─ ┼ ┌ ┐ ┘ └`, thick vertical and thick horizontal lines `┃ ━` are partially supported, they're rendered as `H` and `=` because there are no equivalents in curses ACS encoding.

Variant bases are for generating extra variants from the specified ones, in the example above will make part loader perform cartesian product between each base and each of the variants, making finalized variants list the following: `[ "front", "rear", "scooter_front", "scooter_rear", "bike_front", "bike_rear" ]`, the base's `label` field is appended to the variant's label.

For more details on how tilesets interact with variants and ids look into [VEHICLES_JSON.md](VEHICLES_JSON.md#part-variants) "Part Variants" section.

Unless specified as optional, the following fields are mandatory for parts with appropriate flag and are ignored otherwise.
#### The following optional fields are specific to CARGO parts.
```jsonc
"size": "400 L",              // for parts with "CARGO" flag the capacity in liters
"cargo_weight_modifier": 33,  // (Optional, default = 100) Multiplies cargo weight by this percentage.
```

#### The following optional fields are specific to ENGINEs.
```jsonc
"power": "15000 W"            // Engine motive power in watts.
"energy_consumption": "55 W"  // Engine power consumption at maximum power in watts.  Defaults to
                              // electrical power and the E_COMBUSTION flag turns it to thermal
                              // power produced from fuel_type.  Should always be larger than "power".
"m2c": 50,                    // The ratio of safe power to maximum power.
"backfire_threshold": 0.5,    // (Optional, default = 0) The engine will backfire (producing noise
                              // and smoke if the ratio of damaged HP to max HP is below this value.
"backfire_freq": 20,          // (Optional, default = 0) One in X chance of a backfire if the
                              // ratio of damaged HP to max HP is below the backfire_threshold.
"noise_factor": 15,           // (Optional, default = 0). Multiple engine power by this number to
                              // determine noise.
"damaged_power_factor": 0.5,  // (Optional, default = 0) If more than 0, power when damaged is
                              // scaled to power * ( damaged_power_factor +
                              // ( 1 - damaged_power_factor ) * ( damaged HP / max HP )
"muscle_power_factor": 0,     // (Optional, default = 0) Increases engine power by
                              // avatar (ST - 8) * muscle_power_factor.
"exclusions": [ "souls" ]     // (Optional, defaults to empty). A list of words. A new engine can't
                              // be installed on the vehicle if any engine on the vehicle shares a
                              // word from exclusions.
"fuel_options": [ "soul", "black_soul" ] // (Optional field, defaults to fuel_type).  A list of
                              // item_ids. An engine can be fueled by any fuel type in its
                              // fuel_options.  If provided, it overrides fuel_type and should
                              // include the fuel in fuel_type.
                              // To be a fuel an item needs to be made of only one material,
                              // this material has to produce energy, *ie* have a `data_fuel` entry,
                              // and it needs to have consumable charges.
```

#### The following optional fields are specific to WHEELs.
```jsonc
"wheel_offroad_rating": 0.5,  // multiplier of wheel performance offroad
"wheel_terrain_modifiers": { "FLAT": [ 0, 5 ], "ROAD": [ 0, 2 ] }, // see below
"contact_area": 153,          // The surface area of the wheel in contact with the ground under
                              // normal conditions in cm^2.  Wheels with higher contact area
                              // perform better off-road.
"rolling_resistance": 1.0,    // The "squishiness" of the wheel, per SAE standards.  Wheel rolling
                              // resistance increases vehicle drag linearly as vehicle weight
                              // and speed increase.
"diameter": 8,                // diameter of wheel (in inches)
"width": 4,                   // width of the wheel (in inches)
```

`wheel_terrain_modifiers` field provides a way to modify wheel traction according to the flags set on terrain tile under each wheel.

The key is one of the terrain flags, the list of flags can be found in [JSON_FLAGS.md](JSON_FLAGS.md#furniture-and-terrain).

The value expects an array of length 2. The first element is a modifier override applied when wheel is on the flagged terrain, the second element is an additive modifier penalty applied when wheel is NOT on flagged terrain, values of 0 are ignored. The modifier is applied over a base value provided by `map::move_cost_ter_furn`.

Examples:
* Standard `wheel` has the field set to `{ "FLAT": [ 0, 4 ], "ROAD": [ 0, 2 ] }`. If wheel is not on terrain flagged `FLAT` then the traction is 1/4 of base value. If not on terrain flagged `ROAD` then it's 1/2 of base value. If neither flag is present then traction will be 1/6 of base value. If terrain is flagged with both `ROAD` and `FLAT` then the base value from `map::move_cost_ter_furn` is used.
* `rail_wheel` has the field set to `{ "RAIL": [ 2, 8 ] }`. If wheel is on terrain flagged `RAIL` the traction is overridden to be 1/2 of value calculated by `map::move_cost_ter_furn`, this value is the first element and considered an override, so if there had been modifiers applied prior to this they are ignored. If on terrain not flagged with `RAIL` then traction will be 1/8 of base value.


#### The following optional fields are specific to ROTORs.
```jsonc
"rotor_diameter": 15,         // Rotor diameter in meters.  Larger rotors provide more lift. This also determines the size of the rotor during collisions.
"bonus": 5,                   // Adds additional rotor diameter during lift calculations. This does not affect rotor collision. This is useful for things like ducted fans or vtol engines that have good lift but smaller danger zones.
```

#### The following optional fields are specific to WORKBENCHes.
These values apply to crafting tasks performed at the WORKBENCH.
```jsonc
"multiplier": 1.1,            // Crafting speed multiplier.
"mass": 1000000,              // Maximum mass in grams of a completed craft that can be crafted.
"volume": "20L",              // Maximum volume (as a string) of a completed craft that can be craft.
```

#### The following optional fields are specific to SEATs.
```jsonc
"comfort": 3,                 // (Optional, default=0). Sleeping comfort as for terrain/furniture.
"floor_bedding_warmth": 300,  // (Optional, default=0). Bonus warmth as for terrain/furniture. Also affects how comfortable a resting place this is(affects healing). Vanilla values should not exceed 1000.
"bonus_fire_warmth_feet": 200,// (Optional, default=0). Bonus fire warmth as for terrain/furniture.
```

#### The following optional field describes pseudo tools for any part.
Crafting stations (e.g. kitchen, welding rigs etc) have tools that they provide as part
of forming the inventory for crafting as well as providing menu items when `e`xamining
the vehicle tile.
Following example array gives the vpart a pot as passive tool for crafting because it has no hotkey defined.
It also has a hotplate that can be activated by examining it with `e` then `h` on the part's vehicle tile.
```jsonc
"pseudo_tools" : [
  { "id": "hotplate", "hotkey": "h" },
  { "id": "pot" }
],
```

### Part Resistance
Damage resistance values, used by:
- `armor` of [`"type": "body_part"`](#body_parts)
- `damage_reduction` of [`"type": "vehicle_part"`](#vehicle-parts)

```jsonc
"all" : 0.0f,        // Initial value of all resistances, overridden by more specific types
"physical" : 10,     // Initial value for bash, cut and stab
"non_physical" : 10, // Initial value for acid, heat, cold, electricity and biological
"biological" : 0.2f, // Resistances to specific types. Those override the general ones.
"bash" : 3,
"cut" : 3,
"acid" : 3,
"stab" : 3,
"heat" : 3,
"cold" : 3,
"electric" : 3
```

### Vehicle Placement
```jsonc
"id":"road_straight_wrecks",  // Unique ID. Must be one continuous word, use underscores if necessary
"locations":[ {               // List of potential vehicle locations. When this placement is used, one of those locations will be chosen at random.
  "x" : [0,19],               // The x placement. Can be a single value or a range of possibilities.
  "y" : 8,                    // The y placement. Can be a single value or a range of possibilities.
  "facing" : [90,270]         // The facing of the vehicle. Can be a single value or an array of possible values.
} ]
```

### Vehicle Spawn

```jsonc
"id":"default_city",            // Unique ID. Must be one continuous word, use underscores if necessary
"spawn_types":[ {       // List of spawntypes. When this vehicle_spawn is applied, it will choose from one of the spawntypes randomly, based on the weight.
  "description" : "Clear section of road",           //    A description of this spawntype
  "weight" : 33,          //    The chance of this spawn type being used.
  "vehicle_function" : "jack-knifed_semi", // This is only needed if the spawntype uses a built-in json function.
  "vehicle_json" : {      // This is only needed for a json-specified spawntype.
  "vehicle" : "car",      // The vehicle or vehicle_group to spawn.
  "placement" : "%t_parked",  // The vehicle_placement to use when spawning the vehicle. This is not needed if the x, y, and facing are specified.
  "x" : [0,19],     // The x placement. Can be a single value or a range of possibilities. Not needed if placement is specified.
  "y" : 8,   // The y placement. Can be a single value or a range of possibilities. Not needed if placement is specified.
  "facing" : [90,270], // The facing of the vehicle. Can be a single value or an array of possible values. Not needed if placement is specified.
  "number" : 1, // The number of vehicles to spawn.
  "fuel" : -1, // The fuel of the new vehicles. Defined in percentage. 0 is empty, 100 is full tank, -1 is random from 7% to 35% (default).
  "status" : 1  // The status of the new vehicles. -1 = light damage (default), 0 = undamaged, 1 = disabled: destroyed seats, controls, tanks, tires, OR engine.
} } ]
```

### Vehicles

See also [VEHICLES_JSON.md](VEHICLES_JSON.md)

```jsonc
"id": "shopping_cart",                  // Internally-used name.
"name": "Shopping Cart",                // Display name, subject to i18n.
"blueprint": "#",                       // Preview of vehicle - ignored by the code, so use only as documentation
"parts": [                              // Parts list
    {"x": 0, "y": 0, "part": "box"},    // Part definition, positive x direction is to the left, positive y is to the right
    {"x": 0, "y": 0, "part": "casters"} // See vehicle_parts.json for part ids
]
                                        /* Important! Vehicle parts must be defined in the
                                         * same order you would install
                                         * them in the game (ie, frames and mount points first).
                                         * You also cannot break the normal rules of installation
                                         * (you can't stack non-stackable part flags). */
```

### Weakpoint Sets
A thin container for weakpoint definitions. The only unique fields for this object are `"id"` and `"type"`. The `"weakpoints"` array contains weakpoints that are defined the same way as in monster definitions. See [Weakpoints](MONSTERS.md#weakpoints) for details.

```jsonc
{
  "type": "weakpoint_set",
  "id": "wps_zombie_headshot",
  "weakpoints": [
    {
      "id": "wp_head_stun",
      "name": "the head",
      "coverage": 5,
      "crit_mult": { "all": 1.1 },
      "armor_mult": { "physical": 0.75 },
      "difficulty": { "melee": 1, "ranged": 3 },
      "effects": [
        {
          "effect": "stunned",
          "duration": [ 1, 2 ],
          "chance": 5,
          "message": "The %s is stunned!",
          "damage_required": [ 1, 10 ]
        },
        {
          "effect": "stunned",
          "duration": [ 1, 2 ],
          "chance": 25,
          "message": "The %s is stunned!",
          "damage_required": [ 11, 100 ]
        }
      ]
    }
  ]
}
```

Weakpoint sets are applied to a monster using the monster's `"weakpoint_sets"` field. Each subsequent weakpoint set overwrites weakpoints with the same id from the previous set. This allows hierarchical sets that can be applied from general -> specific, so that general weakpoint sets can be reused for many different monsters, and more specific sets can override some general weakpoints for specific monsters. For example:
```jsonc
"//": "(in MONSTER type)",
"weakpoint_sets": [ "humanoid", "zombie_headshot", "riot_gear" ]
```
In the example above, the `"humanoid"` weakpoint set is applied as a base, then the `"zombie_headshot"` set overwrites any previously defined weakpoints with the same id (ex: "wp_head_stun"). Then the `"riot_gear"` set overwrites any matching weakpoints from the previous sets with armour-specific weakpoints. Finally, if the monster type has an inline `"weakpoints"` definition, those weakpoints overwrite any matching weakpoints from all sets.

Weakpoints only match if they share the same id, so it's important to define the weakpoint's id field if you plan to overwrite previous weakpoints.

# `json/` JSONs

### Harvest

```jsonc
{
    "id": "jabberwock",
    "type": "harvest",
    "leftovers": "ruined_candy",
    "message": "You messily hack apart the colossal mass of fused, rancid flesh, taking note of anything that stands out.",
    "entries": [
      { "drop": "meat_tainted", "type": "flesh", "mass_ratio": 0.33 },
      { "drop": "fat_tainted", "type": "flesh", "mass_ratio": 0.1 },
      { "drop": "jabberwock_heart", "base_num": [ 0, 1 ], "scale_num": [ 0.6, 0.9 ], "max": 3, "type": "flesh" }
    ],
},
{
  "id": "mammal_large_fur",
  "//": "drops large stomach",
  "type": "harvest",
  "entries": [
    { "drop": "meat", "type": "flesh", "mass_ratio": 0.32 },
    { "drop": "meat_scrap", "type": "flesh", "mass_ratio": 0.01 },
    { "drop": "lung", "type": "flesh", "mass_ratio": 0.0035 },
    { "drop": "liver", "type": "offal", "mass_ratio": 0.01 },
    { "drop": "brain", "type": "flesh", "mass_ratio": 0.005 },
    { "drop": "sweetbread", "type": "flesh", "mass_ratio": 0.002 },
    { "drop": "kidney", "type": "offal", "mass_ratio": 0.002 },
    { "drop": "stomach_large", "scale_num": [ 1, 1 ], "max": 1, "type": "offal" },
    { "drop": "bone", "type": "bone", "mass_ratio": 0.15 },
    { "drop": "sinew", "type": "bone", "mass_ratio": 0.00035 },
    { "drop": "fur", "type": "skin", "mass_ratio": 0.02 },
    { "drop": "fat", "type": "flesh", "mass_ratio": 0.07 }
  ]
},
{
  "id": "CBM_SCI",
  "type": "harvest",
  "entries": [
    {
      "drop": "bionics_sci",
      "type": "bionic_group",
      "flags": [ "FILTHY", "NO_STERILE", "NO_PACKED" ],
      "faults": [ "fault_bionic_salvaged" ]
    },
    { "drop": "meat_tainted", "type": "flesh", "mass_ratio": 0.25 },
    { "drop": "fat_tainted", "type": "flesh", "mass_ratio": 0.08 },
    { "drop": "bone_tainted", "type": "bone", "mass_ratio": 0.1 }
  ]
},
```

#### `id`

Unique id of the harvest definition.

#### `type`

Should always be `harvest` to mark the object as a harvest definition.

#### `message`

Optional message to be printed when a creature using the harvest definition is butchered. May be omitted from definition.

#### `entries`

Array of dictionaries defining possible items produced on butchering and their likelihood of being produced.
`drop` value should be the `id` string of the item to be produced.

`type` value should refer to an existing `harvest_drop_type` associated with body part the item comes from.
    Acceptable values are as follows:
    `flesh`: the "meat" of the creature.
    `offal`: the "organs" of the creature. these are removed when field dressing.
    `skin`: the "skin" of the creature. this is what is ruined while quartering.
    `bone`: the "bones" of the creature. you will get some amount of these from field dressing, and the rest of them from butchering the carcass.
    `mutagen`: an item from harvested mutagenic samples obtained from dissection.
    `mutagen_group`: an item group that can produce an item from harvested mutagenic samples obtained from dissection.
    `bionic`: an item gained by dissecting the creature. not restricted to CBMs.
    `bionic_group`: an item group that will give an item by dissecting a creature. not restricted to groups containing CBMs.

`flags` value should be an array of strings.  These flags will be added to the items of that entry upon harvesting.

`faults` value should be an array of `fault_id` strings.  These faults will be added to the items of that entry upon harvesting.

For every `type` other then those with "dissect_only" (see below) the following entries scale the results:
    `base_num` value should be an array with two elements in which the first defines the minimum number of the corresponding item produced and the second defines the maximum number.
    `scale_num` value should be an array with two elements, increasing the minimum and maximum drop numbers respectively by element value * survival skill.
    `max` upper limit after `base_num` and `scale_num` are calculated using
    `mass_ratio` value is a multiplier of how much of the monster's weight comprises the associated item. to conserve mass, keep between 0 and 1 combined with all drops. This overrides `base_num`, `scale_num` and `max`

For `type`s with "dissect_only" (see below), the following entries can scale the results:
    `max` this value (in contrary to `max` for other `type`s) corresponds to maximum butchery roll that will be passed to check_butcher_cbm() in activity_handlers.cpp; view check_butcher_cbm() to see corresponding distribution chances for roll values passed to that function

#### `leftovers`

itype_id of the item dropped as leftovers after butchery or when the monster is gibbed.  Default as "ruined_chunks".

### Harvest Drop Type
```jsonc
{
  "type": "harvest_drop_type",
  "id": "mutagen",
  "dissect_only": true,
  "group": false,
  "harvest_skills": [ "firstaid", "chemistry" ],
  "msg_fielddress_fail": "harvest_drop_mutagen_field_dress",
  "msg_fielddress_success": "",
  "msg_butcher_fail": "harvest_drop_mutagen_butcher",
  "msg_butcher_success": "",
  "msg_dissect_fail": "harvest_drop_mutagen_dissect_failed",
  "msg_dissect_success": ""
}
```

Harvest drop types are used in harvest drop entries to control how the drop is processed. `dissect_only` only allows the drop to be produced when dissecting. `group` indicates that an associated `drop` refers to an item group instead of a single item type.

`harvest_skills` refers to the id of skills that affect the yields of this harvest drop type. If omitted, this defaults to the survival skill. For example, dissecting a zomborg for CBMs will produce better results when the "electronics" and "firstaid" skills are high. `harvest_skills` can be either a single string (just one skill) or an array of strings.

`msg_<butcher_type>_<result>` refers to a snippet to be printed when the specified butcher type either succeeds or fails. Currently, the following message types are available:
- `"msg_fielddress_fail"`
- `"msg_fielddress_success"`
- `"msg_butcher_fail"`
- `"msg_butcher_success"`
- `"msg_dissect_fail"`
- `"msg_dissect_success"`

### Weapon Category

```jsonc
{
    "type": "weapon_category",
    "id": "WEAP_CAT",
    "name": "Weapon Category",
    "proficiencies": [ "prof_baz" ]
}
```

`proficiencies` is a list of proficiencies that may apply bonuses when using weapons with this category. See the proficiency documentation for more details.

### Connect group definitions

Connect groups can be used by id in terrain and furniture `connect_groups`, `connects_to` and `rotates_to` properties.

Examples from the actual definitions:

**`group_flags`**: Flags that imply that terrain or furniture is added to this group.

**`connects_to_flags`**: Flags that imply that terrain or furniture connects to this group.

**`rotates_to_flags`**: Flags that imply that terrain or furniture rotates to this group.

```jsonc
[
  {
    "type": "connect_group",
    "id": "WALL",
    "group_flags": [ "WALL", "CONNECT_WITH_WALL" ],
    "connects_to_flags": [ "WALL", "CONNECT_WITH_WALL" ]
  },
  {
    "type": "connect_group",
    "id": "CHAINFENCE"
  },
  {
    "type": "connect_group",
    "id": "INDOORFLOOR",
    "group_flags": [ "INDOORS" ],
    "rotates_to_flags": [ "WINDOW", "DOOR" ]
  }
]
```

### Furniture

```jsonc
{
    "type": "furniture",
    "id": "f_toilet",
    "name": "toilet",
    "symbol": "&",
    "looks_like": "chair",
    "color": "white",
    "move_cost_mod": 2,
    "keg_capacity": "60 L",
    "deployed_item": "plastic_sheet",
    "light_emitted": 5,
    "required_str": 18,
    "flags": [ "TRANSPARENT", "BASHABLE", "FLAMMABLE_HARD" ],
    "connect_groups" : [ "WALL" ],
    "connects_to" : [ "WALL" ],
    "rotates_to" : [ "INDOORFLOOR" ],
    "crafting_pseudo_item": "anvil",
    "examine_action": "toilet",
    "close": "f_foo_closed",
    "open": "f_foo_open",
    "lockpick_result": "f_safe_open",
    "lockpick_message": "With a click, you unlock the safe.",
    "bash": "TODO",
    "deconstruct": "TODO",
    "max_volume": "1000 L",
    "examine_action": "workbench",
    "workbench": { "multiplier": 1.1, "mass": 10000, "volume": "50L" },
    "boltcut": {
      "result": "f_safe_open",
      "duration": "1 seconds",
      "message": "The safe opens.",
      "sound": "Gachunk!",
      "byproducts": [ { "item": "scrap", "count": 3 } ]
    },
    "hacksaw": {
      "result": "f_safe_open",
      "duration": "12 seconds",
      "message": "The safe is hacksawed open!",
      "sound": "Gachunk!",
      "byproducts": [ { "item": "scrap", "count": 13 } ]
    },
    "oxytorch": {
      "result": "f_safe_open",
      "duration": "30 seconds",
      "message": "The safe opens!",
      "byproducts": [ { "item": "scrap", "count": 13 } ]
    },
    "prying": {
      "result": "f_crate_o",
      "message": "You pop open the crate.",
      "prying_data": {
        "difficulty": 6,
        "prying_level": 1,
        "noisy": true,
        "failure": "You pry, but can't pop open the crate."
      }
    }
}
```

#### `type`

Fixed string, must be `furniture` to identify the JSON object as such.

`"id", "name", "symbol", "looks_like", "color", "bgcolor", "max_volume", "open", "close", "bash", "deconstruct", "examine_action", "flags"`

Same as for terrain, see below in the chapter "Common to furniture and terrain".

#### `keg_capacity`

(Optional) Determines capacity of some furnitures with liquid storage that have hardcoded interactions.

#### `deployed_item`

(Optional) Item id string used to create furniture. Allows player to interact with the furniture to 'take it down' (transform it to item form).

#### `lockpick_result`

(Optional) When the furniture is successfully lockpicked, this is the furniture it will turn into.

#### `lockpick_message`

(Optional) When the furniture is successfully lockpicked, this is the message that will be printed to the player. When it is missing, a generic `"The lock opens…"` message will be printed instead.

#### `light_emitted`

(Optional) How much light the furniture produces.  10 will light the tile it's on brightly, 15 will light that tile and the tiles around it brightly, as well as slightly lighting the tiles two tiles away from the source.
For examples: An overhead light is 120, a utility light, 240, and a console, 10.

#### `boltcut`
(Optional) Data for using with an bolt cutter.
```jsonc
"boltcut": {
    "result": "furniture_id", // (optional) furniture it will become when done, defaults to f_null
    "duration": "1 seconds", // ( optional ) time required for bolt cutting, default is 1 second
    "message": "You finish cutting the metal.", // ( optional ) message that will be displayed when finished
    "sound": "Gachunk!", // ( optional ) description of the sound when finished
    "byproducts": [ // ( optional ) list of items that will be spawned when finished
        {
            "item": "item_id",
            "count": 100 // exact amount
        },
        {
            "item": "item_id",
            "count": [ 10, 100 ] // random number in range ( inclusive )
        }
    ]
}
```

#### `hacksaw`
(Optional) Data for using with an hacksaw.
```jsonc
"hacksaw": {
    "result": "furniture_id", // (optional) furniture it will become when done, defaults to f_null
    "duration": "1 seconds", // ( optional ) time required for hacksawing, default is 1 second
    "message": "You finish cutting the metal.", // ( optional ) message that will be displayed when finished
    "byproducts": [ // ( optional ) list of items that will be spawned when finished
        {
            "item": "item_id",
            "count": 100 // exact amount
        },
        {
            "item": "item_id",
            "count": [ 10, 100 ] // random number in range ( inclusive )
        }
    ]
}
```

#### `oxytorch`
(Optional) Data for using with an oxytorch.
```jsonc
oxytorch: {
    "result": "furniture_id", // (optional) furniture it will become when done, defaults to f_null
    "duration": "1 seconds", // ( optional ) time required for oxytorching, default is 1 second
    "message": "You quickly cut the metal", // ( optional ) message that will be displayed when finished
    "byproducts": [ // ( optional ) list of items that will be spawned when finished
        {
            "item": "item_id",
            "count": 100 // exact amount
        },
        {
            "item": "item_id",
            "count": [ 10, 100 ] // random number in range ( inclusive )
        }
    ]
}
```

#### `prying`
(Optional) Data for using with prying tools
```jsonc
"prying": {
    "result": "furniture_id", // (optional) furniture it will become when done, defaults to f_null
    "duration": "1 seconds", // (optional) time required for prying, default is 1 second
    "message": "You finish prying the door.", // (optional) message that will be displayed when finished prying successfully
    "byproducts": [ // (optional) list of items that will be spawned when finished successfully
        {
            "item": "item_id",
            "count": 100 // exact amount
        },
        {
            "item": "item_id",
            "count": [ 10, 100 ] // random number in range inclusive
        }
    ],
    "prying_data": {
        "prying_nails": false, // (optional, default false) if set to true, ALL fields below are ignored
        "difficulty": 0, // (optional, default 0) base difficulty of prying action
        "prying_level": 0, // (optional, default 0) minimum prying level tool needs to have
        "noisy": false, // (optional, default false) makes noise when successfully prying
        "alarm": false, // (optional) has an alarm, on success will trigger the police
        "breakable": false, // (optional) has a chance to trigger the break action on failure
        "failure": "You try to pry the window but fail." // (optional) failure message
    }
}
```

#### `required_str`

Strength required to move the furniture around. Negative values indicate an unmovable furniture.

#### `crafting_pseudo_item`

(Optional) Id of an item (tool) that will be available for crafting when this furniture is range (the furniture acts as an item of that type).

#### `workbench`

(Optional) Can craft here.  Must specify a speed multiplier, allowed mass, and allowed volume.  Mass/volume over these limits incur a speed penalty.  Must be paired with a `"workbench"` `examine_action` to function.

#### `plant_data`

(Optional) This is a plant. Must specify a plant transform, and a base depending on context. You can also add a harvest or growth multiplier if it has the `GROWTH_HARVEST` flag.

#### `surgery_skill_multiplier`

(Optional) Surgery skill multiplier (float) applied by this furniture to survivor standing next to it for the purpose of surgery.


### Terrain

```jsonc
{
    "type": "terrain",
    "id": "t_spiked_pit",
    "name": "spiked pit",
    "symbol": "0",
    "looks_like": "pit",
    "color": "ltred",
    "move_cost": 10,
    "light_emitted": 10,
    "trap": "spike_pit",
    "max_volume": "1000 L",
    "flags": ["TRANSPARENT", "DIGGABLE"],
    "connect_groups" : [ "WALL" ],
    "connects_to" : [ "WALL" ],
    "rotates_to" : [ "INDOORFLOOR" ],
    "close": "t_foo_closed",
    "open": "t_foo_open",
    "lockpick_result": "t_door_unlocked",
    "lockpick_message": "With a click, you unlock the door.",
    "bash": "TODO",
    "deconstruct": "TODO",
    "alias": "TODO",
    "harvestable": "blueberries",
    "transforms_into": "t_tree_harvested",
    "allowed_template_ids": [ "standard_template_construct", "debug_template", "afs_10mm_smart_template" ],
    "shoot": { "reduce_damage": [ 2, 12 ], "reduce_damage_laser": [ 0, 7 ], "destroy_damage": [ 40, 120 ], "no_laser_destroy": true },
    "harvest_by_season": [
      { "seasons": [ "spring", "autumn" ], "id": "burdock_harv" },
      { "seasons": [ "summer" ], "id": "burdock_summer_harv" }
    ],
    "liquid_source": { "id": "water", "min_temp": 7.8, "count": [ 24, 48 ] },
    "roof": "t_roof",
    "examine_action": "pit",
    "boltcut": {
      "result": "t_door_unlocked",
      "duration": "1 seconds",
      "message": "The door opens.",
      "sound": "Gachunk!",
      "byproducts": [ { "item": "scrap", "2x4": 3 } ]
    },
    "hacksaw": {
      "result": "t_door_unlocked",
      "duration": "12 seconds",
      "message": "The door is hacksawed open!",
      "sound": "Gachunk!",
      "byproducts": [ { "item": "scrap", "2x4": 13 } ]
    },
    "oxytorch": {
      "result": "t_door_unlocked",
      "duration": "60 seconds",
      "message": "The door opens!",
      "byproducts": [ { "item": "scrap", "count": 10 } ]
    },
    "prying": {
      "result": "t_fence_post",
      "duration": "30 seconds",
      "message": "You pry out the fence post.",
      "byproducts": [ { "item": "nail", "count": 6 }, { "item": "2x4", "count": 3 } ],
      "prying_data": { "prying_nails": true }
    }
}
```

#### `type`

Fixed string, must be "terrain" to identify the JSON object as such.

`"id", "name", "symbol", "looks_like", "color", "bgcolor", "max_volume", "open", "close", "bash", "deconstruct", "examine_action", "flags"`

Same as for furniture, see below in the chapter "Common to furniture and terrain".

#### `heat_radiation`

(Optional) Heat emitted for a terrain. A value of 0 means no fire (i.e, same as not having it). A value of 1 equals a fire of intensity of 1.

#### `light_emitted`

(Optional) How much light the terrain emits. 10 will light the tile it's on brightly, 15 will light that tile and the tiles around it brightly, as well as slightly lighting the tiles two tiles away from the source.
For examples: An overhead light is 120, a utility light, 240, and a console, 10.

#### `lockpick_result`

(Optional) When the terrain is successfully lockpicked, this is the terrain it will turn into.

#### `lockpick_message`

(Optional) When the terrain is successfully lockpicked, this is the message that will be printed to the player. When it is missing, a generic `"The lock opens…"` message will be printed instead.

#### `trap`

(Optional) Id of the built-in trap of that terrain.

For example the terrain `t_pit` has the built-in trap `tr_pit`. Every tile in the game that has the terrain `t_pit` also has, therefore, an implicit trap `tr_pit` on it. The two are inseparable (the player can not deactivate the built-in trap, and changing the terrain will also deactivate the built-in trap).

A built-in trap prevents adding any other trap explicitly (by the player and through mapgen).

#### `boltcut`
(Optional) Data for using with an bolt cutter.
```jsonc
"boltcut": {
    "result": "ter_id", // terrain it will become when done
    "duration": "1 seconds", // ( optional ) time required for bolt cutting, default is 1 second
    "message": "You finish cutting the metal.", // ( optional ) message that will be displayed when finished
    "sound": "Gachunk!", // ( optional ) description of the sound when finished
    "byproducts": [ // ( optional ) list of items that will be spawned when finished
        {
            "item": "item_id",
            "count": 100 // exact amount
        },
        {
            "item": "item_id",
            "count": [ 10, 100 ] // random number in range ( inclusive )
        }
    ]
}
```

#### `hacksaw`
(Optional) Data for using with an hacksaw.
```jsonc
"hacksaw": {
    "result": "terrain_id", // terrain it will become when done
    "duration": "1 seconds", // ( optional ) time required for hacksawing, default is 1 second
    "message": "You finish cutting the metal.", // ( optional ) message that will be displayed when finished
    "byproducts": [ // ( optional ) list of items that will be spawned when finished
        {
            "item": "item_id",
            "count": 100 // exact amount
        },
        {
            "item": "item_id",
            "count": [ 10, 100 ] // random number in range ( inclusive )
        }
    ]
}
```

#### `oxytorch`
(Optional) Data for using with an oxytorch.
```jsonc
oxytorch: {
    "result": "terrain_id", // terrain it will become when done
    "duration": "1 seconds", // ( optional ) time required for oxytorching, default is 1 second
    "message": "You quickly cut the bars", // ( optional ) message that will be displayed when finished
    "byproducts": [ // ( optional ) list of items that will be spawned when finished
        {
            "item": "item_id",
            "count": 100 // exact amount
        },
        {
            "item": "item_id",
            "count": [ 10, 100 ] // random number in range ( inclusive )
        }
    ]
}
```

#### `prying`
(Optional) Data for using with prying tools
```jsonc
"prying": {
    "result": "terrain_id", // terrain it will become when done
    "duration": "1 seconds", // (optional) time required for prying nails, default is 1 second
    "message": "You finish prying the door.", // (optional) message that will be displayed when finished prying successfully
    "byproducts": [ // (optional) list of items that will be spawned when finished successfully
        {
            "item": "item_id",
            "count": 100 // exact amount
        },
        {
            "item": "item_id",
            "count": [ 10, 100 ] // random number in range inclusive
        }
    ],
    "prying_data": {
        "prying_nails": false, // (optional, default false) if set to true, ALL fields below are ignored
        "difficulty": 0, // (optional, default 0) base difficulty of prying action
        "prying_level": 0, // (optional, default 0) minimum prying level tool needs to have
        "noisy": false, // (optional, default false) makes noise when successfully prying
        "alarm": false, // (optional) has an alarm, on success will trigger the police
        "breakable": false, // (optional) has a chance to trigger the break action on failure
        "failure": "You try to pry the window but fail." // (optional) failure message
    }
}
```

#### `transforms_into`

(Optional) Used for various transformation of the terrain. If defined, it must be a valid terrain id. Used for example:

- When harvesting fruits (to change into the harvested form of the terrain).
- In combination with the `HARVESTED` flag and `harvest_by_season` to change the harvested terrain back into a terrain with fruits.

#### `allowed_template_ids`

(Optional) Array used for specifying templates that a nanofabricator can create

#### `curtain_transform`

(Optional) Terrain id

Transform into this terrain when an `examine_action` of curtains is used and the `Tear down the curtains` option is selected.

#### `shoot`

(Optional) Array of objects

Defines how this terrain will interact with ranged projectiles. Has the following objects: 

```
    // Base chance to hit the object at all (defaults to 100%)
    int chance_to_hit = 0;
    // Minimum damage reduction to apply to shot when hit
    int reduce_dmg_min = 0;
    // Maximum damage reduction to apply to shot when hit
    int reduce_dmg_max = 0;
    // Minimum damage reduction to apply to laser shots when hit
    int reduce_dmg_min_laser = 0;
    // Maximum damage reduction to apply to laser shots when hit
    int reduce_dmg_max_laser = 0;
    // Damage required to have a chance to destroy
    int destroy_dmg_min = 0;
    // Damage required to guarantee destruction
    int destroy_dmg_max = 0;
    // Are lasers incapable of destroying the object (defaults to false)
    bool no_laser_destroy = false;
```

#### `harvest_by_season`

(Optional) Array of objects containing the seasons in which to harvest and the id of the harvest entry used.

Example:
```jsonc
"harvest_by_season": [ { "seasons": [ "spring", "summer", "autumn", "winter" ], "id": "blackjack_harv" } ],
```

#### `liquid_source`

(Optional) Object, that contain liquids this terrain or furniture can give

Example:
```jsonc
"liquid_source": {
  "id": "water",      // id of a liquid given by ter/furn
  "min_temp": 7.8,    // the lowest possible temperature of liquid taken from here, in centigrade; Used only by "water_source" examine action. Liquid is either the ambient temperature or the `min_temp`, whichever is higher.
  "count": [ 24, 48 ] // if source is finite, how much there should be of it. Omit if it need to have infinite amount of liquid. Accepts either single number, or array of two numbers. Used only by "finite_water_source" examine action
}
```

#### `roof`

(Optional) The terrain of the terrain on top of this (the roof).

### Common To Furniture And Terrain

Some values can/must be set for terrain and furniture. They have the same meaning in each case.

#### `id`

Id of the object, this should be unique among all object of that type (all terrain or all furniture types). By convention (but technically not needed), the id should have the "f_" prefix for furniture and the "t_" prefix for terrain. This is not translated. It must not be changed later as that would break save compatibility.

#### `name`

Displayed name of the object. This will be translated.

#### `flags`

(Optional) Various additional flags, see [JSON_FLAGS.md](JSON_FLAGS.md).

#### `connect_groups`

(Optional) Makes the type a member of one or more [Connection groups](#connection-groups).

Does not make the type connect or rotate itself, but serves as the passive side.
For the active, connecting or rotating side, see [`connects_to`](#connects_to) and [`rotates_to`](#rotates_to).

##### Connection groups

Connect groups are defined by JSON types [`connect_group`](#connect-group-definitions).  
Current connect groups are:

```
NONE                 SAND
WALL                 PIT_DEEP
CHAINFENCE           LINOLEUM
WOODFENCE            CARPET
RAILING              CONCRETE
POOLWATER            CLAY
WATER                DIRT
PAVEMENT             ROCKFLOOR
PAVEMENT_MARKING     MULCHFLOOR
RAIL                 METALFLOOR
COUNTER              WOODFLOOR
CANVAS_WALL          INDOORFLOOR
MUD                  PAVEMENT_ZEBRA
DIRTMOUND            CLAYMOUND
SANDMOUND            SANDGLASS
SANDPILE             BRICKFLOOR
MARBLEFLOOR          BEACH_FORMATIONS
GRAVELPILE           LIXATUBE
```

`WALL` is implied by the flags `WALL` and `CONNECT_WITH_WALL`.
`INDOORFLOOR` is implied by the flag `INDOORS`.
Implicit groups can be removed be using tilde `~` as prefix of the group name.

#### `connects_to`

(Optional) Makes the type connect to terrain types in the given groups (see [`connect_groups`](#connect_groups)). This affects tile rotation and connections, and the ASCII symbol drawn by terrain with the flag "AUTO_WALL_SYMBOL".

Example: `-` , `|` , `X` and `Y` are terrain which share a group in `connect_groups` and `connects_to`. `O` does not have it. `X` and `Y` also have the `AUTO_WALL_SYMBOL` flag. `X` will be drawn as a T-intersection (connected to west, south and east), `Y` will be drawn as a horizontal line (going from west to east, no connection to south).

```
-X-    -Y-
 |      O
```

Group `WALL` is implied by the flags `WALL` and `CONNECT_WITH_WALL`.
Implicit groups can be removed be using tilde `~` as prefix of the group name.

#### `rotates_to`

(Optional) Makes the type rotate towards terrain types in the given groups (see [`connect_groups`](#connect_groups)).

Terrain can only rotate depending on terrain, while funiture can rotate depending on terrain and on other funiture.

The parameters can e.g. be used to have window curtains on the indoor side only, or to orient traffic lights depending on the pavement.

Group `INDOORFLOOR` is implied by the flags `DOOR` and `WINDOW`.
Implicit groups can be removed be using tilde `~` as prefix of the group name.

#### `symbol`

ASCII symbol of the object as it appears in the game. The symbol string must be exactly one character long. This can also be an array of 4 strings, which define the symbol during the different seasons. The first entry defines the symbol during spring. If it's not an array, the same symbol is used all year round.

#### `move_cost_mod`

Move cost to move through. A value of 0 means it's impassable (e.g. wall). You should not use negative values. The positive value is multiple of 50 move points, e.g. value 2 means the player uses 2\*50 = 100 move points when moving across the terrain.

#### `comfort`

(Optional) How comfortable this terrain/furniture is. Impact ability to fall asleep on it.
    uncomfortable = -999,
    neutral = 0,
    slightly_comfortable = 3,
    comfortable = 5,
    very_comfortable = 10

#### `floor_bedding_warmth`

(Optional) Bonus warmth offered by this terrain/furniture when used to sleep. Also affects how comfortable a resting place this is(affects healing). Vanilla values should not exceed 1000.

#### `fall_damage_reduction`

(Optional) Flat damage reduction or increase if negative number. Like falling on a bush or soft chair or mattress or sofa.

#### `bonus_fire_warmth_feet`

(Optional) Increase warmth received on feet from nearby fire  (default = 300)

#### `looks_like`

id of a similar item that this item looks like. The tileset loader will try to load the tile for that item if this item doesn't have a tile. looks_like entries are implicitly chained, so if 'throne' has looks_like 'big_chair' and 'big_chair' has looks_like 'chair', a throne will be displayed using the chair tile if tiles for throne and big_chair do not exist. If a tileset can't find a tile for any item in the looks_like chain, it will default to the ascii symbol.

#### `color` or `bgcolor`

Color of the object as it appears in the game. "color" defines the foreground color (no background color), "bgcolor" defines a solid background color. As with the "symbol" value, this can be an array with 4 entries, each entry being the color during the different seasons.

> **NOTE**: You must use ONLY ONE of "color" or "bgcolor"

#### `coverage`

(Optional) The coverage percentage of a furniture piece of terrain. <30 won't cover from sight. (Does not interact with projectiles, gunfire, or other attacks. Only line of sight.)

#### `max_volume`

(Optional) Maximal volume that can be used to store items here. Volume in ml and L can be used - "50 ml" or "2 L"

#### `examine_action`

(Optional) The json function that is called when the object is examined. See [EXAMINE.md](EXAMINE.md).

#### `close` and `open`

(Optional) The value should be a terrain id (inside a terrain entry) or a furniture id (in a furniture entry). If either is defined, the player can open / close the object. Opening / closing will change the object at the affected tile to the given one. For example one could have object "safe_c", which "open"s to "safe_o" and "safe_o" in turn "close"s to "safe_c". Here "safe_c" and "safe_o" are two different terrain (or furniture) types that have different properties.

#### `bash`

(Optional) Defines whether the object can be bashed and if so, what happens. See "map_bash_info".

#### `deconstruct`

(Optional) Defines whether the object can be deconstructed and if so, what the results shall be. See "map_deconstruct_info".

#### `map_bash_info`

Defines the various things that happen when the player or something else bashes terrain or furniture.

```jsonc
{
    "str_min": 80,
    "str_max": 180,
    "str_min_blocked": 15,
    "str_max_blocked": 100,
    "str_min_supported": 15,
    "str_max_supported": 100,
    "sound": "crunch!",
    "sound_vol": 2,
    "sound_fail": "whack!",
    "sound_fail_vol": 2,
    "ter_set": "t_dirt",
    "furn_set": "f_rubble",
    "ter_set_bashed_from_above": "t_rock_floor_no_roof",
    "explosive": 1,
    "collapse_radius": 2,
    "destroy_only": true,
    "bash_below": true,
    "tent_centers": ["f_groundsheet", "f_fema_groundsheet", "f_skin_groundsheet"],
    "items": "bashed_item_result_group"
}
```

##### `str_min`, `str_max`

The bash succeeds if str >= random # between str_min & str_max

##### `str_min_blocked`, `str_max_blocked`
(Optional) Will be used instead of str_min & str_max if the furniture is blocked, for example a washing machine behind a door

##### `str_min_supported`, `str_max_supported`
(Optional) Will be used instead of str_min & str_max if beneath this is something that can support a roof.

##### `sound`, `sound_fail`, `sound_vol`, `sound_fail_vol`
(Optional) Sound and volume of the sound that appears upon destroying the bashed object or upon unsuccessfully bashing it (failing). The sound strings are translated (and displayed to the player).

##### `furn_set`, `ter_set`

The terrain / furniture that will be set when the original is destroyed. This is mandatory for bash entries in terrain, but optional for entries in furniture (it defaults to no furniture).

##### `ter_set_bashed_from_above`

If terrain is bashed from above (like in explosion), this terrain would be spawned instead of `ter_set`. Usually the version of terrain without a roof is used

##### `explosive`
(Optional) If greater than 0, destroying the object causes an explosion with this strength (see `game::explosion`).

##### `destroy_only`

If true, only used for destroying, not normally bashable

##### `bash_below`

This terrain is the roof of the tile below it, try to destroy that too. Further investigation required

##### `tent_centers`, `collapse_radius`
(Optional) For furniture that is part of tents, this defines the id of the center part, which will be destroyed as well when other parts of the tent get bashed. The center is searched for in the given "collapse_radius" radius, it should match the size of the tent.

##### `items`

(Optional) An item group (inline) or an id of an item group, see [ITEM_SPAWN.md](ITEM_SPAWN.md). The default subtype is "collection". Upon successful bashing, items from that group will be spawned.

#### `map_deconstruct_info`

```jsonc
{
    "furn_set": "f_safe",
    "ter_set": "t_dirt",
    "skill": { "skill": "electronics", "multiplier": 0.5, "min": 1, "max": 8 },
    "items": "deconstructed_item_result_group"
}
```

##### `furn_set`, `ter_set`

The terrain / furniture that will be set after the original has been deconstructed. "furn_set" is optional (it defaults to no furniture), "ter_set" is only used upon "deconstruct" entries in terrain and is mandatory there.

##### `skill`

(Optional) The skill that will be practised after deconstruction.
Min is the minimum level to receive xp, max is the level cap after which no xp is received but practise still occurs delaying rust and multiplier multiplies the base xp given which is based on the mean of min and max.
If skill is specified, multiplier defaults to 1.0, min to 0 and max to 10.

##### `items`

(Optional) An item group (inline) or an id of an item group, see [ITEM_SPAWN.md](ITEM_SPAWN.md). The default subtype is "collection". Upon deconstruction the object, items from that group will be spawned.

#### `plant_data`

```jsonc
{
  "transform": "f_planter_harvest",
  "base": "f_planter",
  "growth_multiplier": 1.2,
  "harvest_multiplier": 0.8
}
```

##### `transform`

What the `PLANT` furniture turn into when it grows a stage, or what a `PLANTABLE` furniture turns into when it is planted on.

##### `emissions`

(Optional) An array listing the `emit_id` of the fields the terrain/furniture will produce every 10 seconds.

##### `base`

What the 'base' furniture of the `PLANT` furniture is - what it would be if there was not a plant growing there. Used when monsters 'eat' the plant to preserve what furniture it is.

##### `growth_multiplier`

A flat multiplier on the growth speed on the plant. For numbers greater than one, it will take longer to grow, and for numbers less than one it will take less time to grow.

##### `harvest_multiplier`

A flat multiplier on the harvest count of the plant. For numbers greater than one, the plant will give more produce from harvest, for numbers less than one it will give less produce from harvest.

### clothing_mod

```jsonc
"type": "clothing_mod",
"id": "leather_padded",    // Unique ID.
"flag": "leather_padded",  // flag to add to clothing.
"item": "leather",         // item to consume.
"implement_prompt": "Pad with leather",      // prompt to show when implement mod.
"destroy_prompt": "Destroy leather padding", // prompt to show when destroy mod.
"restricted": true,        // (optional) If true, clothing must list this mod's flag in "valid_mods" list to use it. Defaults to false.
"mod_value": [             // List of mod effect.
    {
        "type": "bash",    // "bash", "cut", "bullet", "fire", "acid", "warmth", and "encumbrance" is available.
        "value": 1,        // value of effect.
        "round_up": false, // (optional) round up value of effect. defaults to false.
        "proportion": [    // (optional) value of effect proportions to clothing's parameter.
            "thickness",   //            "thickness" and "coverage" is available.
            "coverage"
        ]
    }
]
```

### Flags

Flags, that can be used in different entries, can also be made in json, allowing it to be used in pocket restrictions and EoC checks, or having a json flag and information in json, while being backed in code

```jsonc

{
  "type": "json_flag", // define it as json flag
  "id": "COMPLETELY_MADE_UP_FLAG", // id of a flag
  "name": "ultra-light battery", // name of a flag, used in pocket restrictions shown as `compatible magazines: form factors` 
  "info": "This will hook to a <info>Hub 01 proprietary</info> skirt connector", // this information would be shown, if possible - like in item description
  "restriction": "Item must be an armored skirt", // for pocket restriction, this information would be shown in `restrictions` field in pocket info
  "conflicts": "STURDY", // if something with this flag will met something with conflict flag, only one will be applied
  "taste_mod": -5, // for consumables, it will add -5 to taste, that can't be removed with cooking
  "inherit": true, // is this flag inherited to another thing if it's attached/equipped, like if you put ESAPI plate into plate carrier, their `CANT_WEAR` flag won't be applied to plate carrier, and you could wear it as usually
  "craft_inherit": true, // if true, if you craft something with this flag, this flag would be applied to result also
  "item_prefix": "[...",
  "item_suffix": "...]" // `item_prefix` and `item_suffix` will be added to the prefix and suffix of the item name
},

```

# Scenarios

Scenarios are specified as JSON object with `type` member set to `scenario`.

```jsonc
{
    "type": "scenario",
    "id": "schools_out",
    // ...
}
```

The id member should be the unique id of the scenario.

The following properties (mandatory, except if noted otherwise) are supported:


## `description`
(string)

The in-game description.

## `name`
(string or object with members "male" and "female")

The in-game name, either one gender-neutral string, or an object with gender specific names. Example:
```jsonc
"name": {
    "male": "Runaway groom",
    "female": "Runaway bride"
}
```

## `points`
(integer)

Point cost of scenario. Positive values cost points and negative values grant points.

## `items`
(optional, object with optional members "both", "male" and "female")

Items the player starts with when selecting this scenario. One can specify different items based on the gender of the character. Each lists of items should be an array of items ids. Ids may appear multiple times, in which case the item is created multiple times.

Example:
```jsonc
"items": {
    "both": [
        "pants",
        "rock",
        "rock"
    ],
    "male": [ "briefs" ],
    "female": [ "panties" ]
}
```
This gives the player pants, two rocks and (depending on the gender) briefs or panties.

## `flags`
(optional, array of strings)

A list of flags. TODO: document those flags here.

## `cbms`
(optional, array of strings)

A list of CBM ids that are implanted in the character.

## `traits`, `forced_traits`, `forbidden_traits`
(optional, array of strings)

Lists of trait/mutation ids. Traits in "forbidden_traits" are forbidden and can't be selected during the character creation. Traits in "forced_traits" are automatically added to character. Traits in "traits" enables them to be chosen, even if they are not starting traits.

`forced_traits` can also be specified with a variant, as `{ "trait": "trait_id", "variant": "variant_id" }` (replacing just `"trait_id"`).

## `allowed_locs`
(optional, array of strings)

A list of starting location ids (see start_locations.json) that can be chosen when using this scenario.

## `start_name`
(string)

The name that is shown for the starting location. This is useful if the scenario allows several starting locations, but the game can not list them all at once in the scenario description. Example: if the scenario allows to start somewhere in the wilderness, the starting locations would contain forest and fields, but its "start_name" may simply be "wilderness".

## `professions`
(optional, array of strings)

A list of allowed professions that can be chosen when using this scenario. The first entry is the default profession. If this is empty, all professions are allowed.

## `hobbies`
(optional, array of strings)

A list of allowed hobbies that can be chosen when using this scenario. If this is empty, all hobbies are allowed.

## `whitelist_hobbies`
(optional, bool)

When set to false, the hobbies in `hobbies` are hobbies that _cannot_ be chosen when using this scenario. This value defaults to true.

## `map_special`
(optional, string)

Add a map special to the starting location, see JSON_FLAGS for the possible specials.

## `requirement`

(optional, an achievement ID)

The achievement you need to do to access this scenario

## `reveal_locale`

(optional, boolean)

Defaults true. If a road can be found within 3 OMTs of the starting position, reveals a path to the nearest city and that city's center.

## `distance_initial_visibility`

(optional, int)

Defaults 15. How much of the initial map should be known when the game is started? This value is a radius.

## `eocs`
(optional, array of strings)

A list of eocs that are triggered once for each new character on scenario start.

## `missions`
(optional, array of strings)

A list of mission ids that will be started and assigned to the player at the start of the game. Only missions with the ORIGIN_GAME_START origin are allowed. The last mission in the list will be the active mission, if multiple missions are assigned.

## `start_of_cataclysm`
(optional, object with optional members "hour", "day", "season" and "year")

Allows customization of Cataclysm start date. If `start_of_cataclysm` is not set the corresponding default values are used instead - `Year 1, Spring, Day 61, 00:00:00`. Can be changed in new character creation screen.

```jsonc
"start_of_cataclysm": { "hour": 7, "day": 10, "season": "winter", "year": 1 }
```

 Identifier            | Description
---                    | ---
`hour`                 | (optional, integer) Hour of the day. Default value is 0.
`day`                  | (optional, integer) Day of the season. Default value is 61.
`season`               | (optional, integer) Season of the year. Default value is `spring`.
`year`                 | (optional, integer) Year. Default value is 1.

## `start_of_game`
(optional, object with optional members "hour", "day", "season" and "year")

Allows customization of game start date. If `start_of_game` is not set the corresponding default values are used instead - `Year 1, Spring, Day 61, 08:00:00`. Can be changed in new character creation screen.

**Attention**: Game start date is automatically adjusted, so it is not before the Cataclysm start date.

```jsonc
"start_of_game": { "hour": 8, "day": 16, "season": "winter", "year": 2 }
```

 Identifier            | Description
---                    | ---
`hour`                 | (optional, integer) Hour of the day. Default value is 8.
`day`                  | (optional, integer) Day of the season. Default value is 61.
`season`               | (optional, integer) Season of the year. Default value is `spring`.
`year`                 | (optional, integer) Year. Default value is 1.

# Starting locations

Starting locations are specified as JSON object with "type" member set to "start_location":
```jsonc
{
    "type": "start_location",
    "id": "field",
    "name": "An empty field",
    "terrain": [ "field", { "om_terrain": "hospital", "om_terrain_match_type": "PREFIX" } ],
    "city_sizes": [ 0, 16 ],
    "city_distance": [ 0, -1 ],
    "allowed_z_levels": [ 0, 0 ],
    ...
}
```

The id member should be the unique id of the location.

The following properties (mandatory, except if noted otherwise) are supported:

## `name`
(string)

The in-game name of the location.

## `terrain`
(array of strings and/or objects)

String here contains the id of an overmap terrain type (see overmap_terrain.json) of the starting location. The game will chose a random place with that terrain.

If it is an object - it has following attributes:

     Identifier        |                                   Description                                           |
---------------------- | --------------------------------------------------------------------------------------- |
`om_terrain`           | ID of overmap terrain which will be selected as the target. Mandatory.                  |
`om_terrain_match_type`| Optional. Matching rule to use with `om_terrain`. Defaults to TYPE. Details are below.  |
`parameters`           | Optional. Parameter key/value pairs to set. Details are below.                          |


`om_terrain_match_type` defaults to TYPE if unspecified, and has the following possible values:

* `EXACT` - The provided string must completely match the overmap terrain id,
  including linear direction suffixes for linear terrain types or rotation
  suffixes for rotated terrain types.

* `TYPE` - The provided string must completely match the base type id of the
  overmap terrain id, which means that suffixes for rotation and linear terrain
  types are ignored.
 
* `SUBTYPE` - The provided string must completely match the base type id of the
  overmap terrain id as well as the linear terrain type ie "road_curved" will match
  "road_ne", "road_es", "road_sw" and "road_wn".

* `PREFIX` - The provided string must be a complete prefix (with additional
  parts delimited by an underscore) of the overmap terrain id. For example,
  "forest" will match "forest" or "forest_thick" but not "forestcabin".

* `CONTAINS` - The provided string must be contained within the overmap terrain
  id, but may occur at the beginning, end, or middle and does not have any rules
  about underscore delimiting.

`parameters` is an object containing one or more keys to set to a specific value provided, say to pick a safe variant of a map.
The keys and values must be valid for the overmap terrains in question.
This can also be used with 0 weight values to provide unique starting map variations that don't spawn normally.

### Examples

Any overmap terrain that is either `"shelter"` or begins with `shelter_`

```jsonc
{
    "om_terrain": "shelter",
    "om_terrain_match_type": "PREFIX"
}
```

Any overmap terrain that is either `"mansion"` or begins with `mansion_`, and forces the parameter `mansion_variant` to be set to `haunted_scenario_only`

```jsonc
{
    "om_terrain": "mansion",
    "om_terrain_match_type": "PREFIX",
    "parameters": { "mansion_variant": "haunted_scenario_only" }
}
```

## `city_sizes`
(array of two integers)

Restricts possible start location based on nearest city size (similar to how overmap specials are restricted).

## `city_distance`
(array of two integers)

Restricts possible start location based on distance to nearest city (similar to how overmap specials are restricted).

## `allowed_z_levels`
(array of two integers)

Restricts possible start location based on z-level (e.g. there is no need to search forests on z-levels other than 0).

## `flags`
(optional, array of strings)

Arbitrary flags.  Two flags are supported in the code: `ALLOW_OUTSIDE` and `BOARDED` (see [JSON_FLAGS.md](JSON_FLAGS.md)). Mods can modify this via "extend" / "delete".
```jsonc
{
    "type": "start_location",
    "id": "sloc_house_boarded",
    "copy-from": "sloc_house",
    "name": "House (boarded up)",
    "extend": { "flags": [ "BOARDED" ] }
},
```

# Mutation overlay ordering

The file `mutation_ordering.json` defines the order that visual mutation and bionic overlays are rendered on a character ingame. The layering value from 0 (bottom) - 9999 (top) sets the order.

Example:
```jsonc
[
    {
        "type" : "overlay_order",
        "overlay_ordering" :
        [
        {
            "id" : [ "BEAUTIFUL", "BEAUTIFUL2", "BEAUTIFUL3", "LARGE", "PRETTY", "RADIOACTIVE1", "RADIOACTIVE2", "RADIOACTIVE3", "REGEN" ],
            "order" : 1000
        },{
            "id" : [ "HOOVES", "ROOTS1", "ROOTS2", "ROOTS3", "TALONS" ],
            "order" : 4500
        },{
            "id" : "FLOWERS",
            "order" : 5000
        },{
            "id" : [ "PROF_CYBERCOP", "PROF_FED", "PROF_PD_DET", "PROF_POLICE", "PROF_SWAT", "PHEROMONE_INSECT" ],
            "order" : 8500
        },{
            "id" : [ "bio_armor_arms", "bio_armor_legs", "bio_armor_torso", "bio_armor_head", "bio_armor_eyes" ],
            "order" : 500
        }
        ]
    }
]
```

## `id`
(string)

The internal ID of the mutation. Can be provided as a single string, or an array of strings. The order value provided will be applied to all items in the array.

## `order`
(integer)

The ordering value of the mutation overlay. Values range from 0 - 9999, 9999 being the topmost drawn layer. Mutations that are not in any list will default to 9999.

# MOD tileset

MOD tileset defines additional sprite sheets. It is specified as JSON object with `type` member set to `mod_tileset`.

Example:
```jsonc
[
    {
    "type": "mod_tileset",
    "compatibility": [ "MshockXottoplus" ],
    "tiles-new": [
        {
        "file": "test_tile.png",
        "tiles": [
            {
            "id": "player_female",
            "fg": 1,
            "bg": 0
            },
            {
            "id": "player_male",
            "fg": 2,
            "bg": 0
            }
        ]
        }
    ]
    }
]
```

## `compatibility`
(string)

The internal ID of the compatible tilesets. MOD tileset is only applied when base tileset's ID exists in this field.

## `tiles-new`

Setting of sprite sheets. Same as `tiles-new` field in `tile_config`. Sprite files are loaded from the same folder json file exists.

# Obsoletion and migration

[OBSOLETION_AND_MIGRATION.md](#OBSOLETION_AND_MIGRATION.md)


# Field types

Fields can exist on top of terrain/furniture, and support different intensity levels. Each intensity level inherits its properties from the lower one, so any field not explicitly overwritten will carry over. They affect both characters and monsters, but a lot of fields have hardcoded effects that are potentially different for both (found in `map_field.cpp:creature_in_field`). Gaseous fields that have a chance to do so are spread depending on the local wind force when outside, preferring spreading down to on the same Z-level, which is preferred to spreading upwards. Indoors and by weak winds fields spread randomly.

```jsonc
  {
    "type": "field_type", // this is a field type
    "id": "fd_gum_web", // id of the field
    "immune_mtypes": [ "mon_spider_gum" ], // list of monster immune to this field
    "legacy_enum_id": -1, // Not used anymore, default -1
    "legacy_make_rubble": true, // Transform terrain into rubble, was used when rubble was a field, not used now
    "priority": 4, // Fields with higher priority are drawn above another - smoke is drawn above the acid pool, not vice versa
    "intensity_levels":  // The below fields are all tied to the specific intensity unless they got defined in the lower-level one
    [
      { "name": "shadow",  // name of this level of intensity
        "sym": "{", // symbol of this level of intensity
        "color":  "brown", // color of this level of intensity
        "transparent": false, // default true, on false this intensity blocks vision
        "dangerous": false, // is this level of intensity considered dangerous for monster avoidance and player warnings
        "move_cost": 120, // Extra movement cost for moving through this level of intensity (on top of base terrain/furniture movement costs)
        "extra_radiation_min": 1,
        "extra_radiation_max": 5, // This level of intensity will add a random amount of radiation between the min and the max value to its tile
        "radiation_hurt_damage_min": 5,
        "radiation_hurt_damage_max": 7, // When standing in this field hurt every limb for a random amount of damage between the min and max value. Requires extra_radiation_min > 0
        "radiation_hurt_message": "Ouch", // String to print when you get hurt by radiation_hurt_damage
        "intensity_upgrade_chance": 1,
        "intensity_upgrade_duration": "1 days", // Every "upgrade_duration" a "1-in-upgrade_chance" roll is made. On success, increase the field intensity by 1
        "monster_spawn_chance": 5,
        "monster_spawn_count": 2,
        "monster_spawn_radius": 15,
        "monster_spawn_group": "GROUP_NETHER", // 1-in-spawn_chance roll to spawn spawn_count entries from spawn_group monstergroup and place them in a spawn_radius radius around the field
        "light_emitted": 2.5, // light level emitted by this intensity
        "light_override": 3.7 }, //light level on the tile occupied by this field will be set at 3.7 no matter the ambient light.
        "translucency": 2.0, // How much light the field blocks (higher numbers mean less light can penetrate through)
        "concentration": 1, // How concentrated this intensity of gas is. Generally the thin/hazy cloud intensity will be 1, the standard gas will be 2, and thick gas will be 4. The amount of time a gas mask filter will last will be divided by this value.
        "convection_temperature_mod": 12, // Heat given off by this level of intensity
        "effects":  // List of effects applied to any creatures within the field as long as they aren't immune to the effect or the field itself
        [
          {
            "effect_id": "webbed", // Effect ID
            "min_duration": "1 minutes",
            "max_duration": "5 minutes", // Effect duration randomized between min and max duration
            "intensity": 1, // Intensity of the effect to apply
            "body_part": "head", // Bodypart the effect gets applied to, default BP_NULL ("whole body")
            "is_environmental": false, // If true the environmental effect roll is used to determine if the effect gets applied: <intensity>d3 > <target BP's armor/bionic env resist>d3
            "immune_in_vehicle": // If true, *standing* inside a vehicle (like without walls or roof) protects from the effect
            "immune_inside_vehicle": false, // If true being inside a vehicle protects from the effect
            "immune_outside_vehicle": false, // If true being *outside* a vehicle protects from the effect,
            "chance_in_vehicle": 2,
            "chance_inside_vehicle": 2,
            "chance_outside_vehicle": 2, // 1-in-<chance> chance of the effect being applied when traversing a field in a vehicle, inside a vehicle (as in, under a roof), and outside a vehicle
            "message": "You're debilitated!", // Message to print when the effect is applied to the player
            "message_npc": "<npcname> is debilitated!", // Message to print when the effect is applied to an NPC
            "message_type": "bad", // Type of the above messages - good/bad/mixed/neutral
            "immunity_data": {...} // See Immunity Data below
          }
        ]
        "scent_neutralization": 3, // Reduce scents at the field's position by this value        
    ],
    "npc_complain": { "chance": 20, "issue": "weed_smoke", "duration": "10 minutes", "speech": "<weed_smoke>" }, // NPCs in this field will complain about being in it once per <duration> if a 1-in-<chance> roll succeeds, giving off a <speech> bark that supports snippets
    "immunity_data": {...} // See Immunity Data below
    "decay_amount_factor": 2, // The field's rain decay amount is divided by this when processing the field, the rain decay is a function of the weather type's precipitation class: very_light = 5s, light = 15s, heavy = 45 s
    "half_life": "3 minutes", // If above 0 the field will disappear after two half-lifes on average
    "underwater_age_speedup": "25 minutes", // Increase the field's age by this time every tick if it's on a terrain with the SWIMMABLE flag
    "linear_half_life": "true", // If true the half life decay is converted to a non-random, linear wait based on the defined half_life time. 
    "outdoor_age_speedup": "20 minutes", // Increase the field's age by this duration if it's on an outdoor tile
    "accelerated_decay": true, // If the field should use a more simple decay calculation, used for cosmetic fields like gibs
    "percent_spread": 90, // The field must succeed on a `rng( 1, 100 - local windpower ) > percent_spread` roll to spread. The field must have a non-zero spread percent and the GAS phase to be eligible to spread in the first place
    "phase": "gas", // Phase of the field. Gases can spread after spawning and can be spawned out of emitters through impassable terrain with the PERMEABLE flag
    "apply_slime_factor": 10, // Intensity of slime to apply to creatures standing in this field (Why not use an effect? No idea.)
    "gas_absorption_factor": "80m", // Length a full 100 charge gas mask filter will last in this gas. Will be divided by the concentration of the gas, and should be 80m for concentration 1 toxic gas or similar. The worst gas should still be kept out for 20 minutes in a concentration 4 thick gas.
    "is_splattering": true, // If splatters of this field should bloody vehicle parts
    "dirty_transparency_cache": true, // Should the transparency cache be recalculated when the field is modified (used for nontransparent, spreading fields)
    "has_fire": false, // Is this field a kind of fire (for immunity, monster avoidance and similar checks)
    "has_acid": false, // See has_fire
    "has_elec": false, // See has_fire
    "has_fume": false, // See has_fire, non-breathing monsters are immune to this field
    "display_items": true, // If the field should obscure items on this tile
    "display_field": true, // If the field has a visible sprite or symbol, default false
    "description_affix": "covered_in", // Description affix for items in this field, possible values are "in", "covered_in", "on", "under", and "illuminated_by"
    "wandering_field": "fd_toxic_gas", // Spawns the defined field in an `intensity-1` radius, or increases the intensity of such fields until their intensity is the same as the parent field
    "decrease_intensity_on_contact": true, // Decrease the field intensity by one each time a character walk on it.
    "mopsafe": false, // field is safe to use in a mopping zone
    "bash": {
      "str_min": 1, // lower bracket of bashing damage required to bash
      "str_max": 3, // higher bracket
      "sound_vol": 2, // noise made when successfully bashing the field
      "sound_fail_vol": 2, // noise made when failing to bash the field
      "sound": "shwip", // sound on success
      "sound_fail": "shwomp", // sound on failure
      "msg_success": "You brush the gum web aside.", // message on success
      "move_cost": 120, // how many moves it costs to successfully bash that field (default: 100)
      "items": [                                   // item dropped upon successful bashing
        { "item": "2x4", "count": [ 5, 8 ] },
        { "item": "nail", "charges": [ 6, 8 ] },
        { "item": "splinter", "count": [ 3, 6 ] },
        { "item": "sheet_cotton", "count": [ 40, 55 ] },
        { "item": "scrap", "count": [ 10, 20 ] }
      ]
    },
    "indestructible": true // Field cannot be bashed or destroyed, but may still expire over time.  Useful when combined with the bash field because it can allow fields to prevent bashing terrain but not be destructible.
  }
```

## Emits
Defines field emissions 

```jsonc
{
  "id": "emit_rad_cloud", // id of emission
  "type": "emit",         
  "field": "fd_nuke_gas", // field, that would be emitted
  "intensity": 3,         // intensity of the field to be emitted
  "qty": 100,             // amount of fields that would be emitted, in a circle, 1 means 1 field; 9 would be 3x3, 16 would be 4x4 square etc
  "chance": 50            // chance to emit one unit of field, from 1 to 100
},
```

## Immunity data
Immunity data can be provided at the field level or at the effect level based on intensity and body part. At the field level it applies immunity to all effects.

```jsonc
"immunity_data": {  // Object containing the necessary conditions for immunity to this field.  Any one fulfilled condition confers immunity:
      "flags": [ "WEBWALK" ],  // Immune if the character has any of the defined character flags (see Character flags)
      "body_part_env_resistance": [ [ "mouth", 15 ], [ "sensor", 10 ] ], // Immune if ALL bodyparts of the defined types have the defined amount of env protection
      "immunity_flags_worn": [ [ "sensor", "FLASH_PROTECTION" ] ], // Immune if ALL parts of the defined types wear an item with the defined flag
      "immunity_flags_worn_any": [ [ "sensor", "BLIND" ], [ "hand", "PADDED" ] ], // Immune if ANY part of the defined type wears an item with the corresponding flag -- in this example either a blindfold OR padded gloves confer immunity
},
```

# Option sliders

```jsonc
{
  "type": "option_slider",
  "id": "world_difficulty",
  "context": "WORLDGEN",
  "name": "Difficulty",
  "default": 3,
  "levels": [
    {
      "level": 0,
      "name": "Cakewalk?",
      "description": "Monsters are much easier to deal with, and plenty of items can be found.",
      "options": [
        { "option": "MONSTER_SPEED", "type": "int", "val": 90 },
        { "option": "MONSTER_RESILIENCE", "type": "int", "val": 75 },
        { "option": "SPAWN_DENSITY", "type": "float", "val": 0.8 },
        { "option": "MONSTER_UPGRADE_FACTOR", "type": "float", "val": 8 },
        { "option": "ITEM_SPAWNRATE", "type": "float", "val": 1.5 }
      ]
    },
    ...
  ]
}
```

## Option sliders - Fields

| Field       | Description
|---          |---
| `"type"`    | _(mandatory)_ Always `"option_slider"`
| `"id"`      | _(mandatory)_ Uniquely identifies this `option_slider`
| `"context"` | The hardcoded context in which this `option_slider` is used (ex: the world creation menu shows option sliders in the `WORLDGEN` context)
| `"name"`    | _(mandatory)_ The translated name of this `option_slider`
| `"default"` | The default level for this `option_slider` (defaults to 0)
| `"levels"`  | _(mandatory)_ A list of definitions for each level of this `option_slider`

## Option sliders - Levels

Each object in the `"levels"` field uses these fields:

| Field | Description
|--- |---
| `"level"` | _(mandatory)_ The numeric index of this level in the slider.  Indexes start at 0 and increase sequentially.
| `"name"` | _(mandatory)_ The name of this slider level, acts as a short descriptor for the selected level.
| `"description"` | A longer description for the effects of this slider level.
| `"options"` | _(mandatory)_ A list of option values to apply when selecting this slider level.

Each option defines an `"option"` tag that corresponds to an option ID as listed in the
`options_manager::add_options_*` functions in src/options.cpp. The `"type"` field determines
how the `"val"` field is interpreted:

| `type`     | `val`
|---         |---
| `"int"`    | An integer.  Ex: `"type": "int", "val": 5`
| `"float"`  | A decimal number.  Ex: `"type": "float", "val": 0.8`
| `"bool"`   | A boolean.  Ex: `"type": "bool", "val": false`
| `"string"` | A text value.  Ex: `"type": "string", "val": "crops"`
