# TILESETS

## Terminology

##### Tileset
A package of images for the game.

##### Sprite 
A single image that represents either an individual game entity or a common background

##### Root name
File name without an extension.

##### Tile
A unit square in the game world with certain coordinates, where entities can exist and on which sprites are drawn. Sometimes used (incorrectly) in place of "sprite".

##### Tilesheet
A collection of sprites with identical sizes and offsets composited into one image file that the game will read.

##### `tile_config.json` file
Machine-readable description mapping the contents of tilesheets to game entities.

##### `tileset.txt` file
A collection of tileset metadata.

##### Tile entry
JSON configuration object that describes which sprites are used for which game entities and how.

##### Compositing Tileset
A tileset that is stored as directories of individual sprites and tile entries.

##### `compose.py` script
Converts a compositing tileset into package suitable for the game: a set of tilesheets and the `tile_config.json`.

##### Tilesheet directory
Compositing tileset subdirectory containing sprites and tile entries.

##### `tile_info.json` file

Describes tilesheet directories for `compose.py`

## JSON Schema

### tile entry
```C++
{                                           // The simplest version
    "id": "mon_cat",                        // a game entity ID
    "fg": "mon_cat_black",                  // a sprite root name that will be put on foreground
    "bg": "shadow_bg_1"                     // another sprite root name that will be the background; can be empty for no background
}
```

Sprites can be referenced across tilesheet directories, but they must be stored in a tilesheet directory with their size and offset.

`id` can be an array of multiple game entity IDs sharing the same sprite configuration, like `"id": ["vp_door", "vp_hddoor"]`.  `id` game values that are used as-is include terrain, furniture, items (except corpses), monsters, fields, traps.

#### Hardcoded IDs

The special ID `unknown` provides a sprite that is displayed when an entity has no other sprite. Other hardcoded IDs also exist, and most of them are referenced in [`src/cata_tiles.cpp`](/src/cata_tiles.cpp). A full list of hardcoded IDs _may_ be present in [`tools/json_tools/generate_overlay_ids.py`](/tools/json_tools/generate_overlay_ids.py) stored as `CPP_IDS` but it's updated manually and may lag behind.

#### Complex IDs

Special prefixes that are used include:

`overlay_effect_` for effects.

`overlay_mutation_` and `overlay_mutation_active_` for both mutations and bionics.

`overlay_worn_` and `overlay_wielded_` for items being worn or wielded by characters.

`corpse_` for corpses.

`overlay_` for movement modes.

`vp_` for vehicle parts (see also [`symbols` and `standard_symbols` JSON keys](JSON_INFO.md#symbols-and-variants) that are used as suffixes).

`explosion_` for spell explosion effects.  Multitile is required; only supports "center", "edge" and "corner".

#### Optional gendered variants

Are defined by adding `_female` or `_male` part to the `overlay_` part of a prefix: `overlay_female_` or `overlay_male_`.

#### Optional seasonal variants

Are defined by adding `_season_spring`, `_season_summer`, `_season_autumn`, or `_season_winter` suffix to any tile entry `id`. For example `"id": "mon_wolf_season_winter"`.

#### Rotations

You can add `"rotates": true` to allow sprites to be rotated by the game automatically. Alternatively, `fg` and `bg` can be an array of 2 or 4 pre-rotated variants, like `"fg": ["mon_dog_left", "mon_dog_right"]` or `"bg": ["t_wall_n", "t_wall_e", "t_wall_s", "t_wall_w"]`.

#### Random variations

`fg` and `bg` can also be an array of objects of weighted, randomly chosen options, any of which can also be an array of rotations:
```C++
    "fg": [
        { "weight": 50, "sprite": "t_dirt_brown"},       // appears approximately 50 times in 53 tiles
        { "weight": 1, "sprite": "t_dirt_black_specks"}, // appears 1 time in 53 tiles
        { "weight": 1, "sprite": "t_dirt_specks_gray"},
        { "weight": 1, "sprite": "t_patchy_grass"}       // file names are arbitrary, but see `--use-all` option
    ],
```

#### Multitile

`"multitile": true` signifies that there is an `additional_tiles` object (redundant? [probably](https://github.com/CleverRaven/Cataclysm-DDA/issues/46253)) with one or more objects that define sprites for game entities associated with this tile, such as broken versions of an item, or wall connections.  Each object in the array has an `id` field, as above, and an `fg` field, which can be a single [root name](#root-name), an array of root names, or an array of objects as above. `"rotates": true` is implied with it and can be omitted.

#### Multiple tile entries in the same file

Each JSON file can have either a single object or an array of one or more objects:
```C++
[
    { "id": "mon_zombie", "fg": "mon_zombie", "bg": "mon_zombie_bg" },
    { "id": "corpse_mon_zombie", "fg": "mon_zombie_corpse", "bg": "mon_zombie_bg" },
    { "id": "overlay_wielded_corpse_mon_zombie", "fg": "wielded_mon_zombie_corpse" }
]
```

### `tile_info.json`
```c++
[
  {                         // default sprite size
    "width": 32,
    "height": 32,
    "pixelscale": 1
  }, {
    "tiles.png": {}         // Each tilesheet directory must have a corresponding object
  }, {                      // with a single key, which will become the tilesheet output filename.
    "expan.png": {}         // Empty object means the default sprite size and no offsets.
  }, {
    "tree.png": {
      "sprite_width": 64,   // Overriding values example
      "sprite_height": 80,
      "sprite_offset_x": -16,
      "sprite_offset_y": -48
    }
  }, {
    "fillerhoder.png": {    // Unknown keys like `source` will be ignored by `compose.py` and can be used as comments.
      "source": "https://github.com/CleverRaven/Cataclysm-DDA/tree/b2d1f9f6cf6fae9c5076d29f9779e0ca6c03c222/gfx/HoderTileset",
      "filler": true 
    }
  }, {
    "fallback.png": {
      "fallback": true
    }
  }
]
```

Tilesheet directory names are expected to use the following format: `pngs_{tilesheet_root_name}_{sprite_width}x{sprite_height}` - such as `pngs_tiles_32x32`, `pngs_expan_32x32`, `pngs_tree_64x80`, etc. To improve performance, keep the number of tilesheets to a minimum.

`"filler": true` means the tilesheet is a filler; tile entries within it will be used only if IDs in them were not mentioned in any preceding tile entry.  Sprites within a filler directory will be ignored if another one with the same name was already encountered.  A filler tilesheet is useful when upgrading the art in a tileset: old, low-quality art can be placed on filler tilesheet and will be automatically replaced as better images are added to the non-filler tilesheets.

`"fallback": true` means the tilesheet is a fallback; it will be treated as a source of fallback ASCII character sprites.  `compose.py` will also append the fallback tilesheet to the end of the tileset, and will add a "fallback.png" to `tile_config.json` if there is no `fallback` entry in `tile_info.json`.

### Expansion tile entries

A tilesheet can be an expansion from a mod.  Each expansion tilesheet is a single `id` value, where the `"rotates": false"`, and `"fg": 0` keys are set.  Expansion tile entry JSONs are the only tile entry JSONs that may use an integer value for `fg`, and that value must be 0.  Expansion tile entry JSONs must be located at the top layer of each tilesheet directory.

## `compose.py`

[`tools/gfx_tools/compose.py`](/tools/gfx_tools/compose.py)

### Usage

`compose.py [-h] [--use-all] [--obsolete-fillers] source_dir [output_dir]`

`source_dir` - the compositing tileset directory.

`output_dir` will be set to the `source_dir` unless provided separately. Expected to have `tileset.txt` and `fallback.png`.

`--use-all`: instead of warning about unused sprites, will treat their [root name](#root-name) as the `id` value to use them as `fg` for. In other words, just naming your sprite `overlay_wielded_spear_survivor.png` will imply this tile entry **unless** any tile entry already references `overlay_wielded_spear_survivor` in a `fg` or `bg` value:
```JSON
{
    "id": "overlay_wielded_spear_survivor",
    "fg": "overlay_wielded_spear_survivor"
}
```

`--obsolete-fillers`: print which fillers were skipped and are thus ready to be removed.

Requires `pyvips` module.

### Installing pyvips on Windows

- Download Python https://www.python.org/downloads/
- Download `libvips` https://libvips.github.io/libvips/install.html
- Make sure  directories with `python`, `pip`, and `libvips` are in your `PATH` [environment variable](https://en.wikipedia.org/wiki/Environment_variable#Windows)
- Press `Windows key + r` to open the "Run" dialog box
- run `cmd`; it should open the console
- run `py -m pip install --upgrade pip` to get the latest version of `pip`
- run `pip install --user pyvips` to install `pyvips`

## Legacy tilesets

Prior to October 2019, when `compose.py` was made, sprite indices in `tile_config.json` had to be calculated by hand. Following is a description for them.

### tilesheets
Each tilesheet contains 1 or more sprites with the same width and height.  Each tilesheet contains one or more rows of exactly 16 sprites.  Sprite index 0 is special and the first sprite of the first tilesheet in a tileset should be blank.  Indices run sequentially through each sheet and continue incrementing for each new sheet without resetting, so index 32 is the first sprite in the third row of the first sheet.  If the first sheet has 320 sprites in it, index 352 would be the first sprite of the third row of the second sheet.

### `tile_config`
Each legacy tileset has a `tile_config.json` describing how to map the contents of a sprite sheet to various tile identifiers, different orientations, etc. The ordering of the overlays used for displaying mutations can be controlled as well. The ordering can be used to override the default ordering provided in `mutation_ordering.json`. Example:

```C++
  {                                             // whole file is a single object
    "tile_info": [                              // tile_info is mandatory
      {
        "height": 32,
        "width": 32,
        "iso" : true,                             //  Optional. Indicates an isometric tileset. Defaults to false.
        "pixelscale" : 2                          //  Optional. Sets a multiplier for resizing a tileset. Defaults to 1.
      }
    ],
    "tiles-new": [                              // tiles-new is an array of sprite sheets
      {                                           //   alternately, just one "tiles" array
        "file": "tiles.png",                      // file containing sprites in a grid
        "tiles": [                                // array with one entry per tile
          {
            "id": "10mm",                         // ID is how the game maps things to sprites
            "fg": 1,                              //   lack of prefix mostly indicates items
            "bg": 632,                            // fg and bg can be sprite indexes in the image
            "rotates": false
          },
          {
            "id": "t_wall",                       // "t_" indicates terrain
            "fg": [2918, 2919, 2918, 2919],       // 2 or 4 sprite numbers indicates pre-rotated
            "bg": 633,
            "rotates": true,
            "multitile": true,
            "additional_tiles": [                 // connected/combined versions of sprite
              {                                   //   or variations, see below
                "id": "center",
                "fg": [2919, 2918, 2919, 2918]
              },
              {
                "id": "corner",
                "fg": [2924, 2922, 2922, 2923]
              },
              {
                "id": "end_piece",
                "fg": [2918, 2919, 2918, 2919]
              },
              {
                "id": "t_connection",
                "fg": [2919, 2918, 2919, 2918]
              },
              {
                "id": "unconnected",
                "fg": 2235
              }
            ]
          },
          {
            "id": "vp_atomic_lamp",               // "vp_" vehicle part
            "fg": 3019,
            "bg": 632,
            "rotates": false,
            "multitile": true,
            "additional_tiles": [
              {
                "id": "broken",                   // variant sprite
                "fg": 3021
              }
            ]
          },
          {
            "id": "t_dirt",
            "rotates": false,
            "fg": [
              { "weight":50, "sprite":640},       // weighted random variants
              { "weight":1, "sprite":3620},
              { "weight":1, "sprite":3621},
              { "weight":1, "sprite":3622}
            ]
          },
          {
            "id": [
              "overlay_mutation_GOURMAND",        // character overlay for mutation
              "overlay_mutation_male_GOURMAND",   // overlay for specified gender
              "overlay_mutation_active_GOURMAND"  // overlay for activated mutation
            ],
            "fg": 4040
          }
        ]
      },
      {                                           // second entry in tiles-new
        "file": "moretiles.png",                  // another sprite sheet
        "tiles": [
          {
            "id": ["xxx","yyy"],                  // define two ids at once
            "fg": 1,
            "bg": 234
          }
        ]
      }d
    ],
    "overlay_ordering": [
      {
        "id" : "WINGS_BAT",                         // mutation name, in a string or array of strings
        "order" : 1000                              // range from 0 - 9999, 9999 being the topmost layer
      },
      {
        "id" : [ "PLANTSKIN", "BARK" ],             // mutation name, in a string or array of strings
        "order" : 3500                              // order is applied to all items in the array
      },
      {
        "id" : "bio_armor_torso",                   // Overlay order of bionics is controlled in the same way
        "order" : 500
      }
    ]
  }
```

### decompose.py

[`tools/gfx_tools/decompose.py`](/tools/gfx_tools/decompose.py)

This is a Python script that will convert a legacy tileset into a compositing tileset.  It reads the `tile_config.json` and assigns semi-arbitrary file names to each sprite index.  Then it changes all sprite index references to file names, breaks up `tile_config.json` into many small tile entry JSON files with arbitrary file names, and writes each sprite into a separate file.

It requires the `pyvips` module to perform the image processing.

It takes a single mandatory argument, which is the path to the tileset directory.  For example:
`python3 tools/gfx_tools/decompose.py gfx/ChestHole16Tileset` will convert the legacy ChestHole16 tileset to a compositing tileset.

decompose.py creates a sufficient directory hierarchy and file names for a tileset to be compositing, but it is machine-generated and will be badly organized.  New compositing tilesets should use more sensible file names and a better organization.

It shouldn't be necessary to run decompose.py very often.  Legacy tilesets should only need to be converted to composite tilesets one time.
