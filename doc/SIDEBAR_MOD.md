# Sidebar Modification

- [Overview](#overview)
- [Widgets](#widgets)
  - [Sidebar widgets](#sidebar-widgets)
  - [Layout widgets](#layout-widgets)
  - [Variable widgets](#variable-widgets)
    - [Number widget](#number-widget)
    - [Graph widget](#graph-widget)
- [Other fields](#other-fields)
  - [fill](#fill)
  - [var_max](#var_max)
  - [direction](#direction)
  - [height](#height)
  - [alignment](#text_align--label_align)
  - [colors](#colors)
  - [phrases](#phrases)
  - [flags](#flags)
- [Variables](#variables)
  - [Numeric variables](#numeric-variables)
  - [Text variables](#text-variables)
- [Predefined widgets](#predefined-widgets)
  - [Number widgets](#number-widgets)
  - [Graph widgets](#graph-widgets)
  - [Text widgets](#text-widgets)
  - [Layout widgets](#layout-widgets)

## Overview

Some parts of the main CDDA sidebar are now moddable, meaning they are data-driven and can be
customized simply by editing JSON files, without recompiling the game.

You can add a custom sidebar section to your regular sidebar via the Sidebar Options menu `}`
by enabling the "Custom" section from the left-hand column for any of the regular sidebar layouts
(classic, labels, narrow etc.)

You can also switch to an almost completely custom sidebar, by selecting "custom" from the
right-hand column of the sidebar options menu. This layout is built from the "custom_sidebar" widget
defined in `data/json/ui/sidebar.json`, with sections you can toggle or rearrange in-game according
to your preference.

In both cases, you can further customize your sidebar widgets by modifying (or modding) the JSON
that describes them. This document explains how they work.


## Widgets

All "custom" sidebar UI elements are defined in objects called widgets. A widget can display a
variety of player character attributes in numeric form, or as a bar graph of arbitrary width. A
widget can also make a layout of other widgets.

Widget instances are defined by JSON data, with the main game sidebar widgets and layouts being in
`data/json/ui/sidebar.json`. You may customize yours by editing this file, or by loading a mod that
adds or modifies widget definitions.

For example, here is a widget to display the player character's "Focus" attribute as a number:

```json
{
  "id": "focus_num",
  "type": "widget",
  "label": "Focus",
  "var": "focus",
  "style": "number"
}
```

All widgets must have a unique "id", and "type": "widget".

Each widget has a "style" field that may be:

- `number`: Show value as a plain integer number
- `graph`: Show a bar graph of the value with colored text characters
- `text`: Show text from a `*_text` variable
- `layout`: Layout container for arranging other widgets in rows or columns
- `sidebar`: Special top-level widget for defining custom sidebars

Let's start at the top, with the "sidebar" widget, composed of several "layout" widgets.


### Sidebar widgets

The highest-level widget is the "sidebar", which represents the entire display region on the right
(or left) edge of the screen. It includes a "width" in characters, a "label" displayed in the
sidebar options menu, and a list of "widgets", shown as sections that may be rearranged or
toggled from the sidebar options menu.

These sub-widgets are typically [layout widgets](#layout-widgets), with other widgets arranged
inside them, but they could also be plain [variable widgets](#variable-widgets), used for showing
character attributes or other information.

Here is how a simple sidebar definition might look in JSON:

```json
{
  "id": "my_sidebar",
  "style": "sidebar",
  "width": 40,
  "widgets": [
    "sound_focus_move_layout",
    "stats_layout"
  ]
}
```

Each widget in the "sidebar" will be associated with a `panel_layout` instance in the code, which is
what allows them to be toggled and rearranged like the classic sidebar sections.

You may define any number of "sidebar" widgets, each with their own width, label, and collection of
sub-widgets and layouts.

Sidebar widgets aside, there are two major types of widget: [variable widgets](#variable-widgets),
showing some piece of information (with a label); and [layout widgets](#layout-widgets), used for
arranging other widgets in rows or columns.

We will look at layout widgets first, since they are easier to explain.


### Layout widgets

Use widgets with "style": "layout" to arrange child widgets in sidebar panels, giving widget ids in
the "widgets" list field.

The arrangement of child widgets is defined by the "arrange" field, which may be "columns" (default)
to array widgets horizontally, or "rows" to arrange them vertically, one widget per row.  Widgets in
the same row will have their horizontal space split as equally as possible.

```json
[
  {
    "id": "sound_focus_move_layout",
    "type": "widget",
    "style": "layout",
    "arrange": "columns",
    "widgets": [ "sound_num", "focus_num", "move_num" ]
  },
  {
    "id": "stats_layout",
    "type": "widget",
    "style": "layout",
    "arrange": "columns",
    "widgets": [ "str_num", "dex_num", "int_num", "per_num" ]
  },
  {
    "id": "sound_focus_move_stats_layout",
    "type": "widget",
    "style": "layout",
    "arrange": "rows",
    "widgets": [
      "sound_focus_move_layout",
      "stats_layout"
    ]
  }
]
```

The above might yield:

```
Sound: 8  Focus: 105  Move: 120
Str: 8  Dex: 9  Int: 7  Per: 11
```

Where do all these numeric widgets and their values come from? These are variable widgets, discussed
next.


### Variable widgets

Variable widgets define a "var" field, with the name of a predefined widget variable. This tells the
widget what information it should show. Most of the time, these are attributes of the player
character, but they can also be attributes of the world, environment, or vehicle where they are.

See the [Variables](#variables) section for a list of them.

For example, a widget to show the current STR stat would define this "var":

```json
{
  "var": "stat_str"
}
```

And a widget to show the HP of the right arm would define "var" and "bodypart" like so:

```json
{
  "var": "bp_hp",
  "bodypart": "arm_r"
}
```

#### Number widget

The simplest and usually most compact widget for displaying a value, "style": "number" appears as a
label with an integer number.

```json
{
  "style": "number",
  "label": "Focus"
}
```

Result:

```
Focus: 100
```

The numeric value comes from the given "var", displayed as a decimal integer.


#### Graph widget

The graph shows an arrangement of symbols. It has two important parameters:

- `width`: how many characters wide is the graph
- `symbols`: single-character strings to map to 0-N

Given a graph of width 3 with two symbols, "-" and "=":

```json
{
  "width": 3,
  "symbols": "-="
}
```

The first symbol is the zero or blank filler symbol, and remaining symbols are cycled through to
fill the width of the graph. This simple graph uses the "-" symbol for its zero-filler, leaving one
symbol "=" that expands to fill the width from left to right. The graph has four possible states -
the all-zero state, and one state as each position up to "width" (3) is filled by the "=" symbol:

```
0: -
1: =

--- 000
=-- 100
==- 110
=== 111
```

The simplest possible graph is one character wide, with one symbol. It always shows the same value,
so is not very useful:

```json
{
  "width": 1,
  "symbols": "X"
}
```

The simplest *useful* graph is one character wide, with two symbols:

```json
{
  "width": 1,
  "symbols": "XO"
}
```

Such a graph would effectively compress its variable's value range to a simple threshold (off/on),
and could be used to create a single-character "pain alarm" or "loud sound" widget, for example.

Returning to the example with 3 width, and using three symbols, "-", "=", and "#", we may define:

```json
{
  "width": 3,
  "symbols": "-=#"
}
```

Here, the number of states increases to 7 - the zero state with all "-", plus two layers of filling
with "=" and "#":

```
0: -
1: =
2: #

--- 000
=-- 100
#-- 200
#=- 210
##- 220
##= 221
### 222
```

See the [fill](#fill), [var_max](#var_max), and [colors](#colors) fields for more ways to customize
the graph.


# Other fields

This section documents a few more widget fields with special uses.


## fill

For "graph" widgets with more than two sybols, different ways of filling up the graph become
possible.  The method is specified with the "fill" field. By default the "bucket" fill method is
used, but there is also a "pool" method described helow.

With "bucket" fill, positions are filled like a row of buckets, using all symbols in the first
position before beginning to fill the next position.  This is like the classic 5-bar HP meter.

```json
{
  "width": 5,
  "symbols": ".\\|",
  "fill": "bucket"
}
```

Result:

```
..... 00000
\.... 10000
|.... 20000
|\... 21000
||... 22000
||\.. 22100
|||.. 22200
|||\. 22210
||||. 22220
||||\ 22221
||||| 22222
```

Using "pool" fill, positions are filled like a swimming pool, with each symbol filling all positions
before the next symbol appears.

```json
{
  "width": 5,
  "symbols": "-=#",
  "fill": "pool"
}
```

Result:

```
----- 00000
=---- 10000
==--- 11000
===-- 11100
====- 11110
===== 11111
#==== 21111
##=== 22111
###== 22211
####= 22221
##### 22222
```

The total number of possible graphs is the same in each case, so both have the same resolution.


## `var_max`

Using "graph" style widgets, usually you should provide a "var_max" value (integer) with the maximum
typical value of "var" that will ever be rendered.

Some "var" fields such as "stamina", or "hp_bp" (hit points for body part) have a known maximum, but
others like character stats, move speed, or encumbrance have no predefined cap - for these you can
provide an explicit "var_max" that indicates where the top / full point of the graph is.

This helps the graph widget know whether it needs to show values up to 10000 (like stamina) or only
up to 100 or 200 (like focus). If a var usually varies within a range `[low, high]`, select a
"var_max" greater than `high` to be sure the normal variance is captured in the graph's range.


## `direction`

Widgets using `compass_text` expect the additional fields `direction` and `width` to
identify (respectively) the cardinal direction and number of creatures displayed:

```json
{
  "var": "compass_text",
  "direction": "N",
  "width": 6
}
```

`compass_legend_text` makes use of the "height" field (see below), which tells the display
function to reserve that many lines for the compass legend:

```json
{
  "var": "compass_legend_text",
  "height": 3
}
```

Plain numeric values can be displayed as-is, up to any maximum. For "graph" widgets, it is useful to
define a "var_max" as a cutoff point; see the "Graph widget" section for more.

You may also define "var_min" if it's relevant. By default this is 0.


## `height`

Some widgets can make use of multiple lines by specifying the `"height"` field, which reserves
vertical space in the sidebar. Display functions can make use of this extra space to render
multi-line widgets.

Warning: implementation details ahead.

The max width and height available for a widget is passed to its `display::` function through
`widget::color_text_function_string()`. The display function can use this data to format the
widget text as a series of lines delimited by a newline (`\n`).

The formatted string is passed to `widget::show()` and `widget::layout()`, which format each
line individually for drawing in `widget::custom_draw_multiline()`.

Adding new multi-line-capable widgets involves ensuring the new display function formats the
widget's text according to the available width and height.

Some multi-line widgets can dynamically adjust their height based on how many lines they are using.
To enable this behavior, add the `W_DYNAMIC_HEIGHT` flag to the widget (ex: see the compass legend).


## `text_align` / `label_align`

The widget's label and text/value can be aligned using the `label_align` and `text_align` respectively.
This is useful for widgets in "rows"-style layouts where the labels are different lengths, as the text
can be aligned along a common vertical across the column:

```json
{
  "label_align": "right",
  "text_align": "left"
}
```
```
   Temp: Mildly cold
Comfort: Cozy
   Pain: No Pain
```

By default, labels are assumed to be left-aligned while text/values are assumed to be right-aligned.


## `colors`

Widgets with "number" or "graph" style may define "colors", which will be used as a spectrum across
the widget's values ("var_min" to "var_max"), applying the appropriate color at each level.

For example, a lower movement number (move cost) is better, while higher numbers are worse. Around
500 is quite bad, while less than 100 is ideal. This range might be colored with green, white, and
red, given in a "colors" list:

```json
{
  "id": "move_num",
  "type": "widget",
  "label": "Move",
  "var": "move",
  "style": "number",
  "var_max": 500,
  "colors": [ "c_green", "c_white", "c_red" ]
}
```

Graphs can be colorized in the same way. For example, the classic stamina graph is a 5-character
bar, a dark green `|||||` when full. As stamina diminishes, the bar's color goes to light green,
yellow, light red, and red. Such coloration could be represented with "colors" like so:

```json
{
  "id": "stamina_graph_classic",
  "type": "widget",
  "label": "Stam",
  "var": "stamina",
  "var_max": 10000,
  "style": "graph",
  "width": 5,
  "symbols": ".\\|",
  "colors": [ "c_red", "c_light_red", "c_yellow", "c_light_green", "c_green" ]
}
```


## `phrases`

Some widgets can take advantage of "phrases" - definitions for what text/values to display and
how to display them. These take the form of a nested object containing several optional fields:

```json
{
  "id": "bp_status_indicator_template",
  "type": "widget",
  "style": "phrase",
  "phrases": [
    { "id": "bitten", "text": "bitten", "sym": "B", "color": "yellow" },
    { "id": "infected", "text": "infected", "sym": "I", "color": "pink" },
    { "id": "broken", "text": "broken", "sym": "%", "color": "magenta" },
    { "id": "splinted", "text": "splinted", "sym": "=", "color": "light_gray" },
    { "id": "bandaged", "text": "bandaged", "sym": "+", "color": "white" },
    { "id": "disinfected", "text": "disinfected", "sym": "$", "color": "light_green" },
    { "id": "bleeding", "text": "bleeding", "value": 0, "sym": "b", "color": "light_red" },
    { "id": "bleeding", "text": "bleeding", "value": 11, "sym": "b", "color": "red" },
    { "id": "bleeding", "text": "bleeding", "value": 21, "sym": "b", "color": "red_red" }
  ]
}
```

| JSON Field | Description
|---         |---
| `id`       | Which "phrase" this definition should apply to.
| `text`     | Translated text that may be interpreted and displayed in the widget.
| `sym`      | A shortened symbol representing the text.
| `color`    | Defines the color for the text derived from this "phrase".
| `value`    | A numeric value for this "phrase", which may be interpreted differently based on the context of the parent widget.

In the above example, the widget is simply used as a template for other widgets to `copy-from`,
which provides text and color definitions for different bodypart status conditions.

## `flags`

Widgets can use flags to specify special behaviors:

```json
{
  "id": "my_widget",
  "type": "widget",
  "style": "text",
  "label": "My Widget",
  "var": "my_widget_var",
  "flags": [ "W_LABEL_NONE", "W_DISABLED" ]
}
```

Here are some flags that can be included:

| Flag id            | Description
|---                 |---
| `W_LABEL_NONE`     | Prevents the widget's label from being displayed in the sidebar
| `W_DISABLED`       | Makes this widget disabled by default (only applies to top-level widgets/layouts)
| `W_DYNAMIC_HEIGHT` | Allows certain multi-line widgets to dynamically adjust their height


# Variables

Below are most of the available `widget_var` values and what they mean. See the `widget_var` list in
`widget.h` for the definitive list of available variables.

Many vars are numeric in nature. These may use style "number" or style "graph".  Some examples:

## Numeric variables

| var               | description
|--                 |--
| `cardio_acc`      | Cardio accumulator, integer
| `cardio_fit`      | Cardio fitness, integer near BMR
| `fatigue`         | tiredness, 0-600+
| `focus`           | focus level, 0-100+
| `health`          | Current hidden health value, -200 to +200
| `mana`            | available mana, 0-MAX_MANA
| `morale_level`    | morale level, -100 to +100
| `move`            | movement counter, 0-100+
| `pain`            | perceived pain, 0-80+
| `sound`           | sound, 0-20+
| `speed`           | speed, 0-500+
| `stamina`         | stamina reserves, 0-MAX (approx. 8700)
| `stat_dex`        | dexterity stat, 0-20+
| `stat_int`        | intelligence stat, 0-20+
| `stat_per`        | perception stat, 0-20+
| `stat_str`        | strength stat, 0-20+
| `weariness_level` | weariness level, 0-6+

Variables with a `bp_` prefix refer to body part variables, and require a "bodypart" field.
These variables have separate values for each part of the body, and include:

| var               | description
|--                 |--
| `bp_hp`           | hit points of given "bodypart", like "arm_l" or "torso", 0-MAX_HP
| `bp_encumb`       | encumbrance given "bodypart", 0-50+
| `bp_warmth`       | warmth of given "bodypart", 0-10000
| `bp_wetness`      | wetness of given "bodypart", 0-100+

In the `widget.cpp` code, `get_var_value` returns the numeric value of the widget's "var" variable,
which in turn is used for rendering numeric widgets as well as graphs of that value. Graphs are
rendered with reference to the maximum value for the variable, or "var_max" if none is known.


## Text variables

Some vars refer to text descriptors. These must use style "text". Examples:

| var                     | description
|--                       |--
| `activity_text`         | Activity level - "None", "Light". "Moderate", "Brisk", "Active", "Extreme"
| `body_temp_text`        | Felt body temperature "Comfortable", "Chilly", "Warm" etc.
| `bp_outer_armor_text`   | Item name and damage bars of armor/clothing worn on the given "bodypart"
| `bp_status_text`        | Status of given "bodypart" - "bitten", "bleeding", "infected", etc.
| `bp_status_sym_text`    | Same as above, but in a more compact format using 1 character per status.
| `bp_status_legend_text` | (_multiline_) Displays the meaning of the symbols from `bp_status_sym_text`
| `compass_legend_text`   | (_multiline_) A list of creatures visible by the player, corresponding to compass symbols
| `compass_text`          | A compass direction (ex: NE), displaying visible creatures in that direction
| `date_text`             | Current day within season, like "Summer, day 15"
| `env_temp_text`         | Environment temperature, if thermometer is available
| `fatigue_text`          | Fatigue level - "Tired", "Dead Tired", "Exhausted"
| `health_text`           | Hidden health - "OK", "Good", "Very good", "Bad", "Very bad", etc.
| `hunger_text`           | Hunger level - "Engorged", "Full", "Hungry", "Famished", etc.
| `lighting_text`         | Lighting conditions at avatar position - "bright", "cloudy", "dark" etc.
| `mood_text`             | Avatar mood represented as an emoticon face
| `moon_phase_text`       | Phase of the moon - "New moon", "Waxing gibbous", "Full moon" etc.
| `move_mode_letter`      | Movement mode - "W": walking, "R": running, "C": crouching, "P": prone
| `move_mode_text`        | Movement mode - "walking", "running", "crouching", "prone"
| `overmap_loc_text`      | Overmap coordinates, same as shown in the lower corner of overmap screen
| `overmap_text`          | (_multiline_) Colored text rendering of the local overmap; may define "width" and "height"
| `pain_text`             | "Mild pain", "Distracting pain", "Intense pain", etc.
| `place_text`            | Location place name
| `power_text`            | Bionic power available
| `rad_badge_text`        | Radiation badge color indicator, if radiation badge is available
| `safe_mode_text`        | Status of safe mode - "On" or "Off", with color for approaching turn limit
| `style_text`            | Name of current martial arts style
| `thirst_text`           | Thirst level - "Slaked", "Thirsty", "Dehydrated", etc.
| `time_text`             | Current time - exact if clock is available, approximate otherwise
| `veh_azimuth_text`      | Heading of vehicle in degrees
| `veh_cruise_text`       | Target and actual cruising velocity, positive or negative
| `veh_fuel_text`         | Percentage of fuel remaining for current vehicle engine
| `weariness_text`        | Weariness level - "Fresh", "Light", "Moderate", "Weary" etc.
| `weary_malus_text`      | Percentage penalty affecting speed due to weariness
| `weather_text`          | Weather conditions - "Sunny", "Cloudy", "Drizzle", "Portal Storm" etc.
| `weight_text`           | Body weight - "Emaciated", "Normal", "Overweight", etc.
| `wielding_text`         | Name of current weapon or wielded item
| `wind_text`             | Wind direction and intensity


# Predefined widgets

Many widgets for numbers, text, graphs, and layouts are already defined in `data/json/ui/`, and you
can save time customizing your sidebar by using these existing components. This section includes a
list and demo/mockup of many of them.


## Number widgets

Numerical widget ids typically have a `_num` suffix.

| id               | example
| --               | --
| `cardio_fit_num` | `Cardio Fit: 1750`
| `focus_num`      | `Focus: 100`
| `health_num`     | `Health: -20`
| `morale_num`     | `Morale: 95`
| `move_cost_num`  | `Move cost: 300`
| `move_num`       | `Move count: 150`
| `pain_num`       | `Pain: 15`
| `sound_num`      | `Sound: 8`
| `speed_num`      | `Speed: 100`
| `stamina_num`    | `Stamina: 8714`
| `str_num`        | `Str: 8`
| `dex_num`        | `Dex: 8`
| `int_num`        | `Int: 8`
| `per_num`        | `Per: 8`


## Graph widgets

Graph widget ids typically have a `_graph` suffix.

| id                      | example
| --                      | --
| `stamina_graph`         | `Stamina: \|\|\|\|\|\|\|\|\|\|`
| `stamina_graph_classic` | `Stam: \|\|\|\|\|`
| `hp_head_graph`         | `HEAD: \|\|\|\|\|`
| `hp_torso_graph`        | `TORSO: \|\|\|\|\|`
| `hp_left_arm_graph`     | `L ARM: \|\|\|\|\|`
| `hp_right_arm_graph`    | `L ARM: \|\|\|\|\|`
| `hp_left_leg_graph`     | `L LEG: \|\|\|\|\|`
| `hp_right_leg_graph`    | `L LEG: \|\|\|\|\|`


## Text widgets

Text widget ids typically have a `_desc` suffix.

| id                     | example
| --                     | --
| `activity_desc`        | `Activity: Moderate`
| `date_desc`            | `Date: Summer day 25`
| `env_temp_desc`        | `Temperature: 65F`
| `fatigue_desc`         | `Rest: Tired`
| `health_desc`          | `Health: Good`
| `hunger_desc`          | `Hunger: Satisfied`
| `lighting_desc`        | `Lighting: Bright`
| `mood_desc`            | `Mood: :-)`
| `move_count_mode_desc` | `Move: 100(W)`
| `pain_desc`            | `Pain: Unmanageable pain`
| `place_desc`           | `Place: Evac Shelter J-38`
| `power_desc`           | `Bionic Power: 250mJ`
| `style_desc`           | `Style: Brawling`
| `time_desc`            | `Time: 10:45:32 am`
| `weary_malus_desc`     | `Weary Malus: +10%`
| `weather_desc`         | `Weather: Sunny`
| `weight_desc`          | `Weight: Overweight`
| `wind_desc`            | `Wind: <= Calm`


## Layout widgets

Layout widget ids typically have a `_layout` suffix. Complex layouts may be composed of simpler
layouts, most often by building single-line layouts in "columns" first, then arranging several
"columns" layouts into a multi-line layout using a "rows" arrangement.


### Single-line layouts

This table gives some examples of single-line "columns" layouts:

| id | example
| -- | --
| `hitpoint_graphs_top_layout` | `L ARM: \|\|\|\|\|  HEAD:  \|\|\|\|\|  R ARM: \|\|\|\|\|`
| `hitpoint_graphs_bottom_layout` | `L LEG: \|\|\|\|\|  TORSO: \|\|\|\|\|  R LEG: \|\|\|\|\|`
| `hitpoints_head_torso_layout` | `HEAD:  \|\|\|\|\|  TORSO: \|\|\|\|\|`
| `hitpoints_arms_layout` | `L ARM: \|\|\|\|\|  R ARM: \|\|\|\|\|`
| `hitpoints_legs_layout` | `L LEG: \|\|\|\|\|  R LEG: \|\|\|\|\|`
| `mood_focus_layout` | `Mood: :-)  Focus: 100`
| `safe_sound_layout` | `Safe: Off  Sound:  15`
| `sound_fatigue_focus_layout` | `Sound:  15  Fatigue: Fresh  Focus: 100`
| `sound_focus_layout` | `Sound:  15  Focus: 100`
| `stats_layout` | `Str: 9  Dex: 8  Int: 10  Per: 7`


### Multi-line layouts

Below are examples of multi-line "rows" layouts:

#### `hitpoints_all_graphs_layout`

Combines `hitpoint_graphs_top_layout` and `hitpoint_graphs_bottom_layout` into a single layout
showing all body part hit points:

```
L ARM: |||||  HEAD:  |||||  R ARM: |||||
L LEG: |||||  TORSO: |||||  R LEG: |||||
```

#### `hitpoints_all_narrow_graphs_layout`

Alternative hitpoint graph, better for narrower sidebars. Combines `hitpoints_head_torso_layout`,
`hitpoints_arms_layout`, and `hitpoints_legs_layout`

```
HEAD:  |||||  TORSO: |||||
L ARM: |||||  R ARM: |||||
L LEG: |||||  R LEG: |||||
```

#### `compass_all_layout`

Full compass rose showing colored symbols for nearby monsters, along with a legend for what monster
names belong to each symbol.

```
NW:   zZZ  N:     a  NE:    ZZ
NW:               Z  E:     da
SW:   ddd  S:        SE:  zZZZ
a 2 wasps  d 5 zombie dogs
Z 6 zombies  z 2 zombie cops
```


# Modding the Sidebar

One great advantage of a data-driven, JSON-based sidebar is that it can be customized with mods. A
mod may extend the main "custom" sidebar with mod-specific widgets, or define an entirely new
sidebar if desired.

For example, the Magiclysm mod extends the main custom sidebar with two optional mana widgets. It
does this in a JSON file in the mod directory, `data/mods/Magiclysm/ui/sidebar.json`, by defining a
few custom widgets, then using "copy-from" and "extend" on the custom sidebar object:

```json
[
  {
    "copy-from": "custom_sidebar",
    "type": "widget",
    "id": "custom_sidebar",
    "//": "Extend the custom sidebar with Magiclysm-specific sections",
    "extend": { "widgets": [ "current_max_mana_nums_layout", "mana_graph_layout" ] }
  }
]
```

These two extra widgets, "current_max_mana_nums_layout" and "mana_graph_layout", will be appended to
the custom sidebar sections whenever a game with the Magiclysm mod is loaded.

