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
| ![#000000](https://via.placeholder.com/20/000000/000000?text=+) | `black`            | `BLACK`             | `0,0,0`              |            |                                                        |
| ![#ff0000](https://via.placeholder.com/20/ff0000/000000?text=+) | `red`              | `RED`               | `255,0,0`            | `R`        |                                                        |
| ![#006e00](https://via.placeholder.com/20/006e00/000000?text=+) | `green`            | `GREEN`             | `0,110,0`            | `G`        |                                                        |
| ![#5c3317](https://via.placeholder.com/20/5c3317/000000?text=+) | `brown`            | `BROWN`             | `92,51,23`           | `br`       |                                                        |
| ![#0000c8](https://via.placeholder.com/20/0000c8/000000?text=+) | `blue`             | `BLUE`              | `0,0,200`            | `B`        |                                                        |
| ![#8b3a62](https://via.placeholder.com/20/8b3a62/000000?text=+) | `magenta`          | `MAGENTA`           | `139,58,98`          | `P`        |                                                        |
| ![#0096b4](https://via.placeholder.com/20/0096b4/000000?text=+) | `cyan`             | `CYAN`              | `0,150,180`          | `C`        |                                                        |
| ![#969696](https://via.placeholder.com/20/969696/000000?text=+) | `light_gray`       | `GRAY`              | `150,150,150`        | `lg`       | deprecated `lt` prefix can be used instead of `light_` |
| ![#636363](https://via.placeholder.com/20/636363/000000?text=+) | `dark_gray`        | `DGRAY`             | `99,99,99`           | `dg`       | deprecated `dk` prefix can be used instead of `dark_`  |
| ![#ff9696](https://via.placeholder.com/20/ff9696/000000?text=+) | `light_red`        | `LRED`              | `255,150,150`        |            | deprecated `lt` prefix can be used instead of `light_` |
| ![#00ff00](https://via.placeholder.com/20/00ff00/000000?text=+) | `light_green`      | `LGREEN`            | `0,255,0`            | `g`        | deprecated `lt` prefix can be used instead of `light_` |
| ![#ffff00](https://via.placeholder.com/20/ffff00/000000?text=+) | `yellow`           | `YELLOW`            | `255,255,0`          |            |                                                        |
| ![#6464ff](https://via.placeholder.com/20/6464ff/000000?text=+) | `light_blue`       | `LBLUE`             | `100,100,255`        | `b`        | deprecated `lt` prefix can be used instead of `light_` |
| ![#fe00fe](https://via.placeholder.com/20/fe00fe/000000?text=+) | `pink`             | `LMAGENTA`          | `254,0,254`          | `lm`       |                                                        |
| ![#00f0ff](https://via.placeholder.com/20/00f0ff/000000?text=+) | `light_cyan`       | `LCYAN`             | `0,240,255`          | `c`        | deprecated `lt` prefix can be used instead of `light_` |
| ![#ffffff](https://via.placeholder.com/20/ffffff/000000?text=+) | `white`            | `WHITE`             | `255,255,255`        | `W`        |                                                        |

**Note:** Default RGB values are taken from file `\data\raw\colors.json`.
**Note:** RGB values can be redefined in file `\config\base_colors.json`.

## Color rules

There are two types of special color transformation which can affect both foreground and background color:

* inversion;
* highlight.

**Note:** Color rules can be redefined (for example, `\data\raw\color_templates\no_bright_background.json`).


## Color tags

Color tags can be used in most places where strings can be displayed to the player:

```json
    "name": "[Ψ]Stop Concentrating",
    "description": "End your concentration on all of your maintained powers.\n\nChanneling this power <color_green>always succeeds</color>.",
```

```json
    "text": [
      "<color_light_blue>You see a Russian sheepdog off at the edge of your vision.  It walks past some scenery and vanishes.</color>"
    ]
```
and
```json
    { "u_message": "The <color_light_green>zombie</color> bursts in <color_red>flames!</color>", "type": "mixed" }
```

will be displayed respectively as:

![image](https://github.com/user-attachments/assets/56a68737-3265-464f-bc81-09b9fb4b8dd9)

![image](https://github.com/user-attachments/assets/eec18405-9205-4778-af13-30a3ccc7f589)

and 

![image](https://github.com/user-attachments/assets/054348a1-b4d4-4890-a72d-3dd8084516f6)


Do note how the color tags override the default color for a `"mixed"` dialogue message (`magenta`).  This is not always the case, so it requires testing to find out the intended combination.

Additionally, color tags can be combined to generate color gradients, and be nested into each other:

```json
    { "u_message": "<color_red>H</color><color_magenta>ell</color><color_red>fire</color> and brimstone." },
    { "u_message": "<color_light_gray><color_white>This text is white</color> while this text is light_gray, <color_light_red>this is light_red</color>. This is light_gray again</color>", "type": "mixed" }
```

which are displayed as:

![image](https://github.com/user-attachments/assets/fcf5589c-d8b9-47e0-a24d-6dd2441d241b)

![image](https://github.com/user-attachments/assets/4cc6d7b4-40f5-45dc-a333-951eaf486a2e)


**Note:** Items that use `relic data` automatically turn the item's displayed name in `pink`.  Color tags override this, in case one wants to "hide" it from the player, or to make them look "mundane".  Similarly, if one wants, otherwise mundane items can have colored names.

# User color customization
## Base colors
Users can customize the color appearance by setting the color triplets in `config/base_colors.json`. The default config looks like this:
```json
[
  {
    "type": "colordef",
    "BLACK": [ 20, 30, 38 ],
    "DGRAY": [ 121, 97, 49 ],
    "GRAY": [ 173, 135, 59 ],
    "WHITE": [ 215, 181, 109 ],
    "BROWN": [ 120, 83, 42 ],
    "YELLOW": [ 231, 155, 32 ],
    "RED": [ 181, 45, 26 ],
    "LRED": [ 200, 105, 65 ],
    "GREEN": [ 142, 140, 61 ],
    "LGREEN": [ 153, 171, 79 ],
    "BLUE": [ 35, 65, 86 ],
    "LBLUE": [ 88, 102, 105 ],
    "MAGENTA": [ 217, 131, 22 ],
    "LMAGENTA": [ 218, 153, 71 ],
    "CYAN": [ 77, 72, 127 ],
    "LCYAN": [ 131, 109, 175 ]
  }
]
```
The mapping values are [Red, Green, Blue] color triplets, with numbers for each ranging from 0 to 255.

If for whatever reason one wanted to make all the "red" colors appear blue on the screen, they could change the `"RED": [ 181, 45, 26 ],` to something like `"RED": [ 10, 10, 220 ],`.

The game provides a nice built-in color manager with several pre-defined color themes that can be easily switched between.


## GUI colors
The UI *menus* do not (always) adhere to the base color customization, and can be customized separately, in the `config/imgui_style.json`. The default config is:
```json
{
  "inherit_base_colors": false,
  "colors": {
  }
}
```
Changing  `inherit_base_colors` to `true` would make the imgui menus follow the base colors theme with some reasonable defaults.

It is, however, also possible to tweak the specific element colors with higher fidelity and not be restricted to the 8-color pallete by specifying the respective elements and colors in the `colors` object. Like so:
```json
{
  "inherit_base_colors": false,
  "colors": {
    "ImGuiCol_Text"                 : [255, 255, 255, 255],
    "ImGuiCol_TextDisabled"         : [128, 128, 128, 255],
    "ImGuiCol_WindowBg"             : [15, 15, 15, 240],
    "ImGuiCol_ChildBg"              : [0, 0, 0, 0],
    "ImGuiCol_PopupBg"              : [20, 20, 20, 240],
    "ImGuiCol_Border"               : [110, 110, 128, 128],
    "ImGuiCol_BorderShadow"         : [0, 0, 0, 0],
    "ImGuiCol_FrameBg"              : [41, 74, 122, 138],
    "ImGuiCol_FrameBgHovered"       : [66, 150, 250, 102],
    "ImGuiCol_FrameBgActive"        : [66, 150, 250, 171],
    "ImGuiCol_TitleBg"              : [10, 10, 10, 255],
    "ImGuiCol_TitleBgActive"        : [41, 74, 122, 255],
    "ImGuiCol_TitleBgCollapsed"     : [0, 0, 0, 130],
    "ImGuiCol_MenuBarBg"            : [36, 36, 36, 255],
    "ImGuiCol_ScrollbarBg"          : [5, 5, 5, 135],
    "ImGuiCol_ScrollbarGrab"        : [79, 79, 79, 255],
    "ImGuiCol_ScrollbarGrabHovered" : [105, 105, 105, 255],
    "ImGuiCol_ScrollbarGrabActive"  : [130, 130, 130, 255],
    "ImGuiCol_CheckMark"            : [66, 150, 250, 255],
    "ImGuiCol_SliderGrab"           : [61, 133, 224, 255],
    "ImGuiCol_SliderGrabActive"     : [66, 150, 250, 255],
    "ImGuiCol_Button"               : [66, 150, 250, 102],
    "ImGuiCol_ButtonHovered"        : [66, 150, 250, 255],
    "ImGuiCol_ButtonActive"         : [15, 135, 250, 255],
    "ImGuiCol_Header"               : [66, 150, 250, 79],
    "ImGuiCol_HeaderHovered"        : [66, 150, 250, 204],
    "ImGuiCol_HeaderActive"         : [66, 150, 250, 255],
    "ImGuiCol_Separator"            : [110, 110, 128, 128],
    "ImGuiCol_SeparatorHovered"     : [26, 102, 191, 199],
    "ImGuiCol_SeparatorActive"      : [26, 102, 191, 255],
    "ImGuiCol_ResizeGrip"           : [66, 150, 250, 51],
    "ImGuiCol_ResizeGripHovered"    : [66, 150, 250, 171],
    "ImGuiCol_ResizeGripActive"     : [66, 150, 250, 242],
    "ImGuiCol_Tab"                  : [46, 89, 148, 219],
    "ImGuiCol_TabHovered"           : [66, 150, 250, 204],
    "ImGuiCol_TabActive"            : [51, 105, 173, 255],
    "ImGuiCol_TabUnfocused"         : [18, 26, 38, 247],
    "ImGuiCol_TabUnfocusedActive"   : [36, 66, 107, 255],
    "ImGuiCol_PlotLines"            : [156, 156, 156, 255],
    "ImGuiCol_PlotLinesHovered"     : [255, 110, 89, 255],
    "ImGuiCol_PlotHistogram"        : [230, 178, 0, 255],
    "ImGuiCol_PlotHistogramHovered" : [255, 153, 0, 255],
    "ImGuiCol_TableHeaderBg"        : [48, 48, 51, 255],
    "ImGuiCol_TableBorderStrong"    : [79, 79, 89, 255],
    "ImGuiCol_TableBorderLight"     : [59, 59, 64, 255],
    "ImGuiCol_TableRowBg"           : [0, 0, 0, 0],
    "ImGuiCol_TableRowBgAlt"        : [255, 255, 255, 15],
    "ImGuiCol_TextSelectedBg"       : [66, 150, 250, 89],
    "ImGuiCol_DragDropTarget"       : [255, 255, 0, 230],
    "ImGuiCol_NavHighlight"         : [66, 150, 250, 255],
    "ImGuiCol_NavWindowingHighlight": [255, 255, 255, 178],
    "ImGuiCol_NavWindowingDimBg"    : [204, 204, 204, 51],
    "ImGuiCol_ModalWindowDimBg"     : [204, 204, 204, 89]
  }
}
```
The keys in the mapping are the ones provided by Imgui library, and it should be "obvious" or perhaps "intuitive" what those do. Values are `[red, green, blue, opacity]` quadruplets with values between `0` and `255`. However `[red, green, blue]` triplets are also accepted, and in those cases opacity is assumed to be `255` (i.e. fully non-transparent).   
The values for those in the above snippet are those of the default imgui theme that the game uses, so copy-pasting that into your own `config/imgui_style.json` should yield no visual difference from the empty config. They can be used as a reference.

You can play around with specific values in the imgui playground (accessible from the game main menu -> imgui Demo Screen -> Configuration -> Style -> Colors). Note that "Export"ing the color configuration gives the values in the range of `0.0` to `1.0`, which should be multiplied by `255` to be used in the `imgui_style.json` config.
