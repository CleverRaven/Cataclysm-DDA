# TILESETS
A tileset provides graphic images for the game.  Each tileset has one or more tilesheets of image sprites and a `tile_config.json` file that describes how to map the contents of the sprite sheets to various entities in the game.  It also has a `tileset.txt` file that provides metadata.

## Compositing Tilesets
Prior September 2019, tilesets had to be submitted to the repo with each tilesheet fully composited and the sprite indices in `tile_config.json` calculated by hand.  After September 2019, tilesets could be submitted to repos as directories of individual sprite files and tile entry JSON files that used sprite file names, and a Python script that runs at compile time would merge the sprite images into tilesheets, convert the files names into sprite indices for the tile entries, and merge the tile entries into a `tile_config.json`.

For the rest of this document, tilesets that are submitted as fully composited tilesheets are called legacy tilesets, and tilesets that submitted as individual sprite image files are compositing tilesets.

### tools/gfx_tools/decompose.py
This is a Python script that will convert a legacy tileset into a compositing tileset.  It reads the `tile_config.json` and assigns semi-arbitrary file names to each sprite index.  Then it changes all the sprite indexes references to the file names.  Then it breaks up `tile_config.json` into many small tile_entry JSON files with arbitrary file names, and pulls out each sprite and writes it to aseparate file.

It requires ImageMagick to do the image processing.

It takes a single mandatory argument, which is the path to the tileset directory.  For example:
`python tools/gfx_tools/decompose.py gfx/ChestHole16Tileset` will convert the legacy ChestHole16 tileset to a compositing tileset.

decompose.py creates a sufficient directory hierarchy and file names for a tileset to be compositing, but it is machine generated and badly organized.  New compositing tilesets should use more sensible file names and a better organization.

It shouldn't be necessary to run decompose.py very often.  Legacy tilesets should only need to be converted to composite tilesets one time.

### tools/gfx_tools/compose.py
This is a Python script that creates the tilesheets for a compositing tileset.  It reads all of the directories in a tileset's directory with names that start with `pngs_` for sprite files and `tile_entry` JSON files, creates mappings of sprite file names to indices, merges the sprite files into tilesheets, changes all of the sprite file name references in the `tile_entries` to indices, and merges the `tile_entries` into a `tile_config.json`.

The original sprite files and `tile_entry` JSON files are preserved.

### directory structure
Each compositing tileset has one or more directories in it with a name that starts with `pngs_`, such as `pngs_tree_32x40` or `pngs_overlay`.  These are the image directories.  All of the sprites in an image directory must have the same height and width and will be merged into a single tilesheet.

It is recommended that tileset developers include the sprite dimensions in the image directory name, but this is not required.  `pngs_overlay_24x24` is preferred over `pngs_overlay` but both are allowed.  As each image directory creates its own tilesheet, and tilesheets should be as large as possible for performance reasons, tileset developers are strongly encouraged to minimize the number of image directories.

Each image directory contains a hierarchy of subdirectories, `tile_entry` JSON files, and sprite files.  There is no restriction on the arrangement or names of these files, except for `tile_entry` JSON files for expansion tilesheets must be at the top level of the image directory.  Subdirectories are not required but are recommended to keep things manageable.

### `tile_entry` JSON
Each `tile_entry` JSON is a dictionary that describes how to map one or more game entities to one or more sprites.  The simplest version has a single game entity, a single foreground sprite, an *optional* background sprite, and a rotation value.  For instance:
```C++
{                                           // this is an object and doesn't require a list
    "id": "mon_cat",                        // the game entity represented by this sprite
    "fg": "mon_cat_black",                  // some sprite name 
    "bg": "shadow_bg_1",                    // some sprite name; always a single value
    "rotates": false                        // true for things that rotate like vehicle parts
}
```

The values in `"id"`, `"fg"`, and `"bg"` can be repeated within an image directory or in different image directories.  `"fg"` and `"bg"` sprite images can be referenced across image directories, but the sprites must be stored in an image directory with other sprites of the same height and width.

`"id"` can also be a list of multiple game entities sharing the same sprite, like `"id": ["vp_door"], ["vp_hddoor"]`.

`"fg"` can also be a list of 2 or 4 pre-rotated rotational variants, like `"id": ["t_wall_n", "t_wall_e", "t_wall_s", "t_wall_w"]` or `"id": ["mon_dog_left", "mon_dog_right"]`.

`"fg"` can also be a list of dictionaries of weighted, randomly chosen options, any of which can also be a rotated list:
```C++
    "fg": [
        { "weight": 50, "sprite": "t_dirt_brown"},       // appears in 50 of 53 tiles
        { "weight": 1, "sprite": "t_dirt_black_specks"}, // appears 1 in 53 tiles
        { "weight": 1, "sprite": "t_dirt_specks_gray"},
        { "weight": 1, "sprite": "t_patchy_grass"}       // file names are arbitrary
    ],
```

`"multitle"` is an *optional* field.  If it is present and `true`, there must be an `additional_tiles` list with 1 or more dictionaries for entities and sprites associated with this tile, such as broken versions of an item or wall connections.  Each dictionary in the list has an `"id`" field, as above, and a `"fg"` field, which can be a single filename, a list of filenames, or a list of dictionaries.

#### expansion `tile_entry` JSON
Tilesheets can have expansion tilesheets, which are tilesheets from mods.  Each expansion tilesheet is a single `"id"` value, `"rotates": false"`, and `"fg": 0`.  Expansion `tile_entry` JSON are the only `tile_entry` JSONs that use an integer value for `"fg"` and that value must be 0.  Expansion `tile_entry` JSONs must be located at the top layer of each image directory.

### Sprite Images
Every sprite inside an image directory must have the same height and width as every other sprite in the image directory.  

Sprites can be organized into subdirectories within the image directory however the tileset developer prefers.  Sprite filenames are completely arbitrary and should be chosen using a scheme that makes sense to the tileset developer.

## Legacy tilesets
### tilesheets
Each tilesheet contains 1 or more sprites with the same width and height.  Each tilesheet contains one or more rows of exactly 16 sprites.  Sprite index 0 is special and the first sprite of the first tilesheet in a tileset should be blank.  Indices run sequentially through each sheet and continue incrementing for each new sheet without reseting, so index 32 is the first sprite in the third row of the first sheet.  If the first sheet has 320 sprites in it, index 352 would be the first sprite of the third row of the second sheet.

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
            "id": "10mm",                         // id is how the game maps things to sprites
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
      }
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

