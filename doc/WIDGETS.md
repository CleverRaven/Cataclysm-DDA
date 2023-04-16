<!-- START doctoc generated TOC please keep comment here to allow auto update -->
<!-- DON'T EDIT THIS SECTION, INSTEAD RE-RUN doctoc TO UPDATE -->
**Table of Contents**  *generated with [DocToc](https://github.com/thlorenz/doctoc)*

- [Sidebar Modification](#sidebar-modification)
- [Widgets](#widgets)
  - [Required fields](#required-fields)
  - [Optional fields](#optional-fields)
- [Styles](#styles)
  - [Sidebar style](#sidebar-style)
  - [Layout style](#layout-style)
  - [Variable widgets](#variable-widgets)
    - [Number style](#number-style)
    - [Graph style](#graph-style)
    - [Text style](#text-style)
- [Fields](#fields)
  - [label](#label)
  - [string](#string)
  - [fill](#fill)
  - [style](#style)
  - [direction](#direction)
  - [height](#height)
  - [text_align and label_align](#text_align-and-label_align)
  - [colors](#colors)
  - [flags](#flags)
- [Clauses and conditions](#clauses-and-conditions)
  - [Conditions](#conditions)
  - [Default clause](#default-clause)
- [Variable ranges](#variable-ranges)
- [Variables](#variables)
  - [Numeric variables](#numeric-variables)
  - [Text variables](#text-variables)
- [Predefined widgets](#predefined-widgets)
  - [Number widgets](#number-widgets)
  - [Graph widgets](#graph-widgets)
  - [Text widgets](#text-widgets)
  - [Layout widgets](#layout-widgets)
    - [Single-line layouts](#single-line-layouts)
    - [Multi-line layouts](#multi-line-layouts)
      - [hitpoints_all_graphs_layout](#hitpoints_all_graphs_layout)
      - [hitpoints_all_narrow_graphs_layout](#hitpoints_all_narrow_graphs_layout)
      - [compass_all_layout](#compass_all_layout)
  - [Modding the Sidebar](#modding-the-sidebar)

<!-- END doctoc generated TOC please keep comment here to allow auto update -->

# Sidebar Modification

Most of the main CDDA sidebar is now moddable, meaning it is data-driven and can be customized
simply by editing JSON files, without recompiling the game.

From the in-game Sidebar Options menu `}`, select the "custom" layout to switch to a basic moddable
theme built from the "custom_sidebar" widget defined in `data/json/ui/sidebar.json`, with sections
you can toggle or rearrange in-game according to your preference.

You can deeply customize the look of your sidebar widgets, by directly modifying the JSON that
defines them in `data/json/ui`, or by creating your own sidebar mod in `data/mods`. This document is
all about widgets: what they do, and how to use them.


# Widgets

All "custom" sidebar UI elements are defined in objects called widgets. A widget can display a
variety of player character attributes in numeric or text form, or as a bar graph of arbitrary
width or height. A widget can also group other widgets together in a horizontal or vertical layout.

Widget instances are defined by JSON data, with the main game sidebar widgets and layouts being in
`data/json/ui/sidebar.json`. You may customize yours by editing this file, or by loading a mod that
adds or modifies widget definitions (see [Modding the sidebar](#modding-the-sidebar)).

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

And how it may look in-game:

```
Focus: 105
```

The [label](#label), [var](#variables), and [style](#style) fields define what info is shown, and
how.  Several other fields provide more detailed customization, depending on context.


## Required fields

Two fields are required for all widgets:

| field | description
| --    | --
| id    | Unique identifier for this widget, usually like "lowercase_with_underscores"
| type  | Must be set to "widget" for all widget objects.

**NOTE** For cleanliness and readability, many JSON examples in this document omit "id" and "type",
because they are always required. Assume they are implied in all examples, unless otherwise noted:

```json
{
  "id": "some_unique_id",
  "type": "widget",

  "//": "...and so on"
}
```


## Optional fields

This table lists all other widget fields and what they do. Many are explained in more detail in the
linked sections:

| field                   | type                  | description
| --                      | --                    | --
| arrange                 | string                | For "layout" style, display child widgets as "rows", "columns" or "minimum_columns"; for ["graph style"](#graph-style) draw vertical ("rows") or horizontal ("columns")
| bodypart                | string                | For "bp_*" variables, body part id like "leg_r" or "torso"
| separator               | string                | The string used to separate the label from the widget data. Children will inherit if this is not defined. Mandatory if style is "sidebar".
| padding                 | int                   | Amount of padding between columns for this widget. Children will inherit if this is not defined. Mandatory if style is "sidebar".
| [colors](#colors)       | list of strings       | Color names in a spectrum across variable range, or a single color for text widgets
| [breaks](#breaks)       | list of integers      | Color breaks as percentages in the variable range. Optional, overwrites default algorithm.                                                 |
| [direction](#direction) | string                | Cardinal compass direction like "N" or "SE"
| [fill](#fill)           | string                | For [graph style](#graph-style), fill using ike "bucket" or "pool"
| [flags](#flags)         |                       | list of strings | Optional toggles
| [height](#height)       | integer               | Maximum number of lines of text to take up
| [label](#label)         | string or translation | Visible descriptor or heading
| [string](#string)       | string or translation | Visible descriptor or heading. Used as a last resort by "text" style widgets. Mandatory if the widget has no clauses.
| [clauses](#clauses-and-conditions) | list of objects | Arbitrary conditional expressions mapped to colored text, symbols, or numbers
| [style](#style)         | string                | Sub-type or visual theme: "number", "graph", "text", "layout"
| symbols                 | string                | For [graph style](#graph-style), text characters for ascending values
| var                     | string                | Variable name from `widget_var`; see [Variables](#variables)
| widgets                 | list of strings       | For "layout" and "sidebar" style, list of string IDs of child widgets.
| width                   | integer               | Total width in characters or symbols.
| [label_align and text_align](#text_align-and-label_align) | string | How to orient the label and value: "left", "center", or "right"
| [pad_labels](#pad_labels) | bool                | Aligns values in layouts by padding to the longest label

See [Fields](#fields) for details.


# Styles

Two widget [styles](#style) are for high-level organization and layout: [sidebar](#sidebar-style)
and [layout](#layout-style); three others are [variable widgets](#variable-widgets) for displaying
specific information: [number](#number-style), [graph](#graph-style), and [text](#text-style).

This section describes them in a top-down fashion.


## Sidebar style

The highest-level widget is the "sidebar", which represents the entire display region on the right
(or left) edge of the screen. It includes a "width" in characters, a "label" displayed in the
sidebar options menu, and a list of "widgets", shown as sections that may be rearranged or
toggled from the sidebar options menu.

These sub-widgets are typically [layout widgets](#layout-style), with other widgets arranged
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
showing some piece of information (with a label); and [layout widgets](#layout-style), used for
arranging other widgets in rows or columns.

We will look at layout widgets first, since they are easier to explain.


## Layout style

Use widgets with "style": "layout" to arrange child widgets in sidebar panels, giving widget ids in
the "widgets" list field.

The arrangement of child widgets is defined by the "arrange" field, which may be "columns" (default)
to array widgets horizontally, or "rows" to arrange them vertically, one widget per row.  Normal columns
will split their horizontal space as equally as possible. Whereas minimum_columns will take their exact
amount of space (defaulting to space split like columns) with the last column in the row taking all remaining space.

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

These layout widgets can be nested to produce web-style layouts:

```json
[
  {
    "id": "overmap_5x5",
    "type": "widget",
    "var": "overmap_text",
    "style": "text",
    "width": 5,
    "height": 5,
    "flags": [ "W_LABEL_NONE" ]
  },
  {
    "id": "location_text_layout",
    "type": "widget",
    "style": "layout",
    "arrange": "rows",
    "widgets": [ "lighting_desc", "moon_phase_desc", "wind_desc", "env_temp_desc" ]
  },
  {
    "id": "layout_location_columns",
    "type": "widget",
    "style": "layout",
    "arrange": "columns",
    "label": "Location",
    "widgets": [ "overmap_5x5", "location_text_layout" ],
    "flags": [ "W_LABEL_NONE" ]
  }
]
```

The above would produce something like:

```
FFF..  Lighting:    bright
FF...  Moon:        Waxing crescent
FF..P  Wind:        Light Breeze =>
F...|  Temperature: 16C
F...|
```

Where do all these numeric widgets and their values come from? These are variable widgets, discussed
next.


## Variable widgets

Variable widgets define a "var" field, with the name of a predefined widget variable. This tells the
widget what information it should show. Most of the time, these are attributes of the player
character, but they can also be attributes of the world, environment, or vehicle where they are.

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

Some widgets can take advantage of multiple "bodyparts" like so:

```json
{
  "bodyparts": [ "head", "torso", "arm_l", "arm_r" ]
}
```

See [Variables](#variables) for a list of available "var" values.


### Number style

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

The numeric value comes from the given "var", displayed as a decimal integer. By default it will be
plain gray, but providing a spectrum of [colors](#colors), will colorize the number based on the
[variable range](#variable-ranges) of the given "var".

See [Number widgets](#number-widgets) for several pre-defined numeric widgets you can use or extend.


### Graph style

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

See the [fill](#fill) and [colors](#colors) fields for more ways to customize the graph, and see
[Variable ranges](#variable-ranges) for details on how the minimum and maximum extents of the graph
are determined.

Also see [Graph widgets](#graph-widgets) for some predefined ones you can use or extend.

#### Vertical graphs

By setting the `arrange` property to `rows`, graphs can be displayed vertically.
For vertical graphs, `height` should be used instead of `width`.

```json
{
  "arrange": "rows",
  "height": 5,
  "width": 1,
  "symbols": ".▁▂▃▄▅▆▇█"
}
```

> Note: As with other multi-line widgets, the `width` needs to be set to achieve narrow packing.

Vertical graphs do not work well with the `label` property.
Best to disable labels, and make a custom `text` style widget to place above or below bars.

### Text style

Text style widgets display text. They can be very powerful, but are also pretty complex.

The simplest text widget is one that displays static text using the `string` field. If a text widget
does not have any clauses or a `var` field, it _must_ have the `string` field. The widget below
displays a single dot.
```JSON
{
  "id": "lcom_spacer",
  "type": "widget",
  "style": "text",
  "string": ".",
  "flags": [ "W_LABEL_NONE" ]
}
```

In the vast majority of cases, text widgets will display text conditionally using [clauses](#clauses-and-conditions).
These clauses use dialogue conditions to determine what text to show and in what color.
The below widget is a prime example of a text widget, and is used to display a player's thirst level.
```JSON
{
  "id": "thirst_desc_label",
  "type": "widget",
  "label": "Thirst",
  "style": "text",
  "clauses": [
    {
      "id": "parched",
      "text": "Parched",
      "color": "light_red",
      "condition": { "compare_num": [ { "u_val": "thirst" }, ">", { "const": 520 } ] }
    },
    {
      "id": "dehydrated",
      "text": "Dehydrated",
      "color": "light_red",
      "condition": {
        "and": [
          { "compare_num": [ { "u_val": "thirst" }, ">", { "const": 240 } ] },
          { "compare_num": [ { "u_val": "thirst" }, "<=", { "const": 520 } ] }
        ]
      }
    },
    {
      "id": "very_thirsty",
      "text": "Very thirsty",
      "color": "yellow",
      "condition": {
        "and": [
          { "compare_num": [ { "u_val": "thirst" }, ">", { "const": 80 } ] },
          { "compare_num": [ { "u_val": "thirst" }, "<=", { "const": 240 } ] }
        ]
      }
    },
    {
      "id": "thirsty",
      "text": "Thirsty",
      "color": "yellow",
      "condition": {
        "and": [
          { "compare_num": [ { "u_val": "thirst" }, ">", { "const": 40 } ] },
          { "compare_num": [ { "u_val": "thirst" }, "<=", { "const": 80 } ] }
        ]
      }
    },
    {
      "id": "neutral",
      "text": "",
      "color": "white",
      "condition": {
        "and": [
          { "compare_num": [ { "u_val": "thirst" }, ">=", { "const": 0 } ] },
          { "compare_num": [ { "u_val": "thirst" }, "<=", { "const": 40 } ] }
        ]
      }
    },
    {
      "id": "slaked",
      "text": "Slaked",
      "color": "green",
      "condition": {
        "and": [
          { "compare_num": [ { "u_val": "thirst" }, ">=", { "const": -20 } ] },
          { "compare_num": [ { "u_val": "thirst" }, "<", { "const": 0 } ] }
        ]
      }
    },
    {
      "id": "hydrated",
      "text": "Hydrated",
      "color": "green",
      "condition": {
        "and": [
          { "compare_num": [ { "u_val": "thirst" }, ">=", { "const": -60 } ] },
          { "compare_num": [ { "u_val": "thirst" }, "<", { "const": -20 } ] }
        ]
      }
    },
    {
      "id": "turgid",
      "text": "Turgid",
      "color": "green",
      "condition": { "compare_num": [ { "u_val": "thirst" }, "<", { "const": -60 } ] }
    }
  ]
},
```

See [Text widgets](#text-widgets) for a variety of predefined text widgets you can use or extend.


# Fields

## label

The "label" is the word or phrase that appears in the UI to identify this widget. For "number",
"graph", or "text" widgets, the label is shown to the left of the the value unless the
`W_LABEL_NONE` [flag](#flags) is given. For "layout" widgets, the label may appear as the name of a
section within the sidebar.

Labels may be a plain string:

```JSON
{
  "id": "sound_num",
  "label": "Sound",
  "var": "sound"
}
```

Or it may define a translatable string object, as commonly seen in item names:

```JSON
{
  "id": "place_name",
  "label": { "str": "Place", "ctxt": "location" },
  "var": "place_text"
}
```

The English word "place" can be a verb, to put something down. Here "place" is a noun meaning a
location. The "ctxt" part provides this context to translators so they can choose the most
appropriate words in other languages.

See the [Translatable strings section of JSON_INFO.md](JSON_INFO.md#translatable-strings)
for more on how these work.

## string

If you have a `text` style widget that has no other options for what to display, it must have a
`string` field to display instead. This will cause the widget to display a static string.

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




## style

The "style" field says what kind of info this widget shows or how it will be rendered. It may be:

- `number`: Show value as a plain integer number
- `graph`: Show a bar graph of the value with colored text characters
- `text`: Show text from a `*_text` variable
- `layout`: Layout container for arranging other widgets in rows or columns
- `sidebar`: Special top-level widget for defining custom sidebars, having several layouts

"style" can also be `symbol` or `legend`, which are specific to [clauses](#clauses-and-conditions).


## direction

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


## height

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


## text_align and label_align

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

Values may be "left", "right", or "center". The default is "left" alignment for both labels and text.

## pad_labels

In layouts, values can be aligned to match the longest label:

```plaintext
Mood:     :)
Morale:   95
Activity: Brisk
```

`pad_labels` can be used on layouts to enable/disable label padding of child widgets.
It can also be used on non-layout widgets, to disable alignment individually.

```json
{
  "pad_labels": true
}
```

Defaults to `true` for row layouts and all non-layout widgets. Defaults to `false` for column layouts.

## colors

Widgets with "number" or "graph" style may define "colors", which will be used as a spectrum across
the widget's values (`var_min` to `var_max`), applying the appropriate color to each value based on
the [Variable range](#variable-ranges) of the specified "var".

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
  "colors": [ "c_green", "c_white", "c_red" ]
}
```

Color names may be any of those described in [COLOR.md](COLOR.md). You can also see the available
colors in-game from the "Settings" menu, under "Colors".

Graphs can be colorized in the same way. For example, the classic stamina graph is a 5-character
bar, a dark green `|||||` when full. As stamina diminishes, the bar's color goes to light green,
yellow, light red, and red. Such coloration could be represented with "colors" like so:

```json
{
  "id": "stamina_graph_classic",
  "type": "widget",
  "label": "Stam",
  "var": "stamina",
  "style": "graph",
  "width": 5,
  "symbols": ".\\|",
  "colors": [ "c_red", "c_light_red", "c_yellow", "c_light_green", "c_green" ]
}
```

The number of colors you use is arbitrary; the [range of possible values](#variable-ranges) will be
mapped as closely as possible to the spectrum of colors, with one exception - variables with a
"normal" value or range always use white (`c_white`) when the value is within normal.

The color scale can be further customized using [`breaks`](#breaks).

Widgets with "text" style can specify a single-element list of colors to overwrite the text color.
Here is an example of colored place test:

```json
{
  "id": "place_green",
  "type": "widget",
  "style": "text",
  "label": "Place",
  "var": "place_text",
  "colors": [ "c_green" ]
}
```

## breaks

Color scales for widgets with "number" or "graph" style can be further customized by defining `breaks`.
There must be one break less than the number of colors.

For example, you may want the stamina bar to turn red at much higher values already, as a warning sign:

```json
{
  "id": "stamina_graph_classic",
  "type": "widget",
  "label": "Stam",
  "var": "stamina",
  "style": "graph",
  "width": 5,
  "symbols": ".\\|",
  "colors": [ "c_red", "c_light_red", "c_yellow", "c_light_green", "c_green" ],
  "breaks":[ 50, 70, 90, 95 ]
}
```

Breaks are percentages in the spectrum across the widget's values (`var_min` to `var_max`).
So, 0 stands for `var_min` and 100 for `var_max`. Values <0 and >100 are allowed.

## flags

Widgets can use flags to specify special behaviors:

```json
{
  "id": "my_widget",
  "type": "widget",
  "style": "text",
  "label": "My Widget",
  "var": "my_widget_var",
  "flags": [ "W_LABEL_NONE", "W_DISABLED_BY_DEFAULT" ]
}
```

Here are the flags that can be included:

| Flag id                 | Description
|---                      |---
| `W_LABEL_NONE`          | Prevents the widget's label from being displayed in the sidebar
| `W_DISABLED_BY_DEFAULT` | Makes this widget disabled by default (only applies to top-level widgets/layouts)
| `W_DISABLED_WHEN_EMPTY` | Automatically hides this widget when the widget's text is empty
| `W_DYNAMIC_HEIGHT`      | Allows certain multi-line widgets to dynamically adjust their height
| `W_NO_PADDING`          | Prevents the sidebar from doing any sort of whitespace-based alignment. All widgets are packed as tightly as possible. Use this flag only if you plan to align things yourself.


# Clauses and conditions

Widgets can take advantage of "clauses" - definitions for what text/values to display and
how to display them. These take the form of a nested object containing several optional fields:

```json
{
  "id": "bp_status_indicator_template",
  "type": "widget",
  "style": "text",
  "clauses": [
    { "id": "bitten", "text": "bitten", "sym": "B", "color": "yellow", "condition": "..." },
    { "id": "infected", "text": "infected", "sym": "I", "color": "pink", "condition": "..." },
    { "id": "bandaged", "text": "bandaged", "sym": "+", "color": "white", "condition": "..." }
  ]
}
```

In the above example, the widget is simply used as a template for other widgets to `copy-from`,
which provides text and color definitions for different bodypart status conditions.

| JSON Field  | Description
|---          |---
| `id`        | An optional identifier for this clause
| `text`      | Translated text that may be interpreted and displayed in the widget.
| `sym`       | A shortened symbol representing the text.
| `color`     | Defines the color for the text derived from this "clause".
| `value`     | A numeric value for this "clause", which may be interpreted differently based on the context of the parent widget.
| `widgets`   | For "layout" style widgets, the child widgets used for this "clause".
| `condition` | A dialogue condition (see [Dialogue conditions](NPCs.md#dialogue-conditions)) that dictates whether this clause will be used or not. If the condition is true (or when no condition is defined), the clause can be used to its text/symbol/color in the widget's value.


## Conditions

Widget clauses and conditions can be used to define new widgets completely from JSON, using
[dialogue conditions](NPCs.md#dialogue-conditions). By omitting the widget's `var` field, the
widget is interpreted as either a "text", "number", "symbol", or "legend" depending on the given
`style`. The widget will evaluate each of its clauses to determine which ones to draw values from:

| Widget style | Clause field used    | Details | Example
|---           |---                   |---      |---
| `"number"`   | `"value"`            | Lists values as comma-separated-values from all clauses that have true conditions. | `Next threshold: 30, 40, 55`
| `"text"`     | `"text"`             | Lists text as comma-separated-values from all clauses that have true conditions. | `TORSO: bleeding, broken, infected`
| `"symbol"`   | `"sym"`              | Lists syms sequentially from all clauses that have true conditions. | `TORSO: b%I`
| `"legend"`   | `"sym"` and `"text"` | Lists syms and text in a paragraph format, with spaces between pairs, from all clauses that have true conditions. | `b bleeding  % broken  I infected`

Widgets using the `legend` style can be multiple lines high using a `height` > 1 (and optionally, the `W_DYNAMIC_HEIGHT` flag), so that the generated list can span the given vertical space.

Some conditions can be specific to certain bodyparts. In order to simplify clauses, these conditions can pull from the parent widget's `bodypart` field (or `bodyparts` field if defining multiple). This allows the same clauses to be `copy-from`'d to multiple widgets, and each widget can display the clauses depending on whether its bodypart(s) passes the condition (assuming the condition relies on a bodypart).


## Default clause

Widgets can define a default clause that will be used if none of the clauses in the `clauses`
array pass their conditions:

```json
{
  "id": "observ_widget",
  "type": "widget",
  "style": "text",
  "label": "Observation",
  "clauses": [
    {
      "text": "Good!",
      "color": "light_green",
      "condition": { "u_has_trait": "EAGLEEYED" }
    },
    {
      "text": "Bad!",
      "color": "light_red",
      "condition": { "u_has_trait": "UNOBSERVANT" }
    }
  ],
  "default_clause": {
    "text": "Neutral!",
    "color": "white"
  }
}
```

In the example above, the widget would print out the following text:

| Player has trait | Widget text
|---               |---
| Scout            | `Observation: Good!`
| Topographagnosia | `Observation: Bad!`
| -                | `Observation: Neutral!`


# Variable ranges

Widgets using a numeric "var" (those without a `_text` suffix) have a predetermined absolute range
(minimum and maximum), as well as a predermined normal value or range.  These limits are not
customizable in widget JSON, but knowing about them will make it easier to understand how "graph"
widgets are drawn, and how the "colors" list is mapped to the variable's numeric range.

Within the code, these three `widget` class attributes store the variable range info:

- `var_norm`: Range (minimum, maximum) of normal or baseline `var` values
- `var_min`: Value of `var` mapped to the zero-point of graphs, and the lowest-index color
- `var_max`: Value of `var` mapped to the full-point of graphs, and the highest-index color

All these values are integer numbers only, not floating-point numbers. They may be negative.

The `var_norm` range defines what value(s) of `var` are considered normal, average, or baseline.
For a character starting with 9 STR, their `var_norm` for the `stat_str` variable will be set to
`(9, 9)`. When the character's STR is in the normal range, it will be displayed in white.

Usually, `var_min` is simply 0, but some variables such as hidden health have a negative minimum
value (-200 in this case). When using "colors", the `var_min` value is mapped to the first color.
This is not necessarily the absolute minimum value that the variable can have; it is only the
minimum value displayable on a graph, and below which the color stays fixed at the first color.

All widgets have some positive `var_max` value, again depending on what `var` is being displayed.
This helps graph widgets know whether they must show values up to 7000 or more (like stamina) or
only up to 100 or 200 (like focus). It also determines the value mapped to the last color in
"colors", if given. Again, this is not an absolute maximum; "number" widgets will continue to
display numbers far in excess of the `var_max`, but "graph" widgets will stop increasing at this
value, and the color will stay at the last color.

These ranges may change dynamically during gameplay. For instance, as cardio fitness increases from
day to day, the `var_max` of corresponding cardio widgets must reflect this. Other variables with a
potentially dynamic `var_max` include "stamina", "mana", and "bp_hp".

Likewise, when a character's STR stat increases from a mutation, the `var_norm` of corresponding
widgets must adjust.  Variables using "var_norm" include the stat attributes "stat_str", "stat_dex",
"stat_int", and "stat_per".


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
| `move_remainder`  | remaining moves for the current turn, 0-9999+
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
rendered with reference to the maximum value for the variable; see [Variable ranges](#variable-ranges).


## Text variables

Some vars refer to text descriptors. These must use style "text". Examples:

| var                      | description
|--                        |--
| `activity_text`          | Activity level - "None", "Light". "Moderate", "Brisk", "Active", "Extreme"
| `bp_outer_armor_text`    | Item name and damage bars of armor/clothing worn on the given "bodypart"
| `compass_legend_text`    | (_multiline_) A list of creatures visible by the player, corresponding to compass symbols
| `compass_text`           | A compass direction (ex: NE), displaying visible creatures in that direction
| `date_text`              | Current day within season, like "Summer, day 15"
| `env_temp_text`          | Environment temperature, if thermometer is available
| `mood_text`              | Avatar mood represented as an emoticon face
| `move_mode_letter`       | Movement mode - "W": walking, "R": running, "C": crouching, "P": prone
| `move_mode_text`         | Movement mode - "walking", "running", "crouching", "prone"
| `overmap_loc_text`       | Overmap coordinates, same as shown in the lower corner of overmap screen
| `overmap_text`           | (_multiline_) Colored text rendering of the local overmap; may define "width" and "height"
| `pain_text`              | "Mild pain", "Distracting pain", "Intense pain", etc.
| `place_text`             | Location place name
| `power_text`             | Bionic power available
| `safe_mode_text`         | Status of safe mode - "On" or "Off", with color for approaching turn limit
| `safe_mode_classic_text` | Status of safe mode - "SAFE", with color for approaching turn limit
| `style_text`             | Name of current martial arts style
| `sundial_text`           | Current position of the Sun/Moon in the sky
| `time_text`              | Current time - exact if clock is available, approximate otherwise
| `veh_azimuth_text`       | Heading of vehicle in degrees
| `veh_cruise_text`        | Target and actual cruising velocity, positive or negative
| `veh_fuel_text`          | Percentage of fuel remaining for current vehicle engine
| `weariness_text`         | Weariness level - "Fresh", "Light", "Moderate", "Weary" etc.
| `weary_malus_text`       | Percentage penalty affecting speed due to weariness
| `weather_text`           | Weather conditions - "Sunny", "Cloudy", "Drizzle", "Portal Storm" etc.
| `wielding_text`          | Name of current weapon or wielded item
| `wind_text`              | Wind direction and intensity


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

#### hitpoints_all_graphs_layout

Combines `hitpoint_graphs_top_layout` and `hitpoint_graphs_bottom_layout` into a single layout
showing all body part hit points:

```
L ARM: |||||  HEAD:  |||||  R ARM: |||||
L LEG: |||||  TORSO: |||||  R LEG: |||||
```

#### hitpoints_all_narrow_graphs_layout

Alternative hitpoint graph, better for narrower sidebars. Combines `hitpoints_head_torso_layout`,
`hitpoints_arms_layout`, and `hitpoints_legs_layout`

```
HEAD:  |||||  TORSO: |||||
L ARM: |||||  R ARM: |||||
L LEG: |||||  R LEG: |||||
```

#### compass_all_layout

Full compass rose showing colored symbols for nearby monsters, along with a legend for what monster
names belong to each symbol.

```
NW:   zZZ  N:     a  NE:    ZZ
NW:               Z  E:     da
SW:   ddd  S:        SE:  zZZZ
a 2 wasps  d 5 zombie dogs
Z 6 zombies  z 2 zombie cops
```


## Modding the Sidebar

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
