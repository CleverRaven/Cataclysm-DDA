# JSON INFO

Use the `Home` key to return to the top.

- [Introduction](#introduction)
- [Navigating the JSON](#navigating-the-json)
- [File descriptions](#file-descriptions)
  * [`data/json/`](#datajson)
  * [`data/json/items/`](#datajsonitems)
    + [`data/json/items/comestibles`](#datajsonitemscomestibles)
  * [`data/json/requirements/`](#datajsonrequirements)
  * [`data/json/vehicles/`](#datajsonvehicles)
- [Generic properties and formatting](#generic-properties-and-formatting)
  * [Generic properties](#generic-properties)
  * [Formatting](#formatting)
    + [Time duration](#time-duration)
    + [Other formatting](#other-formatting)
- [Description and content of each JSON file](#description-and-content-of-each-json-file)
  * [`data/json/` JSONs](#datajson-jsons)
    + [Ascii_arts](#ascii_arts)
    + [Body_parts](#body_parts)
    + [Bionics](#bionics)
    + [Dreams](#dreams)
    + [Disease](#disease_type)
    + [Item Groups](#item-groups)
    + [Item Category](#item-category)
    + [Materials](#materials)
    + [Monster Groups](#monster-groups)
      - [Group definition](#group-definition)
      - [Monster definition](#monster-definition)
    + [Monster Factions](#monster-factions)
    + [Monsters](#monsters)
    + [Names](#names)
    + [Profession item substitution](#profession-item-substitution)
      - [`description`](#-description-)
      - [`name`](#-name-)
      - [`points`](#-points-)
      - [`addictions`](#-addictions-)
      - [`skills`](#-skills-)
      - [`items`](#-items-)
      - [`pet`](#-pet-)
      - [`flags`](#-flags-)
      - [`cbms`](#-cbms-)
      - [`traits`](#-traits-)
    + [Recipes](#recipes)
    + [Constructions](#constructions)
    + [Scent Types](#scent_types)
    + [Scores and Achievements](#scores-and-achievements)
      - [`event_transformation`](#event_transformation)
      - [`event_statistic`](#event_statistic)
      - [`score`](#score)
      - [`achievement`](#achievement)
    + [Skills](#skills)
    + [Traits/Mutations](#traits-mutations)
    + [Vehicle Groups](#vehicle-groups)
    + [Vehicle Parts](#vehicle-parts)
    + [Part Resistance](#part-resistance)
    + [Vehicle Placement](#vehicle-placement)
    + [Vehicle Spawn](#vehicle-spawn)
    + [Vehicles](#vehicles)
- [`data/json/items/` JSONs](#datajsonitems-jsons)
    + [Generic Items](#generic-items)
    + [Ammo](#ammo)
    + [Magazine](#magazine)
    + [Armor](#armor)
    + [Pet Armor](#pet-armor)
    + [Books](#books)
      - [Color Key](#color-key)
    + [Comestibles](#comestibles)
    + [Containers](#containers)
    + [Melee](#melee)
    + [Gun](#gun)
    + [Gunmod](#gunmod)
    + [Batteries](#batteries)
    + [Tools](#tools)
    + [Seed Data](#seed-data)
    + [Artifact Data](#artifact-data)
    + [Brewing Data](#brewing-data)
      - [`Charge_type`](#charge-type)
      - [`Effects_carried`](#effects-carried)
      - [`effects_worn`](#effects-worn)
      - [`effects_wielded`](#effects-wielded)
      - [`effects_activated`](#effects-activated)
    + [Software Data](#software-data)
    + [Fuel data](#fuel-data)
    + [Use Actions](#use-actions)
- [`json/` JSONs](#json-jsons)
    + [Harvest](#harvest)
      - [`id`](#-id-)
      - [`type`](#-type-)
      - [`message`](#-message-)
      - [`entries`](#-entries-)
    + [Furniture](#furniture)
      - [`type`](#-type--1)
      - [`move_cost_mod`](#-move-cost-mod-)
      - [`light_emitted`](#-light-emitted-)
      - [`required_str`](#-required-str-)
      - [`crafting_pseudo_item`](#-crafting-pseudo-item-)
      - [`workbench`](#-workbench-)
      - [`plant_data`](#-plant-data-)
    + [Terrain](#terrain)
      - [`type`](#-type--2)
      - [`move_cost`](#-move-cost-)
      - [`light_emitted`](#-light-emitted--1)
      - [`trap`](#-trap-)
      - [`harvestable`](#-harvestable-)
      - [`transforms_into`](#-transforms-into-)
      - [`harvest_season`](#-harvest-season-)
      - [`roof`](#-roof-)
    + [Common To Furniture And Terrain](#common-to-furniture-and-terrain)
      - [`id`](#-id--1)
      - [`name`](#-name--1)
      - [`flags`](#-flags--1)
      - [`connects_to`](#-connects-to-)
      - [`symbol`](#-symbol-)
      - [`looks_like`](#-looks-like-)
      - [`color` or `bgcolor`](#-color--or--bgcolor-)
      - [`max_volume`](#-max-volume-)
      - [`examine_action`](#-examine-action-)
      - [`close" And "open`](#-close--and--open-)
      - [`bash`](#-bash-)
      - [`deconstruct`](#-deconstruct-)
      - [`map_bash_info`](#-map-bash-info-)
      - [`str_min`, `str_max`, `str_min_blocked`, `str_max_blocked`, `str_min_supported`, `str_max_supported`](#-str-min----str-max----str-min-blocked----str-max-blocked----str-min-supported----str-max-supported-)
      - [`sound`, `sound_fail`, `sound_vol`, `sound_fail_vol`](#-sound----sound-fail----sound-vol----sound-fail-vol-)
      - [`furn_set`, `ter_set`](#-furn-set----ter-set-)
      - [`explosive`](#-explosive-)
      - [`destroy_only`](#-destroy-only-)
      - [`bash_below`](#-bash-below-)
      - [`tent_centers`, `collapse_radius`](#-tent-centers----collapse-radius-)
      - [`items`](#-items--1)
      - [`map_deconstruct_info`](#-map-deconstruct-info-)
      - [`furn_set`, `ter_set`](#-furn-set----ter-set--1)
    + [`items`](#-items-2)
    + [`plant_data`](#plant_data-1)
      - [`transform`](#-transform-)
      - [`base`](#-base-)
      - [`growth_multiplier`](#-growth-multiplier-)
      - [`harvest_multiplier`](#-harvest-multiplier-)
    + [clothing_mod](#clothing_mod)
- [Scenarios](#scenarios)
  * [`description`](#-description--1)
  * [`name`](#-name--2)
  * [`points`](#-points--1)
  * [`items`](#-items--3)
  * [`flags`](#-flags--2)
  * [`cbms`](#-cbms--1)
  * [`traits", "forced_traits", "forbidden_traits`](#-traits----forced-traits----forbidden-traits-)
  * [`allowed_locs`](#-allowed-locs-)
  * [`start_name`](#-start-name-)
  * [`professions`](#-professions-)
  * [`map_special`](#-map-special-)
  * [`missions`](#-missions-)
- [Starting locations](#starting-locations)
  * [`name`](#-name--3)
  * [`target`](#-target-)
  * [`flags`](#-flags--3)
    + [`tile_config`](#-tile-config-)
- [Mutation overlay ordering](#mutation-overlay-ordering)
  * [`id`](#-id--2)
  * [`order`](#-order-)
- [MOD tileset](#mod-tileset)
  * [`compatibility`](#-compatibility-)
  * [`tiles-new`](#-tiles-new-)
- [Field types](#-field-types-)

# Introduction
This document describes the contents of the json files used in Cataclysm: Dark days ahead. You are probably reading this if you want to add or change content of Catacysm: Dark days ahead and need to learn more about what to find where and what each file and property does.

# Navigating the JSON
A lot of the JSON involves cross-references to other JSON entities.  To make it easier to navigate, we provide a script `tools/json_tools/cddatags.py` that can build a `tags` file for you.

To run the script you'll need Python 3.  On Windows you'll probably need to install that, and associate `.py` files with Python.  Then open a command prompt, navigate to your CDDA folder, and run `tools\json_tools\cddatags.py`.

To use this feature your editor will need [ctags support](http://ctags.sourceforge.net/).  When that's working you should be able to easily jump to the definition of any entity.  For example, by positioning your cursor over an id and hitting the appropriate key combination.

* In Vim, this feature exists by default, and you can jump to a definition using [`^]`](http://vimdoc.sourceforge.net/htmldoc/tagsrch.html#tagsrch.txt).
* In Notepad++ go to "Plugins" -> "Plugins Admin" and enable the "TagLEET" plugin.  Then select any id and press Alt+Space to open the references window.

# File descriptions
Here's a quick summary of what each of the JSON files contain, broken down by folder. This list is not comprehensive, but covers the broad strokes.

## `data/json/`

| Filename                    | Description
|---                          |---
| achievements.json           | achievements
| anatomy.json                | a listing of player body parts - do not edit
| ascii_arts.json             | ascii arts for item descriptions
| bionics.json                | bionics, does NOT include bionic effects
| body_parts.json             | an expansion of anatomy.json - do not edit
| clothing_mods.json          | definition of clothing mods
| construction.json           | definition of construction menu tasks
| default_blacklist.json      | a standard blacklist of joke monsters
| doll_speech.json            | talk doll speech messages
| dreams.json                 | dream text and linked mutation categories
| disease.json                | disease definitions
| effects.json                | common effects and their effects
| emit.json                   | smoke and gas emissions
| flags.json                  | common flags and their descriptions
| furniture.json              | furniture, and features treated like furniture
| game_balance.json           | various options to tweak game balance
| gates.json                  | gate terrain definitions
| harvest.json                | item drops for butchering corpses
| health_msgs.json            | messages displayed when the player wakes
| item_actions.json           | descriptions of standard item actions
| item_category.json          | item categories and their default sort
| item_groups.json            | item spawn groups
| lab_notes.json              | lab computer messages
| martialarts.json            | martial arts styles and buffs
| materials.json              | material types
| monster_attacks.json        | monster attacks
| monster_drops.json          | monster item drops on death
| monster_factions.json       | monster factions
| monstergroups.json          | monster spawn groups
| monstergroups_egg.json      | monster spawn groups from eggs
| monsters.json               | monster descriptions, mostly zombies
| morale_types.json           | morale modifier messages
| mutation_category.json      | messages for mutation categories
| mutation_ordering.json      | draw order for mutation and CBM overlays in tiles mode
| mutations.json              | traits/mutations
| names.json                  | names used for NPC/player name generation
| overmap_connections.json    | connections for roads and tunnels in the overmap
| overmap_terrain.json        | overmap terrain
| player_activities.json      | player activities
| professions.json            | profession definitions
| recipes.json                | crafting/disassembly recipes
| regional_map_settings.json  | settings for the entire map generation
| road_vehicles.json          | vehicle spawn information for roads
| rotatable_symbols.json      | rotatable symbols - do not edit
| scent_types.json            | type of scent available
| scores.json                 | scores
| skills.json                 | skill descriptions and ID's
| snippets.json               | flier/poster descriptions
| species.json                | monster species
| speech.json                 | monster vocalizations
| statistics.json             | statistics and transformations used to define scores and achievements
| start_locations.json        | starting locations for scenarios
| techniques.json             | generic for items and martial arts
| terrain.json                | terrain types and definitions
| test_regions.json           | test regions
| tips.json                   | tips of the day
| tool_qualities.json         | standard tool qualities and their actions
| traps.json                  | standard traps
| tutorial.json               | messages for the tutorial (that is out of date)
| vehicle_groups.json         | vehicle spawn groups
| vehicle_parts.json          | vehicle parts, does NOT affect flag effects
| vitamin.json                | vitamins and their deficiencies

selected subfolders

## `data/json/items/`

See below for specifics on the various items

| Filename           | Description
|---                 |---
| ammo.json          | common base components like batteries and marbles
| ammo_types.json    | standard ammo types by gun
| archery.json       | bows and arrows
| armor.json         | armor and clothing
| bionics.json       | Compact Bionic Modules (CBMs)
| biosignatures.json | animal waste
| books.json         | books
| chemicals_and_resources.json   | chemical precursors
| comestibles.json   | food/drinks
| containers.json    | containers
| crossbows.json     | crossbows and bolts
| fake.json          | fake items for bionics or mutations
| fuel.json          | liquid fuels
| grenades.json      | grenades and throwable explosives
| handloaded_bullets.json | random ammo
| melee.json         | anything that doesn't go in the other item jsons, melee weapons
| migration.json     | conversions of non-existent items from save games to current items
| newspaper.json     | flyers, newspapers, and survivor notes. snippets.json for messages
| obsolete.json      | items being removed from the game
| ranged.json        | guns
| software.json      | software for SD-cards and USB sticks
| tool_armor.json    | clothes and armor that can be (a)ctivated
| toolmod.json       | modifications of tools
| tools.json         | tools and items that can be (a)ctivated
| vehicle_parts.json | components of vehicles when they aren't on the vehicle

### `data/json/items/comestibles`

## `data/json/requirements/`

Standard components and tools for crafting

| Filename                     | Description
|---                           |---
| ammo.json                    | ammo components
| cooking_components.json      | common ingredient sets
| cooking_requirements.json    | cooking tools and heat sources
| materials.json               | thread, fabric, and other basic materials
| toolsets.json                | sets of tools commonly used together
| uncraft.json                 | common results of taking stuff apart
| vehicle.json                 | tools to work on vehicles


## `data/json/vehicles/`

Groups of vehicle definitions with self-explanatory names of files:

| Filename
|---
| bikes.json
| boats.json
| cars.json
| carts.json
| custom_vehicles.json
| emergency.json
| farm.json
| helicopters.json
| military.json
| trains.json
| trucks.json
| utility.json
| vans_busses.json
| vehicles.json

# Generic properties and formatting
This section describes properties and formatting applied to all of the JSON files.

## Generic properties
A few properties are applicable to most if not all json files and do not need to be described for each json file. These properties are:

| Identifier               | Description
|---                       |---
| type                     | The type of object this json entry is describing. Setting this entry to 'armor' for example means the game will expect properties specific to armor in that entry. Also ties in with 'copy-from' (see below), if you want to inherit properties of another object, it must be of the same tipe.
| [copy-from](https://github.com/CleverRaven/Cataclysm-DDA/tree/master/doc/JSON_INHERITANCE.md)                | The identifier of the item you wish to copy properties from. This allows you to make an exact copy of an item __of the same type__ and only provide entries that should change from the item you copied from.
| [extends](https://github.com/CleverRaven/Cataclysm-DDA/tree/master/doc/JSON_INHERITANCE.md)                  | Modders can add an "extends" field to their definition to append entries to a list instead of overriding the entire list.
| [delete](https://github.com/CleverRaven/Cataclysm-DDA/tree/master/doc/JSON_INHERITANCE.md)                   | Modders can also add a "delete" field that removes elements from lists instead of overriding the entire list.
| [abstract](https://github.com/CleverRaven/Cataclysm-DDA/tree/master/doc/JSON_INHERITANCE.md)                 | Creates an abstract item (an item that does not end up in the game and solely exists in the json to be copied-from. Use this _instead of_ 'id'.



## Formatting
When editing JSON files make sure you apply the correct formatting as shown below.

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

### Other formatting

```C++
"//" : "comment", // Preferred method of leaving comments inside json files.
```

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

Currently, only some JSON values support this syntax (see [here](https://github.com/CleverRaven/Cataclysm-DDA/blob/master/doc/TRANSLATING.md#translation) for a list of supported values and more detailed explanation).

# Description and content of each JSON file
This section describes each json file and their contents. Each json has their own unique properties that are not shared with other Json files (for example 'chapters' property used in books does not apply to armor). This will make sure properties are only described and used within the context of the appropriate JSON file.


## `data/json/` JSONs

### Ascii_arts

| Identifier        | Description
|---                |---
| id                | Unique ID. Must be one continuous word, use underscores if necessary.
| picture           | Array of string, each entry is a line of an ascii picture and must be at most 42 columns long.

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

### Body_parts

| Identifier        | Description
|---                |---
| id                | Unique ID. Must be one continuous word, use underscores if necessary.
| name              | In-game name displayed.
| accusative        | Accusative form for this bodypart.
| heading           | How it's displayed in headings.
| heading_multiple  | Plural form of heading.
| hp_bar_ui_text    | How it's displayed next to the hp bar in the panel.
| main_part         | What is the main part this one is attached to. (If this is a main part it's attached to itself)
| opposite_part     | What is the opposite part ot this one in case of a pair.
| hit_size          | Size of the body part when doing an unweighted selection.
| hit_size_relative | Hit sizes for attackers who are smaller, equal in size, and bigger.
| hit_difficulty    | How hard is it to hit a given body part, assuming "owner" is hit. Higher number means good hits will veer towards this part, lower means this part is unlikely to be hit by inaccurate attacks. Formula is `chance *= pow(hit_roll, hit_difficulty)`
| stylish_bonus     | Mood bonus associated with wearing fancy clothing on this part. (default: `0`)
| hot_morale_mod    | Mood effect of being too hot on this part. (default: `0`)
| cold_morale_mod   | Mood effect of being too cold on this part. (default: `0`)
| squeamish_penalty | Mood effect of wearing filthy clothing on this part. (default: `0`)
| bionic_slots      | How many bionic slots does this part have.

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
    "bionic_slots": 80
  }
```

### Bionics

| Identifier                  | Description
|---                          |---
| id                          | Unique ID. Must be one continuous word, use underscores if necessary.
| name                        | In-game name displayed.
| active                      | Whether the bionic is active or passive. (default: `passive`)
| power_source                | Whether the bionic provides power. (default: `false`)
| faulty                      | Whether it is a faulty type. (default: `false`)
| act_cost                    | How many kJ it costs to activate the bionic.  Strings can be used "1 kJ"/"1000 J"/"1000000 mJ" (default: `0`)
| deact_cost                  | How many kJ it costs to deactivate the bionic.  Strings can be used "1 kJ"/"1000 J"/"1000000 mJ" (default: `0`)
| react_cost                  | How many kJ it costs over time to keep this bionic active, does nothing without a non-zero "time".  Strings can be used "1 kJ"/"1000 J"/"1000000 mJ" (default: `0`)
| time                        | How long, when activated, between drawing cost. If 0, it draws power once. (default: `0`)
| description                 | In-game description.
| encumbrance                 | (_optional_) A list of body parts and how much this bionic encumber them.
| weight_capacity_bonus       | (_optional_) Bonus to weight carrying capacity in grams, can be negative.  Strings can be used - "5000 g" or "5 kg" (default: `0`)
| weight_capacity_modifier    | (_optional_) Factor modifying base weight carrying capacity. (default: `1`)
| canceled_mutations          | (_optional_) A list of mutations/traits that are removed when this bionic is installed (e.g. because it replaces the fault biological part).
| included_bionics            | (_optional_) Additional bionics that are installed automatically when this bionic is installed. This can be used to install several bionics from one CBM item, which is useful as each of those can be activated independently.
| included                    | (_optional_) Whether this bionic is included with another. If true this bionic does not require a CBM item to be defined. (default: `false`)
| env_protec                  | (_optional_) How much environmental protection does this bionic provide on the specified body parts.
| bash_protec                 | (_optional_) How much bash protection does this bionic provide on the specified body parts.
| cut_protec                  | (_optional_) How much cut protection does this bionic provide on the specified body parts.
| occupied_bodyparts          | (_optional_) A list of body parts occupied by this bionic, and the number of bionic slots it take on those parts.
| capacity                    | (_optional_) Amount of power storage added by this bionic.  Strings can be used "1 kJ"/"1000 J"/"1000000 mJ" (default: `0`)
| fuel_options                | (_optional_) A list of fuel that this bionic can use to produce bionic power.
| is_remote_fueled            | (_optional_) If true this bionic allows you to plug your power banks to an external power source (solar backpack, UPS, vehicle etc) via a cable. (default: `false`)
| fuel_capacity               | (_optional_) Volume of fuel this bionic can store.
| fuel_efficiency             | (_optional_) Fraction of fuel energy converted into power. (default: `0`)
| passive_fuel_efficiency     | (_optional_) Fraction of fuel energy passively converted into power. Useful for CBM using PERPETUAL fuel like `muscle`, `wind` or `sun_light`. (default: `0`)
| exothermic_power_gen        | (_optional_) If true this bionic emits heat when producing power. (default: `false`)
| coverage_power_gen_penalty  | (_optional_) Fraction of coverage diminishing fuel_efficiency. Float between 0.0 and 1.0. (default: `nullopt`)
| power_gen_emission          | (_optional_) `emit_id` of the field emitted by this bionic when it produces energy. Emit_ids are defined in `emit.json`.
| stat_bonus                  | (_optional_) List of passive stat bonus. Stat are designated as follow: "DEX", "INT", "STR", "PER".
| enchantments                | (_optional_) List of enchantments applied by this CBM (see MAGIC.md for instructions on enchantment. NB: enchantments are not necessarily magic.)

```C++
{
    "id"           : "bio_batteries",
    "name"         : "Battery System",
    "active"       : false,
    "power_source" : false,
    "faulty"       : false,
    "act_cost"     : 0,
    "time"         : 1,
    "fuel_efficiency": 1,
    "stat_bonus": [ [ "INT", 2 ], [ "STR", 2 ] ],
    "fuel_options": [ "battery" ],
    "fuel_capacity": 500,
    "encumbrance"  : [ [ "TORSO", 10 ], [ "ARM_L", 10 ], [ "ARM_R", 10 ], [ "LEG_L", 10 ], [ "LEG_R", 10 ], [ "FOOT_L", 10 ], [ "FOOT_R", 10 ] ],
    "description"  : "You have a battery draining attachment, and thus can make use of the energy contained in normal, everyday batteries. Use 'E' to consume batteries.",
    "canceled_mutations": ["HYPEROPIC"],
    "included_bionics": ["bio_blindfold"]
},
{
    "id": "bio_purifier",
    "type": "bionic",
    "name": "Air Filtration System",
    "description": "Surgically implanted in your trachea is an advanced filtration system.  If toxins, or airborne diseases find their way into your windpipe, the filter will attempt to remove them.",
    "occupied_bodyparts": [ [ "TORSO", 4 ], [ "MOUTH", 2 ] ],
    "env_protec": [ [ "mouth", 7 ] ],
    "bash_protec": [ [ "leg_l", 3 ], [ "leg_r", 3 ] ],
    "cut_protec": [ [ "leg_l", 3 ], [ "leg_r", 3 ] ],
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
| affected_bodyparts | The list of bodyparts on which the effect is applied. (optional, default to num_bp)


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
| `ident`          | Unique ID. Lowercase snake_case. Must be one continuous word, use underscores if necessary.
| `name`           | In-game name displayed.
| `bash_resist`    | How well a material resists bashing damage.
| `cut_resist`     | How well a material resists cutting damage.
| `acid_resist`    | Ability of a material to resist acid.
| `elec_resist`    | Ability of a material to resist electricity.
| `fire_resist`    | Ability of a material to resist fire.
| `chip_resist`    | Returns resistance to being damaged by attacks against the item itself.
| `bash_dmg_verb`  | Verb used when material takes bashing damage.
| `cut_dmg_verb`   | Verb used when material takes cutting damage.
| `dmg_adj`        | Description added to damaged item in ascending severity.
| `dmg_adj`        | Adjectives used to describe damage states of a material.
| `density`        | Density of a material.
| `vitamins`       | Vitamins in a material. Usually overridden by item specific values.
| `specific_heat_liquid` | Specific heat of a material when not frozen (J/(g K)). Default 4.186.
| `specific_heat_solid`  | Specific heat of a material when frozen (J/(g K)). Default 2.108.
| `latent_heat`    | Latent heat of fusion for a material (J/g). Default 334.
| `freeze_point`   | Freezing point of this material (F). Default 32 F ( 0 C ).
| `edible`   | Optional boolean. Default is false.
| `rotting`   | Optional boolean. Default is false.
| `soft`   | Optional boolean. Default is false.
| `reinforces`   | Optional boolean. Default is false.

There are six -resist parameters: acid, bash, chip, cut, elec, and fire. These are integer values; the default is 0 and they can be negative to take more damage.

```C++
{
    "type": "material",
    "ident": "hflesh",
    "name": "Human Flesh",
    "density": 5,
    "specific_heat_liquid": 3.7,
    "specific_heat_solid": 2.15,
    "latent_heat": 260,
    "edible": true,
    "rotting": true,
    "bash_resist": 1,
    "cut_resist": 1,
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
    "by_mood"      : ["blob"],
    "neutral"      : ["nether"],
    "friendly"     : ["blob"],
    "hate"         : ["fungus"]
}
```

### Monsters

See MONSTERS.md

### Names

```C++
{ "name" : "Aaliyah", "gender" : "female", "usage" : "given" }, // Name, gender, "given"/"family"/"city" (first/last/city name).
// NOTE: Please refrain from adding name PR's in order to maintain kickstarter exclusivity
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
```C++
This defines characters with the WOOLALLERGY trait get some items replaced:
- "blazer" is converted into "jacket_leather_red",
- each "hat_hunting" is converted into *two* "hat_cotton" items.

### Professions

Professions are specified as JSON object with "type" member set to "profession":

```C++
{
    "type": "profession",
    "ident": "hunter",
    ...
}
```

The ident member should be the unique id of the profession.

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

Mods can modify this list (requires `"edit-mode": "modify"`, see example) via "add:addictions" and "remove:addictions", removing requires only the addiction type. Example:
```C++
{
    "type": "profession",
    "ident": "hunter",
    "edit-mode": "modify",
    "remove:addictions": [
        "nicotine"
    ],
    "add:addictions": [
        { "type": "alcohol", "intensity": 10 }
    ]
}
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

Mods can modify this list (requires `"edit-mode": "modify"`, see example) via "add:skills" and "remove:skills", removing requires only the skill id. Example:
```C++
{
    "type": "profession",
    "ident": "hunter",
    "edit-mode": "modify",
    "remove:skills": [
        "archery"
    ],
    "add:skills": [
        { "name": "computer", "level": 2 }
    ]
}

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

Mods can modify the lists of existing professions. This requires the "edit-mode" member with value "modify" (see example). Adding items to the lists can be done with via "add:both" / "add:male" / "add:female". It allows the same content (it allows adding items with snippet ids). Removing items is done via "remove:both" / "remove:male" / "remove:female", which may only contain items ids.

Example for mods:

```C++
{
    "type": "profession",
    "ident": "hunter",
    "edit-mode": "modify",
    "items": {
        "remove:both": [
            "rock",
            "tshirt_text"
        ],
        "add:both": [ "2x4" ],
        "add:female": [
            ["tshirt_text", "allyourbase"]
        ]
    }
}
```

This mod removes one of the rocks (the other rock is still created), the t-shirt, adds a 2x4 item and gives female characters a t-shirt with the special snippet id.

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

Mods can modify this via `add:flags` and `remove:flags`.

#### `cbms`

(optional, array of strings)

A list of CBM ids that are implanted in the character.

Mods can modify this via `add:CBMs` and `remove:CBMs`.

#### `traits`

(optional, array of strings)

A list of trait/mutation ids that are applied to the character.

Mods can modify this via `add:traits` and `remove:traits`.

### Recipes

```C++
"result": "javelin",         // ID of resulting item
"category": "CC_WEAPON",     // Category of crafting recipe. CC_NONCRAFT used for disassembly recipes
"id_suffix": "",             // Optional (default: empty string). Some suffix to make the ident of the recipe unique. The ident of the recipe is "<id-of-result><id_suffix>".
"override": false,           // Optional (default: false). If false and the ident of the recipe is already used by another recipe, loading of recipes fails. If true and a recipe with the ident is already defined, the existing recipe is replaced by the new recipe.
"delete_flags": [ "CANNIBALISM" ], // Optional (default: empty list). Flags specified here will be removed from the resultant item upon crafting. This will override flag inheritance, but *will not* delete flags that are part of the item type itself.
"skill_used": "fabrication", // Skill trained and used for success checks
"skills_required": [["survival", 1], ["throw", 2]], // Skills required to unlock recipe
"book_learn": [              // (optional) Array of books that this recipe can be learned from. Each entry contains the id of the book and the skill level at which it can be learned.
    [ "textbook_anarch", 7, "something" ], // The optional third entry defines a name for the recipe as it should appear in the books description (default is the name of resulting item of the recipe)
    [ "textbook_gaswarfare", 8, "" ] // If the name is empty, the recipe is hidden, it will not be shown in the description of the book.
],
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
"batch_time_factors": [25, 15], // Optional factors for batch crafting time reduction. First number specifies maximum crafting time reduction as percentage, and the second number the minimal batch size to reach that number. In this example given batch size of 20 the last 6 crafts will take only 3750 time units.
"flags": [                   // A set of strings describing boolean features of the recipe
  "BLIND_EASY",
  "ANOTHERFLAG"
],
"construction_blueprint": "camp", // an optional string containing an update_mapgen_id.  Used by faction camps to upgrade their buildings
"on_display": false,         // this is a hidden construction item, used by faction camps to calculate construction times but not available to the player
"qualities": [               // Generic qualities of tools needed to craft
  {"id":"CUT","level":1,"amount":1}
],
"tools": [                   // Specific tools needed to craft
[
    [ "fire", -1 ]           // Charges consumed when tool is used, -1 means no charges are consumed
]],
"components": [              // Equivalent tools or components are surrounded by a single set of brackets
[
  [ "spear_wood", 1 ],       // Number of charges/items required
  [ "pointy_stick", 1 ]
],
[
  [ "rag", 1 ],
  [ "leather", 1 ],
  [ "fur", 1 ]
],
[
  [ "plant_fibre", 20, false ], // Optional flag for recoverability, default is true.
  [ "sinew", 20, false ],
  [ "thread", 20, false ],
  [ "duct_tape", 20 ]  // Certain items are flagged as unrecoverable at the item definition level.
]
]
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
case, the game may not be able to corectly predict whether it can be crafted.

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
"description": "Spike Pit",                                         // Description string displayed in the construction menu
"category": "DIG",                                                  // Construction category
"required_skills": [ [ "survival", 1 ] ],                           // Skill levels required to undertake construction
"time": "30 m",                                                         // Time required to complete construction. Integers will be read as minutes or a time string can be used.
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

### Scores and Achievements

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
    // "equals" can be used to specify a constant string value the field must take.
    // "equals_statistic" specifies that the value must match the value of some statistic (see below)
    "mount" : { "equals": "mon_horse" }
}
// Since we are filtering to only those events where 'mount' is 'mon_horse', we
// might as well drop the 'mount' field, since it provides no useful information.
"drop_fields" : [ "mount" ]
```

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

Another optional field is

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

### Skills

```C++
"ident" : "smg",  // Unique ID. Must be one continuous word, use underscores if necessary
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
"prereqs": ["SKIN_ROUGH"], // Needs these mutations before you can mutate toward this mutation
"prereqs2": ["LEAVES"], //Also need these mutations before you can mutate towards this mutation. When both set creates 2 different mutation paths, random from one is picked. Only use together with "prereqs"
"threshreq": ["THRESH_SPIDER"], //Required threshold for this mutation to be possible
"cancels": ["ROT1", "ROT2", "ROT3"], // Cancels these mutations when mutating
"changes_to": ["FASTHEALER2"], // Can change into these mutations when mutating further
"leads_to": [], // Mutations that add to this one
"passive_mods" : { //increases stats with the listed value. Negative means a stat reduction
            "per_mod" : 1, //Possible values per_mod, str_mod, dex_mod, int_mod
            "str_mod" : 2
},
"wet_protection":[{ "part": "HEAD", // Wet Protection on specific bodyparts
                    "good": 1 } ] // "neutral/good/ignored" // Good increases pos and cancels neg, neut cancels neg, ignored cancels both
"vitamin_rates": [ [ "vitC", -1200 ] ], // How much extra vitamins do you consume per minute. Negative values mean production
"vitamins_absorb_multi": [ [ "flesh", [ [ "vitA", 0 ], [ "vitB", 0 ], [ "vitC", 0 ], [ "calcium", 0 ], [ "iron", 0 ] ], [ "all", [ [ "vitA", 2 ], [ "vitB", 2 ], [ "vitC", 2 ], [ "calcium", 2 ], [ "iron", 2 ] ] ] ], // multiplier of vitamin absorption based on material. "all" is every material. supports multiple materials.
"craft_skill_bonus": [ [ "electronics", -2 ], [ "tailor", -2 ], [ "mechanics", -2 ] ], // Skill affected by the mutation and their bonuses. Bonuses can be negative, a bonus of 4 is worth 1 full skill level.
"restricts_gear" : [ "TORSO" ], //list of bodyparts that get restricted by this mutation
"allow_soft_gear" : true, //If there is a list of 'restricts_gear' this sets if the location still allows items made out of soft materials (Only one of the types need to be soft for it to be considered soft). (default: false)
"destroys_gear" : true, //If true, destroys the gear in the 'restricts_gear' location when mutated into. (default: false)
"encumbrance_always" : [ // Adds this much encumbrance to selected body parts
    [ "ARM_L", 20 ],
    [ "ARM_R", 20 ]
],
"encumbrance_covered" : [ // Adds this much encumbrance to selected body parts, but only if the part is covered by not-OVERSIZE worn equipment
    [ "HAND_L", 50 ],
    [ "HAND_R", 50 ]
],
"armor" : [ // Protects selected body parts this much. Resistances use syntax like `PART RESISTANCE` below.
    [
        [ "ALL" ], // Shorthand that applies the selected resistance to the entire body
        { "bash" : 2 } // The resistance provided to the body part(s) selected above
    ],
    [   // NOTE: Resistances are applies in order and ZEROED between applications!
        [ "ARM_L", "ARM_R" ], // Overrides the above settings for those body parts
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
"no_cbm_on_bp": [ "TORSO", "HEAD", "EYES", "MOUTH", "ARM_L" ], // List of body parts that can't receive cbms. (default: empty)
"lumination": [ [ "HEAD", 20 ], [ "ARM_L", 10 ] ], // List of glowing bodypart and the intensity of the glow as a float. (default: empty)
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
"symbol": "0",                // ASCII character displayed when part is working
"looks_like": "small_wheel",  // hint to tilesets if this part has no tile, use the looks_like tile
"color": "dark_gray",         // Color used when part is working
"broken_symbol": "x",         // ASCII character displayed when part is broken
"broken_color": "light_gray", // Color used when part is broken
"damage_modifier": 50,        // (Optional, default = 100) Dealt damage multiplier when this part hits something, as a percentage. Higher = more damage to creature struck
"durability": 200,            // How much damage the part can take before breaking
"description": "A wheel."     // A description of this vehicle part when installing it
"size": 2000                  // If vehicle part has flag "FLUIDTANK" then capacity in mLs, else divide by 4 for volume on space
"cargo_weight_modifier": 100  // Special function to multiplicatively modify item weight on space. Divide by 100 for ratio.
"wheel_width": 9,             /* (Optional, default = 0)
                               * SPECIAL: A part may have at most ONE of the following fields:
                               *    wheel_width = base wheel width in inches
                               *    size        = trunk/box storage volume capacity
                               *    power       = base engine power in watts
                               *    bonus       = bonus granted; muffler = noise reduction%, seatbelt = bonus to not being thrown from vehicle
                               *    par1        = generic value used for unique bonuses, like the headlight's light intensity */
"wheel_type":                 // (Optional: standard, off-road)
"contact_area":               // (Optional) Affects vehicle ground pressure
"cargo_weight_modifier": 33,  // (Optional, default = 100) Modifies cargo weight by set percentage
"fuel_type": "NULL",          // (Optional, default = "NULL") Type of fuel/ammo the part consumes, as an item id

"item": "wheel",              // The item used to install this part, and the item obtained when removing this part
"difficulty": 4,              // Your mechanics skill must be at least this level to install this part
"breaks_into" : [             // When the vehicle part is destroyed, items from this item group (see ITEM_SPAWN.md) will be spawned around the part on the ground.
  {"item": "scrap", "count": [0,5]} // instead of an array, this can be an inline item group,
],
"breaks_into" : "some_item_group", // or just the id of an item group.
"flags": [                    // Flags associated with the part
     "EXTERNAL", "MOUNT_OVER", "WHEEL", "MOUNT_POINT", "VARIABLE_SIZE"
],
"damage_reduction" : {        // Flat reduction of damage, as described below. If not specified, set to zero
    "all" : 10,
    "physical" : 5
},
                              // The following optional fields are specific to ENGINEs.
"m2c": 50,                    // Mandatory field for parts with the ENGINE flag, indicates ratio of cruise power to maximum power
"backfire_threshold": 0.5,    // Optional field, defaults to 0. Indicates maximum ratio of damaged HP to max HP to trigger backfires
"backfire_freq": 20,          // Optional field unless backfire threshold > 0, then mandatory, defaults to 0. One in X chance of a backfire.
"noise_factor": 15,           // Optional field, defaults to 0. Multiple engine power by this number to declare noise.
"damaged_power_factor": 0.5,  // Optional field, defaults to 0. If more than 0, power when damaged is scaled to power * ( damaged_power_factor + ( 1 - damaged_power_factor ) * ( damaged HP / max HP )
"muscle_power_factor": 0,     // Optional field, defaults to 0. If more than 0, each point of the survivor's Strength over 8 adds this much power to the engine, and each point less than 8 removes this much power.
"exclusions": [ "souls" ]     // Optional field, defaults to empty. A list of words. A new engine can't be installed on the vehicle if any engine on the vehicle shares a word from exclusions.
"fuel_options": [ "soul", "black_soul" ] // Optional field, defaults to fuel_type.  A list of words. An engine can be fueled by any fuel type in its fuel_options.  If provided, it overrides fuel_type and should include the fuel in fuel_type.
"comfort": 3,                 // Optional field, defaults to 0. How comfortable this terrain/furniture is. Impact ability to fall asleep on it. (uncomfortable = -999, neutral = 0, slightly_comfortable = 3, comfortable = 5, very_comfortable = 10)
"floor_bedding_warmth": 300,  // Optional field, defaults to 0. Bonus warmth offered by this terrain/furniture when used to sleep.
"bonus_fire_warmth_feet": 200,// Optional field, defaults to 300. Increase warmth received on feet from nearby fire.
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
  "fuel" : -1, // The fuel of the new vehicles.
  "status" : 1  // The status of the new vehicles.
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
                                           /* Important! Vehicle parts must be defined in the same order you would install
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
"integral_volume": 0,                        // Volume added to base item when item is integrated into another (eg. a gunmod integrated to a gun). Volume in ml and L can be used - "50 ml" or "2 L".
"integral_weight": 0,                        // Weight added to base item when item is integrated into another (eg. a gunmod integrated to a gun)
"rigid": false,                              // For non-rigid items volume (and for worn items encumbrance) increases proportional to contents
"insulation": 1,                             // (Optional, default = 1) If container or vehicle part, how much insulation should it provide to the contents
"price": 100,                                // Used when bartering with NPCs. For stackable items (ammo, comestibles) this is the price for stack_size charges. Can use string "cent" "USD" or "kUSD".
"price_post": "1 USD",                       // Same as price but represent value post cataclysm. Can use string "cent" "USD" or "kUSD".
"material": ["COTTON"],                      // Material types, can be as many as you want.  See materials.json for possible options
"cutting": 0,                                // (Optional, default = 0) Cutting damage caused by using it as a melee weapon.  This value cannot be negative.
"bashing": 0,                                // (Optional, default = 0) Bashing damage caused by using it as a melee weapon.  This value cannot be negative.
"to_hit": 0,                                 // (Optional, default = 0) To-hit bonus if using it as a melee weapon (whatever for?)
"flags": ["VARSIZE"],                        // Indicates special effects, see JSON_FLAGS.md
"environmental_protection_with_filter": 6,   // the resistance to environmental effects if an item (for example a gas mask) requires a filter to operate and this filter is installed. Used in combination with use_action 'GASMASK' and 'DIVE_TANK'
"magazine_well": 0,                          // Volume above which the magazine starts to protrude from the item and add extra volume
"magazines": [                               // Magazines types for each ammo type (if any) which can be used to reload this item
    [ "9mm", [ "glockmag" ] ]                // The first magazine specified for each ammo type is the default
    [ "45", [ "m1911mag", "m1911bigmag" ] ],
],
"explode_in_fire": true,                     // Should the item explode if set on fire
"explosion": {                               // Physical explosion data
    "power": 10,                             // Measure of explosion power in grams of TNT equivalent explosive, affects damage and range.
    "distance_factor": 0.9,                  // How much power is retained per traveled tile of explosion. Must be lower than 1 and higher than 0.
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
                                 // additional some magazine specific entries:
"ammo_type": [ "40", "357sig" ], // What types of ammo this magazine can be loaded with
"capacity" : 15,                 // Capacity of magazine (in equivalent units to ammo charges)
"count" : 0,                     // Default amount of ammo contained by a magazine (set this for ammo belts)
"default_ammo": "556",           // If specified override the default ammo (optionally set this for ammo belts)
"reliability" : 8,               // How reliable this this magazine on a range of 0 to 10? (see GAME_BALANCE.md)
"reload_time" : 100,             // How long it takes to load each unit of ammo into the magazine
"linkage" : "ammolink"           // If set one linkage (of given type) is dropped for each unit of ammo consumed (set for disintegrating ammo belts)
```

### Armor

Armor can be defined like this:

```C++
"type" : "ARMOR",     // Defines this as armor
...                   // same entries as above for the generic item.
                      // additional some armor specific entries:
"covers" : ["FEET"],  // Where it covers.  Possible options are TORSO, HEAD, EYES, MOUTH, ARMS, HANDS, LEGS, FEET
"storage" : 0,        //  (Optional, default = 0) How many volume storage slots it adds
"warmth" : 10,        //  (Optional, default = 0) How much warmth clothing provides
"environmental_protection" : 0,  //  (Optional, default = 0) How much environmental protection it affords
"encumbrance" : 0,    // Base encumbrance (unfitted value)
"max_encumbrance" : 0,    // When a character is completely full of volume, the encumbrance of a non-rigid storage container will be set to this. Otherwise it'll be between the encumbrance and max_encumbrance following the equation: encumbrance + (max_encumbrance - encumbrance) * character volume.
"weight_capacity_bonus": "20 kg",    // (Optional, default = 0) Bonus to weight carrying capacity, can be negative. Strings must be used - "5000 g" or "5 kg"
"weight_capacity_modifier": 1.5, // (Optional, default = 1) Factor modifying base weight carrying capacity.
"coverage" : 80,      // What percentage of body part
"material_thickness" : 1,  // Thickness of material, in millimeter units (approximately).  Generally ranges between 1 - 5, more unusual armor types go up to 10 or more
"power_armor" : false, // If this is a power armor item (those are special).
"valid_mods" : ["steel_padded"] // List of valid clothing mods. Note that if the clothing mod doesn't have "restricted" listed, this isn't needed.
```
Alternately, every item (book, tool, gun, even food) can be used as armor if it has armor_data:
```C++
"type" : "TOOL",      // Or any other item type
...                   // same entries as for the type (e.g. same entries as for any tool),
"armor_data" : {      // additionally the same armor data like above
    "covers" : ["FEET"],
    "storage" : 0,
    "warmth" : 10,
    "environmental_protection" : 0,
    "encumbrance" : 0,
    "coverage" : 80,
    "material_thickness" : 1,
    "power_armor" : false
}
```

### Pet Armor
Pet armor can be defined like this:

```C++
"type" : "PET_ARMOR",     // Defines this as armor
...                   // same entries as above for the generic item.
                      // additional some armor specific entries:
"storage" : 0,        //  (Optional, default = 0) How many volume storage slots it adds
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
    "storage" : 0,
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

####CBMs

CBMs can be defined like this:

```C++
"type" : "BIONIC_ITEM",         // Defines this as a CBM
...                             // same entries as above for the generic item.
                                // additional some CBM specific entries:
"bionic_id" : "bio_advreactor", // ID of the installed bionic if not equivalent to "id"
"difficulty" : 11,              // Difficulty of installing CBM
"is_upgrade" : true             // Whether the CBM is an upgrade of another bionic.
```

### Comestibles

```C++
"type" : "COMESTIBLE",      // Defines this as a COMESTIBLE
...                         // same entries as above for the generic item.
                            // additional some comestible specific entries:
"addiction_type" : "crack", // Addiction type
"spoils_in" : 0,            // A time duration: how long a comestible is good for. 0 = no spoilage.
"use_action" : "CRACK",     // What effects a comestible has when used, see special definitions below
"stim" : 40,                // Stimulant effect
"fatigue_mod": 3,           // How much fatigue this comestible removes. (Negative values add fatigue)
"radiation": 8,             // How much radiation you get from this comestible.
"comestible_type" : "MED",  // Comestible type, used for inventory sorting
"quench" : 0,               // Thirst quenched
"heal" : -2,                // Health effects (used for sickness chances)
"addiction_potential" : 80, // Ability to cause addictions
"monotony_penalty" : 0,     // (Optional, default: 2) Fun is reduced by this number for each one you've consumed in the last 48 hours.
                            // Can't drop fun below 0, unless the comestible also has the "NEGATIVE_MONOTONY_OK" flag.
"calories" : 0,             // Hunger satisfied (in kcal)
"nutrition" : 0,            // Hunger satisfied (OBSOLETE)
"tool" : "apparatus",       // Tool required to be eaten/drank
"charges" : 4,              // Number of uses when spawned
"stack_size" : 8,           // (Optional) How many uses are in the above-defined volume. If omitted, is the same as 'charges'
"fun" : 50                  // Morale effects when used
"freezing_point": 32,       // (Optional) Temperature in F at which item freezes, default is water (32F/0C)
"cooks_like": "meat_cooked" // (Optional) If the item is used in a recipe, replaces it with its cooks_like
"parasites": 10,            // (Optional) Probability of becoming parasitised when eating
"contamination": [ { "disease": "bad_food", "probability": 5 } ],         // (Optional) List of diseases carried by this comestible and their associated probability. Values must be in the [0, 100] range.
```

### Containers

```C++
"type": "CONTAINER",  // Defines this as a container
...                   // same data as for the generic item (see above).
"contains": 200,      // How much volume this container can hold
"seals": false,       // Can be resealed, this is a required for it to be used for liquids. (optional, default: false)
"watertight": false,  // Can hold liquids, this is a required for it to be used for liquids. (optional, default: false)
"preserves": false,   // Contents do not spoil. (optional, default: false)
```
Alternately, every item can be used as container:
```C++
"type": "ARMOR",      // Any type is allowed here
...                   // same data as for the type
"container_data" : {  // The container specific data goes here.
    "contains": 200,
}
```
This defines a armor (you need to add all the armor specific entries), but makes it usable as container.
It could also be written as a generic item ("type": "GENERIC") with "armor_data" and "container_data" entries.

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
"aim_speed": 3,            // A measure of how quickly the player can aim, in moves per point of dispersion.
"recoil": 0,               // Recoil caused when firing, in quarter-degrees of dispersion.
"durability": 8,           // Resistance to damage/rusting, also determines misfire chance
"blackpowder_tolerance": 8,// One in X chance to get clogged up (per shot) when firing blackpowder ammunition (higher is better). Optional, default is 8.
"min_cycle_recoil": 0,     // Minimum ammo recoil for gun to be able to fire more than once per attack.
"burst": 5,                // Number of shots fired in burst mode
"clip_size": 100,          // Maximum amount of ammo that can be loaded
"ups_charges": 0,          // Additionally to the normal ammo (if any), a gun can require some charges from an UPS. This also works on mods. Attaching a mod with ups_charges will add/increase ups drain on the weapon.
"reload": 450,             // Amount of time to reload, 100 = 1 second = 1 "turn"
"built_in_mods": ["m203"], //An array of mods that will be integrated in the weapon using the IRREMOVABLE tag.
"default_mods": ["m203"]   //An array of mods that will be added to a weapon on spawn.
"barrel_length": "30 mL",  // Amount of volume lost when the barrel is sawn. Approximately 250 ml per inch is a decent approximation.
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
"location": "stock",           // Mandatory. Where is this gunmod is installed?
"mod_targets": [ "crossbow" ], // Mandatory. What kind of weapons can this gunmod be used with?
"acceptable_ammo": [ "9mm" ],  // Optional filter restricting mod to guns with those base (before modifiers) ammo types
"install_time": "30 s",        // Optional time installation takes. Installation is instantaneous if unspecified. An integer will be read as moves or a time string can be used.
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
"material": "wood",   // Material types.  See materials.json for possible options
"techniques": "FLAMING", // Combat techniques used by this tool
"flags": "FIRE",      // Indicates special effects
"weight": 831,        // Weight, measured in grams
"volume": "1500 ml",  // Volume, volume in ml and L can be used - "50 ml" or "2 L"
"bashing": 12,        // Bashing damage caused by using it as a melee weapon
"cutting": 0,         // Cutting damage caused by using it as a melee weapon
"to_hit": 3,          // To-hit bonus if using it as a melee weapon
"max_charges": 75,    // Maximum charges tool can hold
"initial_charges": 75, // Charges when spawned
"rand_charges: [10, 15, 25], // Randomize the charges when spawned. This example has a 50% chance of rng(10, 15) charges and a 50% chance of rng(15, 25) (The endpoints are included)
"sub": "hotplate",    // optional; this tool has the same functions as another tool
"charge_factor": 5,   // this tool uses charge_factor charges for every charge required in a recipe; intended for tools that have a "sub" field but use a different ammo that the original tool
"charges_per_use": 1, // Charges consumed per tool use
"turns_per_charge": 20, // Charges consumed over time, deprecated in favor of power_draw
"power_draw": 50,       // Energy consumption rate in mW
"ammo": [ "NULL" ],       // Ammo types used for reloading
"revert_to": "torch_done", // Transforms into item when charges are expended
"use_action": "firestarter" // Action performed when tool is used, see special definition below
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

### Artifact Data

Every item type can have optional artifact properties (which makes it an artifact):

```C++
"artifact_data" : {
    "charge_type": "ARTC_PAIN",
    "effects_carried": ["AEP_INT_DOWN"],
    "effects_wielded": ["AEP_DEX_UP"],
    "effects_activated": ["AEA_BLOOD", "AEA_NOISE"],
    "effects_worn": ["AEP_STR_UP"]
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

#### `Charge_type`

(optional, default: `ARTC_NULL`)

How the item is recharged.

For this to work, the item needs to be a tool that consumes charges upon invocation and has non-zero max_charges. Possible values (see src/artifact.h for an up-to-date list):

- `ARTC_NULL` Never recharges!
- `ARTC_TIME` Very slowly recharges with time
- `ARTC_SOLAR` Recharges in sunlight
- `ARTC_PAIN` Creates pain to recharge
- `ARTC_HP` Drains HP to recharge
- `ARTC_FATIGUE` Creates fatigue to recharge
- `ARTC_PORTAL` Consumes portals to recharge

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

Effects of the artifact when it's activated (which require it to have a `"use_action": "ARTIFACT"` and it must have a non-zero max_charges value).

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

### Fuel data

Every item type can have fuel data that determines how much horse power it produces per unit consumed. Currently, gasses and plasmas cannot really be fuels.

If a fuel has the PERPETUAL flag, engines powered by it never use any fuel.  This is primarily intended for the muscle pseudo-fuel, but mods may take advantage of it to make perpetual motion machines.

```C++
"fuel" : {
    energy": 34.2,               // battery charges per mL of fuel. batteries have energy 1
                                 // is also MJ/L from https://en.wikipedia.org/wiki/Energy_density
                                 // assumes stacksize 250 per volume 1 (250mL). Multiply
                                 // by 250 / stacksize * volume for other stack sizes and
                                 // volumes
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
    "need_worn": true;                        // Whether the item needs to be worn to be transformed, is false by default.
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
    "effects": [ { "id": "fetid_goop", "duration": 360, "bp": "TORSO", "permanent": true } ], // List of effects with their id, duration, bodyparts, and permanent bool
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
    "skill1": "throw", // Id of a skill, higher skill level means more likely to place a friendly monster.
    "skill2": "unarmed", // Another id, just like the skill1. Both entries are optional.
    "moves": 60 // how many move points the action takes.
},
"use_action": {
    "type": "place_npc", // place npc of specific class on the map
    "npc_class_id": "true_foodperson", // npc class id, see npcs/classes.json
    "summon_msg": "You summon a food hero!", // (optional) message when summoning the npc.
    "place_randomly": true, // if true: places npc randomly around the player, if false: let the player decide where to put it (default: false)
    "moves": 50 // how many move points the action takes.
},
"use_action": {
    "type": "ups_based_armor", // Armor that can be activated and uses power from an UPS, needs additional json code to work
    "activate_msg": "You activate your foo.", // Message when the player activates the item.
    "deactive_msg": "You deactivate your foo.", // Message when the player deactivates the item.
    "out_of_power_msg": "Your foo runs out of power and deactivates itself." // Message when the UPS runs out of power and the item is deactivated automatically.
}
"use_action" : {
    "type" : "delayed_transform", // Like transform, but it will only transform when the item has a certain age
    "transform_age" : 600, // The minimal age of the item. Items that are younger wont transform. In turns (60 turns = 1 minute)
    "not_ready_msg" : "The yeast has not been done The yeast isn't done culturing yet." // A message, shown when the item is not old enough
},
"use_action": {
    "type": "picklock", // picking a lock on a door
    "pick_quality": 3 // "quality" of the tool, higher values mean higher success chance, and using it takes less moves.
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
    "type": "enzlave" // Make a zlave.
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
    "bleed" : 0.4,          // Chance to remove bleed effect.
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

`"id", "name", "symbol", "looks_like", "color", "bgcolor", "max_volume", "open", "close", "bash", "deconstruct", "examine_action", "flgs`

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

`"id", "name", "symbol", "looks_like", "color", "bgcolor", "max_volume", "open", "close", "bash", "deconstruct", "examine_action", "flgs`

Same as for furniture, see below in the chapter "Common to furniture and terrain".

#### `move_cost`

Move cost to move through. A value of 0 means it's impassable (e.g. wall). You should not use negative values. The positive value is multiple of 50 move points, e.g. value 2 means the player uses 2\*50 = 100 move points when moving across the terrain.

#### `light_emitted`

How much light the terrain emits. 10 will light the tile it's on brightly, 15 will light that tile and the tiles around it brightly, as well as slightly lighting the tiles two tiles away from the source.
For examples: An overhead light is 120, a utility light, 240, and a console, 10.

#### `trap`

(Optional) Id of the build-in trap of that terrain.

For example the terrain `t_pit` has the built-in trap `tr_pit`. Every tile in the game that has the terrain `t_pit` also has, therefore, an implicit trap `tr_pit` on it. The two are inseparable (the player can not deactivate the built-in trap, and changing the terrain will also deactivate the built-in trap).

A built-in trap prevents adding any other trap explicitly (by the player and through mapgen).

#### `harvestable`

(Optional) If defined, the terrain is harvestable. This entry defines the item type of the harvested fruits (or similar). To make this work, you also have to set one of the `harvest_*` `examine_action` functions.

#### `transforms_into`

(Optional) Used for various transformation of the terrain. If defined, it must be a valid terrain id. Used for example:

- When harvesting fruits (to change into the harvested form of the terrain).
- In combination with the `HARVESTED` flag and `harvest_season` to change the harvested terrain back into a terrain with fruits.

#### `harvest_season`

(Optional) On of "SUMMER", "AUTUMN", "WINTER", "SPRING", used in combination with the "HARVESTED" flag to revert the terrain back into a terrain that can be harvested.

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
- `CHAINFENCE`
- `RAILING`
- `WALL`
- `WATER`
- `WOODFENCE`

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

#### `base`

What the 'base' furniture of the `PLANT` furniture is - what it would be if there was not a plant growing there. Used when monsters 'eat' the plant to preserve what furniture it is.

#### `growth_multiplier`

A flat multiplier on the growth speed on the plant. For numbers greater than one, it will take longer to grow, and for numbers less than one it will take less time to grow.

#### `harvest_multiplier`

A flat multiplier on the harvest count of the plant. For numbers greater than one, the plant will give more produce from harvest, for numbers less than one it will give less produce from harvest.

### clothing_mod

```JSON
"type": "clothing_mod",
"id": "leather_padded",   // Unique ID.
"flag": "leather_padded", // flag to add to clothing.
"item": "leather",        // item to consume.
"implement_prompt": "Pad with leather",      // prompt to show when implement mod.
"destroy_prompt": "Destroy leather padding", // prompt to show when destroy mod.
"restricted": true,       // (optional) If true, clothing must list this mod's flag in "valid_mods" list to use it. Defaults to false.
"mod_value": [            // List of mod effect.
    {
        "type": "bash",   // "bash", "cut", "fire", "acid", "warmth", "storage", and "encumbrance" is available.
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
    "ident": "schools_out",
    ...
}
```

The ident member should be the unique id of the scenario.

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

Mods can modify the lists of an existing scenario via "add:both" / "add:male" / "add:female" and "remove:both" / "remove:male" / "remove:female".

Example for mods:
```C++
{
    "type": "scenario",
    "ident": "schools_out",
    "edit-mode": "modify",
    "items": {
        "remove:both": [ "rock" ],
        "add:female": [ "2x4" ]
    }
}
```

## `flags`
(optional, array of strings)

A list of flags. TODO: document those flags here.

Mods can modify this via "add:flags" and "remove:flags".

## `cbms`
(optional, array of strings)

A list of CBM ids that are implanted in the character.

Mods can modify this via "add:CBMs" and "remove:CBMs".

## `traits", "forced_traits", "forbidden_traits`
(optional, array of strings)

Lists of trait/mutation ids. Traits in "forbidden_traits" are forbidden and can't be selected during the character creation. Traits in "forced_traits" are automatically added to character. Traits in "traits" enables them to be chosen, even if they are not starting traits.

Mods can modify this via "add:traits" / "add:forced_traits" / "add:forbidden_traits" and "remove:traits" / "remove:forced_traits" / "remove:forbidden_traits".

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

# Starting locations

Starting locations are specified as JSON object with "type" member set to "start_location":
```C++
{
    "type": "start_location",
    "ident": "field",
    "name": "An empty field",
    "target": "field",
    ...
}
```

The ident member should be the unique id of the location.

The following properties (mandatory, except if noted otherwise) are supported:

## `name`
(string)

The in-game name of the location.

## `target`
(string)

The id of an overmap terrain type (see overmap_terrain.json) of the starting location. The game will chose a random place with that terrain.

## `flags`
(optional, array of strings)

Arbitrary flags. Mods can modify this via "add:flags" / "remove:flags". TODO: document them.

### `tile_config`
Each tileset has a tile_config.json describing how to map the contents of a sprite sheet to various tile identifiers, different orientations, etc. The ordering of the overlays used for displaying mutations can be controlled as well. The ordering can be used to override the default ordering provided in `mutation_ordering.json`. Example:

```C++
  {                                             // whole file is a single object
    "tile_info": [                              // tile_info is mandatory
      {
        "height": 32,
        "width": 32,
        "iso" : true,                             //  Optional. Indicates an isometric tileset. Defaults to false.
        "pixelscale" : 2                          //  Optional. Sets a multiplier for resizing a tileset. Defaults to 1.
      }
    ],
    "tiles-new": [                              // tiles-new is an array of sprite sheets
      {                                           //   alternately, just one "tiles" array
        "file": "tiles.png",                      // file containing sprites in a grid
        "tiles": [                                // array with one entry per tile
          {
            "id": "10mm",                         // id is how the game maps things to sprites
            "fg": 1,                              //   lack of prefix mostly indicates items
            "bg": 632,                            // fg and bg can be sprite indexes in the image
            "rotates": false
          },
          {
            "id": "t_wall",                       // "t_" indicates terrain
            "fg": [2918, 2919, 2918, 2919],       // 2 or 4 sprite numbers indicates pre-rotated
            "bg": 633,
            "rotates": true,
            "multitile": true,
            "additional_tiles": [                 // connected/combined versions of sprite
              {                                   //   or variations, see below
                "id": "center",
                "fg": [2919, 2918, 2919, 2918]
              },
              {
                "id": "corner",
                "fg": [2924, 2922, 2922, 2923]
              },
              {
                "id": "end_piece",
                "fg": [2918, 2919, 2918, 2919]
              },
              {
                "id": "t_connection",
                "fg": [2919, 2918, 2919, 2918]
              },
              {
                "id": "unconnected",
                "fg": 2235
              }
            ]
          },
          {
            "id": "vp_atomic_lamp",               // "vp_" vehicle part
            "fg": 3019,
            "bg": 632,
            "rotates": false,
            "multitile": true,
            "additional_tiles": [
              {
                "id": "broken",                   // variant sprite
                "fg": 3021
              }
            ]
          },
          {
            "id": "t_dirt",
            "rotates": false,
            "fg": [
              { "weight":50, "sprite":640},       // weighted random variants
              { "weight":1, "sprite":3620},
              { "weight":1, "sprite":3621},
              { "weight":1, "sprite":3622}
            ]
          },
          {
            "id": [
              "overlay_mutation_GOURMAND",        // character overlay for mutation
              "overlay_mutation_male_GOURMAND",   // overlay for specified gender
              "overlay_mutation_active_GOURMAND"  // overlay for activated mutation
            ],
            "fg": 4040
          }
        ]
      },
      {                                           // second entry in tiles-new
        "file": "moretiles.png",                  // another sprite sheet
        "tiles": [
          {
            "id": ["xxx","yyy"],                  // define two ids at once
            "fg": 1,
            "bg": 234
          }
        ]
      }
    ],
    "overlay_ordering": [
      {
        "id" : "WINGS_BAT",                         // mutation name, in a string or array of strings
        "order" : 1000                              // range from 0 - 9999, 9999 being the topmost layer
      },
      {
        "id" : [ "PLANTSKIN", "BARK" ],             // mutation name, in a string or array of strings
        "order" : 3500                              // order is applied to all items in the array
      },
      {
        "id" : "bio_armor_torso",                   // Overlay order of bionics is controlled in the same way
        "order" : 500
      }
    ]
  }
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

  {
    "type": "field_type", // this is a field type
    "id": "fd_gum_web", // id of the field
    "immune_mtypes": [ "mon_spider_gum" ], // list of monster immune to this field
    "bash": {
      "str_min": 1, // lower bracket of bashing damage required to bash
      "str_max": 3, // higher bracket
      "sound_vol": 2, // noise made when succesfully bashing the field
      "sound_fail_vol": 2, // noise made when failing to bash the field
      "sound": "shwip", // sound on success
      "sound_fail": "shwomp", // sound on failure
      "msg_success": "You brush the gum web aside.", // message on success
      "move_cost": 120, // how many moves it costs to succesfully bash that field (default: 100)
      "items": [                                   // item dropped upon succesful bashing
        { "item": "2x4", "count": [ 5, 8 ] },
        { "item": "nail", "charges": [ 6, 8 ] },
        { "item": "splinter", "count": [ 3, 6 ] },
        { "item": "rag", "count": [ 40, 55 ] },
        { "item": "scrap", "count": [ 10, 20 ] }
      ]
    }
  }
