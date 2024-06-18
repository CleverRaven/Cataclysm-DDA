# User Interface / Accessibility

Cataclysm: Dark Days Ahead uses ncurses, or in the case of the tiles build, an
ncurses port, for user interface. Window management is achieved by `ui_adaptor`,
which requires a resizing callback and a redrawing callback for each UI to handle
resizing and redrawing. A migration effort is underway for user interface code
to be moved to ImGui, which uses SDL on the tiled build, and ncurses on non-tiled
Linux builds.

Some good examples of the usage of `ui_adaptor` can be found within the following
files:

- `string_input_popup` in `string_input_popup.cpp`, specifically in
 `string_input_popup::query_string()`
- `Messages::dialog` in `messages.cpp`

Details on how to use `ui_adaptor` can be found within `ui_manager.h`.

New user interfaces should be coded using ImGui. ImGui-backed windows are created
using `cataimgui::window` as a base, see `cata_imgui.h/cpp`. `cataimgui::window`
still uses `ui_adaptor` but it is handled internally. `cataimgui::window` handles
creating the ImGui window itself, but any widgets (i.e. text boxes, tables, input
fields) must be created using the correct function under the `ImGui` namespace.
Examples of creating any ImGui widget can be found in `src/third-party/imgui_demo.cpp`

Good examples of implementating an ImGui-based UI in cataclysm:

- `demo_ui` a minimal example (about 80 lines) in `main_menu.cpp` added in
[PR#72171](https://github.com/CleverRaven/Cataclysm-DDA/pull/72171)
- `query_popup` and `static_popup` in `popup.h/cpp`, specifically `query_popup_impl`
 which is a private implementation class used by the aforementioned classes.
- `keybindings_ui` in `input.cpp`, which is a private implementation class used
 by `input_context::display_menu()`.

## SDL version requirement of the tiles build

Some functions used by this project require SDL 2.0.6, and the actual version
requirement might be higher. While using a newer version is optional, new versions add
several hints that are used to fix some incorrect behaviors of older SDL versions.
These hints and the minimum SDL version requirements can be found within `InitSDL`
in `sdltiles.cpp`. For example, SDL 2.0.20 is required for IME candidate list
to show correctly on Windows, and SDL 2.0.22 is required for long IME composition
text to show correctly.

## Compatibility with screen readers

There are people who use screen readers to play Cataclysm DDA.  There are several
things to keep in mind when checking the user interface for compatibility with
screen readers.  These include:

1. Screen reader mode:<br>

An option ("SCREEN_READER_MODE") has been added where the user can indicate that
they are using a screen reader.  If a change to the UI to optimize for screen
readers would be undesirable for other users, consider checking against
"SCREEN_READER_MODE", and including the change when true.

2. Text color:<br>

Text color is not announced by screen readers.  If key information is only
communicated to the player through text color, that information will not be
available to players using screen readers.  For example, a simple
uilist where some options have been disabled:

```
----------------------------------------------------------------------------
|Execute which action?                                                     |
|--------------------------------------------------------------------------|
|<color_c_green>1</color> <color_c_light_gray>Cut up an item</color>       |
|<color_c_green>f</color> <color_c_light_gray>Start a fire quickly</color> |
|<color_c_dark_gray>h Use holster</color>                                  |
----------------------------------------------------------------------------
```
A screen reader user will only know that "Use holster" is unavailable when they
attempt to use it.  When presenting information, consider hiding inaccessible
options and adding text to confirm information conveyed by text color.  As an
example, the display of food spoilage status:

```
<color_c_light_blue>Acorns (fresh)</color>
```
```
<color_c_yellow>Acorns (old)</color>
```

3. Cursor placement:<br>

In general, screen readers will start reading where the program has set the terminal
cursor.  Thus, the cursor should be set to the beginning of the most important
section of the UI.

The recommended way to place the cursor is to use `ui_adaptor`. This ensures the
desired cursor position is preserved when subsequent output code changes the
cursor position. You can call `ui_adaptor::set_cursor` and similar methods at any
position in a redrawing callback, and the last cursor position of the topmost UI
set via the call will be used as the final cursor position. You can also call
`ui_adaptor::disable_cursor` to prevent a UI's cursor from being used as the final
cursor position. ImGui-backed screens handle recording of the cursor automatically.

For debugging purposes, you can set the `DEBUG_CURSES_CURSOR` compile flag to
always show the cursor in the terminal (currently only works with the curses
build).

4. Window layout:<br>

While setting the terminal cursor position is sufficient to set things up for
screen readers in many cases, there are caveats to this.  Screen readers will also
attempt to detect text changes.  If that text change is after the terminal cursor,
the screen reader may skip over text.  As an example (terminal position noted
by X):

```
XEye color: amber
```
changing into
```
XEye color: blue
```
The screen reader will only read the word "blue", even though the cursor is
set to start reading at "Eye".  A simple way to force the screen reader to announce
key information is to add a space at the cursor position if the first letter of the
displayed string has not changed.  In this case,
```
XEye color: amber
```
would change into
```
X Eye color: blue
```

Another complication is if the screen reader detects text changes above the
terminal cursor.  As an example:

```
Lifestyle: underpowered
XStrength: 8
```
changing into
```
Lifestyle: weak
XStrength: 9
```
The screen reader will start reading at "weak" even though that's above the
terminal cursor.

Behaviour like this means that screen readers struggle with certain common UI
layouts.  Some examples are listed below.  Examples of how to implement theses
changes can be found in `src/newcharacter.cpp`.

Note: In all following examples, the terminal cursor is marked "X".

4a. List with additional information at the top or bottom:
```
-----------------------------------------------------
|Choose type of butchery:                           |
|---------------------------------------------------|
XB Quick butchery	6 minutes                   |
|b Full butchery	39 minutes                  |
|---------------------------------------------------|
|This technique is used when you are in a hurry, but|
|still want to harvest something from the corpse.   |
-----------------------------------------------------
```
`Figure 4ai - List with details at the bottom, current configuration`

If the cursor is set at "Quick butchery", then the screen reader will read all of the
other options before getting to the description of what "Quick butchery" entails.  The
preferred layout in SCREEN_READER_MODE is the following:

```
-----------------------------------------------------
|Choose type of butchery:                           |
|---------------------------------------------------|
|                                                   |
|                                                   |
|---------------------------------------------------|
XB Quick butchery       6 minutes                   |
|This technique is used when you are in a hurry, but|
-----------------------------------------------------
```
`Figure 4aii - List with details at the bottom, recommended "SCREEN_READER_MODE"`

That is:
1. Add the currently-selected list entry to the detail pane at the bottom, and
2. Disable display of the list of options.
The screen reader user can then scroll through the list of available options, only
hearing about the details of the currently-selected option.

Hiding the list of options is necessary because if the list scrolls, the screen reader
will detect that and attempt to read the list from the top.  As an example:
```
-----------------------------------------------------
|Choose type of butchery:                           |
|---------------------------------------------------|
Yb Full butchery	39 minutes                  |
|f Field dress corpse	5 minutes                   |
|---------------------------------------------------|
XThis technique is used to properly butcher a corpse|
|and requires a rope & a tree or a butchering rack, |
-----------------------------------------------------
```
`Figure 4aiii - List with details at the bottom with scrolling issues`

When the list scrolls, the text at the top changes, and the screen reader will
start reading at point Y and go through the entire list rather than the current
cursor position X.

4b. List with additional information at the side:
```
_______________________________
|List    | Details relating to|
XItem 1  | item 1.  Lorem     |
|Item 2  | ipsum              |
|Item 3  |                    |
|Item 4  |                    |
|Item 5  |                    |
-------------------------------
```
`Figure 4bi - List with details at the side, current configuration`

In this common layout, the screen reader is unable to differentiate between the two
panes, and ends up reading the list interwoven with the details of the currently-selected
item.  This is true whether the terminal cursor is set to the current item or to the top
left of the details pane.

The recommended layout in SCREEN_READER_MODE is the following:
```
-------------------------------
|        | XItem 1            |
|        | Details relating to|
|        | item 1.  Lorem     |
|        | ipsum              |
|        |                    |
|        |                    |
-------------------------------
```
`Figure 4bii - List with details at the side, recommended "SCREEN_READER_MODE"`

That is:
1. Add the currently-selected list entry to the detail pane at the side, and
2. Disable display of the list of options.
