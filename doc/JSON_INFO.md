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
    - [Body_parts](#body_parts)
    - [Bionics](#bionics)
    - [Dreams](#dreams)
    - [Disease](#disease)
    - [Item Groups](#item-groups)
    - [Item Category](#item-category)
    - [Materials](#materials)
      - [Fuel data](#fuel-data)
    - [Monster Groups](#monster-groups)
      - [Group definition](#group-definition)
      - [Monster definition](#monster-definition)
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
      - [`proficiencies`](#proficiencies)
      - [`items`](#items)
      - [`pets`](#pets)
      - [`vehicle`](#vehicle)
      - [`flags`](#flags)
      - [`cbms`](#cbms)
      - [`traits`](#traits)
    - [Recipes](#recipes)
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
    - [Traits/Mutations](#traitsmutations)
    - [Traps](#traps)
    - [Vehicle Groups](#vehicle-groups)
    - [Vehicle Parts](#vehicle-parts)
      - [Symbols and Variants](#symbols-and-variants)
      - [The following optional fields are specific to CARGO or FLUIDTANK parts.](#the-following-optional-fields-are-specific-to-cargo-or-fluidtank-parts)
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
- [`data/json/items/` JSONs](#datajsonitems-jsons)
    - [Generic Items](#generic-items)
      - [To hit object](#to-hit-object)
    - [Ammo](#ammo)
    - [Magazine](#magazine)
    - [Armor](#armor)
      - [Guidelines for thickness:](#guidelines-for-thickness)
    - [Pet Armor](#pet-armor)
    - [Books](#books)
      - [Conditional Naming](#conditional-naming)
      - [Color Key](#color-key)
      - [CBMs](#cbms)
    - [Comestibles](#comestibles)
    - [Containers](#containers)
    - [Melee](#melee)
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
- [`json/` JSONs](#json-jsons)
    - [Harvest](#harvest)
      - [`id`](#id)
      - [`type`](#type)
      - [`message`](#message)
      - [`entries`](#entries)
    - [leftovers](#leftovers)
    - [Furniture](#furniture)
      - [`type`](#type-1)
      - [`move_cost_mod`](#move_cost_mod)
      - [`light_emitted`](#light_emitted)
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
      - [`trap`](#trap)
      - [`transforms_into`](#transforms_into)
      - [`harvest_by_season`](#harvest_by_season)
      - [`roof`](#roof)
    - [Common To Furniture And Terrain](#common-to-furniture-and-terrain)
      - [`id`](#id-1)
      - [`name`](#name-1)
      - [`flags`](#flags-1)
      - [`connects_to`](#connects_to)
      - [`symbol`](#symbol)
      - [`comfort`](#comfort)
      - [`floor_bedding_warmth`](#floor_bedding_warmth)
      - [`bonus_fire_warmth_feet`](#bonus_fire_warmth_feet)
      - [`looks_like`](#looks_like)
      - [`color` or `bgcolor`](#color-or-bgcolor)
      - [`max_volume`](#max_volume)
      - [`examine_action`](#examine_action)
      - [`close" And "open`](#close-and-open)
      - [`bash`](#bash)
      - [`deconstruct`](#deconstruct)
      - [`map_bash_info`](#map_bash_info)
      - [`str_min`, `str_max`, `str_min_blocked`, `str_max_blocked`, `str_min_supported`, `str_max_supported`](#str_min-str_max-str_min_blocked-str_max_blocked-str_min_supported-str_max_supported)
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
  - [`traits", "forced_traits", "forbidden_traits`](#traits-forced_traits-forbidden_traits)
  - [`allowed_locs`](#allowed_locs)
  - [`start_name`](#start_name)
  - [`professions`](#professions)
  - [`map_special`](#map_special)
  - [`missions`](#missions)
  - [`custom_initial_date`](#custom_initial_date)
- [Starting locations](#starting-locations)
  - [`name`](#name-3)
  - [`terrain`](#terrain)
  - [`flags`](#flags-3)
- [Mutation overlay ordering](#mutation-overlay-ordering)
  - [`id`](#id-2)
  - [`order`](#order)
- [MOD tileset](#mod-tileset)
  - [`compatibility`](#compatibility)
  - [`tiles-new`](#tiles-new)
- [Field types](#field-types)

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

The documentation is organised by file, because objects of the same type tend
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
    "description": "A towering coniferous tree that belongs to the 'Pinus' genus, with the New England species varying from 'P. strobus', 'P. resinosa' and 'P. rigida'.  Some of the branches have been stripped away and many of the pinecones aren't developed fully yet, but given a season, it could be harvestable again.  Also, you could cut it down with the right tools.",
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
conventional SI abbreviations.  For example, a volume of 3 litres would be
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

Currently, only some JSON values support this syntax (see [here](doc/TRANSLATING.md#translation) for a list of supported values and more detailed explanation).

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

| Filename             | Description
|---                   |---
| `ammo.json`          | common base components like batteries and marbles
| `ammo_types.json`    | standard ammo types by gun
| `archery.json`       | bows and arrows
| `armor.json`         | armor and clothing
| `bionics.json`       | Compact Bionic Modules (CBMs)
| `biosignatures.json` | animal waste
| `books.json`         | books
| `chemicals_and_resources.json` | chemical precursors
| `comestibles.json`   | food/drinks
| `containers.json`    | containers
| `crossbows.json`     | crossbows and bolts
| `fake.json`          | fake items for bionics or mutations
| `fuel.json`          | liquid fuels
| `grenades.json`      | grenades and throwable explosives
| `handloaded_bullets.json` | random ammo
| `melee.json`         | melee weapons
| `migration.json`     | conversions of obsolete items from older save games to current items
| `newspaper.json`     | flyers, newspapers, and survivor notes. `snippets.json` for messages
| `obsolete.json`      | items being removed from the game
| `ranged.json`        | guns
| `software.json`      | software for SD-cards and USB sticks
| `tool_armor.json`    | clothes and armor that can be (a)ctivated
| `toolmod.json`       | modifications of tools
| `tools.json`         | tools and items that can be (a)ctivated
| `vehicle_parts.json` | components of vehicles when they aren't on the vehicle

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

| Identifier        | Description
|---                |---
| id                | Unique ID. Must be one continuous word, use underscores if necessary.
| picture           | Array of string, each entry is a line of an ascii picture and must be at most 41 columns long. \ have to be replaced by \\\ in order to be visible.

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
For information about tools with option to export ASCII art in format ready to be pasted into `ascii_arts.json`, see `ASCII_ARTS.md`.

### Body_parts

| Identifier        | Description
|---                |---
| id                | (_mandatory_) Unique ID. Must be one continuous word, use underscores if necessary.
| name              | (_mandatory_) In-game name displayed.
| accusative        | (_mandatory_) Accusative form for this bodypart.
| heading           | (_mandatory_) How it's displayed in headings.
| heading_multiple  | (_mandatory_) Plural form of heading.
| hp_bar_ui_text    | (_mandatory_) How it's displayed next to the hp bar in the panel.
| main_part         | (_mandatory_) What is the main part this one is attached to. (If this is a main part it's attached to itself)
| connected_to      | (_mandatory_ if main_part is itself) What is the next part this one is attached to towards the "root" bodypart (the root bodypart should be connected to itself).  Each anatomy should have a unique root bodypart, usually the head.
| base_hp           | (_mandatory_) The amount of hp this part has before any modification.
| opposite_part     | (_mandatory_) What is the opposite part ot this one in case of a pair.
| hit_size          | (_mandatory_) Size of the body part when doing an unweighted selection.
| hit_size_relative | (_mandatory_) Hit sizes for attackers who are smaller, equal in size, and bigger.
| hit_difficulty    | (_mandatory_) How hard is it to hit a given body part, assuming "owner" is hit. Higher number means good hits will veer towards this part, lower means this part is unlikely to be hit by inaccurate attacks. Formula is `chance *= pow(hit_roll, hit_difficulty)`
| drench_capacity   | (_mandatory_) How wet this part can get before being 100% drenched.
| stylish_bonus     | (_optional_) Mood bonus associated with wearing fancy clothing on this part. (default: `0`)
| hot_morale_mod    | (_optional_) Mood effect of being too hot on this part. (default: `0`)
| cold_morale_mod   | (_optional_) Mood effect of being too cold on this part. (default: `0`)
| squeamish_penalty | (_optional_) Mood effect of wearing filthy clothing on this part. (default: `0`)
| stat_hp_mods      | (_optional_) Values modifying hp_max of this part following this formula: `hp_max += int_mod*int_max + dex_mod*dex_max + str_mod*str_max + per_mod*per_max + health_mod*get_healthy()` with X_max being the unmodified value of the X stat and get_healthy() being the hidden health stat of the character.
| bionic_slots      | (_optional_) How many bionic slots does this part have.
| is_limb           | (_optional_) Is this bodypart a limb. (default: `false`)
| smash_message      | (_optional_) The message displayed when using that part to smash something.
| smash_efficiency  | (_optional_) Modifier applied to your smashing strength when using this part to smash terrain or furniture unarmed. (default: `0.5`)

```C++
  {
    "id": "torso",
    "type": "body_part",
    "name": "torso",
    "accusative": { "ctxt": "bodypart_accusative", "str": "torso" },
    "heading": "Torso",
    "heading_multiple": "Torso",
    "hp_bar_ui_text": "TORSO",
    "encumbrance_text": "Dodging and melee is hampered.",
    "main_part": "torso",
    "opposite_part": "torso",
    "hit_size": 45,
    "hit_size_relative": [ 20, 33.33, 36.57 ],
    "hit_difficulty": 1,
    "side": "both",
    "legacy_id": "TORSO",
    "stylish_bonus": 6,
    "hot_morale_mod": 2,
    "cold_morale_mod": 2,
    "squeamish_penalty": 6,
    "base_hp": 60,
    "drench_capacity": 40,
    "stat_hp_mods": { "int_mod": 4.0, "dex_mod": 1.0, "str_mod": 1.0, "per_mod": 1.0, "health_mod": 1.0 },
    "smash_message": "You smash the %s with a powerful shoulder-check.",
    "bionic_slots": 80
  }
```

### Bionics

| Identifier                  | Description
|---                          |---
| id                          | Unique ID. Must be one continuous word, use underscores if necessary.
| name                        | In-game name displayed.
| description                 | In-game description.
| act_cost                    | (_optional_) How many kJ it costs to activate the bionic.  Strings can be used "1 kJ"/"1000 J"/"1000000 mJ" (default: `0`)
| deact_cost                  | (_optional_) How many kJ it costs to deactivate the bionic.  Strings can be used "1 kJ"/"1000 J"/"1000000 mJ" (default: `0`)
| react_cost                  | (_optional_) How many kJ it costs over time to keep this bionic active, does nothing without a non-zero "time".  Strings can be used "1 kJ"/"1000 J"/"1000000 mJ" (default: `0`)
| time                        | (_optional_) How long, when activated, between drawing cost. If 0, it draws power once. (default: `0`)
| upgraded_bionic             | (_optional_) Bionic that can be upgraded by installing this one.
| available_upgrades          | (_optional_) Upgrades available for this bionic, i.e. the list of bionics having this one referenced by `upgraded_bionic`.
| encumbrance                 | (_optional_) A list of body parts and how much this bionic encumber them.
| weight_capacity_bonus       | (_optional_) Bonus to weight carrying capacity in grams, can be negative.  Strings can be used - "5000 g" or "5 kg" (default: `0`)
| weight_capacity_modifier    | (_optional_) Factor modifying base weight carrying capacity. (default: `1`)
| canceled_mutations          | (_optional_) A list of mutations/traits that are removed when this bionic is installed (e.g. because it replaces the fault biological part).
| included_bionics            | (_optional_) Additional bionics that are installed automatically when this bionic is installed. This can be used to install several bionics from one CBM item, which is useful as each of those can be activated independently.
| included                    | (_optional_) Whether this bionic is included with another. If true this bionic does not require a CBM item to be defined. (default: `false`)
| env_protec                  | (_optional_) How much environmental protection does this bionic provide on the specified body parts.
| bash_protec                 | (_optional_) How much bash protection does this bionic provide on the specified body parts.
| cut_protec                  | (_optional_) How much cut protection does this bionic provide on the specified body parts.
| bullet_protect              | (_optional_) How much bullet protect does this bionic provide on the specified body parts.
| occupied_bodyparts          | (_optional_) A list of body parts occupied by this bionic, and the number of bionic slots it take on those parts.
| capacity                    | (_optional_) Amount of power storage added by this bionic.  Strings can be used "1 kJ"/"1000 J"/"1000000 mJ" (default: `0`)
| fuel_options                | (_optional_) A list of materials that this bionic can use to produce bionic power.
| is_remote_fueled            | (_optional_) If true this bionic allows you to plug your power banks to an external power source (solar backpack, UPS, vehicle etc) via a cable. (default: `false`)
| fuel_capacity               | (_optional_) Volume of fuel this bionic can store.
| fuel_efficiency             | (_optional_) Fraction of fuel energy converted into power. (default: `0`)
| passive_fuel_efficiency     | (_optional_) Fraction of fuel energy passively converted into power. Useful for CBM using PERPETUAL fuel like `muscle`, `wind` or `sun_light`. (default: `0`)
| exothermic_power_gen        | (_optional_) If true this bionic emits heat when producing power. (default: `false`)
| coverage_power_gen_penalty  | (_optional_) Fraction of coverage diminishing fuel_efficiency. Float between 0.0 and 1.0. (default: `nullopt`)
| power_gen_emission          | (_optional_) `emit_id` of the field emitted by this bionic when it produces energy. Emit_ids are defined in `emit.json`.
| stat_bonus                  | (_optional_) List of passive stat bonus. Stat are designated as follow: "DEX", "INT", "STR", "PER".
| enchantments                | (_optional_) List of enchantments applied by this CBM (see MAGIC.md for instructions on enchantment. NB: enchantments are not necessarily magic.)
| learned_spells              | (_optional_) Map of {spell:level} you gain when installing this CBM, and lose when you uninstall this CBM. Spell classes are automatically gained.
| learned_proficiencies       | (_optional_) Array of proficiency ids you gain when installing this CBM, and lose when uninstalling
| installation_requirement    | (_optional_) Requirement id pointing to a requirement defining the tools and components necessary to install this CBM.
| vitamin_absorb_mod          | (_optional_) Modifier to vitamin absorption, affects all vitamins. (default: `1.0`)
| social_modifiers			  | (_optional_) Json object with optional members: persuade, lie, and intimidate which add or subtract that amount from those types of social checks
```C++
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
    "bash_protec": [ [ "leg_l", 3 ], [ "leg_r", 3 ] ],
    "cut_protec": [ [ "leg_l", 3 ], [ "leg_r", 3 ] ],
    "bullet_protec": [ [ "leg_l", 3 ], [ "leg_r", 3 ] ],
    "flags": [ "BIONIC_NPC_USABLE" ]
}
```

Bionics effects are defined in the code and new effects cannot be created through JSON alone.
When adding a new bionic, if it's not included with another one, you must also add the corresponding CBM item in `data/json/items/bionics.json`. Even for a faulty bionic.

### Dreams

| Identifier | Description
|---         |---
| messages   | List of potential dreams.
| category   | Mutation category needed to dream.
| strength   | Mutation category strength required (1 = 20-34, 2 = 35-49, 3 = 50+).

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

| Identifier         | Description
|---                 |---
| id                 | Unique ID. Must be one continuous word, use underscores if necessary.
| min_duration       | The minimum duration the disease can last. Uses strings "x m", "x s","x d".
| max_duration       | The maximum duration the disease can last.
| min_intensity      | The minimum intensity of the effect applied by the disease
| max_intensity      | The maximum intensity of the effect.
| health_threshold   | The amount of health above which one is immune to the disease. Must be between -200 and 200. (optional )
| symptoms           | The effect applied by the disease.
| affected_bodyparts | The list of bodyparts on which the effect is applied. (optional, default to bp_null)


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

### Item Groups

Item groups have been expanded, look at [the detailed docs](ITEM_SPAWN.md) to their new description.
The syntax listed here is still valid.

| Identifier | Description
|---         |---
| id         | Unique ID. Must be one continuous word, use underscores if necessary
| items      | List of potential item ID's. Chance of an item spawning is x/T, where X is the value linked to the specific item and T is the total of all item values in a group.
| groups     | ??

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

| Identifier      | Description
|---              |---
| id              | Unique ID. Must be one continuous word, use underscores if necessary
| name            | The name of the category. This is what shows up in-game when you open the inventory.
| zone            | The corresponding loot_zone (see loot_zones.json)
| sort_rank       | Used to sort categories when displaying.  Lower values are shown first
| priority_zones  | When set, items in this category will be sorted to the priority zone if the conditions are met. If the user does not have the priority zone in the zone manager, the items get sorted into zone set in the 'zone' property. It is a list of objects. Each object has 3 properties: ID: The id of a LOOT_ZONE (see LOOT_ZONES.json), filthy: boolean. setting this means filthy items of this category will be sorted to the priority zone, flags: array of flags

```C++
{
    "id":"armor",
    "name": "ARMOR",
    "zone": "LOOT_ARMOR",
    "sort_rank": -21,
    "priority_zones": [ { "id": "LOOT_FARMOR", "filthy": true, "flags": [ "RAINPROOF" ] } ],
}
```

### Materials

| Identifier       | Description
|---               |---
| `id`             | Unique ID. Lowercase snake_case. Must be one continuous word, use underscores if necessary.
| `name`           | In-game name displayed.
| `bash_resist`    | How well a material resists bashing damage.
| `cut_resist`     | How well a material resists cutting damage.
| `bullet_resist`  | How well a material resists bullet damage.
| `acid_resist`    | Ability of a material to resist acid.
| `elec_resist`    | Ability of a material to resist electricity.
| `fire_resist`    | Ability of a material to resist fire.
| `chip_resist`    | Returns resistance to being damaged by attacks against the item itself.
| `bash_dmg_verb`  | Verb used when material takes bashing damage.
| `cut_dmg_verb`   | Verb used when material takes cutting damage.
| `dmg_adj`        | Description added to damaged item in ascending severity.
| `dmg_adj`        | Adjectives used to describe damage states of a material.
| `density`        | Affects vehicle collision damage, with denser parts having the advantage over less-dense parts.
| `vitamins`       | Vitamins in a material. Usually overridden by item specific values.  An integer percentage of ideal daily value.
| `specific_heat_liquid` | Specific heat of a material when not frozen (J/(g K)). Default 4.186.
| `specific_heat_solid`  | Specific heat of a material when frozen (J/(g K)). Default 2.108.
| `latent_heat`    | Latent heat of fusion for a material (J/g). Default 334.
| `freezing_point`   | Freezing point of this material (C). Default 0 C ( 32 F ).
| `edible`   | Optional boolean. Default is false.
| `rotting`   | Optional boolean. Default is false.
| `soft`   | Optional boolean. Default is false.
| `reinforces`   | Optional boolean. Default is false.

There are seven -resist parameters: acid, bash, chip, cut, elec, fire, and bullet. These are integer values; the default is 0 and they can be negative to take more damage.

```C++
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
    "bash_resist": 1,
    "cut_resist": 1,
    "bullet_resist": 1,
    "acid_resist": 1,
    "fire_resist": 1,
    "elec_resist": 1,
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

Every material can have fuel data that determines how much horse power it produces per unit consumed. Currently, gasses and plasmas cannot really be fuels.

If a fuel has the PERPETUAL flag, engines powered by it never use any fuel.  This is primarily intended for the muscle pseudo-fuel, but mods may take advantage of it to make perpetual motion machines.

```C++
"fuel_data" : {
    energy": 34.2,               // battery charges per mL of fuel. batteries have energy 1
                                 // is also MJ/L from https://en.wikipedia.org/wiki/Energy_density
                                 // assumes stacksize 250 per volume 1 (250mL). Multiply
                                 // by 250 / stacksize * volume for other stack sizes and
                                 // volumes
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

### Monster Groups

#### Group definition

| Identifier  | Description
|---          |---
| `name`      | Unique ID. Must be one continuous word, use underscores if necessary.
| `default`   | Default monster, automatically fills in any remaining spawn chances.
| `monsters`  | To choose a monster for spawning, the game creates 1000 entries and picks one. Each monster will have a number of entries equal to it's "freq" and the default monster will fill in the remaining. See the table below for how to build the single monster definitions.
| `is_safe`   | (bool) Check to not trigger safe-mode warning.
| `is_animal` | (bool) Check if that group has only normal animals.

#### Monster definition

| Identifier        | Description
|---                |---
| `monster`         | The monster's unique ID, eg. `"mon_zombie"`.
| `freq`            | Chance of occurrence, x/1000.
| `cost_multiplier` | How many monsters each monster in this definition should count as, if spawning a limited number of monsters.
| `pack_size`       | (_optional_) The minimum and maximum number of monsters in this group that should spawn together.  (default: `[1,1]`)
| `conditions`      | Conditions limit when monsters spawn. Valid options: `SUMMER`, `WINTER`, `AUTUMN`, `SPRING`, `DAY`, `NIGHT`, `DUSK`, `DAWN`. Multiple Time-of-day conditions (`DAY`, `NIGHT`, `DUSK`, `DAWN`) will be combined together so that any of those conditions makes the spawn valid. Multiple Season conditions (`SUMMER`, `WINTER`, `AUTUMN`, `SPRING`) will be combined together so that any of those conditions makes the spawn valid.
| `starts`          | (_optional_) This entry becomes active after this time. (Measured in hours)
| `ends`            | (_optional_) This entry becomes inactive after this time. (Measured in hours)
| `spawn_data`      | (_optional_) Any properties that the monster only has when spawned in this group. `ammo` defines how much of which ammo types the monster spawns with.

```C++
{
    "name" : "GROUP_ANT",
    "default" : "mon_ant",
    "monsters" : [
        { "monster" : "mon_ant_larva", "freq" : 40, "multiplier" : 0 },
        { "monster" : "mon_ant_soldier", "freq" : 90, "multiplier" : 5 },
        { "monster" : "mon_ant_queen", "freq" : 0, "multiplier" : 0 },
        { "monster" : "mon_thing", "freq" : 100, "multiplier" : 0, "pack_size" : [3,5], "conditions" : ["DUSK","DAWN","SUMMER"] }
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

See MONSTERS.md

### Mutation Categories

A Mutation Category identifies a set of interrelated mutations that as a whole establish an entirely new identity for a mutant character. Categories can and usually do have their own "flavor" of mutagen, the properties of which are defined in the category definition itself. A second kind of mutagen, called a "mutagen serum" or "IV mutagen" is necessary to trigger "threshold mutations" which cause irrevocable changes to the character.

| Identifier         | Description
|---                 |---
| `id`               | Unique ID. Must be one continuous word, use underscores when necessary.
| `name`             | Human readable name for the category of mutations.
| `threshold_mut`    | A special mutation that marks the point at which the identity of the character is changed by the extent of mutation they have experienced.
| `mutagen_message`  | A message displayed to the player when they take a mutagen of the matching type.
| `mutagen_hunger`   | The amount of hunger (per mutation triggered) caused by taking the matching mutagen.
| `mutagen_thirst`   | The amount of thirst (per mutation triggered) caused by taking the matching mutagen.
| `mutagen_pain`     | The amount of pain caused (per mutation triggered) by taking the matching mutagen.
| `mutagen_fatigue`  | The amount of fatigue caused (per mutation triggered) by taking the matching mutagen.
| `mutagen_morale`   | The amount of morale increase caused by taking the matching mutagen for mutagen junkies.
| `iv_message`       | A message displayed to the player when they take a mutagen serum of the matching type.
| `iv_min_mutations` | The minimum number of mutations to trigger when taking the mutagen serum.
| `iv_additional_mutations`  | The minimum number of mutations to trigger when taking the mutagen serum.
| `iv_additional_mutations_chance`  | The probability of acquiring additional mutations, the formula is one in "additional_mutations_chance" per "additional_mutation".
| `iv_hunger`        | The amount of hunger (per mutation triggered) caused by taking the matching mutagen serum.
| `iv_thirst`        | The amount of thirst (per mutation triggered) caused by taking the matching mutagen serum.
| `iv_pain`          | The amount of pain (per mutation triggered) caused by taking the matching mutagen serum.
| `iv_fatigue`       | The amount of fatigue caused (per mutation triggered) by taking the matching mutagen serum.
| `iv_morale`        | The minimum amount of morale caused by taking the matching mutagen serum.
| `iv_morale_max`    | The maximum amount of morale caused by taking the matching mutagen serum.
| `iv_sound`         | A flag indicating that taking the matching mutagen serum causes the character to make noise.
| `iv_sound_message` | The message describing the noise made by the character when taking the mutagen serum.
| `iv_sound_id`      | The id of a sound clip to play depicting the noise made by the character.
| `iv_sound_variant` | The id of a variant clip to play depicting the noise made by the character.
| `iv_noise`         | The volume of the noise the character makes.
| `iv_sleep`         | A flag indicating that the player will involuntarily sleep after taking the matching mutation serum.
| `iv_sleep_message` | The message to display notifying the player that their character has fallen sleep.
| `iv_sleep_dur`     | The duration of the involuntary sleep in seconds.
| `memorial_message` | The memorial message to display when a character crosses the associated mutation threshold.
| `junkie_message`   | The message to display if the character is addicted to the associated mutagen and takes some.
| `wip`              | A flag indicating that a mutation category is unfinished and shouldn't have consistency tests run on it. See tests/mutation_test.cpp.

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
- "type": the string id of the addiction (see JSON_FLAGS.md),
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

### Recipes

Crafting recipes are defined as a JSON object with the following fields:

```C++
"result": "javelin",         // ID of resulting item
"byproducts": [ [ "" ] ],    // Optional (default: empty). Additional items generated by crafting this recipe.
"category": "CC_WEAPON",     // Category of crafting recipe. CC_NONCRAFT used for disassembly recipes
"subcategory": "CSC_WEAPON_PIERCING",
"id_suffix": "",             // Optional (default: empty string). Some suffix to make the ident of the recipe unique. The ident of the recipe is "<id-of-result><id_suffix>".
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
"reversible": false,         // Can be disassembled.
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
      "fail_multiplier": 2.5 // The multiplier on failure chance when crafting without this proficiency. Defaults to 2.5. Multiple proficiencies will multiply this value. (if all have the default, it's fail_multiplier ^ n, where n is the number of proficiencies that are lacked)
      "learning_time_multiplier": 1.2 // The multiplier on learning speed for this proficiency. By default, it's the time of the recipe, divded by the time multiplier, and by the number of proficiencies that can also be learned from it.
      "max_experience": "15 m" // This recipe cannot raise your experience for that proficiency above 15 minutes worth.
    }
]
"batch_time_factors": [25, 15], // Optional factors for batch crafting time reduction. First number specifies maximum crafting time reduction as percentage, and the second number the minimal batch size to reach that number. In this example given batch size of 20 the last 6 crafts will take only 3750 time units.
"flags": [                   // A set of strings describing boolean features of the recipe
  "BLIND_EASY",
  "ANOTHERFLAG"
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
]
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
"pre_terrain": "t_pit",                                             // Required terrain to build on
"post_terrain": "t_pit_spiked"                                      // Terrain type after construction is complete
```

### Scent_types

| Identifier               | Description
|---                       |---
| id                       | Unique ID. Must be one continuous word, use underscores if necessary.
| receptive_species        | Species able to track this scent. Must use valid ids defined in `species.json`

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
  [`event_field_transformation.cpp`](../src/event_field_transformation.cpp).
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
    // "equals_statistic" specifies that the value must match the value of some statistic (see below)
    "mount" : { "equals": [ "mtype_id", "mon_horse" ] }
}
// Since we are filtering to only those events where 'mount' is 'mon_horse', we
// might as well drop the 'mount' field, since it provides no useful information.
"drop_fields" : [ "mount" ]
```

The parameter to `"equals"` is normally a length-two array specifying a
`cata_variant_type` and a value.  As a short cut, you can simply specify an
`int` or `bool` (e.g. `"equals": 7` or `"equals": true`) for fields which have
those types.

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
term as popularised in other games.

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

### Traits/Mutations

```C++
"id": "LIGHTEATER",  // Unique ID
"name": "Optimist",  // In-game name displayed
"points": 2,         // Point cost of the trait. Positive values cost points and negative values give points
"visibility": 0,     // Visibility of the trait for purposes of NPC interaction (default: 0)
"ugliness": 0,       // Ugliness of the trait for purposes of NPC interaction (default: 0)
"cut_dmg_bonus": 3, // Bonus to unarmed cut damage (default: 0)
"pierce_dmg_bonus": 3, // Bonus to unarmed pierce damage (default: 0.0)
"bash_dmg_bonus": 3, // Bonus to unarmed bash damage (default: 0)
"butchering_quality": 4, // Butchering quality of this mutations (default: 0)
"rand_cut_bonus": { "min": 2, "max": 3 }, // Random bonus to unarmed cut damage between min and max.
"rand_bash_bonus": { "min": 2, "max": 3 }, // Random bonus to unarmed bash damage between min and max.
"bodytemp_modifiers" : [100, 150], // Range of additional bodytemp units (these units are described in 'weather.h'. First value is used if the person is already overheated, second one if it's not.
"bodytemp_sleep" : 50, // Additional units of bodytemp which are applied when sleeping
"initial_ma_styles": [ "style_crane" ], // (optional) A list of ids of martial art styles of which the player can choose one when starting a game.
"mixed_effect": false, // Whether the trait has both positive and negative effects. This is purely declarative and is only used for the user interface. (default: false)
"description": "Nothing gets you down!" // In-game description
"starting_trait": true, // Can be selected at character creation (default: false)
"valid": false,      // Can be mutated ingame (default: true)
"purifiable": false, //Sets if the mutation be purified (default: true)
"profession": true, //Trait is a starting profession special trait. (default: false)
"debug": false,     //Trait is for debug purposes (default: false)
"player_display": true, //Trait is displayed in the `@` player display menu
"category": ["MUTCAT_BIRD", "MUTCAT_INSECT"], // Categories containing this mutation
// prereqs and prereqs2 specify prerequisites of the current mutation
// Both are optional, but if prereqs2 is specified prereqs must also be specified
// The example below means: ( "SLOWREADER" OR "ILLITERATE") AND ("CEPH_EYES" OR "LIZ_EYE")
"prereqs": [ "SLOWREADER", "ILLITERATE"],
"prereqs2": [ "CEPH_EYES", "LIZ_EYE" ],
"threshreq": ["THRESH_SPIDER"], //Required threshold for this mutation to be possible
"cancels": ["ROT1", "ROT2", "ROT3"], // Cancels these mutations when mutating
"changes_to": ["FASTHEALER2"], // Can change into these mutations when mutating further
"leads_to": [], // Mutations that add to this one
"passive_mods" : { //increases stats with the listed value. Negative means a stat reduction
            "per_mod" : 1, //Possible values per_mod, str_mod, dex_mod, int_mod
            "str_mod" : 2
},
"wet_protection":[{ "part": "head", // Wet Protection on specific bodyparts
                    "good": 1 } ] // "neutral/good/ignored" // Good increases pos and cancels neg, neut cancels neg, ignored cancels both
"vitamin_rates": [ [ "vitC", -1200 ] ], // How much extra vitamins do you consume per minute. Negative values mean production
"vitamins_absorb_multi": [ [ "flesh", [ [ "vitA", 0 ], [ "vitB", 0 ], [ "vitC", 0 ], [ "calcium", 0 ], [ "iron", 0 ] ], [ "all", [ [ "vitA", 2 ], [ "vitB", 2 ], [ "vitC", 2 ], [ "calcium", 2 ], [ "iron", 2 ] ] ] ], // multiplier of vitamin absorption based on material. "all" is every material. supports multiple materials.
"craft_skill_bonus": [ [ "electronics", -2 ], [ "tailor", -2 ], [ "mechanics", -2 ] ], // Skill affected by the mutation and their bonuses. Bonuses can be negative, a bonus of 4 is worth 1 full skill level.
"restricts_gear" : [ "torso" ], //list of bodyparts that get restricted by this mutation
"allow_soft_gear" : true, //If there is a list of 'restricts_gear' this sets if the location still allows items made out of soft materials (Only one of the types need to be soft for it to be considered soft). (default: false)
"destroys_gear" : true, //If true, destroys the gear in the 'restricts_gear' location when mutated into. (default: false)
"encumbrance_always" : [ // Adds this much encumbrance to selected body parts
    [ "arm_l", 20 ],
    [ "arm_r", 20 ]
],
"encumbrance_covered" : [ // Adds this much encumbrance to selected body parts, but only if the part is covered by not-OVERSIZE worn equipment
    [ "hand_l", 50 ],
    [ "hand_r", 50 ]
],
"encumbrance_multiplier_always": { // if the bodypart has encumbrance caused by a mutation, multiplies that encumbrance penalty by this multiplier.
  "arm_l": 0.75,                   // DOES NOT AFFECT CLOTHING ENCUMBRANCE
  "arm_r": 0.75
},
"armor" : [ // Protects selected body parts this much. Resistances use syntax like `PART RESISTANCE` below.
    [
        [ "head" ],
        { "bash" : 2 } // The resistance provided to the body part(s) selected above
    ],
    [   // NOTE: Resistances are applies in order and ZEROED between applications!
        [ "arm_l", "arm_r" ], // Overrides the above settings for those body parts
        { "bash" : 1 }        // ...and gives them those resistances instead
    ]
],
"stealth_modifier" : 0, // Percentage to be subtracted from player's visibility range, capped to 60. Negative values work, but are not very effective due to the way vision ranges are capped
"active" : true, //When set the mutation is an active mutation that the player needs to activate (default: false)
"starts_active" : true, //When true, this 'active' mutation starts active (default: false, requires 'active')
"cost" : 8, // Cost to activate this mutation. Needs one of the hunger, thirst, or fatigue values set to true. (default: 0)
"time" : 100, //Sets the amount of (turns * current player speed ) time units that need to pass before the cost is to be paid again. Needs to be higher than one to have any effect. (default: 0)
"hunger" : true, //If true, activated mutation increases hunger by cost. (default: false)
"thirst" : true, //If true, activated mutation increases thirst by cost. (default: false)
"fatigue" : true, //If true, activated mutation increases fatigue by cost. (default: false)
"scent_modifier": 0.0,// float affecting the intensity of your smell. (default: 1.0)
"scent_intensity": 800,// int affecting the target scent toward which you current smell gravitates. (default: 500)
"scent_mask": -200,// int added to your target scent value. (default: 0)
"scent_type": "sc_flower",// scent_typeid, defined in scent_types.json, The type scent you emit. (default: empty)
"consume_time_modifier": 1.0f,//time to eat or drink is multiplied by this
"bleed_resist": 1000, // Int quantifiying your resistance to bleed effect, if its > to the intensity of the effect you don't get any bleeding. (default: 0)
"fat_to_max_hp": 1.0, // Amount of hp_max gained for each unit of bmi above character_weight_category::normal. (default: 0.0)
"healthy_rate": 0.0, // How fast your health can change. If set to 0 it never changes. (default: 1.0)
"weakness_to_water": 5, // How much damage water does to you, negative values heal you. (default: 0)
"ignored_by": [ "ZOMBIE" ], // List of species ignoring you. (default: empty)
"anger_relations": [ [ "MARSHMALLOW", 20 ], [ "GUMMY", 5 ], [ "CHEWGUM", 20 ] ], // List of species angered by you and by how much, can use negative value to calm.  (default: empty)
"can_only_eat": [ "junk" ], // List of materiel required for food to be comestible for you. (default: empty)
"can_only_heal_with": [ "bandage" ], // List of med you are restricted to, this includes mutagen,serum,aspirin,bandages etc... (default: empty)
"can_heal_with": [ "caramel_ointement" ], // List of med that will work for you but not for anyone. See `CANT_HEAL_EVERYONE` flag for items. (default: empty)
"allowed_category": [ "ALPHA" ], // List of category you can mutate into. (default: empty)
"no_cbm_on_bp": [ "torso", "head", "eyes", "mouth", "arm_l" ], // List of body parts that can't receive cbms. (default: empty)
"lumination": [ [ "head", 20 ], [ "arm_l", 10 ] ], // List of glowing bodypart and the intensity of the glow as a float. (default: empty)
"metabolism_modifier": 0.333, // Extra metabolism rate multiplier. 1.0 doubles usage, -0.5 halves.
"fatigue_modifier": 0.5, // Extra fatigue rate multiplier. 1.0 doubles usage, -0.5 halves.
"fatigue_regen_modifier": 0.333, // Modifier for the rate at which fatigue and sleep deprivation drops when resting.
"healing_awake": 1.0, // Healing rate per turn while awake.
"healing_resting": 0.5, // Healing rate per turn while resting.
"mending_modifier": 1.2 // Multiplier on how fast your limbs mend - This value would make your limbs mend 20% faster
"transform": { "target": "BIOLUM1", // Trait_id of the mutation this one will transfomr into
               "msg_transform": "You turn your photophore OFF.", // message displayed upon transformation
               "active": false , // Will the target mutation start powered ( turn ON ).
               "moves": 100 // how many moves this costs. (default: 0)
},
"triggers": [ // List of sublist of triggers, all sublists must be True for the mutation to activate
  [ // Sublist of trigger: at least one trigger must be true for the sublist to be true
    {
      "trigger_type": "MOOD", // What variable is tracked by this trigger
      "threshold_high": -50, // Is True if the value is below threshold_high
      "msg_on": { "text": "Everything is terrible and this makes you so ANGRY!", "rating": "mixed" } // message displayed when the trigger activates
    }
  ],
  [
    {
      "trigger_type": "TIME", // What variable is tracked by this trigger
      "threshold_low": 20, // Is True if the value is above threshold_low
      "threshold_high": 2, // Is True if the value is below threshold_high
      "msg_on": { "text": "Everything is terrible and this makes you so ANGRY!", "rating": "mixed" } // message displayed when the trigger activates
      "msg_off": { "text": "Your glow fades." } // message displayed when the trigger deactivates the trait
    }
  ]
],
"enchantments": [ "ench_id_1" ],   // List of IDs of enchantments granted by this mutation
"temperature_speed_modifier": 0.5, // If nonzero, become slower when cold, and faster when hot
                                   // 1.0 gives +/-1% speed for each degree above or below 65F
"mana_modifier": 100               // Positive or negative change to total mana pool

```
	**Triggers:**
		| trigger_type  | Description
		|---            |---
		| MOOD          | Trigger depends on the mood value.
		| MOON          | Trigger depends on the phase of the moon. MOON_NEW =0, WAXING_CRESCENT =1, HALF_MOON_WAXING =2, WAXING_GIBBOUS =3, FULL =4, WANING_GIBBOUS =5, HALF_MOON_WANING =6, WANING_CRESCENT =7
		| HUNGER        | Trigger depends on the hunger value. Very Hungry ~= 110
		| THIRST        | Trigger depends on the thirst value.
		| PAIN          | Trigger depends on the pain value.
		| STAMINA       | Trigger depends on the stamina value.
		| TIME          | Trigger depends on the time of the day. [ 1am = 1, Midnight = 24 ]

### Traps

```C++
    "type": "trap",
    "id": "tr_beartrap",  // Unique ID
    "name": "bear trap",  // In-game name displayed
    "color": "blue",
    "symbol": "^",
    "visibility": 2,  // 1 to ??, affects detection
    "avoidance": 7,  // 0 to ??, affects avoidance
    "difficulty": 3,  // 0 to ??, difficulty of assembly & disassembly
    "trap_radius": 1,  // 0 to ??, trap radius
    "action": "blade",
    "map_regen": "microlab_shifting_hall",  // a valid overmap id, for map_regen action traps
    "benign": true,
    "always_invisible": true,
    "funnel_radius": 200,  // milimiters?
    "comfort": 4,
    "floor_bedding_warmth": -500,
    "spell_data": { "id": "bear_trap" },   // data required for trapfunc::spell()
    "trigger_weight": "200 g",  // If an item with this weight or more is thrown onto the trap, it triggers. TODO: what is the default?
    "drops": [ "beartrap" ],  // For disassembly?
    "flags": [ "EXAMPLE_FLAG" ], // A set of valid flags for this trap
    "vehicle_data": {
      "damage": 300,
      "sound_volume": 8,
      "sound": "SNAP!",
      "sound_type": "trap",
      "sound_variant": "bear_trap",
      "remove_trap": true,
      "spawn_items": [ "beartrap" ]
    }
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
"id": "wheel",                // Unique identifier
"name": "wheel",              // Displayed name
"symbol": "0",                // (Optional) ASCII character displayed when part is working
"symbols": {                  // (Optional) ASCII characters displayed when the part is working,
  "left": "0", "right": "0"   // listed by variant suffix.  See below for more on variants
"standard_symbols: false,     // (Optional) Use the standard ASCII characters for variants
                              // must have one of symbol, symbols, or standard_symbols
"looks_like": "small_wheel",  // (Optional) hint to tilesets if this part has no tile,
                              // use the looks_like tile.
"color": "dark_gray",         // Color used when part is working
"broken_symbol": "x",         // ASCII character displayed when part is broken
"broken_color": "light_gray", // Color used when part is broken
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
"requirements": {             // (Optional) Special installation, removal, or repair requirementsi
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
"damage_reduction" : {        // Flat reduction of damage, as described below. If not specified, set to zero
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
```

#### Symbols and Variants
Vehicle parts can have multiple identical variants that use different symbols (and potentially
tileset sprites).  They are declared by the `"standard_symbols"` boolean or the "symbols" object.
Variants are used in the vehicle prototype as a suffix following the part id (ie `id_variant`), such as `"frame_nw"` or `"halfboard_cover"`.

setting `"standard_symbols"` to true gives the vehicle the following variants:
```
"cover": "^", "cross": "c", "horizontal": "h", "horizontal_2": "=", "vertical": "j",
"vertical_2": "H", "ne": "u", "nw": "y", "se": "n", "sw": "b"
```

Otherwise, variants can use any of the following suffices:
```
"cover", "cross", "cross_unconnected", "front", "rear", "left," "right",
"horizontal",  "horizontal_front", "horizontal_front_edge",
"horizontal_rear", "horizontal_rear_edge",
"horizontal_2", "horizontal_2_front", "horizontal_2_rear",
"vertical", "vertical_right", "vertical_left", "vertical_T_right", "vertical_T_left",
"vertical_2", "vertical_2_right", "vertical_2_left",
"ne", "nw", "se", "sw", "ne_edge", "nw_edge", "se_edge", "sw_edge"
```

Unless specified as optional, the following fields are mandatory for parts with appropriate flag and are ignored otherwise.
#### The following optional fields are specific to CARGO or FLUIDTANK parts.
```c++
"size": 2000,                 // with flag "FLUIDTANK" this is capacity in mLs,
                              // else with "CARGO" flag the capacity in 250mL volume units.
"cargo_weight_modifier": 33,  // (Optional, default = 100) Multiplies cargo weight by this percentage.
```

#### The following optional fields are specific to ENGINEs.
```c++
"power": 15000                // Engine motive power in watts.
"energy_consumption": 17500   // Engine power consumption at maximum power in watts.  Defaults to
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
"wheel_type": "standard",     // Must be one of "standard", "rigid", "racing", "off_road", "treads", or "rail".
                              // Indicates the class of wheel for determining off-road performance.
"contact_area": 153,          // The surface area of the wheel in contact with the ground under
                              // normal conditions in cm^2.  Wheels with higher contact area
                              // perform better off-road.
"rolling_resistance": 1.0,    // The "squishiness" of the wheel, per SAE standards.  Wheel rolling
                              // resistance increases vehicle drag linearly as vehicle weight
                              // and speed increase.
```
The following `wheel_types` are available:
* `standard`: typical car wheel with some grooves, intended primarily for road use.  Large penalty when not on a FLAT tile, small penalty when not on a ROAD tile.
* `rigid`: hard roller wheel like a caster that only performs well on smooth, flat surface.  Massive penalty when not on a FLAT tile, moderate penalty when not on a ROAD tile.
* `racing`: a smooth, ungrooved tile for maximum traction under optimum conditions.  Very large penalty when not on a FLAT tile, small penalty when not on a ROAD tile.
* `off_road`: a knobbed, heavily grooved tire for maximum traction under a wide variety of conditions.  Moderate penalty when not on a FLAT tile, tiny penalty when not a ROAD tile.
* `treads`: a link in a continuous track.  moderate penalty when not on a FLAT tile, no penalty when not on a ROAD tile.
* `rail`: a rigid metal wheel with a flange on one edge, meant to keep it on a railroad track.  No penalty when on a RAIL tile, extreme penalty when not on a RAIL tile.

#### The following optional fields are specific to ROTORs.
```c++
"rotor_diameter": 15,         // Rotor diamater in meters.  Larger rotors provide more lift.
```

#### The following optional fields are specific to WORKBENCHes.
These values apply to crafting tasks performed at the WORKBENCH.
```c++
"multiplier": 1.1,            // Crafting speed multipler.
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
  "status" : 1  // The status of the new vehicles. -1 = light damage (default), 0 = undamaged, 1 = disabled, destroyed tires OR engine.
} } ]
```

### Vehicles
See also VEHICLE_JSON.md

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

# `data/json/items/` JSONs

### Generic Items

```C++
"type": "GENERIC",                // Defines this as some generic item
"id": "socks",                    // Unique ID. Must be one continuous word, use underscores if necessary
"name": {
    "ctxt": "clothing",           // Optional translation context. Useful when a string has multiple meanings that need to be translated differently in other languages.
    "str": "pair of socks",       // The name appearing in the examine box.  Can be more than one word separated by spaces
    "str_pl": "pairs of socks"    // Optional. If a name has an irregular plural form (i.e. cannot be formed by simply appending "s" to the singular form), then this should be specified. "str_sp" can be used if the singular and plural forms are the same
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
"material": ["COTTON"],                      // Material types, can be as many as you want.  See materials.json for possible options
"cutting": 0,                                // (Optional, default = 0) Cutting damage caused by using it as a melee weapon.  This value cannot be negative.
"bashing": 0,                                // (Optional, default = 0) Bashing damage caused by using it as a melee weapon.  This value cannot be negative.
"to_hit": 0,                                 // (Optional, deprecated, default = 0) To-hit bonus if using it as a melee weapon (whatever for?).  The object version is preferred
"to_hit" {                                   // (Optional, Preferred) To hit bonus values, see below
  "grip": "solid",                           // the item's grip value
  "length": "long",                          // the item's length value
  "surface": "point",                        // the item's striking surface value
  "balance": "neutral"                       // the item's balance value
}
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
See `GAME_BALANCE.md`'s `MELEE_WEAPONS` section for the criteria for selecting each value.

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
"dispersion" : 0,     // Inaccuracy of ammo, measured in quarter-degrees
"recoil" : 18,        // Recoil caused when firing
"count" : 25,         // Number of rounds that spawn together
"stack_size" : 50,    // (Optional) How many rounds are in the above-defined volume. If omitted, is the same as 'count'
"show_stats" : true,  // (Optional) Force stat display for combat ammo. (for projectiles lacking both damage and prop_damage)
"effects" : ["COOKOFF", "SHOT"]
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
"type" : "ARMOR",     // Defines this as armor
...                   // same entries as above for the generic item.
                      // additional some armor specific entries:
"covers" : [ "foot_l", "foot_r" ],  // Where it covers.  Use bodypart_id defined in body_parts.json
"warmth" : 10,        //  (Optional, default = 0) How much warmth clothing provides
"environmental_protection" : 0,  //  (Optional, default = 0) How much environmental protection it affords
"encumbrance" : 0,    // Base encumbrance (unfitted value)
"max_encumbrance" : 0,    // When a character is completely full of volume, the encumbrance of a non-rigid storage container will be set to this. Otherwise it'll be between the encumbrance and max_encumbrance following the equation: encumbrance + (max_encumbrance - encumbrance) * non-rigid volume / non-rigid capacity.  By default, max_encumbrance is encumbrance + (non-rigid volume / 250ml).
"weight_capacity_bonus": "20 kg",    // (Optional, default = 0) Bonus to weight carrying capacity, can be negative. Strings must be used - "5000 g" or "5 kg"
"weight_capacity_modifier": 1.5, // (Optional, default = 1) Factor modifying base weight carrying capacity.
"coverage" : 80,      // What percentage of body part
"material_thickness" : 1,  // Thickness of material, in millimeter units (approximately).  Ordinary clothes range from 0.1 to 0.5. Particularly rugged cloth may reach as high as 1-2mm, and armor or protective equipment can range as high as 10 or rarely more.
"power_armor" : false, // If this is a power armor item (those are special).
"valid_mods" : ["steel_padded"] // List of valid clothing mods. Note that if the clothing mod doesn't have "restricted" listed, this isn't needed.
```
Alternately, every item (book, tool, gun, even food) can be used as armor if it has armor_data:
```C++
"type" : "TOOL",      // Or any other item type
...                   // same entries as for the type (e.g. same entries as for any tool),
"armor_data" : {      // additionally the same armor data like above
    "covers" : [ "foot_l", "foot_r" ],
    "warmth" : 10,
    "environmental_protection" : 0,
    "encumbrance" : 0,
    "coverage" : 80,
    "material_thickness" : 1,
    "power_armor" : false
}
```
#### Guidelines for thickness: ####
According to <https://propercloth.com/reference/fabric-thickness-weight/>, dress shirts and similar fine clothing range from 0.15mm to 0.35mm.
According to <https://leathersupreme.com/leather-hide-thickness-in-leather-jackets/>:
* Fashion leather clothes such as thin leather jackets, skirts, and thin vests are 1.0mm or less.
* Heavy leather clothes such as motorcycle suits average 1.5mm.

From [this site](http://cci.one/site/marine/design-tips-fabrication-overview/tables-of-weights-and-measures/?doing_wp_cron=1611725817.5850219726562500000000), an equivalency guideline for fabric weight to mm:

| Cloth | oz/yd2 | g/m2 | Inches | mm |
| ----- | ------- | ----- | ------- | ---- |
|  Fiberglass (plain weave)  | 2.3 |  78  | 0.004 | 0.10 |
|  Fiberglass (plain weave)  | 6.0 | 203 | 0.007 | 0.17 |
| Kevlar (TM) (plain weave) | 5.0 | 170 | 0.010 | 0.25 |
| Carbon Fiber (plain weave) | 5.8 | 197 | 0.009 | 0.23 |
| Carbon Fiber (unidirectional) | 9.0 | 305 | 0.011 | 0.28 |

Chart cobbled together from several sources for more general materials:

|  Fabric  | oz/yd2 | Max g/m2 | Inches | mm to use |
| ------- | --------| ---------- | ------- | ---------- |
| Very light | 0-4 | 136 | 0.006-0.007 | 0.15 |
|   Light   | 4-7 | 237 |    0.008    |  0.2  |
|  Medium  | 7-11 | 373 | 0.009-0.011 | 0.25 |
|   Heavy   | 11-14 | 475 | 0.012-0.014 | 0.3 |

Shoe thicknesses are outlined at <https://secretcobbler.com/choosing-leather/>; TL;DR: upper 1.2 - 2.0mm, lining 0.8 - 1.2mm, for a total of 2.0 - 3.2mm.

For turnout gear, see <http://bolivar.mo.us/media/uploads/2014/09/2014-06-bid-fire-gear-packet.pdf>.


### Pet Armor
Pet armor can be defined like this:

```C++
"type" : "PET_ARMOR",     // Defines this as armor
...                   // same entries as above for the generic item.
                      // additional some armor specific entries:
"environmental_protection" : 0,  //  (Optional, default = 0) How much environmental protection it affords
"material_thickness" : 1,  // Thickness of material, in millimeter units (approximately).  Generally ranges between 1 - 5, more unusual armor types go up to 10 or more
"pet_bodytype":        // the body type of the pet that this monster will fit.  See MONSTERS.md
"max_pet_vol:          // the maximum volume of the pet that will fit into this armor. Volume in ml and L can be used - "50 ml" or "2 L".
"min_pet_vol:          // the minimum volume of the pet that will fit into this armor. Volume in ml and L can be used - "50 ml" or "2 L".
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
    }
  ]
}
```

You can list as many conditional names for a given item as you want. Each conditional name must consist of 3 elements:
1. The condition type:
    - `COMPONENT_ID` searches all the components of the item (and all of *their* components, and so on) for an item with the condition string in their ID. The ID only needs to *contain* the condition, not match it perfectly (though it is case sensitive). For example, supplying a condition `mutant` would match `mutant_meat`.
    - `FLAG` which checks if an item has the specified flag (exact match).
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
"addiction_type" : "crack", // Addiction type
"spoils_in" : 0,            // A time duration: how long a comestible is good for. 0 = no spoilage.
"use_action" : [ "CRACK" ],     // What effects a comestible has when used, see special definitions below
"stim" : 40,                // Stimulant effect
"fatigue_mod": 3,           // How much fatigue this comestible removes. (Negative values add fatigue)
"radiation": 8,             // How much radiation you get from this comestible.
"comestible_type" : "MED",  // Comestible type, used for inventory sorting
"quench" : 0,               // Thirst quenched
"healthy" : -2,             // Health effects (used for sickness chances)
"addiction_potential" : 80, // Ability to cause addictions
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
"parasites": 10,            // (Optional) Probability of becoming parasitised when eating
"contamination": [ { "disease": "bad_food", "probability": 5 } ],         // (Optional) List of diseases carried by this comestible and their associated probability. Values must be in the [0, 100] range.
"vitamins": [ [ "calcium", 5 ], [ "iron", 12 ] ],         // Vitamins provided by consuming a charge (portion) of this.  An integer percentage of ideal daily value average.  Vitamins array keys include the following: calcium, iron, vitA, vitB, vitC, mutant_toxin, bad_food, blood, and redcells.  Note that vitB is B12.
"material": [ "flesh", "wheat" ], // All materials (IDs) this food is made of
"primary_material": "meat",       // What the primary material ID is. Materials determine specific heat.
"rot_spawn": "MONSTERGROUP_NAME", // Monster group that spawns when food becomes rotten (used for egg hatching)
"rot_spawn_chance": 10,           // Percent chance of monstergroup spawn when food rots. Max 100.
"smoking_result": "dry_meat",     // Food that results from drying this food in a smoker
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
    "moves": 100,                     // Indicates the number of moves it takes to remove an item from this pocket, assuming best conditions.
    "rigid": false,                   // Default false. If true, this pocket's size is fixed, and does not expand when filled.  A glass jar would be rigid, while a plastic bag is not.
    "magazine_well": "0 ml",          // Amount of space you can put items in the pocket before it starts expanding.  Only works if rigid = false.
    "watertight": false,              // Default false. If true, can contain liquid.
    "airtight": false,                // Default false. If true, can contain gas.
    "holster": false,                 // Default false. If true, only one stack of items can be placed inside this pocket, or one item if that item is not count_by_charges.
    "open_container": false,          // Default false. If true, the contents of this pocket will spill if this item is placed into another item.
    "fire_protection": false,         // Default false. If true, the pocket protects the contained items from exploding if tossed into a fire.
    "ammo_restriction": { "ammotype": count }, // Restrict pocket to a given ammo type and count.  This overrides mandatory volume, weight, watertight and airtight to use the given ammo type instead.  A pocket can contain any number of unique ammo types each with different counts, and the container will only hold one type (as of now).  If this is left out, it will be empty.
    "flag_restriction": [ "FLAG1", "FLAG2" ],  // Items can only be placed into this pocket if they have a flag that matches one of these flags.
    "item_restriction": [ "item_id" ],         // Only these item IDs can be placed into this pocket. Overrides ammo and flag restrictions.

    "sealed_data": { "spoil_multiplier": 0.0 } // If a pocket has sealed_data, it will be sealed when the item spawns.  The sealed version of the pocket will override the unsealed version of the same datatype.
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
"material": ["iron", "wood"], // Material types.  See materials.json for possible options
"weight": 907,         // Weight, measured in grams
"volume": "1500 ml",   // Volume, volume in ml and L can be used - "50 ml" or "2 L"
"bashing": 12,         // Bashing damage caused by using it as a melee weapon
"cutting": 12,         // Cutting damage caused by using it as a melee weapon
"flags" : ["CHOP"],    // Indicates special effects
"to_hit": 1            // To-hit bonus if using it as a melee weapon
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
"dispersion": 32,          // Inaccuracy of gun, measured in quarter-degrees
// When sight_dispersion and aim_speed are present in a gun mod, the aiming system picks the "best"
// sight to use for each aim action, which is the fastest sight with a dispersion under the current
// aim threshold.
"sight_dispersion": 10,    // Inaccuracy of gun derived from the sight mechanism, also in quarter-degrees
"recoil": 0,               // Recoil caused when firing, in quarter-degrees of dispersion.
"durability": 8,           // Resistance to damage/rusting, also determines misfire chance
"blackpowder_tolerance": 8,// One in X chance to get clogged up (per shot) when firing blackpowder ammunition (higher is better). Optional, default is 8.
"min_cycle_recoil": 0,     // Minimum ammo recoil for gun to be able to fire more than once per attack.
"burst": 5,                // Number of shots fired in burst mode
"clip_size": 100,          // Maximum amount of ammo that can be loaded
"ups_charges": 0,          // Additionally to the normal ammo (if any), a gun can require some charges from an UPS. This also works on mods. Attaching a mod with ups_charges will add/increase ups drain on the weapon.
"ammo_to_fire" 1,          // Amount of ammo used
"reload": 450,             // Amount of time to reload, 100 = 1 second = 1 "turn"
"built_in_mods": ["m203"], //An array of mods that will be integrated in the weapon using the IRREMOVABLE tag.
"default_mods": ["m203"]   //An array of mods that will be added to a weapon on spawn.
"barrel_volume": "30 mL",  // Amount of volume lost when the barrel is sawn. Approximately 250 ml per inch is a decent approximation.
"valid_mod_locations": [ [ "accessories", 4 ], [ "grip", 1 ] ],  // The valid locations for gunmods and the mount of slots for that location.
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
"burst_modifier": 3,           // Optional field increasing or decreasing base gun burst size
"damage_modifier": -1,         // Optional field increasing or decreasing base gun damage
"dispersion_modifier": 15,     // Optional field increasing or decreasing base gun dispersion
"loudness_modifier": 4,        // Optional field increasing or decreasing base guns loudness
"range_modifier": 2,           // Optional field increasing or decreasing base gun range
"recoil_modifier": -100,       // Optional field increasing or decreasing base gun recoil
"ups_charges_modifier": 200,   // Optional field increasing or decreasing base gun UPS consumption (per shot) by adding given value
"ups_charges_multiplier": 2.5, // Optional field increasing or decreasing base gun UPS consumption (per shot) by multiplying by given value
"reload_modifier": -10,        // Optional field increasing or decreasing base gun reload time in percent
"min_str_required_mod": 14,    // Optional field increasing or decreasing minimum strength required to use gun
"aim_speed": 3,                // A measure of how quickly the player can aim, in moves per point of dispersion.
"ammo_effects": [ "BEANBAG" ], // List of IDs of ammo_effect types
"consume_chance": 5000,        // Odds against CONSUMABLE mod being destroyed when gun is fired (default 1 in 10000)
"consume_divisor": 10,         // Divide damage against mod by this amount (default 1)
"handling_modifier": 4,        // Improve gun handling. For example a forward grip might have 6, a bipod 18
"mode_modifier": [ [ "AUTO", "auto", 4 ] ], // Modify firing modes of the gun, to give AUTO or REACH for example
```

Alternately, every item (book, tool, armor, even food) can be used as a gunmod if it has gunmod_data:
```
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
"material": [ "wood" ],   // Material types.  See materials.json for possible options
"techniques": [ "FLAMING" ], // Combat techniques used by this tool
"flags": [ "FIRE" ],      // Indicates special effects
"weight": 831,        // Weight, measured in grams
"volume": "1500 ml",  // Volume, volume in ml and L can be used - "50 ml" or "2 L"
"bashing": 12,        // Bashing damage caused by using it as a melee weapon
"cutting": 0,         // Cutting damage caused by using it as a melee weapon
"to_hit": 3,          // To-hit bonus if using it as a melee weapon
"turns_per_charge": 20, // Charges consumed over time, deprecated in favor of power_draw
"use_action": [ "firestarter" ] // Action performed when tool is used, see special definition below
// Only TOOL type items may define the following fields:
"ammo": [ "NULL" ],        // Ammo types used for reloading
"charge_factor": 5,        // this tool uses charge_factor charges for every charge required in a recipe; intended for tools that have a "sub" field but use a different ammo that the original tool
"charges_per_use": 1,      // Charges consumed per tool use
"initial_charges": 75,     // Charges when spawned
"max_charges": 75,         // Maximum charges tool can hold
"rand_charges: [10, 15, 25], // Randomize the charges when spawned. This example has a 50% chance of rng(10, 15) charges and a 50% chance of rng(15, 25) (The endpoints are included)
"power_draw": 50,          // Energy consumption rate in mW
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
    "msg": "You turn the lamp on.", // Message to display when activated.
    "need_fire": 1,                 // Whether fire is needed to activate.
    "need_fire_msg": "You need a lighter!", // Message to display if there is no fire.
    "need_charges": 1,                      // Number of charges the item needs to transform.
    "need_charges_msg": "The lamp is empty.", // Message to display if there aren't enough charges.
    "need_worn": true;                        // Whether the item must be worn to be transformed; false by default.
    "need_wielding": true;                    // Whether the item must be wielded to be transformed; false by default.
    "target_charges" : 3, // Number of charges the transformed item has.
    "rand_target_charges: [10, 15, 25], // Randomize the charges the transformed item has. This example has a 50% chance of rng(10, 15) charges and a 50% chance of rng(15, 25) (The endpoints are included)
    "container" : "jar",  // Container holding the target item.
    "moves" : 500         // Moves required to transform the item in excess of a normal action.
},
"use_action": {
    "type": "explosion", // An item that explodes when it runs out of charges.
    "sound_volume": 0, // Volume of a sound the item makes every turn.
    "sound_msg": "Tick.", // Message describing sound the item makes every turn.
    "no_deactivate_msg": "You've already pulled the %s's pin, try throwing it instead.", // Message to display if the player tries to activate the item, prevents activation from succeeding if defined.
    "explosion": { // Optional: physical explosion data
        // Specified like `"explosion"` field in generic items
    },
    "draw_explosion_radius" : 5, // How large to draw the radius of the explosion.
    "draw_explosion_color" : "ltblue", // The color to use when drawing the explosion.
    "do_flashbang" : true, // Whether to do the flashbang effect.
    "flashbang_player_immune" : true, // Whether the player is immune to the flashbang effect.
    "fields_radius": 3, // The radius of spread for fields produced.
    "fields_type": "fd_tear_gas", // The type of fields produced.
    "fields_min_density": 3,
    "fields_max_density": 3,
    "emp_blast_radius": 4,
    "scrambler_blast_radius": 4
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
"use_action": {
    "type": "unfold_vehicle", // Transforms the item into a vehicle.
    "vehicle_name": "bicycle", // Vehicle name to create.
    "unfold_msg": "You painstakingly unfold the bicycle and make it ready to ride.", // Message to display when transforming.
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
"use_action" : {
    "type" : "delayed_transform", // Like transform, but it will only transform when the item has a certain age
    "transform_age" : 600, // The minimal age of the item. Items that are younger wont transform. In turns (60 turns = 1 minute)
    "not_ready_msg" : "The yeast has not been done The yeast isn't done culturing yet." // A message, shown when the item is not old enough
},
"use_action": {
    "type": "firestarter", // Start a fire, like with a lighter.
    "moves_cost": 15 // Number of moves it takes to start the fire.
},
"use_action": {
    "type": "unpack", // unpack this item
    "group": "gobag_contents", // itemgroup this unpacks into
    "items_fit": true, // Do the armor items in this fit? Defaults to false.
    "filthy_volume_threshold": "10 L" // If the items unpacked from this item have volume, and this item is filthy, at what amount of held volume should they become filthy
},
"use_action": {
    "type": "extended_firestarter", // Start a fire (like with magnifying glasses or a fire drill). This action can take many turns, not just some moves like firestarter, it can also be canceled (firestarter can't).
    "need_sunlight": true // Whether the character needs to be in direct sunlight, e.g. to use magnifying glasses.
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
    "type": "cauterize", // Cauterize the character.
    "flame": true // If true, the character needs 4 charges of fire (e.g. from a lighter) to do this action, if false, the charges of the item itself are used.
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
    "long_action" : true,   // Is using this item a long action. Setting this to true will divide move cost by (first aid skill + 1).
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
```

###random Descriptions

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
        // entries can also bo of this form to have a id to reference that specific snippet.
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

```C++
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

`type` value should be a string with the associated body part the item comes from.
    Acceptable values are as follows:
    `flesh`: the "meat" of the creature.
    `offal`: the "organs" of the creature. these are removed when field dressing.
    `skin`: the "skin" of the creature. this is what is ruined while quartering.
    `bone`: the "bones" of the creature. you will get some amount of these from field dressing, and the rest of them from butchering the carcass.
    `bionic`: an item gained by dissecting the creature. not restricted to CBMs.
    `bionic_group`: an item group that will give an item by dissecting a creature. not restricted to groups containing CBMs.

`flags` value should be an array of strings.  It's the flags that will be added to te items of that entry upon harvesting.

`faults` value should be an array of `fault_id` strings.  It's the faults that will be added to te items of that entry upon harvesting.

For every `type` other then `bionic` and `bionic_group` following entries scale the results:
    `base_num` value should be an array with two elements in which the first defines the minimum number of the corresponding item produced and the second defines the maximum number.
    `scale_num` value should be an array with two elements, increasing the minimum and maximum drop numbers respectively by element value * survival skill.
    `max` upper limit after `bas_num` and `scale_num` are calculated using
    `mass_ratio` value is a multiplier of how much of the monster's weight comprises the associated item. to conserve mass, keep between 0 and 1 combined with all drops. This overrides `base_num`, `scale_num` and `max`


For `type`s: `bionic` and `bionic_group` following enrties can scale the results:
    `max` this value (in contrary to `max` for other `type`s) corresponds to maximum butchery roll that will be passed to check_butcher_cbm() in activity_handlers.cpp; view check_butcher_cbm() to see corresponding distribution chances for roll values passed to that function

### leftovers

itype_id of the item dropped as leftovers after butchery or when the monster is gibbed.  Default as "ruined_chunks".

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
    "light_emitted": 5,
    "required_str": 18,
    "flags": [ "TRANSPARENT", "BASHABLE", "FLAMMABLE_HARD" ],
    "crafting_pseudo_item": "anvil",
    "examine_action": "toilet",
    "close": "f_foo_closed",
    "open": "f_foo_open",
    "bash": "TODO",
    "deconstruct": "TODO",
    "max_volume": "1000 L",
    "examine_action": "workbench",
    "workbench": { "multiplier": 1.1, "mass": 10000, "volume": "50L" }
}
```

#### `type`

Fixed string, must be `furniture` to identify the JSON object as such.

`"id", "name", "symbol", "looks_like", "color", "bgcolor", "max_volume", "open", "close", "bash", "deconstruct", "examine_action", "flags"`

Same as for terrain, see below in the chapter "Common to furniture and terrain".

#### `move_cost_mod`

Movement cost modifier (`-10` = impassable, `0` = no change). This is added to the movecost of the underlying terrain.


#### `light_emitted`

How much light the furniture produces.  10 will light the tile it's on brightly, 15 will light that tile and the tiles around it brightly, as well as slightly lighting the tiles two tiles away from the source.
For examples: An overhead light is 120, a utility light, 240, and a console, 10.

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
    "connects_to" : "WALL",
    "close": "t_foo_closed",
    "open": "t_foo_open",
    "bash": "TODO",
    "deconstruct": "TODO",
    "harvestable": "blueberries",
    "transforms_into": "t_tree_harvested",
    "harvest_season": "WINTER",
    "roof": "t_roof",
    "examine_action": "pit"
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

#### `trap`

(Optional) Id of the build-in trap of that terrain.

For example the terrain `t_pit` has the built-in trap `tr_pit`. Every tile in the game that has the terrain `t_pit` also has, therefore, an implicit trap `tr_pit` on it. The two are inseparable (the player can not deactivate the built-in trap, and changing the terrain will also deactivate the built-in trap).

A built-in trap prevents adding any other trap explicitly (by the player and through mapgen).

#### `transforms_into`

(Optional) Used for various transformation of the terrain. If defined, it must be a valid terrain id. Used for example:

- When harvesting fruits (to change into the harvested form of the terrain).
- In combination with the `HARVESTED` flag and `harvest_season` to change the harvested terrain back into a terrain with fruits.

#### `harvest_by_season`

(Optional) Array of objects containing the seasons in which to harvest and the id of the harvest entry used.

Exemple:
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

(Optional) Various additional flags, see "doc/JSON_FLAGS.md".

#### `connects_to`

(Optional) The group of terrains to which this terrain connects. This affects tile rotation and connections, and the ASCII symbol drawn by terrain with the flag "AUTO_WALL_SYMBOL".

Current values:
- `WALL`
- `CHAINFENCE`
- `WOODFENCE`
- `RAILING`
- `WATER`
- `POOLWATER`
- `PAVEMENT`
- `RAIL`



Example: `-` , `|` , `X` and `Y` are terrain which share the same `connects_to` value. `O` does not have it. `X` and `Y` also have the `AUTO_WALL_SYMBOL` flag. `X` will be drawn as a T-intersection (connected to west, south and east), `Y` will be drawn as a horizontal line (going from west to east, no connection to south).

```
-X-    -Y-
 |      O
```

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

#### `max_volume`

(Optional) Maximal volume that can be used to store items here. Volume in ml and L can be used - "50 ml" or "2 L"

#### `examine_action`

(Optional) The json function that is called when the object is examined. See "src/iexamine.h".

#### `close" And "open`

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

#### `str_min`, `str_max`, `str_min_blocked`, `str_max_blocked`, `str_min_supported`, `str_max_supported`

TODO

#### `sound`, `sound_fail`, `sound_vol`, `sound_fail_vol`
(Optional) Sound and volume of the sound that appears upon destroying the bashed object or upon unsuccessfully bashing it (failing). The sound strings are translated (and displayed to the player).

#### `furn_set`, `ter_set`

The terrain / furniture that will be set when the original is destroyed. This is mandatory for bash entries in terrain, but optional for entries in furniture (it defaults to no furniture).

#### `explosive`
(Optional) If greater than 0, destroying the object causes an explosion with this strength (see `game::explosion`).

#### `destroy_only`
TODO

#### `bash_below`
TODO

#### `tent_centers`, `collapse_radius`
(Optional) For furniture that is part of tents, this defines the id of the center part, which will be destroyed as well when other parts of the tent get bashed. The center is searched for in the given "collapse_radius" radius, it should match the size of the tent.

#### `items`

(Optional) An item group (inline) or an id of an item group, see doc/ITEM_SPAWN.md. The default subtype is "collection". Upon successful bashing, items from that group will be spawned.

#### `map_deconstruct_info`

```C++
{
    "furn_set": "f_safe",
    "ter_set": "t_dirt",
    "items": "deconstructed_item_result_group"
}
```

#### `furn_set`, `ter_set`

The terrain / furniture that will be set after the original has been deconstructed. "furn_set" is optional (it defaults to no furniture), "ter_set" is only used upon "deconstruct" entries in terrain and is mandatory there.

### `items`

(Optional) An item group (inline) or an id of an item group, see doc/ITEM_SPAWN.md. The default subtype is "collection". Upon deconstruction the object, items from that group will be spawned.

### `plant_data`

```JSON
{
  "transform": "f_planter_harvest",
  "base": "f_planter",
  "growth_multiplier": 1.2,
  "harvest_multiplier": 0.8
}
```

#### `transform`

What the `PLANT` furniture turn into when it grows a stage, or what a `PLANTABLE` furniture turns into when it is planted on.

#### `emissions`

(Optional) An array listing the `emit_id` of the fields the terrain/furniture will produce every 10 seconds.

#### `base`

What the 'base' furniture of the `PLANT` furniture is - what it would be if there was not a plant growing there. Used when monsters 'eat' the plant to preserve what furniture it is.

#### `growth_multiplier`

A flat multiplier on the growth speed on the plant. For numbers greater than one, it will take longer to grow, and for numbers less than one it will take less time to grow.

#### `harvest_multiplier`

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
        "proportion": [   // (optional) value of effect propotions to clothing's parameter.
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

## `traits", "forced_traits", "forbidden_traits`
(optional, array of strings)

Lists of trait/mutation ids. Traits in "forbidden_traits" are forbidden and can't be selected during the character creation. Traits in "forced_traits" are automatically added to character. Traits in "traits" enables them to be chosen, even if they are not starting traits.

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

## `missions`
(optional, array of strings)

A list of mission ids that will be started and assigned to the player at the start of the game. Only missions with the ORIGIN_GAME_START origin are allowed. The last mission in the list will be the active mission, if multiple missions are assigned.

## `custom_initial_date`
(optional, object with optional members "hour", "day", "season" and "year")

Allows customizing initial date. If not set - corresponding values from world options are used. Random value is used for each parameter that is not explicitly.

```C++
"custom_initial_date": { "hour": 3, "day": 10, "season": "winter", "year": 1 }
```

 Identifier            | Description
---                    | ---
`hour`                 | (optional, integer) Hour of the day for initial date
`day`                  | (optional, integer) Day of the season for initial date
`season`               | (optional, integer) Season for initial date
`year`                 | (optional, integer) Year for initial date

# Starting locations

Starting locations are specified as JSON object with "type" member set to "start_location":
```C++
{
    "type": "start_location",
    "id": "field",
    "name": "An empty field",
    "terrain": [ "field", { "om_terrain": "hospital", "om_terrain_match_type": "PREFIX" } ],
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

# Field types
```C++
  {
    "type": "field_type", // this is a field type
    "id": "fd_gum_web", // id of the field
    "immune_mtypes": [ "mon_spider_gum" ], // list of monster immune to this field
    "intensity_levels": [
      { "name": "shadow",  // name of this level of intensity
        "light_override": 3.7 } //light level on the tile occupied by this field will be set at 3.7 not matter the ambient light.
     ],
    "decrease_intensity_on_contact": true, // Decrease the field intensity by one each time a character walk on it.
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
