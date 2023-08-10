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
    - [Emitters](#emitters)
    - [Item Groups](#item-groups)
    - [Item Category](#item-category)
    - [Item Properties](#item-properties)
    - [Item Variables](#item-variables)
    - [Item faults](#item-faults)
    - [Item fault fixes](#item-fault-fixes)
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
      - [`description`](#description)
      - [`name`](#name)
      - [`points`](#points)
      - [`addictions`](#addictions)
      - [`skills`](#skills)
      - [`missions`](#missions)
      - [`proficiencies`](#proficiencies)
      - [`items`](#items)
      - [`pets`](#pets)
      - [`vehicle`](#vehicle)
      - [`flags`](#flags)
      - [`cbms`](#cbms)
      - [`traits`](#traits)
      - [`requirement`](#requirement)
    - [Recipes](#recipes)
      - [Practice recipes](#practice-recipes)
      - [Nested recipes](#nested-recipes)
      - [Recipe requirements](#recipe-requirements)
      - [Defining common requirements](#defining-common-requirements)
      - [Overlapping recipe component requirements](#overlapping-recipe-component-requirements)
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
- [`data/json/items/` JSONs](#datajsonitems-jsons)
    - [Generic Items](#generic-items)
      - [To hit object](#to-hit-object)
    - [Ammo](#ammo)
    - [Ammo Effects](#ammo-effects)
    - [Magazine](#magazine)
    - [Armor](#armor)
      - [Armor Portion Data](#armor-portion-data)
        - [Encumbrance](#encumbrance)
        - [Encumbrance_modifiers](#encumbrance_modifiers)
        - [Coverage](#coverage)
        - [Covers](#covers)
        - [Specifically Covers](#specifically-covers)
        - [Part Materials](#part-materials)
        - [Armor Data](#armor-data)
      - [Guidelines for thickness:](#guidelines-for-thickness)
    - [Pet Armor](#pet-armor)
    - [Books](#books)
      - [Conditional Naming](#conditional-naming)
      - [Color Key](#color-key)
      - [CBMs](#cbms)
    - [Comestibles](#comestibles)
    - [Containers](#containers)
    - [Melee](#melee)
    - [Memory Cards](#memory-cards)
    - [Gun](#gun)
    - [Gunmod](#gunmod)
    - [Batteries](#batteries)
    - [Tools](#tools)
    - [Seed Data](#seed-data)
    - [Brewing Data](#brewing-data)
      - [`Effects_carried`](#effects_carried)
      - [`effects_worn`](#effects_worn)
      - [`effects_wielded`](#effects_wielded)
      - [`effects_activated`](#effects_activated)
    - [Software Data](#software-data)
    - [Use Actions](#use-actions)
    - [Tick Actions](#tick-actions)
      - [Delayed Item Actions](#delayed-item-actions)
    - [Random Descriptions](#random-descriptions)
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
      - [`move_cost_mod`](#move_cost_mod)
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
      - [`move_cost`](#move_cost)
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
      - [`roof`](#roof)
    - [Common To Furniture And Terrain](#common-to-furniture-and-terrain)
      - [`id`](#id-1)
      - [`name`](#name-1)
      - [`flags`](#flags-1)
      - [`connect_groups`](#connect_groups)
        - [Connection groups](#connection-groups)
      - [`connects_to`](#connects_to)
      - [`rotates_to`](#rotates_to)
      - [`symbol`](#symbol)
      - [`comfort`](#comfort)
      - [`floor_bedding_warmth`](#floor_bedding_warmth)
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
        - [`explosive`](#explosive)
        - [`destroy_only`](#destroy_only)
        - [`bash_below`](#bash_below)
        - [`tent_centers`, `collapse_radius`](#tent_centers-collapse_radius)
        - [`items`](#items-1)
      - [`map_deconstruct_info`](#map_deconstruct_info)
        - [`furn_set`, `ter_set`](#furn_set-ter_set-1)
        - [`items`](#items-2)
      - [`plant_data`](#plant_data-1)
        - [`transform`](#transform)
        - [`emissions`](#emissions)
        - [`base`](#base)
        - [`growth_multiplier`](#growth_multiplier)
        - [`harvest_multiplier`](#harvest_multiplier)
    - [clothing_mod](#clothing_mod)
- [Scenarios](#scenarios)
  - [`description`](#description-1)
  - [`name`](#name-2)
  - [`points`](#points-1)
  - [`items`](#items-3)
  - [`flags`](#flags-2)
  - [`cbms`](#cbms-1)
  - [`traits`, `forced_traits`, `forbidden_traits`](#traits-forced_traits-forbidden_traits)
  - [`allowed_locs`](#allowed_locs)
  - [`start_name`](#start_name)
  - [`professions`](#professions)
  - [`map_special`](#map_special)
  - [`requirement`](#requirement-1)
  - [`reveal_locale`](#reveal_locale)
  - [`eocs`](#eocs)
  - [`missions`](#missions-1)
  - [`start_of_cataclysm`](#start_of_cataclysm)
  - [`start_of_game`](#start_of_game)
- [Starting locations](#starting-locations)
  - [`name`](#name-3)
  - [`terrain`](#terrain)
  - [`city_sizes`](#city_sizes)
  - [`city_distance`](#city_distance)
  - [`allowed_z_levels`](#allowed_z_levels)
  - [`flags`](#flags-3)
- [Mutation overlay ordering](#mutation-overlay-ordering)
  - [`id`](#id-2)
  - [`order`](#order)
- [MOD tileset](#mod-tileset)
  - [`compatibility`](#compatibility)
  - [`tiles-new`](#tiles-new)
- [Obsoletion and migration](#obsoletion-and-migration)
  - [Charge and temperature removal](#charge-and-temperature-removal)
- [Field types](#field-types)
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
```json
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

```json
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

```json
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

```json
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

```JSON
"name": { "ctxt": "foo", "str": "bar", "str_pl": "baz" }
```

or, if the plural form is the same as the singular form:

```JSON
"name": { "ctxt": "foo", "str_sp": "foo" }
```

You can also add comments for translators by adding a "//~" entry like below. The
order of the entries does not matter.

```JSON
"name": {
    "//~": "as in 'foobar'",
    "str": "bar"
}
```

Currently, only some JSON values support this syntax (see [here](/doc/TRANSLATING.md#translation) for a list of supported values and more detailed explanation).

## Comments

JSON has no intrinsic support for comments.  However, by convention in CDDA
JSON, any field starting with `//` is a comment.

```json
{
  "//" : "comment"
}
```

If you want multiple comments in a single object then append a number to `//`.
For example:

```json
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
| `snippets.json`               | flier/poster descriptions
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

```C++
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

### Addiction types

Addictions are defined in JSON using `"addiction_type"`:

```JSON
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

```JSON
{
  "type": "effect_on_condition",
  "id": "EOC_MARLOSS_R_ADDICTION",
  "condition": { "compare_num": [ { "rand": 800 }, "<", { "u_val": "addiction_intensity", "addiction": "marloss_r", "mod": 20 } ] },
  "effect": [
    { "u_add_morale": "morale_craving_marloss", "bonus": -5, "max_bonus": -30 },
    { "u_message": "You daydream about luscious pink berries as big as your fist.", "type": "info" },
    {
      "run_eocs": [
        {
          "id": "EOC_MARLOSS_R_ADDICTION_MODFOCUS",
          "condition": { "compare_num": [ { "u_val": "focus" }, ">", { "const": 40 } ] },
          "effect": { "arithmetic": [ { "u_val": "focus" }, "-=", { "const": 1 } ] }
        }
      ]
    }
  ]
}
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

```JSON
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
| `drying_chance`        | (_optional_) Base chance the bodypart will succeed in the drying roll ( `x/80` chance, modified by ambient temperature etc)
| `drying_increment`     | (_optonal_) Units of wetness the limb will dry each turn, if it succeeds in the drying roll (base chance `drench_capacity / 80`, modified by ambient temperature).
| `wet_morale`           | (_optional_) Mood bonus/malus when the limb gets wet, representing the morale effect at 100% limb saturation. Modified by worn clothing and ambient temperature.
| `stylish_bonus`        | (_optional_) Mood bonus associated with wearing fancy clothing on this part. (default: `0`)
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
| `armor`                | (_optional_) An object containing damage resistance values. Ex: `"armor": { "bash": 2, "cut": 1 }`. See [Part Resistance](#part-resistance) for details.

```json
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


```json
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
Limb scores act as the basis of calculating the effect of limb encumbrance and damage on the abilities of characters. They are defined using the `"limb_score"` type:

```json
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

```json
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
```C++
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
| `weight_capacity_bonus`      | (_optional_) Bonus to weight carrying capacity in grams, can be negative.  Strings can be used - "5000 g" or "5 kg" (default: `0`)
| `weight_capacity_modifier`   | (_optional_) Factor modifying base weight carrying capacity. (default: `1`)
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
| `vitamin_absorb_mod`         | (_optional_) Modifier to vitamin absorption, affects all vitamins. (default: `1.0`)
| `dupes_allowed`              | (_optional_) Boolean to determine if multiple copies of this bionic can be installed.  Defaults to false.
| `cant_remove_reason`         | (_optional_) String message to be displayed as the reason it can't be uninstalled.  Having any value other than `""` as this will prevent unistalling the bionic. Formatting includes two `%s` for example: `The Telescopic Lenses are part of %1$s eyes now. Removing them would leave %2$s blind.`  (default: `""`)
| `social_modifiers`			     | (_optional_) Json object with optional members: persuade, lie, and intimidate which add or subtract that amount from those types of social checks
| `dispersion_mod`             | (_optional_) Modifier to change firearm dispersion.
| `activated_on_install`       | (_optional_) Auto-activates this bionic when installed.
| `required_bionic`       | (_optional_) Bionic which is required to install this bionic, and which cannot be uninstalled if this bionic is installed

```JSON
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

```JSON
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

```JSON
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

```C++
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


```json
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

### Emitters

Emitters randomly place [fields](#field-types) around their positions - every turn for monster emissions, every ten seconds for furniture/terrain.

| Identifier  | Description
|---          |---
| `id`        | Unique ID
| `field`     | Field type emitted
| `intensity` | Initial intensity of the spawned fields (spawning multiple fields will still cause their intensity to increase). Default 1.
| `chance`    | **Percent** chance of the emitter emitting, values above 100 will increase the quantity of fields placed via `roll_remainder` (ex: `chance: 150` will place one field 50% of the time and two fields the other 50% ). Failing the roll will disable the whole emission for the tick, not rolled for every `qty`! Default 100.
| `qty`       | Number of fields placed. Fields are placed using the field propagation rules, allowing fields to spread. Default 1.

```JSON
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

```C++
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
| `name`           | The name of the category. This is what shows up in-game when you open the inventory.
| `zone`           | The corresponding loot_zone (see loot_zones.json)
| `sort_rank`      | Used to sort categories when displaying.  Lower values are shown first
| `priority_zones` | When set, items in this category will be sorted to the priority zone if the conditions are met. If the user does not have the priority zone in the zone manager, the items get sorted into zone set in the 'zone' property. It is a list of objects. Each object has 3 properties: ID: The id of a LOOT_ZONE (see LOOT_ZONES.json), filthy: boolean. setting this means filthy items of this category will be sorted to the priority zone, flags: array of flags
| `spawn_rate`      | Sets amount of items from item category that might spawn.  Checks for `spawn_rate` value for item category.  If `spawn_chance` is 0.0, the item will not spawn. If `spawn_chance` is greater than 0.0 and less than 1.0, it will make a random roll (0.0-1.0) to check if the item will have a chance to spawn.  If `spawn_chance` is more than or equal to 1.0, it will add a chance to spawn additional items from the same category.  Items will be taken from item group which original item was located in.  Therefore this parameter won't affect chance to spawn additional items for items set to spawn solitary in mapgen (e.g. through use of `item` or `place_item`).

```C++
{
    "id": "armor",
    "name": "ARMOR",
    "zone": "LOOT_ARMOR",
    "sort_rank": -21,
    "priority_zones": [ { "id": "LOOT_FARMOR", "filthy": true, "flags": [ "RAINPROOF" ] } ],
    "spawn_rate": 0.5
}
```

### Item Properties

Properties are bound to item's type definition and code checks for them for special behaviour,
for example the property below makes a container burst open when filled over 75% and it's thrown.

```json
  {
    "properties": [ [ "burst_when_filled", "75" ] ]
  }
```

### Item Variables

Item variables are bound to the item itself and used to serialize special behaviour,
for example folding a vehicle serializes the folded vehicle's name and list of parts
(part type ids, part damage, degradation etc) into json string for use when unfolding.

They can originate from code - like in the example above when folding a vehicle.

Alternatively item variables may also originate from the item's prototype. Specifying them
can be done in the item's definition, add the `variables` key and inside write a key-value
map.

Example:
```json
    "variables": {
      "special_key": "spiffy value"
    }
```

This will make any item instantiated from that prototype get assigned this variable, once
the item is spawned the variables set on the prototype no longer affect the item's variables,
a migration can clear out the item's variables and reassign the prototype ones if reset_item_vars
flag is set.

### Item faults

Faults can be defined for more specialized damage of an item.

```C++
{
  "type": "fault",
  "id": "fault_gun_chamber_spent", // unique id for the fault
  "name": { "str": "Spent casing in chamber" }, // fault name for display
  "description": "This gun currently...", // fault description
  "item_prefix": "jammed", // optional string, items with this fault will be prefixed with this
  "flags": [ "JAMMED_GUN" ] // optional flags, see below
}
```

`flags` trigger hardcoded C++ chunks that provide effects, see [JSON_FLAGS.md](JSON_FLAGS.md#faults) for a list of possible flags.

### Item fault fixes

Fault fixes are methods to fix faults, the fixes can optionally add other faults, modify damage, degradation and item variables.

```C++
{
  "type": "fault_fix",
  "id": "mend_gun_fouling_clean", // unique id for the fix
  "name": "Clean fouling", // name for display
  "success_msg": "You clean your %s.", // message printed when fix is applied
  "time": "50 m", // time to apply fix
  "faults_removed": [ "fault_gun_dirt", "fault_gun_blackpowder" ], // faults removed when fix is applied
  "faults_added": [ "fault_gun_unlubricated" ], // faults added when fix is applied
  "skills": { "mechanics": 1 }, // skills required to apply fix
  "set_variables": { "dirt": "0" }, // sets the variables on the item when fix is applied
  "requirements": [ [ "gun_cleaning", 1 ] ], // requirements array, see below
  "mod_damage": 1000, // damage to modify on item when fix is applied, can be negative to repair
  "mod_degradation": 50 // degradation to modify on item when fix is applied, can be negative to reduce degradation
}
```

`requirements` is an array of requirements, they can be specified in 2 ways:
* An array specifying an already defined requirement by it's id and a multiplier, `[ "gun_lubrication", 2 ]` will add `gun_lubrication` requirement and multiply the components and tools ammo required by 2.
* Inline object specifying the requirement in the same way [recipes define it](#recipe-requirements)

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
| `soft`                 | True for pliable materials, whose length doesn't prevent fitting into a container, or through the opening of a container. Default is false.
| `conductive`           | True if the material conducts electricity, defaults to false
| `reinforces`           | Optional boolean. Default is false.

There are seven -resist parameters: acid, bash, chip, cut, elec, fire, and bullet. These are integer values; the default is 0 and they can be negative to take more damage.

```JSON
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

```C++
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

```C++
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
| `cost_multiplier` | (_optional_) How many monsters each monster in this definition should count as, if spawning a limited number of monsters.  (default: 1)
| `pack_size`       | (_optional_) The minimum and maximum number of monsters in this group that should spawn together.  (default: `[1,1]`)
| `conditions`      | (_optional_) Conditions limit when monsters spawn. Valid options: `SUMMER`, `WINTER`, `AUTUMN`, `SPRING`, `DAY`, `NIGHT`, `DUSK`, `DAWN`. Multiple Time-of-day conditions (`DAY`, `NIGHT`, `DUSK`, `DAWN`) will be combined together so that any of those conditions makes the spawn valid. Multiple Season conditions (`SUMMER`, `WINTER`, `AUTUMN`, `SPRING`) will be combined together so that any of those conditions makes the spawn valid.
| `starts`          | (_optional_) This entry becomes active after this time.  Specified using time units.  (**multiplied by the evolution scaling factor**)
| `ends`            | (_optional_) This entry becomes inactive after this time.  Specified using time units.  (**multiplied by the evolution scaling factor**)
| `spawn_data`      | (_optional_) Any properties that the monster only has when spawned in this group. `ammo` defines how much of which ammo types the monster spawns with. Only applies to "monster" type entries.
| `event`           | (_optional_) If present, this entry can only spawn during the specified event. See the `holiday` enum for possible values. Defaults to `none`. (Ex: `"event": "halloween"`)

```C++
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

```C++
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

```C++
{ "name" : "Aaliyah", "gender" : "female", "usage" : "given" }, // Name, gender, "given"/"family"/"city" (first/last/city name).
```

### Profession item substitution

Defines item replacements that are applied to the starting items based upon the starting traits. This allows for example to replace wool items with non-wool items when the characters starts with the wool allergy trait.

If the JSON objects contains a "item" member, it defines a replacement for the given item, like this:

```C++
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
```C++
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
```C++
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

Professions are specified as JSON object with "type" member set to "profession":

```C++
{
    "type": "profession",
    "id": "hunter",
    ...
}
```

The id member should be the unique id of the profession.

The following properties (mandatory, except if noted otherwise) are supported:

#### `description`
(string)

The in-game description.

#### `name`
(string or object with members "male" and "female")

The in-game name, either one gender-neutral string, or an object with gender specific names. Example:
```C++
"name": {
    "male": "Groom",
    "female": "Bride"
}
```

#### `points`
(integer)

Point cost of profession. Positive values cost points and negative values grant points.

#### `addictions`
(optional, array of addictions)

List of starting addictions. Each entry in the list should be an object with the following members:
- "type": the string id of the addiction (see [JSON_FLAGS.md](JSON_FLAGS.md)),
- "intensity": intensity (integer) of the addiction.

Example:
```C++
"addictions": [
    { "type": "nicotine", "intensity": 10 }
]
```

#### `skills`

(optional, array of skill levels)

List of starting skills. Each entry in the list should be an object with the following members:
- "name": the string id of the skill (see skills.json),
- "level": level (integer) of the skill. This is added to the skill level that can be chosen in the character creation.

Example:
```C++
"skills": [
    { "name": "archery", "level": 2 }
]
```

#### `missions`

(optional, array of mission ids)

List of starting missions for this profession/hobby.

Example:
```JSON
"missions": [ "MISSION_LAST_DELIVERY" ]
```

#### `proficiencies`

(optional, array of proficiency ids)

List of starting proficiency ids.

Example:
```json
"proficiencies": [ "prof_knapping" ]
```

#### `items`

(optional, object with optional members "both", "male" and "female")

Items the player starts with when selecting this profession. One can specify different items based on the gender of the character. Each lists of items should be an array of items ids, or pairs of item ids and snippet ids. Item ids may appear multiple times, in which case the item is created multiple times. The syntax for each of the three lists is identical.

Example:
```C++
"items": {
    "both": [
        "pants",
        "rock",
        "rock",
        ["tshirt_text", "allyourbase"],
        "socks"
    ],
    "male": [
        "briefs"
    ],
    "female": [
        "panties"
    ]
}
```

This gives the player pants, two rocks, a t-shirt with the snippet id "allyourbase" (giving it a special description), socks and (depending on the gender) briefs or panties.

#### `pets`

(optional, array of string mtype_ids )

A list of strings, each is the same as a monster id
player will start with these as tamed pets.

#### `vehicle`

(optional, string vproto_id )

A  string, which is the same as a vehicle ( vproto_id )
player will start with this as a nearby vehicle.
( it will find the nearest road and place it there, then mark it as "remembered" on the overmap )

#### `flags`

(optional, array of strings)

A list of flags. TODO: document those flags here.

- `NO_BONUS_ITEMS` Prevent bonus items (such as inhalers with the ASTHMA trait) from being given to this profession

#### `cbms`

(optional, array of strings)

A list of CBM ids that are implanted in the character.

#### `traits`

(optional, array of strings)

A list of trait/mutation ids that are applied to the character.

#### `requirement`

(optional, an achievement ID)

The achievement you need to do to access this profession

### Recipes

Crafting recipes are defined as a JSON object with the following fields:

```C++
"result": "javelin",         // ID of resulting item
"byproducts": [ [ "" ] ],    // Optional (default: empty). Additional items generated by crafting this recipe.
"byproduct_group": [         // Optional (default: empty). Same as above, but using item group definitions.
  { "item": "item_id_1", "count": [ 1, 4 ] },
  { "item": "item_id_2", "charges": [ 8, 15 ] },
],
"category": "CC_WEAPON",     // Category of crafting recipe. CC_NONCRAFT used for disassembly recipes
"subcategory": "CSC_WEAPON_PIERCING",
"id_suffix": "",             // Optional (default: empty string). Some suffix to make the ident of the recipe unique. The ident of the recipe is "<id-of-result><id_suffix>".
"variant": "javelin_striped", // Optional (default: empty string). Specifies a variant of the result that this recipe will always produce. This will append the variant's id to the recipe ident "<id-of-result>_<variant_id>".
"override": false,           // Optional (default: false). If false and the ident of the recipe is already used by another recipe, loading of recipes fails. If true and a recipe with the ident is already defined, the existing recipe is replaced by the new recipe.
"delete_flags": [ "CANNIBALISM" ], // Optional (default: empty list). Flags specified here will be removed from the resultant item upon crafting. This will override flag inheritance, but *will not* delete flags that are part of the item type itself.
"skill_used": "fabrication", // Skill trained and used for success checks
"skills_required": [["survival", 1], ["throw", 2]], // Skills required to unlock recipe
"book_learn": {	             // (optional) Books that this recipe can be learned from.
    "textbook_anarch" : {    // ID of the book the recipe can be learned from
        "skill_level" : 7,   // Skill level at which it can be learned
        "recipe_name" : "something", // (optional) Name of the recipe as it should appear in the book's description (default is the name of resulting item of the recipe)
        "hidden" : true },   // (optional) If set to true, recipe will not be shown in the description of the book
    "textbook_gaswarfare" : { // Additional book this recipe can be learnt from.
        "skill_level" : 8
    }
},
"difficulty": 3,             // Difficulty of success check
"time": "5 m",               // Preferred time to perform recipe, can specify in minutes, hours etc.
"time": 5000,                // Legacy time to perform recipe (where 1000 ~= 10 turns ~= 10 seconds game time).
"reversible": true,          // Can be disassembled. Time taken is as long as to craft the item.
"reversible": { "time": "30 s" }, // Can be disassembled. Time to disassemble as specified.
"autolearn": true,           // Automatically learned upon gaining required skills
"autolearn" : [              // Automatically learned upon gaining listed skills
    [ "survival", 2 ],
    [ "fabrication", 3 ]
],
"decomp_learn" : 4,          // Can be learned by disassembling an item of same type as result at this level of the skill_used
"decomp_learn" : [           // Can be learned by disassembling an item of same type as result at specified levels of skills
    [ "survival", 1 ],
    [ "fabrication", 2 ]
],
"activity_level": "LIGHT_EXERCISE", // Options are NO_EXERCISE, LIGHT_EXERCISE, MODERATE_EXERCISE, BRISK_EXERCISE, ACTIVE_EXERCISE, EXTRA_EXERCISE. How energy intensive of an activity this craft is. E.g. making an anvil is much more exercise than cooking a fish.
"proficiencies" : [ // The proficiencies related to this recipe
    {
      "proficiency": "prof_knapping", // The id of a proficiency
      "required": false, // Whether or not you must have the proficiency to craft it. Incompatible with `time_multiplier`
      "time_multiplier": 2.0 // The multiplier on time taken to craft this recipe if you do not have this proficiency
      "skill_penalty": 1.5 // The effective skill penalty when crafting without this proficiency. Defaults to 1.0. Multiple proficiencies will add to this value.
      "learning_time_multiplier": 1.2 // The multiplier on learning speed for this proficiency. By default, it's the time of the recipe, divided by the time multiplier, and by the number of proficiencies that can also be learned from it.
      "max_experience": "15 m" // This recipe cannot raise your experience for that proficiency above 15 minutes worth.
    }
]
"contained": true, // Boolean value which defines if the resulting item comes in its designated container. Automatically set to true if any container is defined in the recipe. 
"container": "jar_glass_sealed", //The resulting item will be contained by the item set here, overrides default container.
"batch_time_factors": [25, 15], // Optional factors for batch crafting time reduction. First number specifies maximum crafting time reduction as percentage, and the second number the minimal batch size to reach that number. In this example given batch size of 20 the last 6 crafts will take only 3750 time units.
"count": 2,                  // Number of resulting items/charges per craft. Uses default charges if not set. If a container is set, this is the amount that gets put inside it, capped by container capacity.
"result_mult": 2,            // Multiplier for resulting items. Also multiplies container items.
"flags": [                   // A set of strings describing boolean features of the recipe
  "BLIND_EASY",
  "ANOTHERFLAG"
],
"result_eocs": [ {"id": "TEST", "effect": { "u_message": "You feel Test" } } // List of inline effect_on_conditions or effect_on_condition ids that attempt to activate when this recipe is successfully finished.  If a value is provided a result becomes optional, though a name and id will be needed it it is missing.  If no result is provided and a description is present, that will be displayed as the result on the crafting gui.
], 
"construction_blueprint": "camp", // an optional string containing an update_mapgen_id.  Used by faction camps to upgrade their buildings
"on_display": false,         // this is a hidden construction item, used by faction camps to calculate construction times but not available to the player
"qualities": [               // Generic qualities of tools needed to craft
  { "id": "CUT", "level": 1, "amount": 1 }
],
"tools": [                   // Specific tools needed to craft
[
  [ "fire", -1 ]             // Charges consumed when tool is used, -1 means no charges are consumed
]],
"using": [                   // Requirement IDs and multipliers of tools and materials used
  [ "req_a", 3 ],            // Second number multiplies requirement materials by that amount
  [ "req_b", 5 ],            // Need 3x everything in req_a, 5x everything in req_b
],
"components": [              // Items (or item alternatives) required to craft this recipe
  [
    [ "item_a", 5 ]          // First ingredient: need 5 of item_a
  ],
  [
    [ "item_b", 2 ],         // Also need 2 of item_b...
    [ "item_c", 4 ]          // OR 4 of item_c (but do not need both)
  ],
  [
    // ... any number of other component ingredients (see below)
  ]
],
"component_blacklist": [     // List of item types that don't get added to result item components. Reversible recipes won't recover these and comestibles will not include them in calorie calculations.
  "item_a",
  "item_b"
]
```

#### Practice recipes

Recipes may instead be defined with type "practice", to make them appear in the "PRACTICE" tab of
the crafting menu.  These recipes do not have a "result", but they may define "byproducts"/"byproduct_group".
See [PRACTICE_RECIPES.md](PRACTICE_RECIPES.md) for how to define them.

#### Nested recipes

Similar recipes may instead be nested allowing you to save space in the UI.  This is done as such:
```json
{
  "id": "nested_steel_legs",
  "type": "nested_category",
  "activity_level": "BRISK_EXERCISE",
  "category": "CC_ARMOR",
  "subcategory": "CSC_ARMOR_LEGS",
  "name": "steel leg guards",
  "description": "Recipes related to constructing steel leg guards in various thickness and steel variants.",
  "skill_used": "fabrication",
  "nested_category_data": [
    "xl_armor_qt_heavy_leg_guard",
    "armor_qt_heavy_leg_guard",
    "xl_armor_ch_heavy_leg_guard",
    "armor_ch_heavy_leg_guard",
    "xl_armor_hc_heavy_leg_guard",
    "armor_hc_heavy_leg_guard",
    "xl_armor_mc_heavy_leg_guard",
    "armor_mc_heavy_leg_guard",
    "xl_armor_lc_heavy_leg_guard",
    "armor_lc_heavy_leg_guard",
    "xl_armor_qt_leg_guard",
    "armor_qt_leg_guard",
    "xl_armor_ch_leg_guard",
    "armor_ch_leg_guard",
    "xl_armor_hc_leg_guard",
    "armor_hc_leg_guard",
    "xl_armor_mc_leg_guard",
    "armor_mc_leg_guard",
    "xl_armor_lc_leg_guard",
    "armor_lc_leg_guard",
    "xl_armor_qt_light_leg_guard",
    "armor_qt_light_leg_guard",
    "xl_armor_ch_light_leg_guard",
    "armor_ch_light_leg_guard",
    "xl_armor_hc_light_leg_guard",
    "armor_hc_light_leg_guard",
    "xl_armor_mc_light_leg_guard",
    "armor_mc_light_leg_guard",
    "xl_armor_lc_light_leg_guard",
    "armor_lc_light_leg_guard"
  ],
  "difficulty": 5,
  "autolearn": [ [ "fabrication", 5 ] ]
}
```

So it is identical to a normal recipe with the addition of the "nested_category_data" which lists all of the recipe ID's that are in the category.

If you want to hide recipes that are nested you can set their category and subcategory as:

```json
"category": "CC_*",
"subcategory": "CSC_*_NESTED",
```

#### Recipe requirements

The tool quality and component requirements for a recipe may be expressed in a combination of
several ways, with these JSON fields:

- "qualities" defines item qualities like CUT or HAMMER, and quality levels needed to craft
- "tools" lists *item* ids of tools (or several alternative tools) needed for crafting the recipe
- "components" lists *item* or *requirement* ids, intended mainly for material ingredients
- "using" gives *requirement* ids; the requirement may have nested tools, qualities, or components

These fields may be used similarly in uncrafting, constructions, vehicle parts, and vehicle faults.
The first three fields are applicable to "requirement" definitions as well, and may be nested; see
the [requirements section](#datajsonrequirements).

A recipe's "components" lists all the required items or ingredients needed to craft the finished
item from the recipe.  Each component is given as an integer quantity of a specific item id or
requirement id, or as a list of several alternative item/requirement quantities.

The syntax of a component in its simplest form is an item id and quantity.  Continuing the "javelin"
recipe, let's require a single "spear_wood" item:

```json
"components": [
  [ [ "spear_wood", 1 ] ]
]
```

A single component may also have substitutions; for instance, to allow crafting from one
"spear_wood" *or* one "pointy_stick":

```json
"components": [
  [ [ "spear_wood", 1 ], [ "pointy_stick", 1 ] ]
]
```

Notice that the first example with *only* "spear_wood" was simply the degenerate case - a list of
alternatives with only 1 alternative - which is why it was doubly nested in `[ [ ... ] ]`.

The javelin would be better with some kind of leather or cloth grip.  To require 2 rags, 1 leather,
or 1 fur *in addition to* the wood spear or pointy stick:

```json
"components": [
  [ [ "spear_wood", 1 ], [ "pointy_stick", 1 ] ],
  [ [ "rag", 2 ], [ "leather", 1 ], [ "fur", 1 ] ]
]
```

And to bind the grip onto the javelin, some sinew or thread should be required, which can have the
"NO_RECOVER" keyword to indicate they cannot be recovered if the item is deconstructed:

```json
"components": [
  [ [ "spear_wood", 1 ], [ "pointy_stick", 1 ] ],
  [ [ "rag", 2 ], [ "leather", 1 ], [ "fur", 1 ] ],
  [ [ "sinew", 20, "NO_RECOVER" ], [ "thread", 20, "NO_RECOVER" ] ]
]
```

*Note*: Related to "NO_RECOVER", some items such as "superglue" and "duct_tape" have an
"UNRECOVERABLE" flag on the item itself, indicating they can never be reclaimed when disassembling.
See [JSON_FLAGS.md](JSON_FLAGS.md) for how to use this and other item flags.

#### Defining common requirements

To avoid repeating commonly used sets of components, instead of an individual item id, provide
the id of a `requirement` type, along with a quantity, and the `"LIST"`
keyword.  Typically these are defined within
[`data/json/requirements`](#datajsonrequirements).

For example if these `grip_patch` and `grip_wrap` requirements were defined:

```json
[
  {
    "id": "grip_patch",
    "type": "requirement",
    "components": [ [ [ "rag", 2 ], [ "leather", 1 ], [ "fur", 1 ] ] ]
  },
  {
    "id": "grip_wrap",
    "type": "requirement",
    "components": [ [ [ "sinew", 20, "NO_RECOVER" ], [ "thread", 20, "NO_RECOVER" ] ] ]
  }
]
```

Then javelin recipe components could use 1 grip and 1 wrap, for example:

```json
"result": "javelin",
"components": [
  [ [ "spear_wood", 1 ], [ "pointy_stick", 1 ] ],
  [ [ "grip_patch", 1, "LIST" ] ],
  [ [ "grip_wrap", 1, "LIST" ] ]
]
```

And other recipes needing two such grips could simply require 2 of each:

```json
"result": "big_staff",
"components": [
  [ [ "stick_long", 1 ] ],
  [ [ "grip_patch", 2, "LIST" ] ],
  [ [ "grip_wrap", 2, "LIST" ] ]
]
```

The `"using"` field in a recipe works similarly, but `"using"` may only refer
to requirement ids, not specific items or tools.  A requirement included with
`"using"` must also give a multiplier, telling how many units of that
requirement are needed.  As with `"components"`, the "using" list is formatted
as a collection of alternatives, even if there is only one alternative.

For instance, this `"uncraft"` recipe for a motorbike alternator uses either 20 units of the
`"soldering_standard"` requirement, or 5 units of the `"welding_standard"` requirement:

```json
{
  "type": "uncraft",
  "result": "alternator_motorbike",
  "qualities": [ { "id": "SCREW", "level": 1 } ],
  "using": [ [ "soldering_standard", 20 ], [ "welding_standard", 5 ] ],
  "components": [ [ [ "power_supply", 1 ] ], [ [ "cable", 20 ] ], [ [ "bearing", 5 ] ], [ [ "scrap", 2 ] ] ]
}
```

Requirements may include `"tools"` or `"qualities"` in addition to
`"components"`.  Here we have a standard soldering requirement needing either a
`"soldering_iron"` or `"toolset"`, plus 1 unit of the `"solder_wire"` component:


```json
{
  "id": "soldering_standard",
  "type": "requirement",
  "//": "Tools and materials needed for soldering metal items or electronics",
  "tools": [ [ [ "soldering_iron", 1 ], [ "toolset", 1 ] ] ],
  "components": [ [ [ "solder_wire", 1 ] ] ]
}
```

This simplifies recipes needing soldering, via the `"using"` field.  For
instance, a simple `"tazer"` recipe could require 10 units of the soldering
requirement, along with some other components:

```json
{
  "type": "recipe",
  "result": "tazer",
  "using": [ [ "soldering_standard", 10 ] ],
  "components": [ [ [ "amplifier", 1 ] ], [ [ "power_supply", 1 ] ], [ [ "scrap", 2 ] ] ],
  "//": "..."
}

```

Requirements can be used not just for regular crafting and uncrafting recipes,
but also for constructions and vehicle part installation and mending.

***NOTE:*** Requirement lists included in mods overwrite the previously loaded
requirement list with a matching id. This means that two mods modifying the same
requirement id will overwrite each other. This can be avoided by using `"extend"`
to extend from the previously loaded list. Ex.:

```json
{
  "id": "bone_sturdy",
  "type": "requirement",
  "extend": {
    "components": [
      [
        [ "frost_bone_human", 1 ],
        [ "alien_bone", 1 ]
      ]
    ]
  }
}
```


#### Overlapping recipe component requirements

If recipes have requirements which overlap, this makes it more
difficult for the game to calculate whether it is possible to craft a recipe at
all.

For example, the survivor telescope recipe has the following requirements
(amongst others):

```
1 high-quality lens
AND
1 high-quality lens OR 1 small high-quality lens
```

These overlap because both list the high-quality lens.

A small amount of overlap (such as the above) can be handled, but if you have
too many component lists which overlap in too many ways, then you may see an
error during recipe finalization that your recipe is too complex.  In this
case, the game may not be able to correctly predict whether it can be crafted.

To work around this issue, if you do not wish to simplify the recipe
requirements, then you can split your recipe into multiple steps.  For
example, if we wanted to simplify the above survivor telescope recipe we could
introduce an intermediate item "survivor eyepiece", which requires one of
either lens, and then the telescope would require a high-quality lens and an
eyepiece.  Overall, the requirements are the same, but neither recipe has any
overlap.

For more details, see [this pull
request](https://github.com/CleverRaven/Cataclysm-DDA/pull/36657) and the
[related issue](https://github.com/CleverRaven/Cataclysm-DDA/issues/32311).

### Constructions
```C++
"group": "spike_pit",                                               // Construction group, used to group related constructions in UI
"category": "DIG",                                                  // Construction category
"required_skills": [ [ "survival", 1 ] ],                           // Skill levels required to undertake construction
"time": "30 m",                                                     // Time required to complete construction. Integers will be read as minutes or a time string can be used.
"components": [ [ [ "spear_wood", 4 ], [ "pointy_stick", 4 ] ] ],   // Items used in construction
"pre_special": "check_empty",                                       // Required something that isn't terrain
"pre_terrain": "t_pit",                                             // Alternative to pre_special; Required terrain to build on
"pre_flags": [ "WALL", { "flag": "DIGGABLE", "force_terrain": true } ], // Flags beginning furniture/terrain must have. force_ter forces the flag to apply to the underlying terrain
"post_terrain": "t_pit_spiked"                                      // Terrain type after construction is complete
"strict": false                                                     // if true, the build activity for this construction will only look for prerequisites in the same group
```

| pre_special            | Description
|---                     |---
| `check_channel`        | Must be empty and have a current in at least one orthogonal tile
| `check_empty`          | Tile is empty (no furniture, trap, item, or vehicle) and flat terrain
| `check_empty_lite`     | Tile is empty (no furniture, trap, item, or vehicle)
| `check_support`        | Must have at least two solid walls/obstructions nearby on orthogonals (non-diagonal directions only) to support the tile
| `check_support_below`  | Must have at least two solid walls/obstructions at the Z level below on orthogonals (non-diagonal directions only) to support the tile and be empty lite but with a ledge trap acceptable, as well as open air
| `check_stable`         | Tile on level below has a flag `SUPPORTS_ROOF`
| `check_empty_stable`   | Tile is empty and stable
| `check_nofloor_above`  | Tile on level above has a flag `NO_FLOOR`
| `check_deconstruction` | The furniture (or tile, if no furniture) in the target tile must have a "deconstruct" entry
| `check_empty_up_OK`    | Tile is empty and is below the maximum possible elevation (can build up here)
| `check_up_OK`          | Tile is below the maximum possible elevation (can build up here)
| `check_down_OK`        | Tile is above the lowest possible elevation (can dig down here)
| `check_no_trap`        | There is no trap object in this tile
| `check_ramp_low`       | Both this and the next level above can be built up one additional Z level
| `check_ramp_high`      | There is a complete downramp on the next higher level, and both this and next level above can be built up one additional Z level
| `check_no_wiring`      | The tile must either be free of a vehicle, or at least a vehicle that doesn't have the WIRING flag

### Scent_types

| Identifier               | Description
|---                       |---
| `id`                     | Unique ID. Must be one continuous word, use underscores if necessary.
| `receptive_species`      | Species able to track this scent. Must use valid ids defined in `species.json`

```json
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
```C++
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

```C++
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

```C++
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

```C++
"event_type" : "avatar_moves" // Events of this built-in type
"event_transformation" : "moves_on_horse" // Events resulting from this json-defined transformation
```

Then it specifies a particular `stat_type` and potentially additional details
as follows:

The number of events:
```C++
"stat_type" : "count"
```

The sum of the numeric value in the specified field across all events:
```C++
"stat_type" : "total"
"field" : "damage"
```

The maximum of the numeric value in the specified field across all events:
```C++
"stat_type" : "maximum"
"field" : "damage"
```

The minimum of the numeric value in the specified field across all events:
```C++
"stat_type" : "minimum"
"field" : "damage"
```

Assume there is only a single event to consider, and take the value of the
given field for that unique event:
```C++
"stat_type": "unique_value",
"field": "avatar_id"
```

The value of the given field for the first event in the input stream:
```C++
"stat_type": "first_value",
"field": "avatar_id"
```

The value of the given field for the last event in the input stream:
```C++
"stat_type": "last_value",
"field": "avatar_id"
```

Regardless of `stat_type`, each `event_statistic` can also have:
```C++
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

```C++
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

```C++
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

```C++
"hidden_by": [ "other_achievement_id" ]
```

Give a list of other achievement ids.  This achievement will be hidden (i.e.
not appear in the achievements UI) until all of the achievements listed have
been completed.

Use this to prevent spoilers or to reduce clutter in the list of achievements.

If you want an achievement to be hidden until completed, then mark it as
`hidden_by` its own id.

```C++
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

```C++
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

```C++
"id" : "smg",  // Unique ID. Must be one continuous word, use underscores if necessary
"name" : "submachine guns",  // In-game name displayed
"description" : "Your skill with submachine guns and machine pistols. Halfway between a pistol and an assault rifle, these weapons fire and reload quickly, and may fire in bursts, but they are not very accurate.", // In-game description
"tags" : ["gun_type"]  // Special flags (default: none)
```

### Speed Description

```C++
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
```C++
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
```C++
{
  "type": "tool_quality",
  "id": "SAW_W",                      // Unique ID
  "name": { "str": "wood sawing" },   // Description used in tabs in-game when looking at entries with the id
  "usages": [ [ 2, [ "LUMBER" ] ] ]   // Not mandatory.  The special actions that may be performed with the item.
},
```

Examples of various usages syntax:
```C++
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

```C++
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
    "map_regen": "microlab_shifting_hall",  // a valid overmap id, for map_regen action traps
    "benign": true, // For things such as rollmats, funnels etc. They can not be triggered.
    "always_invisible": true, // Super well hidden traps the player can never detect
    "funnel_radius": 200, // millimeters. The higher the more rain it will capture.
    "comfort": 0, // Same property affecting furniture and terrain
    "floor_bedding_warmth": -500, // Same property affecting furniture and terrain
    "spell_data": { "id": "bear_trap" }, // data required for trapfunc::spell()
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
    "trigger_message_npc": "A bear trap closes on <npcname>'s foot!" // This message will be printed when NPC or monster steps on a trap
```

### Vehicle Groups


```C++
"id":"city_parked",            // Unique ID. Must be one continuous word, use underscores if necessary
"vehicles":[                 // List of potential vehicle ID's. Chance of a vehicle spawning is X/T, where
  ["suv", 600],           //    X is the value linked to the specific vehicle and T is the total of all
  ["pickup", 400],          //    vehicle values in a group
  ["car", 4700],
  ["road_roller", 300]
]
```

### Vehicle Parts

Vehicle components when installed on a vehicle.

```C++
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
                              // One of "post_terain", "post_furniture", or "post_field" is required.
  "pre_flags": [ "PLOWABLE" ], // List of flags for the terrain that can be transformed.
  "post_terrain": "t_dirtmound", // (Optional, default to "t_null") The resulting terrain, if any.
  "post_furniture": "f_boulder", // (Optional, default to "f_null") The resulting furniture, if any.
  "post_field": "fd_fire",    // (Optional, default to "fd_null") The resulting field, if any.
  "post_field_intensity": 10, // (Optional, default to 0) The field's intensity, if any.
  "post_field_age": "20 s"    // (Optional, default to 0 turns) The field's time to live, if any.
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
```c++
"size": "400 L",              // for parts with "CARGO" flag the capacity in liters
"cargo_weight_modifier": 33,  // (Optional, default = 100) Multiplies cargo weight by this percentage.
```

#### The following optional fields are specific to ENGINEs.
```c++
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
```c++
"wheel_offroad_rating": 0.5,  // multiplier of wheel performance offroad
"wheel_terrain_modifiers": { "FLAT": [ 0, 5 ], "ROAD": [ 0, 2 ] }, // see below
"contact_area": 153,          // The surface area of the wheel in contact with the ground under
                              // normal conditions in cm^2.  Wheels with higher contact area
                              // perform better off-road.
"rolling_resistance": 1.0,    // The "squishiness" of the wheel, per SAE standards.  Wheel rolling
                              // resistance increases vehicle drag linearly as vehicle weight
                              // and speed increase.
```

`wheel_terrain_modifiers` field provides a way to modify wheel traction according to the flags set on terrain tile under each wheel.

The key is one of the terrain flags, the list of flags can be found in [JSON_FLAGS.md](JSON_FLAGS.md#furniture-and-terrain).

The value expects an array of length 2. The first element is a modifier override applied when wheel is on the flagged terrain, the second element is an additive modifier penalty applied when wheel is NOT on flagged terrain, values of 0 are ignored. The modifier is applied over a base value provided by `map::move_cost_ter_furn`.

Examples:
* Standard `wheel` has the field set to `{ "FLAT": [ 0, 4 ], "ROAD": [ 0, 2 ] }`. If wheel is not on terrain flagged `FLAT` then the traction is 1/4 of base value. If not on terrain flagged `ROAD` then it's 1/2 of base value. If neither flag is present then traction will be 1/6 of base value. If terrain is flagged with both `ROAD` and `FLAT` then the base value from `map::move_cost_ter_furn` is used.
* `rail_wheel` has the field set to `{ "RAIL": [ 2, 8 ] }`. If wheel is on terrain flagged `RAIL` the traction is overriden to be 1/2 of value calculated by `map::move_cost_ter_furn`, this value is the first element and considered an override, so if there had been modifiers applied prior to this they are ignored. If on terrain not flagged with `RAIL` then traction will be 1/8 of base value.


#### The following optional fields are specific to ROTORs.
```c++
"rotor_diameter": 15,         // Rotor diameter in meters.  Larger rotors provide more lift.
```

#### The following optional fields are specific to WORKBENCHes.
These values apply to crafting tasks performed at the WORKBENCH.
```c++
"multiplier": 1.1,            // Crafting speed multiplier.
"mass": 1000000,              // Maximum mass in grams of a completed craft that can be crafted.
"volume": "20L",              // Maximum volume (as a string) of a completed craft that can be craft.
```

#### The following optional fields are specific to SEATs.
```c++
"comfort": 3,                 // (Optional, default=0). Sleeping comfort as for terrain/furniture.
"floor_bedding_warmth": 300,  // (Optional, default=0). Bonus warmth as for terrain/furniture.
"bonus_fire_warmth_feet": 200,// (Optional, default=0). Bonus fire warmth as for terrain/furniture.
```

#### The following optional field describes pseudo tools for any part.
Crafting stations (e.g. kitchen, welding rigs etc) have tools that they provide as part
of forming the inventory for crafting as well as providing menu items when `e`xamining
the vehicle tile.
Following example array gives the vpart a pot as passive tool for crafting because it has no hotkey defined.
It also has a hotplate that can be activated by examining it with `e` then `h` on the part's vehicle tile.
```c++
"pseudo_tools" : [
  { "id": "hotplate", "hotkey": "h" },
  { "id": "pot" }
],
```

### Part Resistance
Damage resistance values, used by:
- `armor` of [`"type": "body_part"`](#body_parts)
- `damage_reduction` of [`"type": "vehicle_part"`](#vehicle-parts)

```C++
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
```C++
"id":"road_straight_wrecks",  // Unique ID. Must be one continuous word, use underscores if necessary
"locations":[ {               // List of potential vehicle locations. When this placement is used, one of those locations will be chosen at random.
  "x" : [0,19],               // The x placement. Can be a single value or a range of possibilities.
  "y" : 8,                    // The y placement. Can be a single value or a range of possibilities.
  "facing" : [90,270]         // The facing of the vehicle. Can be a single value or an array of possible values.
} ]
```

### Vehicle Spawn

```C++
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

```C++
"id": "shopping_cart",                     // Internally-used name.
"name": "Shopping Cart",                   // Display name, subject to i18n.
"blueprint": "#",                          // Preview of vehicle - ignored by the code, so use only as documentation
"parts": [                                 // Parts list
    {"x": 0, "y": 0, "part": "box"},       // Part definition, positive x direction is to the left, positive y is to the right
    {"x": 0, "y": 0, "part": "casters"}    // See vehicle_parts.json for part ids
]
                                           /* Important! Vehicle parts must be defined in the
                                            * same order you would install
                                            * them in the game (ie, frames and mount points first).
                                            * You also cannot break the normal rules of installation
                                            * (you can't stack non-stackable part flags). */
```

### Weakpoint Sets
A thin container for weakpoint definitions. The only unique fields for this object are `"id"` and `"type"`. The `"weakpoints"` array contains weakpoints that are defined the same way as in monster definitions. See [Weakpoints](MONSTERS.md#weakpoints) for details.

```json
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
```json
"//": "(in MONSTER type)",
"weakpoint_sets": [ "humanoid", "zombie_headshot", "riot_gear" ]
```
In the example above, the `"humanoid"` weakpoint set is applied as a base, then the `"zombie_headshot"` set overwrites any previously defined weakpoints with the same id (ex: "wp_head_stun"). Then the `"riot_gear"` set overwrites any matching weakpoints from the previous sets with armour-specific weakpoints. Finally, if the monster type has an inline `"weakpoints"` definition, those weakpoints overwrite any matching weakpoints from all sets.

Weakpoints only match if they share the same id, so it's important to define the weakpoint's id field if you plan to overwrite previous weakpoints.

# `data/json/items/` JSONs

### Generic Items

```C++
"type": "GENERIC",                // Defines this as some generic item
"id": "socks",                    // Unique ID. Must be one continuous word, use underscores if necessary
"name": {
    "ctxt": "clothing",           // Optional translation context. Useful when a string has multiple meanings that need to be translated differently in other languages.
    "str": "pair of socks",       // The name appearing in the examine box.  Can be more than one word separated by spaces
    "str_pl": "pairs of socks"    // Optional. If a name has an irregular plural form (i.e. cannot be formed by simply appending "s" to the singular form), then this should be specified. "str_pl" may also be needed if the unit test cannot determine if the correct plural form can be formed by simply appending "s". "str_sp" should be used instead of "str" or "str_pl" if the singular and plural forms are the same.
},
"conditional_names": [ {          // Optional list of names that will be applied in specified conditions (see Conditional Naming section for more details).
    "type": "COMPONENT_ID",       // The condition type.
    "condition": "leather",       // The condition to check for.
    "name": { "str": "pair of leather socks", "str_pl": "pairs of leather socks" } // Name field, same rules as above.
} ],
"container": "null",             // What container (if any) this item should spawn within
"repairs_like": "scarf",          // If this item does not have recipe, what item to look for a recipe for when repairing it.
"color": "blue",                 // Color of the item symbol.
"symbol": "[",                   // The item symbol as it appears on the map. Must be a Unicode string exactly 1 console cell width.
"looks_like": "rag",              // hint to tilesets if this item has no tile, use the looks_like tile
"description": "Socks. Put 'em on your feet.", // Description of the item
"ascii_picture": "ascii_socks", // Id of the asci_art used for this item
"phase": "solid",                            // (Optional, default = "solid") What phase it is
"weight": "350 g",                           // Weight, weight in grams, mg and kg can be used - "50 mg", "5 g" or "5 kg". For stackable items (ammo, comestibles) this is the weight per charge.
"volume": "250 ml",                          // Volume, volume in ml and L can be used - "50 ml" or "2 L". For stackable items (ammo, comestibles) this is the volume of stack_size charges.
"integral_volume": "50 ml",                        // Volume added to base item when item is integrated into another (eg. a gunmod integrated to a gun). Volume in ml and L can be used - "50 ml" or "2 L". Default is the same as volume.
"integral_weight": "50 g",                        // Weight added to base item when item is integrated into another (eg. a gunmod integrated to a gun). Default is the same as weight.
"longest_side": "15 cm",                     // Length of longest item dimension. Default is cube root of volume.
"rigid": false,                              // For non-rigid items volume (and for worn items encumbrance) increases proportional to contents
"insulation": 1,                             // (Optional, default = 1) If container or vehicle part, how much insulation should it provide to the contents
"price": 100,                                // Used when bartering with NPCs. For stackable items (ammo, comestibles) this is the price for stack_size charges. Can use string "cent" "USD" or "kUSD".
"price_postapoc": "1 USD",                       // Same as price but represent value post cataclysm. Can use string "cent" "USD" or "kUSD".
"degradation_multiplier": 0.8,               // Controls how quickly an item degrades when taking damage. 0 = no degradation. Defaults to 1.0.
"material": [                                // Material types, can be as many as you want.  See materials.json for possible options
  { "type": "cotton", "portion": 9 },        // type indicates the material's ID, portion indicates proportionally how much of the item is composed of that material
  { "type": "plastic" }                      // portion can be omitted and will default to 1. In this case, the item is 90% cotton and 10% plastic.
],
"repairs_with": [ "plastic" ],               // Material types that this item can be repaired with. Defaults to all the item materials.
"weapon_category": [ "WEAPON_CAT1" ],        // (Optional) Weapon categories this item is in for martial arts.
"melee_damage": {                            // (Optional) Damage caused by using it as a melee weapon.  These values cannot be negative.
  "bash": 0,
  "cut": 0
},
"to_hit": 0,                                 // (Optional, deprecated, default = 0) To-hit bonus if using it as a melee weapon (whatever for?).  The object version is preferred
"to_hit" {                                   // (Optional, Preferred) To hit bonus values, see below
  "grip": "solid",                           // the item's grip value
  "length": "long",                          // the item's length value
  "surface": "point",                        // the item's striking surface value
  "balance": "neutral"                       // the item's balance value
},
"variant_type": "gun"      // Possible options: "gun", "generic" - controls which options enable/disable seeing the variants of this item.
"variants": [              // Cosmetic variants this item can have
  {
    "id": "varianta",                           // id used in spawning to spawn this variant specifically
    "name": { "str": "Variant A" },             // The name used instead of the default name when this variant is selected
    "description": "A fancy variant A",         // The description used instead of the default when this variant is selected
    "ascii_picture": "valid_ascii_art_id",      // An ASCII art picture used when this variant is selected. If there is none, the default (if it exists) is used.
    "symbol": "/",                              // Valid unicode character to replace the item symbol. If not specified, no change will be made.
    "color": "red",                             // Replacement color of item symbol. If not specified, no change will be made.
    "weight": 2,                                // The relative chance of this variant being selected over other variants when this item is spawned with no explicit variant. Defaults to 0. If it is 0, this variant will not be selected
    "append": true                              // If this description should just be appended to the base item description instead of completely overwriting it.
  }
],
"flags": ["VARSIZE"],                        // Indicates special effects, see JSON_FLAGS.md
"environmental_protection_with_filter": 6,   // the resistance to environmental effects if an item (for example a gas mask) requires a filter to operate and this filter is installed. Used in combination with use_action 'GASMASK' and 'DIVE_TANK'
"magazine_well": 0,                          // Volume above which the magazine starts to protrude from the item and add extra volume
"magazines": [                               // Magazines types for each ammo type (if any) which can be used to reload this item
    [ "9mm", [ "glockmag" ] ]                // The first magazine specified for each ammo type is the default
    [ "45", [ "m1911mag", "m1911bigmag" ] ],
],
"milling": {                                 // Optional. If given, the item can be milled in a water/wind mill.
  "into": "flour",                           // The item id of the result of the milling.
  "conversion_rate": 1.0                     // Conversion of number of items that are milled (e.g. with a rate of 2, 10 input items will yield 20 milled items).
},
"explode_in_fire": true,                     // Should the item explode if set on fire
"explosion": {                               // Physical explosion data
    "power": 10,                             // Measure of explosion power in grams of TNT equivalent explosive, affects damage and range.
    "distance_factor": 0.9,                  // How much power is retained per traveled tile of explosion. Must be lower than 1 and higher than 0.
    "max_noise": 25,                         // Maximum amount of (auditory) noise the explosion might produce.
    "fire": true,                            // Should the explosion leave fire
    "shrapnel": 200,                         // Total mass of casing, rest of fragmentation variables set to reasonable defaults.
    "shrapnel": {
        "casing_mass": 200,                  // Total mass of casing, casing/power ratio determines fragment velocity.
        "fragment_mass": 0.05,               // Mass of each fragment in grams. Large fragments hit harder, small fragments hit more often.
        "recovery": 10,                      // Percentage chance to drop an item at landing point.
        "drop": "nail"                       // Which item to drop at landing point.
    }
},
```

#### To hit object
For additional clarity, an item's `to_hit` bonus can be encoded as string of 4 fields.  All the fields are mandatory:

```C++
"to_hit": {
    "grip": "weapon",      // one of "bad", "none", "solid", or "weapon"
    "length": "hand",      // one of "hand", "short", or "long"
    "surface": "any",      // one of "point", "line", "any", or "every"
    "balance": "neutral"   // one of "clumsy", "uneven", "neutral", or "good"
}
```
See [GAME_BALANCE.md](GAME_BALANCE.md)'s `MELEE_WEAPONS` section for the criteria for selecting each value.

### Ammo

```C++
"type" : "AMMO",      // Defines this as ammo
...                   // same entries as above for the generic item.
                      // additional some ammo specific entries:
"ammo_type" : "shot", // Determines what it can be loaded in
"damage" : 18,        // Ranged damage when fired
"prop_damage": 2,     // Multiplies the damage of weapon by amount (overrides damage field)
"pierce" : 0,         // Armor piercing ability when fired
"range" : 5,          // Range when fired
"range_multiplier": 2,// Optional field multiplying base gun range
"dispersion" : 0,     // Inaccuracy of ammo, measured in 100ths of Minutes Of Angle (MOA)
"shot_count": 5,      // Optional field specifying that this ammo fires multiple projectiles per round, e.g. shot. If present shot_damage must also be specified.
"shot_damage": { "damage_type": "bullet", "amount": 15 } // Optional field specifying the damage caused by a single projectile fired from this round. If present shot_count must also be specified.
"shot_spread":        // Optional field specifying the additional dispersion of single projectiles. Only meaningful if shot_count is present.
"recoil" : 18,        // Recoil caused when firing
"count" : 25,         // Number of rounds that spawn together
"stack_size" : 50,    // (Optional) How many rounds are in the above-defined volume. If omitted, is the same as 'count'
"show_stats" : true,  // (Optional) Force stat display for combat ammo. (for projectiles lacking both damage and prop_damage)
"loudness": 10,       // (Optional) Modifier that can increase or decrease base gun's noise when firing. If loudness value is not specified, then game calculates it automatically from ammo's range, damage, and armor penetration.

"effects" : ["COOKOFF", "SHOT"]
```

### Ammo Effects

```C++
    "id": "TACTICAL_LASER_EXPLOSION",   // Defines this as some generic item
    "type": "ammo_effect",              // Defines this as an ammo_effect 
    "trigger_chance": 5,                // Option one in X chances for the rest of json defined ammo_effect properties to trigger at the hit location. Defaults to 1
    "explosion": {  }                   // (Optional) Creates an explosion at the hit location. See "explosion" for details.
    "aoe": {  },                        // (Optional) Spawn a square of specified fields on the hit location.
    "trail": {  }                       // (Optional) Spawn a line of fields on the projectiles path.  Not affected by trigger_chance.
    "foamcrete_build": true             // (Optional) Creates foamcrete fields and walls on the hit location.
    "do_flashbang": true                // (Optional) Creates a hardcoded Flashbang explosion.
    "do_emp_blast": true                // (Optional) Creates a one tile radious EMP explosion at the hit location.
```

### Magazine

```C++
"type": "MAGAZINE",              // Defines this as a MAGAZINE
...                              // same entries as above for the generic item.
// Only MAGAZINE type items may define the following fields:
"ammo_type": [ "40", "357sig" ], // What types of ammo this magazine can be loaded with
"capacity" : 15,                 // Capacity of magazine (in equivalent units to ammo charges)
"count" : 0,                     // Default amount of ammo contained by a magazine (set this for ammo belts)
"default_ammo": "556",           // If specified override the default ammo (optionally set this for ammo belts)
"reload_time" : 100,             // How long it takes to load each unit of ammo into the magazine
"linkage" : "ammolink"           // If set one linkage (of given type) is dropped for each unit of ammo consumed (set for disintegrating ammo belts)
```


### Armor

Armor can be defined like this:

```C++
"type" : "ARMOR",                   // Defines this as armor
...                                 // same entries as above for the generic item.
                                    // additional some armor specific entries:
"covers" : [ "foot_l", "foot_r" ],  // Where it covers.  Use bodypart_id defined in body_parts.json  Also note that LEG_EITHER ARM_EITHER HAND_EITHER and FOOT_EITHER are allowed.
"warmth" : 10,                      //  (Optional, default = 0) How much warmth clothing provides
"environmental_protection" : 0,     //  (Optional, default = 0) How much environmental protection it affords
"encumbrance" : 0,                  // Base encumbrance (unfitted value)
"max_encumbrance" : 0,              // When a character is completely full of volume, the encumbrance of a non-rigid storage container will be set to this. Otherwise it'll be between the encumbrance and max_encumbrance following the equation: encumbrance + (max_encumbrance - encumbrance) * non-rigid volume / non-rigid capacity.  By default, max_encumbrance is encumbrance + (non-rigid volume / 250ml).
"weight_capacity_bonus": "20 kg",   // (Optional, default = 0) Bonus to weight carrying capacity, can be negative. Strings must be used - "5000 g" or "5 kg"
"weight_capacity_modifier": 1.5,    // (Optional, default = 1) Factor modifying base weight carrying capacity.
"coverage": 80,                     // What percentage of body part is covered (in general)
"cover_melee": 60,                  // What percentage of body part is covered (against melee)
"cover_ranged": 45,                 // What percentage of body part is covered (against ranged)
"cover_vitals": 10,                 // What percentage of critical hit damage is mitigated
"material_thickness" : 1,           // Thickness of material, in millimeter units (approximately).  Ordinary clothes range from 0.1 to 0.5. Particularly rugged cloth may reach as high as 1-2mm, and armor or protective equipment can range as high as 10 or rarely more.
"power_armor" : false,              // If this is a power armor item (those are special).
"non_functional" : "destroyed",     //this is the itype_id of an item that this turns into when destroyed. Currently only works for ablative armor.
"damage_verb": "makes a crunch, something has shifted", // if an item uses non-functional this will be the description when it turns into its non functional variant.
"valid_mods" : ["steel_padded"],    // List of valid clothing mods. Note that if the clothing mod doesn't have "restricted" listed, this isn't needed.
"armor": [ ... ]
```

#### Armor Portion Data
Encumbrance and coverage can be defined on a piece of armor as such:

```json
"armor": [
  {
    "encumbrance": [ 2, 8 ],
    "coverage": 95,
    "cover_melee": 95,
    "cover_ranged": 50,
    "cover_vitals": 5,
    "covers": [ "torso" ],
    "specifically_covers": [ "torso_upper", "torso_neck", "torso_lower" ],
    "material": [
      { "type": "cotton", "covered_by_mat": 100, "thickness": 0.2 },
      { "type": "plastic", "covered_by_mat": 15, "thickness": 0.8 }
    ]
  },
  {
    "encumbrance": 2,
    "coverage": 80,
    "cover_melee": 80,
    "cover_ranged": 70,
    "cover_vitals": 5,
    "covers": [ "arm_r", "arm_l" ],
    "specifically_covers": [ "arm_shoulder_r", "arm_shoulder_l" ],
    "material": [
      { "type": "cotton", "covered_by_mat": 100, "thickness": 0.2 }
    ]
  }
]
```

##### Encumbrance
(integer, or array of 2 integers)
The value of this field (or, if it is an array, the first value in the array) is the base encumbrance (unfitted) of this item.
When specified as an array, the second value is the max encumbrance - when the pockets of this armor are completely full of items, the encumbrance of a non-rigid item will be set to this. Otherwise it'll be between the first value and the second value following this the equation: first value + (second value - first value) * non-rigid volume / non-rigid capacity.  By default, the max encumbrance is the encumbrance + (non-rigid volume / 250ml).

##### Encumbrance_modifiers
Experimental feature for having an items encumbrance be generated by weight instead of a fixed number. Takes an array of "DESCRIPTORS" described in the code. If you don't need any descriptors put "NONE". This overrides encumbrance putting it as well will make it be ignored. Currently only works for head armor.

##### Coverage
(integer)
What percentage of time this piece of armor will be hit (and thus used as armor) when an attack hits the body parts in `covers`.

`cover_melee` and `cover_ranged` represent the percentage of time this piece of armor will be hit by melee and ranged attacks respectively. Usually these would be the same as `coverage`.

`cover_vitals` represents the percentage of critical hit damage is absorbed. Only the excess damage on top of normal damage is mitigated, so a vital coverage value of 100 means that critical hits would do the same amount as normal hits.

##### Covers
(array of strings)
What body parts this section of the armor covers. See the bodypart_ids defined in body_parts.json for valid values.

##### Specifically Covers
(array of strings)
What sub body parts this section of the armor covers. See the sub_bodypart_ids defined in body_parts.json for valid values.
These are used for wearing multiple armor pieces on a single layer without gaining encumbrance penalties. They are not mandatory
if you don't specify them it is assumed that the section covers all the body parts it covers entirely.
strapped layer items, and outer layer armor should always have these specified otherwise it will conflict with other pieces.

##### Part Materials
(array of objects)
The type, coverage and thickness of the materials that make up this portion of the armor.
- `type` indicates the material ID.
- `covered_by_mat` (_optional_) indicates how much (%) of this armor portion is covered by said material. Defaults to 100.
- `thickness` (_optional_) indicates the thickness of said material for this armor portion. Defaults to 0.0.
The portion coverage and thickness determine how much the material contributes towards the armor's resistances.
**NOTE:** These material definitions do not replace the standard `"material"` tag. Instead they provide more granularity for controlling different armor resistances.

`covered_by_mat` should not be confused with `coverage`. When specifying `covered_by_mat`, treat it like the `portion` field using percentage instead of a ratio value. For example:

```json
"armor": [
  {
    "covers" : [ "arm_r", "arm_l" ],
    "material": [
      {
        "type": "cotton",
        "covered_by_mat": 100,
        "thickness": 0.2
      },
      {
        "type": "plastic",
        "covered_by_mat": 15,
        "thickness": 0.6
      }
    ],
    ...
  }
]
```
The case above describes a portion of armor that covers the arms. This portion is 100% covered by cotton, so a hit to the arm part of the armor will definitely impact the cotton. That portion is also 15% covered by plastic. This means that during damage absorption, the cotton material contributes 100% of its damage absorption, while the plastic material only contributes 15% of its damage absorption. Damage absorption is also affected by `thickness`, so thickness and material cover both provide positive effects for protection.

##### Armor Data
Alternately, every item (book, tool, gun, even food) can be used as armor if it has armor_data:
```C++
"type" : "TOOL",      // Or any other item type
...                   // same entries as for the type (e.g. same entries as for any tool),
"armor_data" : {      // additionally the same armor data like above
    "warmth" : 10,
    "environmental_protection" : 0,
    "armor": [
      {
        "material": [
          { "type": "cotton", "covered_by_mat": 100, "thickness": 0.2 },
          { "type": "plastic", "covered_by_mat": 15, "thickness": 0.6 }
        ],
        "covers" : [ "foot_l", "foot_r" ],
        "encumbrance" : 0,
        "coverage" : 80,
        "cover_melee": 80,
        "cover_ranged": 70,
        "cover_vitals": 5
      }
    ],
    "power_armor" : false
}
```
#### Guidelines for thickness: ####
According to <https://propercloth.com/reference/fabric-thickness-weight/>, dress shirts and similar fine clothing range from 0.15mm to 0.35mm.
According to <https://leathersupreme.com/leather-hide-thickness-in-leather-jackets/>:
* Fashion leather clothes such as thin leather jackets, skirts, and thin vests are 1.0mm or less.
* Heavy leather clothes such as motorcycle suits average 1.5mm.

From [this site](https://cci.one/site/marine/design-tips-fabrication-overview/tables-of-weights-and-measures/), an equivalency guideline for fabric weight to mm:

| Cloth                         | oz/yd2 | g/m2  | Inches | mm   |
| -----                         | ------ | ----- | ------ | ---- |
| Fiberglass (plain weave)      |    2.3 |    78 |  0.004 | 0.10 |
| Fiberglass (plain weave)      |    6.0 |   203 |  0.007 | 0.17 |
| Kevlar (TM) (plain weave)     |    5.0 |   170 |  0.010 | 0.25 |
| Carbon Fiber (plain weave)    |    5.8 |   197 |  0.009 | 0.23 |
| Carbon Fiber (unidirectional) |    9.0 |   305 |  0.011 | 0.28 |

Chart cobbled together from several sources for more general materials:

| Fabric     | oz/yd2  | Max g/m2   | Inches      | mm to use  |
| ---------- | ------- | ---------- | ----------- | ---------- |
| Very light |     0-4 |        136 | 0.006-0.007 |       0.15 |
| Light      |     4-7 |        237 |       0.008 |        0.2 |
| Medium     |    7-11 |        373 | 0.009-0.011 |       0.25 |
| Heavy      |   11-14 |        475 | 0.012-0.014 |        0.3 |

Shoe thicknesses are outlined at <https://secretcobbler.com/choosing-leather/>; TL;DR: upper 1.2 - 2.0mm, lining 0.8 - 1.2mm, for a total of 2.0 - 3.2mm.

For turnout gear, see <https://web.archive.org/web/20220331215535/http://bolivar.mo.us/media/uploads/2014/09/2014-06-bid-fire-gear-packet.pdf>.


### Pet Armor
Pet armor can be defined like this:

```C++
"type" : "PET_ARMOR",     // Defines this as armor
...                   // same entries as above for the generic item.
                      // additional some armor specific entries:
"environmental_protection" : 0,  //  (Optional, default = 0) How much environmental protection it affords
"material_thickness" : 1,  // Thickness of material, in millimeter units (approximately).  Generally ranges between 1 - 5, more unusual armor types go up to 10 or more
"pet_bodytype":        // the body type of the pet that this monster will fit. See MONSTERS.md
"max_pet_vol":          // the maximum volume of the pet that will fit into this armor. Volume in ml or L can be used - "50 ml" or "2 L".
"min_pet_vol":          // the minimum volume of the pet that will fit into this armor. Volume in ml or L can be used - "50 ml" or "2 L".
"power_armor" : false, // If this is a power armor item (those are special).
```
Alternately, every item (book, tool, gun, even food) can be used as armor if it has armor_data:
```C++
"type" : "TOOL",      // Or any other item type
...                   // same entries as for the type (e.g. same entries as for any tool),
"pet_armor_data" : {      // additionally the same armor data like above
    "environmental_protection" : 0,
    "pet_bodytype": "dog",
    "max_pet_vol": "35000 ml",
    "min_pet_vol": "25000 ml",
    "material_thickness" : 1,
    "power_armor" : false
}
```

### Books

Books can be defined like this:

```C++
"type" : "BOOK",      // Defines this as a BOOK
...                   // same entries as above for the generic item.
                      // additional some book specific entries:
"max_level" : 5,      // Maximum skill level this book will train to
"intelligence" : 11,  // Intelligence required to read this book without penalty
"time" : "35 m",          // Time a single read session takes. An integer will be read in minutes or a time string can be used.
"fun" : -2,           // Morale bonus/penalty for reading
"skill" : "computer", // Skill raised
"chapters" : 4,       // Number of chapters (for fun only books), each reading "consumes" a chapter. Books with no chapters left are less fun (because the content is already known to the character).
"required_level" : 2  // Minimum skill level required to learn
```
It is possible to omit the `max_level` field if the book you're creating contains only recipes and it's not supposed to level up any skill. In this case the `skill` field will just refer to the skill required to learn the recipes.

Alternately, every item (tool, gun, even food) can be used as book if it has book_data:
```C++
"type" : "TOOL",      // Or any other item type
...                   // same entries as for the type (e.g. same entries as for any tool),
"book_data" : {       // additionally the same book data like above
    "max_level" : 5,
    "intelligence" : 11,
    "time" : 35,
    "fun" : -2,
    "skill" : "computer",
    "chapters" : 4,
    "use_action" : "MA_MANUAL", // The book_data can have use functions (see USE ACTIONS) that are triggered when the books has been read. These functions are not triggered by simply activating the item (like tools would).
    "required_level" : 2
}
```

Since many book names are proper names, it's often necessary to explicitly specify
the plural forms. The following is the game's convention on plural names of books:

1. For non-periodical books (textbooks, manuals, spellbooks, etc.),
    1. If the book's singular name is a proper name, then the plural name is `copies of (singular name)`. For example, the plural name of `Lessons for the Novice Bowhunter` is `copies of Lessons for the Novice Bowhunter`.
    2. Otherwise, the plural name is the usual plural of the singular name. For example, the plural name of `tactical baton defense manual` is `tactical baton defense manuals`
2. For periodicals (magazines and journals),
    1. If the periodical's singular name is a proper name, and doesn't end with "Magazine", "Weekly", "Monthly", etc., the plural name is `issues of (singular name)`. For example, the plural name of `Archery for Kids` is `issues of Archery for Kids`.
    2. Otherwise, the periodical's plural name is the usual plural of the singular name. For example, the plural name of `Crafty Crafter's Quarterly` is `Crafty Crafter's Quarterlies`.
3. For board games (represented internally as book items),
    1. If the board game's singular name is a proper name, the plural is `sets of (singular name)`. For example, the plural name of `Picturesque` is `sets of Picturesque`.
    2. Otherwise the plural name is the usual plural. For example, the plural of `deck of cards` is `decks of cards`.

#### Conditional Naming

The `conditional_names` field allows defining alternate names for items that will be displayed instead of (or in addition to) the default name, when specific conditions are met. Take the following (incomplete) definition for `sausage` as an example of the syntax:

```json
{
  "name": "sausage",
  "conditional_names": [
    {
      "type": "FLAG",
      "condition": "CANNIBALISM",
      "name": "Mannwurst"
    },
    {
      "type": "COMPONENT_ID",
      "condition": "mutant",
      "name": { "str_sp": "sinister %s" }
    },
    {
      "type": "VAR",
      "condition": "npctalk_var_DISPLAY_NAME_MORALE",
      "name": { "str_sp": "%s (morale)" },
      "value" : "true"
    },
    {
      "type": "SNIPPET_ID",
      "condition": "test",
      "value":"one",
      "name": { "str_sp": "Report 1" } }
  ]
}
```

You can list as many conditional names for a given item as you want. Each conditional name must consist of 3 elements:
1. The condition type:
    - `COMPONENT_ID` searches all the components of the item (and all of *their* components, and so on) for an item with the condition string in their ID. The ID only needs to *contain* the condition, not match it perfectly (though it is case sensitive). For example, supplying a condition `mutant` would match `mutant_meat`.
    - `FLAG` which checks if an item has the specified flag (exact match).
    - `VAR` which checks if an item has a variable with the given name (exact match) and value = `value`. Variables set with effect_on_conditions will have `npctalk_var_` in front of their name.  So a variable created with: `"npc_add_var": "MORALE", "type": "DISPLAY","context":"NAME", "value": "Felt Great" }` would be named: `npctalk_var_DISPLAY_NAME_MORALE`.
    - `SNIPPET_ID`which checks if an item has a snippet id variable set by an effect_on_condition with the given name (exact match) and snippets id = `value`.
2. The condition you want to look for.
3. The name to use if a match is found. Follows all the rules of a standard `name` field, with valid keys being `str`, `str_pl`, and `ctxt`. You may use %s here, which will be replaced by the name of the item. Conditional names defined prior to this one are taken into account.

So, in the above example, if the sausage is made from mutant humanoid meat, and therefore both has the `CANNIBALISM` flag, *and* has a component with `mutant` in its ID:
1. First, the item name is entirely replaced with "Mannwurst" if singular, or "Mannwursts" if plural.
2. Next, it is replaced by "sinister %s", but %s is replaced with the name as it was before this step, resulting in "sinister Mannwurst" or "sinister Mannwursts".

NB: If `"str": "sinister %s"` was specified instead of `"str_sp": "sinister %s"`, the plural form would be automatically created as "sinister %ss", which would become "sinister Mannwurstss" which is of course one S too far. Rule of thumb: If you are using %s in the name, always specify an identical plural form unless you know exactly what you're doing!


#### Color Key

When adding a new book, please use this color key:

* Magazines: `pink`
* “Paperbacks” Short enjoyment books (including novels): `light_cyan`
* “Hardbacks” Long enjoyment books (including novels): `light_blue`
* “Small textbook” Beginner level textbooks, guides and martial arts books: `green`
* “Large textbook” Advanced level textbooks and advanced guides: `blue`
* Religious books: `dark_gray`
* “Printouts” (including spiral-bound, binders, and similar) Technical documents, (technical?) protocols, (lab) journals, personal diaries: `light_green`
* Other reading material/non-books (use only if every other category does not apply): `light_gray`

A few exceptions to this color key may apply, for example for books that don’t are what they seem to be.
Never use `yellow` and `red`, those colors are reserved for sounds and infrared vision.

#### CBMs

CBMs can be defined like this:

```C++
"type" : "BIONIC_ITEM",         // Defines this as a CBM
...                             // same entries as above for the generic item.
                                // additional some CBM specific entries:
"bionic_id" : "bio_advreactor", // ID of the installed bionic if not equivalent to "id"
"difficulty" : 11,              // Difficulty of installing CBM
"is_upgrade" : true,            // Whether the CBM is an upgrade of another bionic.
"installation_data" : "AID_bio_advreactor" // ID of the item which allows for almost guaranteed installation of corresponding bionic.
```

### Comestibles

```C++
"type" : "COMESTIBLE",      // Defines this as a COMESTIBLE
...                         // same entries as above for the generic item.
// Only COMESTIBLE type items may define the following fields:
"spoils_in" : 0,            // A time duration: how long a comestible is good for. 0 = no spoilage.
"use_action" : [ "CRACK" ],     // What effects a comestible has when used, see special definitions below
"stim" : 40,                // Stimulant effect
"fatigue_mod": 3,           // How much fatigue this comestible removes. (Negative values add fatigue)
"comestible_type" : "MED",  // Comestible type, used for inventory sorting. One of 'FOOD', 'DRINK', 'MED', or 'INVALID' (consider using a different "type" than COMESTIBLE instead of using INVALID)
"consumption_effect_on_conditions" : [ "EOC_1" ],  // Effect on conditions to run after consuming.  Inline or string id supported
"quench" : 0,               // Thirst quenched
"healthy" : -2,             // Health effects (used for sickness chances)
"addiction_potential" : 80, // Default strength for this item to cause addictions
"addiction_type" : [ "crack", { "addiction": "cocaine", "potential": 5 } ], // Addiction types (if no potential is given, the "addiction_potential" field is used to determine the strength of that addiction)
"monotony_penalty" : 0,     // (Optional, default: 2) Fun is reduced by this number for each one you've consumed in the last 48 hours.
                            // Can't drop fun below 0, unless the comestible also has the "NEGATIVE_MONOTONY_OK" flag.
"calories" : 0,             // Hunger satisfied (in kcal)
"nutrition" : 0,            // Hunger satisfied (OBSOLETE)
"tool" : "apparatus",       // Tool required to be eaten/drank
"charges" : 4,              // Number of uses when spawned
"stack_size" : 8,           // (Optional) How many uses are in the above-defined volume. If omitted, is the same as 'charges'
"fun" : 50                  // Morale effects when used
"freezing_point": 32,       // (Optional) Temperature in C at which item freezes, default is water (32F/0C)
"cooks_like": "meat_cooked",         // (Optional) If the item is used in a recipe, replaces it with its cooks_like
"parasites": 10,            // (Optional) Probability of becoming parasitized when eating
"contamination": [ { "disease": "bad_food", "probability": 5 } ],         // (Optional) List of diseases carried by this comestible and their associated probability. Values must be in the [0, 100] range.
"vitamins": [ [ "calcium", 5 ], [ "iron", 12 ] ],         // Vitamins provided by consuming a charge (portion) of this.  An integer percentage of ideal daily value average.  Vitamins array keys include the following: calcium, iron, vitA, vitB, vitC, mutant_toxin, bad_food, blood, and redcells.  Note that vitB is B12.
"material": [                     // All materials (IDs) this food is made of
  { "type": "flesh", "portion": 3 }, // See Generic Item attributes for type and portion details
  { "type": "wheat", "portion": 5 }
],
"primary_material": "meat",       // What the primary material ID is. Materials determine specific heat.
"rot_spawn": "MONSTERGROUP_NAME", // Monster group that spawns when food becomes rotten (used for egg hatching)
"rot_spawn_chance": 10,           // Percent chance of monstergroup spawn when food rots. Max 100.
"smoking_result": "dry_meat",     // Food that results from drying this food in a smoker
"petfood": [ "FUNGALFRUIT", "MIGOFOOD" ] // (Optional) Pet food categories this item is in.
```


### Containers

Any Item can be a container. To add the ability to contain things to an item, you need to add pocket_data. The below example is a typical container (shown with optional default values, or mandatory if the value is mandatory)

```C++
"pocket_data": [
  {
    "pocket_type": "CONTAINER",       // Typical container pocket. Pockets can also be MAGAZINE.
    "max_contains_volume": mandatory, // Maximum volume this pocket can hold, totaled among all contained items.  For example "2 L" or "2000 ml" would hold two liters of items.
    "max_contains_weight": mandatory, // Maximum weight this pocket can hold, totaled among all container items.  For example "6 kg" is about enough to contain a bowling ball.
    "min_item_volume": "0 ml",        // Minimum volume of item that can be placed into this pocket.  Items smaller than this cannot be placed in the pocket.
    "max_item_volume": "0 ml",        // Maximum volume of item that can fit through the opening into this pocket.  For example, a 2-liter bottle has a "17 ml" opening.
    "max_item_length": "0 mm",        // Maximum length of items that can fit in this pocket, by their longest_side.  Default is the diagonal opening length assuming volume is a cube (cube_root(vol)*square_root(2))
    "spoil_multiplier": 1.0,          // How putting an item in this pocket affects spoilage.  Less than 1.0 and the item will be preserved longer; 0.0 will preserve indefinitely.
    "weight_multiplier": 1.0,         // The items in this pocket magically weigh less inside than outside.  Nothing in vanilla should have a weight_multiplier.
    "volume_multiplier": 1.0,         // The items in this pocket have less volume inside than outside.  Can be used for containers that'd help in organizing specific contents, such as cardboard rolls for duct tape.
    "moves": 100,                     // Indicates the number of moves it takes to remove an item from this pocket, assuming best conditions.
    "rigid": false,                   // Default false. If true, this pocket's size is fixed, and does not expand when filled.  A glass jar would be rigid, while a plastic bag is not.
    "forbidden": true,                // Default false. If true, this pocket cannot be used by players. 
    "magazine_well": "0 ml",          // Amount of space you can put items in the pocket before it starts expanding.  Only works if rigid = false.
    "watertight": false,              // Default false. If true, can contain liquid.
    "airtight": false,                // Default false. If true, can contain gas.
    "ablative": false,                 // Default false. If true, this item holds a single ablative plate. Make sure to include a flag_restriction on the type of plate that can be added.
    "holster": false,                 // Default false. If true, only one stack of items can be placed inside this pocket, or one item if that item is not count_by_charges.
    "open_container": false,          // Default false. If true, the contents of this pocket will spill if this item is placed into another item.
    "fire_protection": false,         // Default false. If true, the pocket protects the contained items from exploding if tossed into a fire.
    "ammo_restriction": { "ammotype": count }, // Restrict pocket to a given ammo type and count.  This overrides mandatory volume, weight, watertight and airtight to use the given ammo type instead.  A pocket can contain any number of unique ammo types each with different counts, and the container will only hold one type (as of now).  If this is left out, it will be empty.
    "flag_restriction": [ "FLAG1", "FLAG2" ],  // Items can only be placed into this pocket if they have a flag that matches one of these flags.
    "item_restriction": [ "item_id" ],         // Only these item IDs can be placed into this pocket. Overrides ammo and flag restrictions.
    "material_restriction": [ "material_id" ], // Only items that are mainly made of this material can enter.
	// If multiple of "flag_restriction", "material_restriction" and "item_restriction" are used simultaneously then any item that matches any of them will be accepted.

    "sealed_data": { "spoil_multiplier": 0.0 } // If a pocket has sealed_data, it will be sealed when the item spawns.  The sealed version of the pocket will override the unsealed version of the same datatype.

    "inherits_flags": true // if a pocket inherits flags it means any flags that the items inside have contribute to the item that has the pockets itself.
  }
]
```

### Melee

```C++
"id": "hatchet",       // Unique ID. Must be one continuous word, use underscores if necessary
"symbol": ";",         // ASCII character used in-game
"color": "light_gray", // ASCII character color
"name": "hatchet",     // In-game name displayed
"description": "A one-handed hatchet. Makes a great melee weapon, and is useful both for cutting wood, and for use as a hammer.", // In-game description
"price": 95,           // Used when bartering with NPCs.  Can use string "cent" "USD" or "kUSD".
"material": [          // Material types.  See materials.json for possible options
  { "type": "iron", "portion": 2 }, // See Generic Item attributes for type and portion details
  { "type": "wood", "portion": 3 }
],
"weight": 907,         // Weight, measured in grams
"volume": "1500 ml",   // Volume, volume in ml and L can be used - "50 ml" or "2 L"
"melee_damage": {      // Damage caused by using it as a melee weapon
  "bash": 12,
  "cut": 12
},
"flags" : ["CHOP"],    // Indicates special effects
"to_hit": 1            // To-hit bonus if using it as a melee weapon
```
### Memory Cards

Memory card information can be defined on any GENERIC item by adding an object named `memory_card`, this field does not support `extend`/`remove`, only override.

```C++
"id": "memory_card_unread",
"name": "memory card (unread)",
// ...
"memory_card": {
  "data_chance": 0.5,                  // 50% chance to contain data
  "on_read_convert_to": "memory_card", // converted to this itype_id on read
  "photos_chance": 0.33,               // 33% chance to contain new photos
  "photos_amount": 3,                  // contains between 1 and 3 new photos
  "songs_chance": 0.33,                // 33% chance to contain new songs
  "songs_amount": 4,                   // contains between 1 and 4 new songs
  "recipes_chance": 0.33,              // 33% chance to contain new recipes
  "recipes_amount": 5,                 // contains between 1 and 5 new recipes
  "recipes_level_min": 4,              // recipes will have at least level 4
  "recipes_level_max": 8,              // recipes will have at most level 8
  "recipes_categories": [ "CC_FOOD" ]  // recipes from CC_FOOD category
}
```

### Gun

Guns can be defined like this:

```C++
"type": "GUN",             // Defines this as a GUN
...                        // same entries as above for the generic item.
                           // additional some gun specific entries:
"skill": "pistol",         // Skill used for firing
"ammo": [ "357", "38" ],   // Ammo types accepted for reloading
"ranged_damage": 0,        // Ranged damage when fired
"range": 0,                // Range when fired
"dispersion": 32,          // Inaccuracy of gun, measured in 100ths of Minutes Of Angle (MOA)
// When sight_dispersion and aim_speed are present in a gun mod, the aiming system picks the "best"
// sight to use for each aim action, which is the fastest sight with a dispersion under the current
// aim threshold.
"sight_dispersion": 10,    // Inaccuracy of gun derived from the sight mechanism, measured in 100ths of Minutes Of Angle (MOA)
"recoil": 0,               // Recoil caused when firing, measured in 100ths of Minutes Of Angle (MOA)
"durability": 8,           // Resistance to damage/rusting, also determines misfire chance
"blackpowder_tolerance": 8,// One in X chance to get clogged up (per shot) when firing blackpowder ammunition (higher is better). Optional, default is 8.
"min_cycle_recoil": 0,     // Minimum ammo recoil for gun to be able to fire more than once per attack.
"clip_size": 100,          // Maximum amount of ammo that can be loaded
"energy_drain": "2 kJ",    // Additionally to the normal ammo (if any), a gun can require some electric energy. Drains from battery in gun. Use flags "USE_UPS" and "USES_BIONIC_POWER" to drain other sources. This also works on mods. Attaching a mod with energy_drain will add/increase drain on the weapon.
"ammo_to_fire" 1,          // Amount of ammo used
"modes": [ [ "DEFAULT", "semi-auto", 1 ], [ "AUTO", "auto", 4 ] ], // Firing modes on this gun, DEFAULT,AUTO, or MELEE followed by the name of the mode displayed in game, and finally the number of shots of the mod.
"reload": 450,             // Amount of time to reload, 100 = 1 second = 1 "turn"
"built_in_mods": ["m203"], //An array of mods that will be integrated in the weapon using the IRREMOVABLE tag.
"default_mods": ["m203"]   //An array of mods that will be added to a weapon on spawn.
"barrel_volume": "30 mL",  // Amount of volume lost when the barrel is sawn. Approximately 250 ml per inch is a decent approximation.
"valid_mod_locations": [ [ "brass catcher", 1 ], [ "grip", 1 ] ],  // The valid locations for gunmods and the mount of slots for that location.
"loudness": 10             // Amount of noise produced by this gun when firing. If no value is defined, then it's calculated based on loudness value from loaded ammo. Final loudness is calculated as gun loudness + gunmod loudness + ammo loudness. If final loudness is 0, then the gun is completely silent.
```
Alternately, every item (book, tool, armor, even food) can be used as gun if it has gun_data:
```json
"type": "TOOL",      // Or any other item type
...                   // same entries as for the type (e.g. same entries as for any tool),
"gun_data" : {        // additionally the same gun data like above
    "skill": ...,
    "recoil": ...,
    ...
}
```

### Gunmod

Gun mods can be defined like this:

```C++
"type": "GUNMOD",              // Defines this as a GUNMOD
...                            // Same entries as above for the generic item.
                               // Additionally some gunmod specific entries:
// Only GUNMOD type items may define the following fields:
"location": "stock",           // Mandatory. Where is this gunmod is installed?
"mod_targets": [ "crossbow" ], // Mandatory. What kind of weapons can this gunmod be used with?
"install_time": "30 s",        // Mandatory. How long does installation take? An integer will be read as moves or a time string can be used.
"acceptable_ammo": [ "9mm" ],  // Optional filter restricting mod to guns with those base (before modifiers) ammo types
"ammo_modifier": [ "57" ],     // Optional field which if specified modifies parent gun to use these ammo types
"magazine_adaptor": [ [ "223", [ "stanag30" ] ] ], // Optional field which changes the types of magazines the parent gun accepts
"damage_modifier": -1,         // Optional field increasing or decreasing base gun damage
"dispersion_modifier": 15,     // Optional field increasing or decreasing base gun dispersion
"loudness_modifier": 4,        // Optional field increasing or decreasing base guns loudness
"range_modifier": 2,           // Optional field increasing or decreasing base gun range
"range_multiplier": 1.2,       // Optional field multiplying base gun range
"energy_drain_modifier": "200 kJ",  // Optional field increasing or decreasing base gun energy consumption (per shot) by adding given value. This addition is not multiplied by energy_drains_multiplier.
"energy_drains_multiplier": 2.5, // Optional field increasing or decreasing base gun energy consumption (per shot) by multiplying by given value.
"reload_modifier": -10,        // Optional field increasing or decreasing base gun reload time in percent
"min_str_required_mod": 14,    // Optional field increasing or decreasing minimum strength required to use gun
"aim_speed": 3,                // A measure of how quickly the player can aim, in moves per point of dispersion.
"ammo_effects": [ "BEANBAG" ], // List of IDs of ammo_effect types
"consume_chance": 5000,        // Odds against CONSUMABLE mod being destroyed when gun is fired (default 1 in 10000)
"consume_divisor": 10,         // Divide damage against mod by this amount (default 1)
"handling_modifier": 4,        // Improve gun handling. For example a forward grip might have 6, a bipod 18
"mode_modifier": [ [ "AUTO", "auto", 4 ] ], // Modify firing modes of the gun, to give AUTO or REACH for example
"barrel_length": "45 mm"       // Specify a direct barrel length for this gun mod. If used only the first mod with a barrel length will be counted
```

Alternately, every item (book, tool, armor, even food) can be used as a gunmod if it has gunmod_data:
```json
"type": "TOOL",       // Or any other item type
...                   // same entries as for the type (e.g. same entries as for any tool),
"gunmod_data" : {
    "location": ...,
    "mod_targets": ...,
    ...
}
```


### Batteries
```C++
"type": "BATTERY",    // Defines this as a BATTERY
...                   // Same entries as above for the generic item
                      // Additionally some battery specific entries:
"max_energy": "30 kJ" // Mandatory. Maximum energy quantity the battery can hold
```

### Tools

```C++
"id": "torch_lit",    // Unique ID. Must be one continuous word, use underscores if necessary
"type": "TOOL",       // Defines this as a TOOL
"symbol": "/",        // ASCII character used in-game
"color": "brown",     // ASCII character color
"name": "torch (lit)", // In-game name displayed
"description": "A large stick, wrapped in gasoline soaked rags. This is burning, producing plenty of light", // In-game description
"price": 0,           // Used when bartering with NPCs.  Can use string "cent" "USD" or "kUSD".
"material": [ { "type": "wood", "portion": 1 } ], // Material types.  See materials.json for possible options. Also see Generic Item attributes for type and portion details
"techniques": [ "FLAMING" ], // Combat techniques used by this tool
"flags": [ "FIRE" ],      // Indicates special effects
"weight": 831,        // Weight, measured in grams
"volume": "1500 ml",  // Volume, volume in ml and L can be used - "50 ml" or "2 L"
"melee_damage": {     // Damage caused by using it as a melee weapon
  "bash": 12,
  "cut": 0
},
"to_hit": 3,          // To-hit bonus if using it as a melee weapon
"turns_per_charge": 20, // Charges consumed over time, deprecated in favor of power_draw
"fuel_efficiency": 0.2, // When combined with being a UPS this item will burn fuel for its given energy value to produce energy with the efficiency provided. Needs to be > 0 for this to work
"use_action": [ "firestarter" ], // Action performed when tool is used, see special definition below
"qualities": [ [ "SCREW", 1 ] ], // Inherent item qualities like hammering, sawing, screwing (see tool_qualities.json)
"charged_qualities": [ [ "DRILL", 3 ] ], // Qualities available if tool has at least charges_per_use charges left
// Only TOOL type items may define the following fields:
"ammo": [ "NULL" ],        // Ammo types used for reloading
"charge_factor": 5,        // this tool uses charge_factor charges for every charge required in a recipe; intended for tools that have a "sub" field but use a different ammo that the original tool
"charges_per_use": 1,      // Charges consumed per tool use
"initial_charges": 75,     // Charges when spawned
"max_charges": 75,         // Maximum charges tool can hold
"rand_charges": [10, 15, 25], // Randomize the charges when spawned. This example has a 50% chance of rng(10, 15) charges and a 50% chance of rng(15, 25). (The endpoints are included.)
"power_draw": "50 mW",          // Energy consumption per second
"revert_to": "torch_done", // Transforms into item when charges are expended
"sub": "hotplate",         // optional; this tool has the same functions as another tool
```


### Seed Data

Every item type can have optional seed data, if the item has seed data, it's considered a seed and can be planted:

```C++
"seed_data" : {
    "fruit": "weed", // The item id of the fruits that this seed will produce.
    "seeds": false, // (optional, default is true). If true, harvesting the plant will spawn seeds (the same type as the item used to plant). If false only the fruits are spawned, no seeds.
    "fruit_div": 2, // (optional, default is 1). Final amount of fruit charges produced is divided by this number. Works only if fruit item is counted by charges.
    "byproducts": ["withered", "straw_pile"], // A list of further items that should spawn upon harvest.
    "plant_name": "sunflower", // The name of the plant that grows from this seed. This is only used as information displayed to the user.
    "grow" : 91 // A time duration: how long it takes for a plant to fully mature. Based around a 91 day season length (roughly a real world season) to give better accuracy for longer season lengths
                // Note that growing time is later converted based upon the season_length option, basing it around 91 is just for accuracy purposes
                // A value 91 means 3 full seasons, a value of 30 would mean 1 season.
}
```

### Brewing Data

Every item type can have optional brewing data, if the item has brewing data, it can be placed in a vat and will ferment into a different item type.

Currently only vats can only accept and produce liquid items.

```C++
"brewable" : {
    "time": 3600, // A time duration: how long the fermentation will take.
    "result": "beer" // The id of the result of the fermentation.
}
```

#### `Effects_carried`

(optional, default: empty list)

Effects of the artifact when it's in the inventory (main inventory, wielded, or worn) of the player.

Possible values (see src/enums.h for an up-to-date list):

- `AEP_STR_UP` Strength + 4
- `AEP_DEX_UP` Dexterity + 4
- `AEP_PER_UP` Perception + 4
- `AEP_INT_UP` Intelligence + 4
- `AEP_ALL_UP` All stats + 2
- `AEP_SPEED_UP` +20 speed
- `AEP_IODINE` Reduces radiation
- `AEP_SNAKES` Summons friendly snakes when you're hit
- `AEP_INVISIBLE` Makes you invisible
- `AEP_CLAIRVOYANCE` See through walls
- `AEP_SUPER_CLAIRVOYANCE` See through walls to a great distance
- `AEP_STEALTH` Your steps are quieted
- `AEP_EXTINGUISH` May extinguish nearby flames
- `AEP_GLOW` Four-tile light source
- `AEP_PSYSHIELD` Protection from fear paralyze attack
- `AEP_RESIST_ELECTRICITY` Protection from electricity
- `AEP_CARRY_MORE` Increases carrying capacity by 200
- `AEP_SAP_LIFE` Killing non-zombie monsters may heal you
- `AEP_HUNGER` Increases hunger
- `AEP_THIRST` Increases thirst
- `AEP_SMOKE` Emits smoke occasionally
- `AEP_EVIL` Addiction to the power
- `AEP_SCHIZO` Mimicks schizophrenia
- `AEP_RADIOACTIVE` Increases your radiation
- `AEP_MUTAGENIC` Mutates you slowly
- `AEP_ATTENTION` Draws netherworld attention slowly
- `AEP_STR_DOWN` Strength - 3
- `AEP_DEX_DOWN` Dex - 3
- `AEP_PER_DOWN` Per - 3
- `AEP_INT_DOWN` Int - 3
- `AEP_ALL_DOWN` All stats - 2
- `AEP_SPEED_DOWN` -20 speed
- `AEP_FORCE_TELEPORT` Occasionally force a teleport
- `AEP_MOVEMENT_NOISE` Makes noise when you move
- `AEP_BAD_WEATHER` More likely to experience bad weather
- `AEP_SICK` Decreases health over time

#### `effects_worn`
(optional, default: empty list)

Effects of the artifact when it's worn (it must be an armor item to be worn).

Possible values are the same as for effects_carried.

#### `effects_wielded`

(optional, default: empty list)

Effects of the artifact when it's wielded.

Possible values are the same as for effects_carried.

#### `effects_activated`

(optional, default: empty list)

Effects of the artifact when it's activated (which require it to have a `"use_action": [ "ARTIFACT" ]` and it must have a non-zero max_charges value).

Possible values (see src/artifact.h for an up-to-date list):

- `AEA_STORM` Emits shock fields
- `AEA_FIREBALL` Targeted
- `AEA_ADRENALINE` Adrenaline rush
- `AEA_MAP` Maps the area around you
- `AEA_BLOOD` Shoots blood all over
- `AEA_FATIGUE` Creates interdimensional fatigue
- `AEA_ACIDBALL` Targeted acid
- `AEA_PULSE` Destroys adjacent terrain
- `AEA_HEAL` Heals minor damage
- `AEA_CONFUSED` Confuses all monsters in view
- `AEA_ENTRANCE` Chance to make nearby monsters friendly
- `AEA_BUGS` Chance to summon friendly insects
- `AEA_TELEPORT` Teleports you
- `AEA_LIGHT` Temporary light source
- `AEA_GROWTH` Grow plants, a la triffid queen
- `AEA_HURTALL` Hurts all monsters!
- `AEA_FUN` Temporary morale bonus
- `AEA_SPLIT` Split between good and bad
- `AEA_RADIATION` Spew radioactive gas
- `AEA_PAIN` Increases player pain
- `AEA_MUTATE` Chance of mutation
- `AEA_PARALYZE` You lose several turns
- `AEA_FIRESTORM` Spreads minor fire all around you
- `AEA_ATTENTION` Attention from sub-prime denizens
- `AEA_TELEGLOW` Teleglow disease
- `AEA_NOISE` Loud noise
- `AEA_SCREAM` Noise & morale penalty
- `AEA_DIM` Darkens the sky slowly
- `AEA_FLASH` Flashbang
- `AEA_VOMIT` User vomits
- `AEA_SHADOWS` Summon shadow creatures
- `AEA_STAMINA_EMPTY` Empties most of the player's stamina gauge

### Software Data

Every item type can have software data, it does not have any behavior:

```C++
"software_data" : {
    "type": "USELESS", // unused
    "power" : 91 // unused
}
```

### Use Actions

The contents of use_action fields can either be a string indicating a built-in function to call when the item is activated (defined in iuse.cpp), or one of several special definitions that invoke a more structured function.

```C++
"use_action": {
    "type": "transform",  // The type of method, in this case one that transforms the item.
    "target": "gasoline_lantern_on", // The item to transform to.
    "active": true,       // Whether the item is active once transformed.
    "ammo_scale": 0,    // For use when an item automatically transforms into another when its ammo drops to 0, or to allow guns to transform with 0 ammo.
    "msg": "You turn the lamp on.", // Message to display when activated.
    "need_fire": 1,                 // Whether fire is needed to activate.
    "need_fire_msg": "You need a lighter!", // Message to display if there is no fire.
    "need_charges": 1,                      // Number of charges the item needs to transform.
    "need_charges_msg": "The lamp is empty.", // Message to display if there aren't enough charges.
    "need_empty": true,                       // Whether the item must be empty to be transformed; false by default.
    "need_worn": true,                        // Whether the item must be worn to be transformed; false by default.
    "need_wielding": true,                    // Whether the item must be wielded to be transformed; false by default.
    "qualities_needed": { "WRENCH_FINE": 1 }, // Tool qualities needed, e.g. "fine bolt turning 1".
    "target_charges": 3, // Number of charges the transformed item has.
    "rand_target_charges": [10, 15, 25], // Randomize the charges the transformed item has. This example has a 50% chance of rng(10, 15) charges and a 50% chance of rng(15, 25). (The endpoints are included.)
    "ammo_qty": 3,              // If zero or positive set remaining ammo of transformed item to this.
    "random_ammo_qty": [1, 5],  // If this has values, set remaining ammo of transformed item to one of them chosen at random.
    "ammo_type": "tape",        // If both this and ammo_qty are specified then set transformed item to this specific ammo.
    "container": "jar",  // Container holding the target item.
    "sealed": true,      // Whether the transformed container is sealed; true by default.
    "menu_text": "Lower visor"      // (Optional) Text displayed in the activation screen, defaults to "Turn on".
    "moves" : 500         // Moves required to transform the item in excess of a normal action.
},
"use_action": {
    "type": "explosion", // An item that explodes when it runs out of charges.
    "explosion": { // Optional: physical explosion data
        // Specified like `"explosion"` field in generic items
    },
    "draw_explosion_radius" : 5, // How large to draw the radius of the explosion.
    "draw_explosion_color" : "ltblue", // The color to use when drawing the explosion.
    "do_flashbang" : true, // Whether to do the flashbang effect.
    "flashbang_player_immune" : true, // Whether the player is immune to the flashbang effect.
    "fields_radius": 3, // The radius of spread for fields produced.
    "fields_type": "fd_tear_gas", // The type of fields produced.
    "fields_min_intensity": 3, // Minimum intensity of field generated by the explosion.
    "fields_max_intensity": 3, // Maximum intensity of field generated by the explosion.
    "emp_blast_radius": 4, // The radius of EMP blast created by the explosion.
    "scrambler_blast_radius": 4 // The radius of scrambler blast created by the explosion.
},
"use_action": {
    "type": "change_scent", // Change the scent type of the user.
    "scent_typeid": "sc_fetid", // The scenttype_id of the new scent.
    "charges_to_use": 2, // Charges consumed when the item is used.  (Default: 1)
    "scent_mod": 150, // Modifier added to the scent intensity.  (Default: 0)
    "duration": "6 m", // How long does the effect last.
    "effects": [ { "id": "fetid_goop", "duration": 360, "bp": "torso", "permanent": true } ], // List of effects with their id, duration, bodyparts, and permanent bool
    "waterproof": true, // Is the effect waterproof.  (Default: false)
    "moves": 500 // Number of moves required in the process.
},
"use_action" : {
    "type" : "consume_drug", // A drug the player can consume.
    "activation_message" : "You smoke your crack rocks.  Mother would be proud.", // Message, ayup.
    "effects" : { "high": 15 }, // Effects and their duration.
    "damage_over_time": [
        {
          "damage_type": "pure", // Type of damage
          "duration": "1 m", // For how long this damage will be applied
          "amount": -10, // Amount of damage applied every turn, negative damage heals
          "bodyparts": [ "torso", "head", "arm_l", "leg_l", "arm_r", "leg_r" ] // Body parts hit by the damage
        }
    ]
    "stat_adjustments": {"hunger" : -10}, // Adjustment to make to player stats.
    "fields_produced" : {"cracksmoke" : 2}, // Fields to produce, mostly used for smoke.
    "charges_needed" : { "fire" : 1 }, // Charges to use in the process of consuming the drug.
    "tools_needed" : { "apparatus" : -1 }, // Tool needed to use the drug.
    "moves": 50 // Number of moves required in the process, default value is 100.
},
"use_action": {
    "type": "place_monster", // place a turret / manhack / whatever monster on the map
    "monster_id": "mon_manhack", // monster id, see monsters.json
    "difficulty": 4, // difficulty for programming it (manhacks have 4, turrets 6, ...)
    "hostile_msg": "It's hostile!", // (optional) message when programming the monster failed and it's hostile.
    "friendly_msg": "Good!", // (optional) message when the monster is programmed properly and it's friendly.
    "place_randomly": true, // if true: places the monster randomly around the player, if false: let the player decide where to put it (default: false)
    "skills": [ "unarmed", "throw" ], // (optional) array of skill IDs. Higher skill level means more likely to place a friendly monster.
    "moves": 60, // how many move points the action takes.
    "is_pet": false // specifies if the spawned monster is a pet. The monster will only spawn as a pet if it is spawned as friendly, hostile monsters will never be pets.
},
"use_action": {
    "type": "place_npc", // place npc of specific class on the map
    "npc_class_id": "true_foodperson", // npc id, see npcs/npc.json
    "summon_msg": "You summon a food hero!", // (optional) message when summoning the npc.
    "place_randomly": true, // if true: places npc randomly around the player, if false: let the player decide where to put it (default: false)
    "moves": 50, // how many move points the action takes.
    "radius": 1 // maximum radius for random npc placement.
},
"use_action": {
    "type": "link_up", // Connect item to a vehicle or appliance, such as plugging a chargeable device into a power source.
                       // If the item has the CABLE_SPOOL flag, it has special behaviors available, like connecting vehicles together.
    "cable_length": 4 // Maximum length of the cable ( Optional, defaults to the item type's maximum charges ).
                      // If extended by other cables, will use the sum of all cables' lengths.
    "charge_rate": "60 W" // The charge rate of the plugged-in device's batteries in watts. ( Optional, defaults to "0 W" )
                          // A positive value will charge the device's chargeable batteries at the expense of the connected power grid.
                          // A negative value will charge the connected electrical grid's batteries at the expense of the device's. 
                          // A value of 0 won't charge the device's batteries, but will still let the device operate off of the connected power grid.
    "efficiency": 0.85f // (this) out of 1.0 chance to successfully add 1 charge every charge interval ( Optional, defaults to 0.85f, AKA 85% efficiency ).
                        // A value less than 0.001 means the cable won't transfer any electricity at all.
                        // If extended by other cables, will use the product of all cable's efficiencies multiplied together.
    "menu_text": // Text displayed in the activation screen ( Optional, defaults to Plug in / Manage cables" ).
    "move_cost": // Move cost of attaching the cable ( Optional, defaults to 5 ).
    "targets": [ // Array of link_states that are valid connection points of the cable ( Optional, defaults to only allowing disconnection ).
        "no_link",         // Must be included to allow letting the player manually disconnect the cable.
        "vehicle_port",    // Can connect to a vehicle's cable ports / electrical controls or an appliance.
        "vehicle_battery", // Can connect to a vehicle's battery or an appliance.
        "vehicle_tow",     // Can be used as a tow cable between two vehicles.
        "bio_cable",       // Can connect to a cable system bionic.
        "ups",             // Can link to a UPS.
        "solarpack"        // Can link to a worn solar pack.
    ],
    "can_extend": [ // Array of cable items that can be extended by this one ( Optional, defaults to none ).
        "extension_cable",
        "long_extension_cable",
        "ELECTRICAL_DEVICES" // "ELECTRICAL_DEVICES" is a special keyword that lets this cable extend all electrical devices that have link_up actions.
    ]
},
"use_action" : {
    "type" : "delayed_transform", // Like transform, but it will only transform when the item has a certain age
    "transform_age" : 600, // The minimal age of the item. Items that are younger wont transform. In turns (60 turns = 1 minute)
    "not_ready_msg" : "The yeast has not been done The yeast isn't done culturing yet." // A message, shown when the item is not old enough
},
"use_action": {
    "type": "firestarter", // Start a fire, like with a lighter.
    "moves": 15 // Number of moves it takes to start the fire. This is reduced by survival skill.
    "moves_slow": 1500 // Number of moves it takes to start a fire on something that is difficult to ignite. This is reduced by survival skill.
    "need_sunlight": true // Whether the character needs to be in direct sunlight, e.g. to use magnifying glasses.
},
"use_action": {
    "type": "unpack", // unpack this item
    "group": "gobag_contents", // itemgroup this unpacks into
    "items_fit": true, // Do the armor items in this fit? Defaults to false.
    "filthy_volume_threshold": "10 L" // If the items unpacked from this item have volume, and this item is filthy, at what amount of held volume should they become filthy
},
"use_action": {
    "type": "salvage", // Try to salvage base materials from an item, e.g. cutting up cloth to get rags or leather.
    "moves_per_part": 25, // Number of moves it takes (optional).
    "material_whitelist": [ // List of material ids (not item ids!) that can be salvage from.
        "cotton",           // The list here is the default list, used when there is no explicit martial list given.
        "leather",          // If the item (that is to be cut up) has any material not in the list, it can not be cut up.
        "fur",
        "nomex",
        "kevlar",
        "plastic",
        "wood",
        "wool"
    ]
},
"use_action": {
    "type": "inscribe", // Inscribe a message on an item or on the ground.
    "on_items": true, // Whether the item can inscribe on an item.
    "on_terrain": false, // Whether the item can inscribe on the ground.
    "material_restricted": true, // Whether the item can only inscribe on certain item materials. Not used when inscribing on the ground.
    "material_whitelist": [ // List of material ids (not item ids!) that can be inscribed on.
        "wood",             // Only used when inscribing on an item, and only when material_restricted is true.
        "plastic",          // The list here is the default that is used when no explicit list is given.
        "glass",
        "chitin",
        "iron",
        "steel",
        "silver"
    ]
},
"use_action": {
    "type": "fireweapon_off", // Activate a fire based weapon.
    "target_id": "firemachete_on", // The item type to transform this item into.
    "success_message": "Your No. 9 glows!", // A message that is shows if the action succeeds.
    "failure_message": "", // A message that is shown if the action fails, for whatever reason. (Optional, if not given, no message will be printed.)
    "lacks_fuel_message": "Out of fuel", // Message that is shown if the item has no charges.
    "noise": 0, // The noise it makes to active the item, Optional, 0 means no sound at all.
    "moves": 0, // The number of moves it takes the character to even try this action (independent of the result).
    "success_chance": 0 // How likely it is to succeed the action. Default is to always succeed. Try numbers in the range of 0-10.
},
"use_action": {
    "type": "fireweapon_on", // Function for active (burning) fire based weapons.
    "noise_chance": 1, // The chance (one in X) that the item will make a noise, rolled on each turn.
    "noise": 0, // The sound volume it makes, if it makes a noise at all. If 0, no sound is made, but the noise message is still printed.
    "noise_message": "Your No. 9 hisses.", // The message / sound description (if noise is > 0), that appears when the item makes a sound.
    "voluntary_extinguish_message": "Your No. 9 goes dark.", // Message that appears when the item is turned of by player.
    "charges_extinguish_message": "Out of ammo!", // Message that appears when the item runs out of charges.
    "water_extinguish_message": "Your No. 9 hisses in the water and goes out.", // Message that appears if the character walks into water and the fire of the item is extinguished.
    "auto_extinguish_chance": 0, // If > 0, this is the (one in X) chance that the item goes out on its own.
    "auto_extinguish_message": "Your No. 9 cuts out!" // Message that appears if the item goes out on its own (only required if auto_extinguish_chance is > 0).
},
"use_action": {
    "type": "musical_instrument", // The character plays an instrument (this item) while walking around.
    "speed_penalty": 10, // This is subtracted from the characters speed.
    "volume": 12, // Volume of the sound of the instrument.
    "fun": -5, // Together with fun_bonus, this defines how much morale the character gets from playing the instrument. They get `fun + fun_bonus * <character-perception>` morale points out of it. Both values and the result may be negative.
    "fun_bonus": 2,
    "description_frequency": 20, // Once every Nth turn, a randomly chosen description (from the that array) is displayed.
    "player_descriptions": [
        "You play a little tune on your flute.",
        "You play a beautiful piece on your flute.",
        "You play a piece on your flute that sounds harmonious with nature."
    ]
},
"use_action": {
    "type": "holster", // Holster or draw a weapon
    "holster_prompt": "Holster item", // Prompt to use when selecting an item
    "holster_msg": "You holster your %s", // Message to show when holstering an item
    "max_volume": "1500 ml", // Maximum volume of each item that can be holstered. Volume in ml and L can be used - "50 ml" or "2 L".
    "min_volume": "750 ml",  // Minimum volume of each item that can be holstered or 1/3 max_volume if unspecified. volume in ml and L can be used - "50 ml" or "2 L".
    "max_weight": 2000, // Maximum weight of each item. If unspecified no weight limit is imposed
    "multi": 1, // Total number of items that holster can contain
    "draw_cost": 10, // Base move cost per unit volume when wielding the contained item
    "skills": ["pistol", "shotgun"], // Guns using any of these skills can be holstered
    "flags": ["SHEATH_KNIFE", "SHEATH_SWORD"] // Items with any of these flags set can be holstered
},
"use_action": {
    "type": "bandolier", // Store ammo and later reload using it
    "capacity": 10, // Total number of rounds that can be stored
    "ammo": [ "shot", "9mm" ], // What types of ammo can be stored?
},
"use_action": {
    "type": "reveal_map", // reveal specific terrains on the overmap
    "radius": 180, // radius around the player where things are revealed. A single overmap is 180x180 tiles.
    "terrain": ["hiway", "road"], // ids of overmap terrain types that should be revealed (as many as you want).
    "message": "You add roads and tourist attractions to your map." // Displayed after the revelation.
},
"use_action": {
    "type" : "heal",        // Heal damage, possibly some statuses
    "limb_power" : 10,      // How much hp to restore when healing limbs? Mandatory value
    "head_power" : 7,       // How much hp to restore when healing head? If unset, defaults to 0.8 * limb_power.
    "torso_power" : 15,     // How much hp to restore when healing torso? If unset, defaults to 1.5 * limb_power.
    "bleed" : 4,            // How many bleed effect intensity levels can be reduced by it. Base value.
    "bite" : 0.95,          // Chance to remove bite effect.
    "infect" : 0.1,         // Chance to remove infected effect.
    "move_cost" : 250,      // Cost in moves to use the item.
    "limb_scaling" : 1.2,   // How much extra limb hp should be healed per first aid level. Defaults to 0.25 * limb_power.
    "head_scaling" : 1.0,   // How much extra limb hp should be healed per first aid level. Defaults to (limb_scaling / limb_power) * head_power.
    "torso_scaling" : 2.0,  // How much extra limb hp should be healed per first aid level. Defaults to (limb_scaling / limb_power) * torso_power.
    "effects" : [           // Effects to apply to patient on finished healing. Same syntax as in consume_drug effects.
        { "id" : "pkill1", "duration" : 120 }
    ],
    "used_up_item" : "rag_bloody" // Item produced on successful healing. If the healing item is a tool, it is turned into the new type. Otherwise a new item is produced.
},
"use_action": {
    "type": "place_trap", // places a trap
    "allow_underwater": false, // (optional) allow placing this trap when the player character is underwater
    "allow_under_player": false, // (optional) allow placing the trap on the same square as the player character (e.g. for benign traps)
    "needs_solid_neighbor": false, // (optional) trap must be placed between two solid tiles (e.g. for tripwire).
    "needs_neighbor_terrain": "t_tree", // (optional, default is empty) if non-empty: a terrain id, the trap must be placed adjacent to that terrain.
    "outer_layer_trap": "tr_blade", // (optional, default is empty) if non-empty: a trap id, makes the game place a 3x3 field of traps. The center trap is the one defined by "trap", the 8 surrounding traps are defined by this (e.g. tr_blade for blade traps).
    "bury_question": "", // (optional) if non-empty: a question that will be asked if the player character has a shoveling tool and the target location is diggable. It allows to place a buried trap. If the player answers the question (e.g. "Bury the X-trap?") with yes, the data from the "bury" object will be used.
    "bury": { // If the bury_question was answered with yes, data from this entry will be used instead of outer data.
         // This json object should contain "trap", "done_message", "practice" and (optional) "moves", with the same meaning as below.
    },
    "trap": "tr_engine", // The trap to place.
    "done_message": "Place the beartrap on the %s.", // The message that appears after the trap has been placed. %s is replaced with the terrain name of the place where the trap has been put.
    "practice": 4, // How much practice to the "traps" skill placing the trap gives.
    "moves": 10 // (optional, default is 100): the move points that are used by placing the trap.
}
"use_action": {
    "type": "sew_advanced",  // Modify clothing
    "materials": [           // materials to deal with.
        "cotton",
        "leather"
    ],
    "skill": "tailor",       // Skill used.
    "clothing_mods": [       // Clothing mods to deal with.
        "leather_padded",
        "kevlar_padded"
    ]
}
"use_action": {
    "type" :"effect_on_conditions", // activate effect_on_conditions
    "description" :"This debugs the game", // usage description
    "effect_on_conditions" : ["test_cond"] // ids of the effect_on_conditions to activate
    }
"use_action": {
    "type": "message",      // Displays message text
    "message": "Read this.",// Message that is shown
    "name": "Light fuse"    // Optional name for the action. Default "Activate".
}
"use_action": {
    "type": "sound",         // Makes sound
    "name": "Turn on"        // Optional name for the action. Default "Activate".
    "sound_message": "Bzzzz.", // message shown to player if they are able to hear the sound. %s is replaced by item name.
    "sound_id": "misc"       // ID of the audio to be played. Default "misc". See SOUNDPACKS.md for more details.
	"sound_variant": "default" // Default "default"
    "sound_volume": 5        // Loudness of the noise.
}
"use_action": {
    "type": "manualnoise",   // Makes sound. Includes ammo checks and may take moves from player
    "use_message": "You do the thing" // Shown to player who activated it
    "noise_message": "Bzzz"  // Shown if player can hear the sound. Default "hsss".
    "noise_id": "misc"       // ID of the audio to be played. Default "misc". See SOUNDPACKS.md for more details.
	"noise_variant":         // Default "default"
    "noise" : 6              // Loudness of the noise. Default 0.
    "moves" : 40             // How long the action takes. Default 0.
}
```

  ### Tick Actions

`"tick_action"` of active tools is executed once on every turn. This action can be any use action or iuse but some of them may not work properly when not executed by player.

If `"tick_action"` is defined as array of multiple actions they all are executed in order. Multiple use actions of same type cannot be used at once.
  
#### Delayed Item Actions

Item use actions can be used with a timer delay.

Item `"transform"` action can set and start the timer. This timer starts when the player activates the item.
```
"use_action": {
    "type": "transform"
    "target": "grenade_act",
    "target_timer": "5 seconds"  // Sets timer on item to this
}
```

Timer inherent to the item itself can be set by defining `"countdown_interval"` in item json. This timer is started at the birth of the item.

```
    "id": "migo_plate_undergrown",
    "name": { "str": "undergrown iridescent carapace plate" },
    "countdown_interval": "24 hours",
```

Once the duration of the timer has passed the `"countdown_action"` is executed. This action can be any use action but many actions do not behave well when they are not triggered by the player.

```
"countdown_action": {
    "type": "explosion",
    "explosion": { "power": 240, "shrapnel": { "casing_mass": 217, "fragment_mass": 0.08 } }
}
```

Additionally `"revert_to"` can be defined in item definitions (not in use action). The item is deactivated and turned to this type after the `"countdown_action"`. If no revert_to is specified the item is destroyed.

### Random Descriptions

Any item with a "snippet_category" entry will have random descriptions, based on that snippet category:
```
"snippet_category": "newspaper",
```
The item descriptions are taken from snippets, which can be specified like this (the value of category must match the snippet_category in the item definition):
```C++
{
    "type" : "snippet",
    "category" : "newspaper",
    "id" : "snippet-id",          // id is optional, it's used when the snippet is referenced in the item list of professions
    "text": "your flavor text"
}
```
or several snippets at once:
```C++
{
    "type" : "snippet",
    "category" : "newspaper",
    "text": [
        "your flavor text",
        "more flavor",
        // entries can also be of this form to have a id to reference that specific snippet.
        { "id" : "snippet-id", "text" : "another flavor text" }
    ],
    "text": [ "your flavor text", "another flavor text", "more flavor" ]
}
```
Multiple snippets for the same category are possible and actually recommended. The game will select a random one for each item of that type.

One can also put the snippets directly in the item definition:
```
"snippet_category": [ "text 1", "text 2", "text 3" ],
```
This will automatically create a snippet category specific to that item and populate that category with the given snippets.
The format also support snippet ids like above.

# `json/` JSONs

### Harvest

```json
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
    `max` upper limit after `bas_num` and `scale_num` are calculated using
    `mass_ratio` value is a multiplier of how much of the monster's weight comprises the associated item. to conserve mass, keep between 0 and 1 combined with all drops. This overrides `base_num`, `scale_num` and `max`

For `type`s with "dissect_only" (see below), the following entries can scale the results:
    `max` this value (in contrary to `max` for other `type`s) corresponds to maximum butchery roll that will be passed to check_butcher_cbm() in activity_handlers.cpp; view check_butcher_cbm() to see corresponding distribution chances for roll values passed to that function

#### `leftovers`

itype_id of the item dropped as leftovers after butchery or when the monster is gibbed.  Default as "ruined_chunks".

### Harvest Drop Type
```json
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

```c++
{
    "type": "weapon_category",
    "id": "WEAP_CAT"
    "name": "Weapon Category"
}
```

### Connect group definitions

Connect groups can be used by id in terrain and furniture `connect_groups`, `connects_to` and `rotates_to` properties.

Examples from the actual definitions:

**`group_flags`**: Flags that imply that terrain or furniture is added to this group.

**`connects_to_flags`**: Flags that imply that terrain or furniture connects to this group.

**`rotates_to_flags`**: Flags that imply that terrain or furniture rotates to this group.

```json
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

```C++
{
    "type": "furniture",
    "id": "f_toilet",
    "name": "toilet",
    "symbol": "&",
    "looks_like": "chair",
    "color": "white",
    "move_cost_mod": 2,
    "keg_capacity": 240,
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

#### `move_cost_mod`

Movement cost modifier (`-10` = impassable, `0` = no change). This is added to the movecost of the underlying terrain.

#### `keg_capacity`

Determines capacity of some furnitures with liquid storage that have hardcoded interactions. Value is per 250mL (e.g. `"keg_capacity": 8,` = 2L)

#### `deployed_item`

Item id string used to create furniture. Allows player to interact with the furniture to 'take it down' (transform it to item form).

#### `lockpick_result`

(Optional) When the furniture is successfully lockpicked, this is the furniture it will turn into.

#### `lockpick_message`

(Optional) When the furniture is successfully lockpicked, this is the message that will be printed to the player. When it is missing, a generic `"The lock opens…"` message will be printed instead.

#### `light_emitted`

How much light the furniture produces.  10 will light the tile it's on brightly, 15 will light that tile and the tiles around it brightly, as well as slightly lighting the tiles two tiles away from the source.
For examples: An overhead light is 120, a utility light, 240, and a console, 10.

#### `boltcut`
(Optional) Data for using with an bolt cutter.
```cpp
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
```cpp
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
```cpp
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
(Optional) Data for using with pyring tools
```cpp
"prying": {
    "result": "furniture_id", // (optional) furniture it will become when done, defaults to f_null
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

```C++
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
    "harvestable": "blueberries",
    "transforms_into": "t_tree_harvested",
    "allowed_template_ids": [ "standard_template_construct", "debug_template", "afs_10mm_smart_template" ],
    "shoot": { "reduce_damage": [ 2, 12 ], "reduce_damage_laser": [ 0, 7 ], "destroy_damage": [ 40, 120 ], "no_laser_destroy": true },
    "harvest_by_season": [
      { "seasons": [ "spring", "autumn" ], "id": "burdock_harv" },
      { "seasons": [ "summer" ], "id": "burdock_summer_harv" }
    ],
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

#### `move_cost`

Move cost to move through. A value of 0 means it's impassable (e.g. wall). You should not use negative values. The positive value is multiple of 50 move points, e.g. value 2 means the player uses 2\*50 = 100 move points when moving across the terrain.

#### `heat_radiation`

Heat emitted for a terrain. A value of 0 means no fire (i.e, same as not having it). A value of 1 equals a fire of intensity of 1.

#### `light_emitted`

How much light the terrain emits. 10 will light the tile it's on brightly, 15 will light that tile and the tiles around it brightly, as well as slightly lighting the tiles two tiles away from the source.
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
```cpp
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
```cpp
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
```cpp
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
```cpp
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
```json
"harvest_by_season": [ { "seasons": [ "spring", "summer", "autumn", "winter" ], "id": "blackjack_harv" } ],
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

#### `comfort`

How comfortable this terrain/furniture is. Impact ability to fall asleep on it.
    uncomfortable = -999,
    neutral = 0,
    slightly_comfortable = 3,
    comfortable = 5,
    very_comfortable = 10

#### `floor_bedding_warmth`

Bonus warmth offered by this terrain/furniture when used to sleep.

#### `bonus_fire_warmth_feet`

Increase warmth received on feet from nearby fire  (default = 300)

#### `looks_like`

id of a similar item that this item looks like. The tileset loader will try to load the tile for that item if this item doesn't have a tile. looks_like entries are implicitly chained, so if 'throne' has looks_like 'big_chair' and 'big_chair' has looks_like 'chair', a throne will be displayed using the chair tile if tiles for throne and big_chair do not exist. If a tileset can't find a tile for any item in the looks_like chain, it will default to the ascii symbol.

#### `color` or `bgcolor`

Color of the object as it appears in the game. "color" defines the foreground color (no background color), "bgcolor" defines a solid background color. As with the "symbol" value, this can be an array with 4 entries, each entry being the color during the different seasons.

> **NOTE**: You must use ONLY ONE of "color" or "bgcolor"

#### `coverage`

The coverage percentage of a furniture piece of terrain. <30 won't cover from sight. (Does not interact with projectiles, gunfire, or other attacks. Only line of sight.)

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

```C++
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

##### `explosive`
(Optional) If greater than 0, destroying the object causes an explosion with this strength (see `game::explosion`).

##### `destroy_only`
TODO

##### `bash_below`
TODO

##### `tent_centers`, `collapse_radius`
(Optional) For furniture that is part of tents, this defines the id of the center part, which will be destroyed as well when other parts of the tent get bashed. The center is searched for in the given "collapse_radius" radius, it should match the size of the tent.

##### `items`

(Optional) An item group (inline) or an id of an item group, see [ITEM_SPAWN.md](ITEM_SPAWN.md). The default subtype is "collection". Upon successful bashing, items from that group will be spawned.

#### `map_deconstruct_info`

```C++
{
    "furn_set": "f_safe",
    "ter_set": "t_dirt",
    "items": "deconstructed_item_result_group"
}
```

##### `furn_set`, `ter_set`

The terrain / furniture that will be set after the original has been deconstructed. "furn_set" is optional (it defaults to no furniture), "ter_set" is only used upon "deconstruct" entries in terrain and is mandatory there.

##### `items`

(Optional) An item group (inline) or an id of an item group, see [ITEM_SPAWN.md](ITEM_SPAWN.md). The default subtype is "collection". Upon deconstruction the object, items from that group will be spawned.

#### `plant_data`

```JSON
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

```C++
"type": "clothing_mod",
"id": "leather_padded",   // Unique ID.
"flag": "leather_padded", // flag to add to clothing.
"item": "leather",        // item to consume.
"implement_prompt": "Pad with leather",      // prompt to show when implement mod.
"destroy_prompt": "Destroy leather padding", // prompt to show when destroy mod.
"restricted": true,       // (optional) If true, clothing must list this mod's flag in "valid_mods" list to use it. Defaults to false.
"mod_value": [            // List of mod effect.
    {
        "type": "bash",   // "bash", "cut", "bullet", "fire", "acid", "warmth", and "encumbrance" is available.
        "value": 1,       // value of effect.
        "round_up": false // (optional) round up value of effect. defaults to false.
        "proportion": [   // (optional) value of effect proportions to clothing's parameter.
            "thickness",  //            "thickness" and "coverage" is available.
            "coverage"
        ]
    }
]
```

# Scenarios

Scenarios are specified as JSON object with `type` member set to `scenario`.

```C++
{
    "type": "scenario",
    "id": "schools_out",
    ...
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
```C++
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
```C++
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

## `map_special`
(optional, string)

Add a map special to the starting location, see JSON_FLAGS for the possible specials.

## `requirement`

(optional, an achievement ID)

The achievement you need to do to access this scenario

## `reveal_locale`

(optional, boolean)

Defaults true. If a road can be found within 3 OMTs of the starting position, reveals a path to the nearest city and that city's center.

## `eocs`
(optional, array of strings)

A list of eocs that are triggered once for each new character on scenario start.

## `missions`
(optional, array of strings)

A list of mission ids that will be started and assigned to the player at the start of the game. Only missions with the ORIGIN_GAME_START origin are allowed. The last mission in the list will be the active mission, if multiple missions are assigned.

## `start_of_cataclysm`
(optional, object with optional members "hour", "day", "season" and "year")

Allows customization of cataclysm start date. If `start_of_cataclysm` is not set the corresponding default values are used instead - 0 hour, 0 day (which is day 1), Spring season, 1 year.

```C++
"start_of_cataclysm": { "hour": "random", "day": 10, "season": "winter", "year": 1 }
```

 Identifier            | Description
---                    | ---
`hour`                 | (optional, integer or `random` string) Hour of the day. Default value is 0. String `random` randomizes 0-23.
`day`                  | (optional, integer or `random` string) Day of the season. Default value is 0 (which is day 1). String `random` randomizes 0-season length.
`season`               | (optional, integer or `random` string) Season of the year. Default value is `SPRING`. String `random` randomizes to one of 4 season.
`year`                 | (optional, integer or `random` string) Year. Default value is 1. String `random` randomizes 1-11.

## `start_of_game`
(optional, object with optional members "hour", "day", "season" and "year")

Allows customization of game start date. If `start_of_game` is not set the corresponding default values are used instead - random hour (0-23), 0 day (which is day 1), Spring season, 1 year.

If the scenario game start date is before the scenario cataclysm start date then the scenario game start would be automatically set to scenario cataclysm start date.

```C++
"start_of_game": { "hour": "random", "day": "random", "season": "winter", "year": 2 }
```

 Identifier            | Description
---                    | ---
`hour`                 | (optional, integer or `random` string) Hour of the day. Default value is 0. String `random` randomizes 0-23.
`day`                  | (optional, integer or `random` string) Day of the season. Default value is 0 (which is day 1). String `random` randomizes 0-season length.
`season`               | (optional, integer or `random` string) Season of the year. Default value is `SPRING`. String `random` randomizes to one of 4 season.
`year`                 | (optional, integer or `random` string) Year. Default value is 1. String `random` randomizes 1-11.

# Starting locations

Starting locations are specified as JSON object with "type" member set to "start_location":
```C++
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

 Identifier            | Description
---                    | ---
`om_terrain`           | ID of overmap terrain which will be selected as the target. Mandatory.
`om_terrain_match_type`| Matching rule to use with `om_terrain`. Defaults to TYPE. Details are below.


`om_terrain_match_type` defaults to TYPE if unspecified, and has the following possible values:

* `EXACT` - The provided string must completely match the overmap terrain id,
  including linear direction suffixes for linear terrain types or rotation
  suffixes for rotated terrain types.

* `TYPE` - The provided string must completely match the base type id of the
  overmap terrain id, which means that suffixes for rotation and linear terrain
  types are ignored.

* `PREFIX` - The provided string must be a complete prefix (with additional
  parts delimited by an underscore) of the overmap terrain id. For example,
  "forest" will match "forest" or "forest_thick" but not "forestcabin".

* `CONTAINS` - The provided string must be contained within the overmap terrain
  id, but may occur at the beginning, end, or middle and does not have any rules
  about underscore delimiting.

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
```C++
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
```C++
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
```C++
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

If you want to remove some item, never do it with straightforward "remove the item json and call it a day", you **never remove the id from the game**. Primarily because it will cause a harmless, but annoying error, and someone else should spend their time and energy, explaining it was an intended change. To not cause this, everything, that get saved in the game require obsoletion: items, monsters, maps, monster factions, but not, for example, loot groups. Basically there is two ways to remove some entity (except replacing old item with new, while left the old id - this one do not require any additional manipulations) from the game - obsoletion and migration.

Migration is used, when we want to remove one item by replacing it with another item, that do exist in the game, or to maintain a consistent list of item type ids, and happen in `data/json/obsoletion/migration_items.json`

```C++

{
  "id": "arrowhead",  // id of item, that you want to migrate, mandatory
  "type": "MIGRATION", // type, mandatory
  "replace": "steel_chunk", // item, that replace the removed item, mandatory
  "variant": "m1014", // variant of an item, that would be used to replace the item. only for items, that do use variants
  "flags": [ "DIAMOND" ], // additional flag, that item would have when replaced
  "charges": 1930, // amount of charges, that replaced item would have
  "contents": [ { "id": "dogfood", "count": 1 } ], // if `replace` is container, describes what would be inside of it
  "sealed": false // if `replace` is container, will it be sealed or not
}

```

// it seems MIGRATION accept any field actually, but i need someone to confirm it

Migrating vehicle parts is done using `vehicle_part_migration` type, in the example below - when loading the vehicle any part with id `from` will have it's id switched to `to`.
For `VEH_TOOLS` parts only - `add_veh_tools` is a list of itype_ids to add to the vehicle tools after migrating the part.

```json
  {
    "type": "vehicle_part_migration",
    "//": "migration to VEH_TOOLS, remove after 0.H release",
    "from": "afs_metal_rig",
    "to": "veh_tools_workshop",
    "add_veh_tools": [ "welder", "soldering_iron", "forge", "kiln" ]
  }
```

For bionics, you should use `bionic_migration` type. The migration happens when character is loaded; if `to` is `null` the bionic will be deleted, if `to` is not null the id will be changed to the provided value.

```json
  {
    "type": "bionic_migration",
    "from": "bio_tools_extend",
    "to": null
  }
```

Obsoletion is used, when we want to remove the item entirely from the game, without any migration. For this you, again, **do not remove item** from the game.

For items, monsters, furniture, terrain, factions, loot groups and lot of similar stuff, you remove all places, where the entity can spawn (maps, palettes, NPCs etc), mark the item with "OBSOLETE" flag (optional), and move into `data/json/obsoletion/` or inside  - they will stay here till the next developement cycle, to make fluent transfer between one stable and another

For maps, you remove the item from all the places it can spawn, remove the mapgen entries, and add the overmap terrain id into `data/json/obsoletion/migration_oter_ids.json`, to migrate oter_id `hive` and `hive2` into `omt_obsolete` add an entry similar to this, note that if mapgen has already generated this area this will only alter the tile shown on the overmap:
```json
  {
    "type": "oter_id_migration",
    "//": "obsoleted in 0.H",
    "oter_ids": {
      "hive": "omt_obsolete",
      "hive2": "omt_obsolete"
    }
  }
```

For overmap specials add an entry to `data/json/obsoletion/migration_overmap_specials.json`:
```json
  {
    "type": "overmap_special_migration",
    "id": "Farm with silo",
    "//": "Removed in 0.G - no new id, this will remove it"
  },
  {
    "type": "overmap_special_migration",
    "id": "FakeSpecial_cs_open_sewer",
    "new_id": "cs_open_sewer",
    "//": "Removed <when> - this will migrate to 'new_id'"
  },
```

For EOC/dialogue variables you can use `var_migration`. This currently only migrates **Character**, and **Global** vars. If "to" isn't provided the variable will be migrated to a key of "NULL_VALUE".

```json
{
    "type": "var_migration",
    "from": "temp_var",
    "to": "new_temp_var"
}
```

For recipes, deleting the recipe is enough.

For mods, you need to add an `"obsolete": true,` boolean into MOD_INFO, which prevent the mod from showing into the mod list.

## Charge and temperature removal

If an item that used to have charges (e.g. `AMMO` or `COMESTIBLE` types) is changed to another type that does not use charges, migration is needed to ensure correct behavior when loading from existing save files, and prevent spurious error messages from being shown to the player.  Migration lists for this are found in `data/json/obsoletion/charge_removal.json`.

Such items may be added to one of the following:

`data/json/obsoletion/blacklist_charge_migration.json` a `charge_migration_blacklist` list:
Items in existing save files with `n` charges will be converted to `n` items with no charges.  This will preserve item count.

`data/json/obsoletion/blacklist_charge_removal.json` a `charge_removal_blacklist` list
* `charge_removal_blacklist`: items will simply have charges removed.

Additionally, `COMESTIBLE` items have temperature and rot processing, and are thus set as always activated.  When an item is changed from `COMESTIBLE` to a different type, migration is needed to check and unset this if applicable:

`data/json/obsoletion/blacklist_temperature_removal.json` a `temperature_removal_blacklist` list:

* In most cases, the item has no other features that require it to remain activated, in which case it can be simply added to `temperature_removal_blacklist`.  Items in this list will be deactivated and have temperature-related data cleared *without any further checks performed*.
* In case of an item that may be active for additional reasons other than temperature/rot tracking, an instance of the item loaded from existing save file cannot be blindly deactivated -- additional checks are required to see if it should remain active.  Instead of adding to the above list, a separate special case should be added in `src/savegame_json.cpp` to implement the necessary item-specific deactivation logic.


# Field types

Fields can exist on top of terrain/furniture, and support different intensity levels. Each intensity level inherits its properties from the lower one, so any field not explicitly overwritten will carry over. They affect both characters and monsters, but a lot of fields have hardcoded effects that are potentially different for both (found in `map_field.cpp:creature_in_field`). Gaseous fields that have a chance to do so are spread depending on the local wind force when outside, preferring spreading down to on the same Z-level, which is preferred to spreading upwards. Indoors and by weak winds fields spread randomly.

```C++
  {
    "type": "field_type", // this is a field type
    "id": "fd_gum_web", // id of the field
    "immune_mtypes": [ "mon_spider_gum" ], // list of monster immune to this field
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
    "outdoor_age_speedup": "20 minutes", // Increase the field's age by this duration if it's on an outdoor tile
    "accelerated_decay": true, // If the field should use a more simple decay calculation, used for cosmetic fields like gibs
    "percent_spread": 90, // The field must succeed on a `rng( 1, 100 - local windpower ) > percent_spread` roll to spread. The field must have a non-zero spread percent and the GAS phase to be eligible to spread in the first place
    "phase": "gas", // Phase of the field. Gases can spread after spawning and can be spawned out of emitters through impassable terrain with the PERMEABLE flag
    "apply_slime_factor": 10, // Intensity of slime to apply to creatures standing in this field (Why not use an effect? No idea.)
    "gas_absorption_factor": 15, // Amount of gasmask charges the field uses up per tick
    "is_splattering": true, // If splatters of this field should bloody vehicle parts
    "dirty_transparency_cache": true, // Should the transparency cache be recalculated when the field is modified (used for nontransparent, spreading fields)
    "has_fire": false, // Is this field a kind of fire (for immunity, monster avoidance and similar checks)
    "has_acid": false, // See has_fire
    "has_elec": false, // See has_fire
    "has_fume": false, // See has_fire, non-breathing monsters are immune to this field
    "display_items": true, // If the field should obscure items on this tile
    "display_fields": true, // If the field should obscure other fields
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
        { "item": "rag", "count": [ 40, 55 ] },
        { "item": "scrap", "count": [ 10, 20 ] }
      ]
    }
  }
```

## Immunity data
Immunity data can be provided at the field level or at the effect level based on intensity and body part. At the field level it applies immunity to all effects.

```JSON
"immunity_data": {  // Object containing the necessary conditions for immunity to this field.  Any one fulfilled condition confers immunity:
      "flags": [ "WEBWALK" ],  // Immune if the character has any of the defined character flags (see Character flags)
      "body_part_env_resistance": [ [ "mouth", 15 ], [ "sensor", 10 ] ], // Immune if ALL bodyparts of the defined types have the defined amount of env protection
      "immunity_flags_worn": [ [ "sensor", "FLASH_PROTECTION" ] ], // Immune if ALL parts of the defined types wear an item with the defined flag
      "immunity_flags_worn_any": [ [ "sensor", "BLIND" ], [ "hand", "PADDED" ] ], // Immune if ANY part of the defined type wears an item with the corresponding flag -- in this example either a blindfold OR padded gloves confer immunity
},
```

# Option sliders

```JSON
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
