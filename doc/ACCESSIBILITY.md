### Compatibility with screen readers

There are people who uses screen readers to play Cataclysm DDA. In order for screen
readers to announce the most important information in a UI, the terminal cursor has
to be placed at the correct location. This information may be text such as selected
item names in a list, etc, and the cursor has to be placed exactly at the beginning
of the text for screen readers to announce it.

`wmove` in `output.h|cpp` is the function to move the cursor to a specific location.
After calling `wmove` with the target `catacurses::window` and cursor position,
`wrefresh` needs to be called immediately afterwards for `wmove` to take effect.

Here is an example of placing the cursor explicitly at the beginning of a piece
of text:

```cpp
catacurses::window win = ...; // target window

...

// display code
point cursor_position = ...; // default cursor position

...

cursor_position = point_zero; // record the start position of the text
fold_and_print( win, cursor_position, getmaxx( win ), c_white, _( "This text is important" ) );

...

// at the end of display code
wmove( win, cursor_position );
wrefresh( win );
// no output code should follow as they might change the cursor position
```

As shown in the above example, it is preferable to record the intended cursor
position in a variable when the text is printed, and move the cursor later using
the variable to ensure consisitency.
