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

There are people who use screen readers to play Cataclysm DDA. In order for screen
readers to announce the most important information in a UI, the terminal cursor has
to be placed at the correct location. This information may be text such as selected
item names in a list, etc, and the cursor has to be placed exactly at the beginning
of the text for screen readers to announce it. (Note: from my test with an Ubuntu
VM, if the cursor is placed after the end of the text, the cursor might be wrapped
to the next line and cause the screen reader to announce the text incorrectly. It
also seems to be easier for people to control the screen reader to read from the
beginning of the text where the cursor is placed.)

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

For more information and examples, please see the documentation in `ui_manager.h`.
