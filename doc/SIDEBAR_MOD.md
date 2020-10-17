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

Each widget is bound to a special "var" name, referring to where its value should come from.


## Widget variables

The "var" field of a widget tells what variable data gives the widget its value.  Valid var names
are given by the `widget_var` enum defined in `widget.h`. In the widget's `show` method, these var
enums determine which avatar method(s) to get their values from.

Below are a few examples of vars and what they mean. See the `widget_var` list in `widget.h` for the
definitive list of vars.

- `bp_hp`: HP of given "bodypart", like "arm_l" or "torso", scale of 0-max HP
- `bp_encumb`: encumbrance given "bodypart", scale of 0-??
- `bp_warmth`: warmth of given "bodypart", scale of 0-10000
- `stat_(str|dex|int|per)`: base character stat values
- `stamina`: 0-10000, greater is fuller stamina reserves
- `fatigue`: 0-1000, greater is more fatigued/tired
- `move`, `pain`, `speed`, `mana`: other numeric avatar attributes


### var_max

Using "graph" style widgets, usually you should provide a "var_max" value (integer) that tells the
maximum typical value of "var" that will ever be rendered.

Some "var" fields such as "stamina", or "hp_bp" (hit points for body part) have a known maximum, but
others like character stats, move speed, or encumbrance have no predefined cap - for these you can
provide an explicit "var_max" that indicates where the top / full point of the graph is.

This helps the graph widget know whether it needs to show values up to 10000 (like stamina) or only
up to 100 or 200 (like focus). For a stat that usually varies within a range `[low, high]`, select a
"var_max" greater than `high` to be sure the normal variance is captured in the graph's range.


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
- "symbols": single-character strings to map to 0-N

Given a graph of width 3 with two symbols, "-" and "=":

```
"width": 3,
"symbols": "-="
```

Each symbol is mapped to a numeric value, starting with 0, so a graph can be represented numerically
by replacing each symbol with its numerical index:

```
--- 000
=-- 100
==- 110
=== 111
```

With three symbols, "-", "=", and "#":


```
"width": 3,
"symbols": "-=#"
```

The numeric values range from 0 to 2:

```
--- 000
=-- 100
==- 110
=== 111
#== 211
##= 221
### 222
```

When using more than two sybols, different ways of filling up the graph become possible. This is
specified with the "fill" field.


### fill

With "bucket" fill, positions are filled like a row of buckets, using all symbols in the first
position before beginning to fill the next position.  This is like the classic 5-bar HP meter.

```
"width": 5,
"symbols": ".\\|",
"fill": "bucket"

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

```
"width": 5,
"symbols": "-=#",
"fill": "pool"

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


### start

*Wishlist*

Use the "start" field to change which side the bar starts filling from

```
"start": "left" (default)

--- 000
x-- 100
xx- 110
xxx 111

"start": "right"

--- 000
--x 001
-xx 011
xxx 111
```

The simplest graph is one character wide, with one symbol:

```
"width": 1,
"symbols": "-"
```

The simplest *useful* graph is one character wide, with two symbols:

```
"width": 1,
"symbols": "-="
```


## Layout widget

Lay out widgets with "style": "layout" widgets, providing a "widgets" list of widget ids or a
"layout" object with row ids mapping to widget id lists. Widgets in the same row will have their
horizontal space split equally if possible.

The arrangement of widgets is defined by the "arrange" field, which may be "columns" (default) to
array widgets horizontally, or "rows" to arrange them vertically, one widget per row.

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


