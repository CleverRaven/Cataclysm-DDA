# Configuring Fonts

Fonts can be configured through changing the config/fonts.json file. This file is created with
default options on game load, so if it doesn't exist try loading the game. 

The default options (available in data/fontdata.json) might look like the following:
```json
{
  "typeface": [
    { "path": "data/font/Terminus.ttf", "hinting": "Bitmap" },
    "data/font/unifont.ttf"
  ],
  "gui_typeface": [
    { "path": "data/font/Roboto-Medium.ttf", "hinting": "Auto" },
    { "path": "data/font/Terminus.ttf", "hinting": "Bitmap" },
    "data/font/unifont.ttf"
  ],
  "map_typeface": [
    { "path": "data/font/Terminus.ttf", "hinting": "Bitmap" },
    "data/font/unifont.ttf"
  ],
  "overmap_typeface": [
    { "path": "data/font/Terminus.ttf", "hinting": "Bitmap" },
    "data/font/unifont.ttf"
  ]
}
```

There are four different font categories: `typeface`, `gui_typeface`, `map_typeface`, and `overmap_typeface`, 
which are used for the old-style game interface, ImGui menus, the game display and the overmap, respectively. 
If more than one font is specified for a typeface the list is treated as a fallback order. Unifont will always
be used as a 'last resort' fallback even if not listed here. 

Fonts can be provided as a list, as seen above, or as single entries, e.g.:
```json
"typeface": { "path": "data/font/Terminus.ttf", "hinting": "Bitmap" },
"map_typeface": "unifont.ttf"
```
Each entry may be a string or an object. If a string is provided it is interpreted as the path to the typeface file.
If an object is provided, then it must have a path attribute, which provides the path to the font. It may also have 
a 'hinting' attribute, which determines the font hinting mode. If it's not specified then 'Default' is used.
Hinting may be one of: Auto, NoAuto, Default, Light, or Bitmap. Antialiasing may also be provided and is default set
to true, but can be disabled by setting to off.

A full object may look like the following:
```json
"gui_typeface": {
	"path": "data/font/Roboto-Medium.ttf",
	"hinting": "Auto",
	"antialiasing": true,
}
```