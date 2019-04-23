# Colors

DDA is colorful game. You can use several foreground and background colors in various places:

* map data (terrain and furniture);
* item data;
* text data;
* etc.

**Note:** Map data objects can only have one color-related node defined (either `color` or `bgcolor`).

## Color string format

Whenever color is defined in JSON it should be defined in following format: `Prefix_Foreground_Background`.

`Prefix` can take one of following values:

* `c_` - default color prefix (can be omitted);
* `i_` - optional prefix which indicates that foreground color should be inverted (special rules will be applied to foreground and background colors);
* `h_` - optional prefix which indicates that foreground color should be highlighted (special rules will be applied to foreground and background colors).

`Foreground` - defines mandatory color of foreground/ink/font.

`Background` - defines optional color of background/paper.

**Note:** Not all foreground + background pairs are defined by their full name. Use in-game Color manager to see all color names.

**Note:** If color was not found by its name, then `c_unset` is used for `Foreground` and `i_white` for `Background`.

## Examples of color strings

- `c_white` - `white` color (with default prefix `c_`);
- `black` -  `black` color (default prefix `c_` is omitted);
- `i_red` - inverted `red` color;
- `dark_gray_white` - `dark_gray` foreground color with `white` background color;
- `light_gray_light_red` - `light_gray` foreground color with `light_red` background color;
- `dkgray_red` - `dark_gray` foreground color with `red` background color (deprecated prefix `dk` instead of `dark_`);
- `ltblue_red` - `light_blue` foreground color with `red` background color (deprecated prefix `lt` instead of `light_`).

## Color code

Color code is short string which defines color and can be used, for example, in maps notes.

## Possible colors

| Color (image)                                            | Color name (dda)   | Color name (curses) | Default R,G,B values | Color code | Notes                                                  |
|:--------------------------------------------------------:|:------------------:|:-------------------:|:--------------------:|:----------:|:------------------------------------------------------:|
| ![#000000](https://placehold.it/20/000000/000000?text=+) | `black`            | `BLACK`             | `0,0,0`              |            |                                                        |
| ![#ff0000](https://placehold.it/20/ff0000/000000?text=+) | `red`              | `RED`               | `255,0,0`            | `R`        |                                                        |
| ![#006e00](https://placehold.it/20/006e00/000000?text=+) | `green`            | `GREEN`             | `0,110,0`            | `G`        |                                                        |
| ![#5c3317](https://placehold.it/20/5c3317/000000?text=+) | `brown`            | `BROWN`             | `92,51,23`           | `br`       |                                                        |
| ![#0000c8](https://placehold.it/20/0000c8/000000?text=+) | `blue`             | `BLUE`              | `0,0,200`            | `B`        |                                                        |
| ![#8b3a62](https://placehold.it/20/8b3a62/000000?text=+) | `magenta` or `pink`| `MAGENTA`           | `139,58,98`          | `P`        |                                                        |
| ![#0096b4](https://placehold.it/20/0096b4/000000?text=+) | `cyan`             | `CYAN`              | `0,150,180`          | `C`        |                                                        |
| ![#969696](https://placehold.it/20/969696/000000?text=+) | `light_gray`       | `GRAY`              | `150,150,150`        | `lg`       | deprecated `lt` prefix can be used instead of `light_` |
| ![#636363](https://placehold.it/20/636363/000000?text=+) | `dark_gray`        | `DGRAY`             | `99,99,99`           | `dg`       | deprecated `dk` prefix can be used instead of `dark_`  |
| ![#ff9696](https://placehold.it/20/ff9696/000000?text=+) | `light_red`        | `LRED`              | `255,150,150`        |            | deprecated `lt` prefix can be used instead of `light_` |
| ![#00ff00](https://placehold.it/20/00ff00/000000?text=+) | `light_green`      | `LGREEN`            | `0,255,0`            | `g`        | deprecated `lt` prefix can be used instead of `light_` |
| ![#ffff00](https://placehold.it/20/ffff00/000000?text=+) | `light_yellow`     | `YELLOW`            | `255,255,0`          |            | deprecated `lt` prefix can be used instead of `light_` |
| ![#6464ff](https://placehold.it/20/6464ff/000000?text=+) | `light_blue`       | `LBLUE`             | `100,100,255`        | `b`        | deprecated `lt` prefix can be used instead of `light_` |
| ![#fe00fe](https://placehold.it/20/fe00fe/000000?text=+) | `light_magenta`    | `LMAGENTA`          | `254,0,254`          | `lm`       | deprecated `lt` prefix can be used instead of `light_` |
| ![#00f0ff](https://placehold.it/20/00f0ff/000000?text=+) | `light_cyan`       | `LCYAN`             | `0,240,255`          | `c`        | deprecated `lt` prefix can be used instead of `light_` |
| ![#ffffff](https://placehold.it/20/ffffff/000000?text=+) | `white`            | `WHITE`             | `255,255,255`        | `W`        |                                                        |

**Note:** Default RGB values are taken from file `\data\raw\colors.json`.
**Note:** RGB values can be redefined in file `\config\base_colors.json`.

## Color rules

There are two types of special color transformation which can affect both foreground and background color:

* inversion;
* highlight.

**Note:** Color rules can be redefined (for example, `\data\raw\color_templates\no_bright_background.json`).
