# User Interface

Cataclysm: Dark Days Ahead uses ncurses, or in the case of the tiles build, an
ncurses port, for user interface. Window management is achieved by `ui_adaptor`,
which requires a resizing callback and a redrawing callback for each UI to handle
resizing and redrawing.

Some good examples of the usage of `ui_adaptor` can be found within the following
files:
- `query_popup` and `static_popup` in `popup.h/cpp`
- `Messages::dialog` in `messages.cpp`

Details on how to use `ui_adaptor` can be found within `ui_manager.h`.

## Compatibility with screen readers

There are people who use screen readers to play Cataclysm DDA. In order for screen
readers to announce the most important information in a UI, the terminal cursor has
to be placed at the correct location. This information may be text such as selected
item names in a list, etc, and the cursor has to be placed exactly at the beginning
of the text for screen readers to announce it.

The recommended way to place the cursor is to use `ui_adaptor`. This ensures the
desired cursor position is preserved when subsequent output code changes the
cursor position. You can call `ui_adaptor::set_cursor` and similar methods at any
position in a redrawing callback, and the last cursor position of the topmost UI
set via the call will be used as the final cursor position. You can also call
`ui_adaptor::disable_cursor` to prevent a UI's cursor from being used as the final
cursor position.

For more information and examples, please see the documentation in `ui_manager.h`.
