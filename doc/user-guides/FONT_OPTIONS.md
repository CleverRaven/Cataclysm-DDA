# Configuring Fonts

Fonts can be configured through changing the config/fonts.json file. This file is created with
default options on game load, so if it doesn't exist try loading the game.

The default options (available in data/fontdata.json) might look like the following:
```jsonc
{
  "typeface": [
    { "path": "data/font/Terminus.ttf", "hinting": "Bitmap" },
    "data/font/unifont.ttf"
  ],
  "gui_typeface": [
    { "path": "data/font/Roboto-Medium.ttf", "hinting": "Light" },
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
```jsonc
"typeface": { "path": "data/font/Terminus.ttf", "hinting": "Bitmap" },
"map_typeface": "unifont.ttf"
```
Each entry may be a string or an object. If a string is provided it is interpreted as the path to the typeface file.
If an object is provided, then it must have a path attribute, which provides the path to the font. It may also have
a 'hinting' attribute, which determines the font hinting mode. If it's not specified then 'Default' is used.
Hinting may be one of: Auto, NoAuto, Default, Light, or Bitmap. Antialiasing may also be provided and is default set
to true, but can be disabled by setting to off.

A full object may look like the following:
```jsonc
"gui_typeface": {
	"path": "data/font/Roboto-Medium.ttf",
	"hinting": "Light",
	"antialiasing": true,
}
```

## Hinting

Hinting is the process of modifying the shapes of glyphs so that
they line up better with the pixels in the display device. This
improves readability at low resolutions while potentially changing the
shape of the glyphs. Six different settings are available:

### Default

The default hinter uses tables built into the font by the font
designer. Many fonts look best with this hinter because the font
designer has meticulously crafted the hinting tables so that the font
looks as good as possible.

If the font has no hinting tables at all, then this mode falls back to
using the autohinter.

### Light

Many generated glyphs are fuzzier but better resemble their original
shape. This is achieved by snapping glyphs to the pixel grid only
vertically (Y-axis), as is done by Microsoft's ClearType and Adobe's
proprietary font renderer. This preserves inter-glyph spacing in
horizontal text.

### Auto

This mode ignores the font’s hinting tables in favor of
heuristics. The autohinter produces acceptable results for most fonts,
but may produce terrible results on some fonts. Usually this is only
used when the font doesn’t have any hinting tables of its own.

### NoAuto

This is like the Default setting, but won’t use the autohinter if the
font doesn’t have any hinting tables.

### None

This completely skips the hinting step. This will likely produce
blurry text, but the glyph shapes will always be correct. On very
high resolution monitors the lack of hinting may not be noticable.

### Bitmap

This setting turns off hinting just like None does, but additionally
tells the renderer to ignore any vector glyphs in the font in favor of
bitmapped glyphs. As bitmapped glyphs are never antialiased at all
they will be extremely sharp, but also pixelated. In most contexts the
pixelation would be inappropriate, but it may lend the game a retro
aesthetic that many find appealing. Currently the game defaults to
using the Terminus font with this setting.
