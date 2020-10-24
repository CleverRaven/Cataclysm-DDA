Data-Driven Moddable Sidebar
====

Sidebar UI elements are defined in objects called widgets. A widget can display a value as a number,
sequence of phrases, or bar graph of arbitrary width; or a widget can make a layout of other widgets.

All widgets must have a unique "id", and "type: "widget".

Widgets have the following "style" options:

- "number": Display value as a plain integer number
- "phrase": Use words or phrases to represent different values
- "graph": Show a bar graph of the value with colored text characters
- "layout": Special style; this widget will be a layout container for other widgets

Widgets that show data (number, phrase, or graph) may be bound to variables with the "var" field.


## Widget variables

The "var" field of a widget tells what variable data gives the widget its value.  Valid var names
are given by the `widget_var` enum defined in `widget.h`. In the widget's `show` method, these var
enums determine which avatar method(s) to get their values from.

Here are most vars and what they mean. See `widget_var` in `widget.h` for the definitive list.

| var           | description
|--             |--
| `bp_hp`       | HP of given "bodypart", like "arm_l" or "torso", 0-MAX_HP
| `bp_encumb`   | encumbrance given "bodypart", 0-50+
| `bp_warmth`   | warmth of given "bodypart", 0-10000
| `stat_str`    | strength stat, 0-20+
| `stat_dex`    | dexterity stat, 0-20+
| `stat_int`    | intelligence stat, 0-20+
| `stat_per`    | perception stat, 0-20+
| `stamina`     | stamina reserves, 0-10000
| `fatigue`     | tiredness, 0-1000
| `weariness`   | weariness level, 0-6+
| `move`        | movement counter, 0-100+
| `pain`        | perceived pain, 0+
| `focus`       | focus level, 0-100+
| `speed`       | speed, 0-500+
| `sound`       | sound, 0-20+
| `mana`        | available mana, 0-MAX_MANA

In the `widget.cpp` code, `get_var_value` returns the numeric value of the widget's "var" variable,
which in turn is used for rendering numeric widgets as well as graphs of that value. Graphs are
rendered with reference to the maximum value for the variable, or "var_max" if none is known.


### var_max

Using "graph" style widgets, usually you should provide a "var_max" value (integer) that tells the
maximum typical value of "var" that will ever be rendered.

Some "var" fields such as "stamina", or "hp_bp" (hit points for body part) have a known maximum, but
others like character stats, move speed, or encumbrance have no predefined cap - for these you can
provide an explicit "var_max" that indicates where the top / full point of the graph is.

This helps the graph widget know whether it needs to show values up to 10000 (like stamina) or only
up to 100 or 200 (like focus). For a stat that usually varies within a range `[low, high]`, select a
"var_max" greater than `high` to be sure the normal variance is captured in the graph's range.

You may also define "var_min" if it's relevant. By default this is 0.


## Number widget

The simplest and usually most compact widget for displaying a value, "style": "number" appears as a
label with an integer number.

```
"style": "number",
"label": "Focus",

Focus: 100
```

The numeric value comes from the pseudo-function given by "var".



## Phrase widget

*Wishlist*

With "style": "phrase", numeric values are mapped to arbitrary words or phrases, letters, emoticons
or whatever.

For example, given a "var" that returns mood values 0, 1, or 2, with 0 being the worst mood and 2
being the best, you could define phrase widgets to show the same mood value in two different ways:

```
"id": "mood_words",
"style": "phrase",
"label": "Mood",
"strings": [ "Bummed", "Meh", "Squee!" ]

[0] Mood: Bummed
[1] Mood: Meh
[2] Mood: Squee!

"id": "mood_emote",
"style": "phrase",
"label": "Mood",
"strings": [ "T_T", "o_o", "^_^" ]

[0] Mood: T_T
[1] Mood: o_o
[2] Mood: ^_^

```

More familiar in-game are hunger, thirst, body temperature and pain indicators.


## Graph widget

The graph shows an arrangement of symbols. It has two important parameters:

- "width": how many characters wide is the graph
- "symbols": single-character strings to map to 0-N values

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
---
=--
==-
===
```

With three symbols, "-", "=", and "#", using the same width:

```json
{
    "width": 3,
    "symbols": "-=#"
}
```

The number of states increases to 7 - the zero state with all "-", plus two layers of filling with
"=" and "#":

```
---
=--
==-
===
#==
##=
###
```

Each symbol corresponds to a numeric value, starting with 0, so a graph can be visualized
numerically by replacing each symbol with its numerical index:

```
--- 000
=-- 100
==- 110
=== 111
#== 211
##= 221
### 222
```

When using more than two sybols, different ways of filling up the graph become possible. The method
can be specified with the "fill" field.


### fill

With "bucket" fill, positions are filled like a row of buckets, using all symbols in the first
position before beginning to fill the next position.  This is like the classic 5-bar HP meter.

```json
{
    "width": 5,
    "symbols": ".\\|",
    "fill": "bucket"
}
```

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

The total number of graph states is the same for both fills, so they have the same resolution.


## Layout widget

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
    "id": "root_layout",
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

The widget with id "root_layout" defines the top-level layout widget for all sidebar customization.


## Colors

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


