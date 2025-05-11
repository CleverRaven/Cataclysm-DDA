# Modding guide

Certain features of the game can be modified without rebuilding the game from source code. This includes professions, monsters, npcs, and more. Just modify the pertinent files and run the game to see your changes.

The majority of modding is done by editing JSON files. An in-depth review of all json files and their appropriate fields is available in [JSON/JSON_INFO.md](JSON/JSON_INFO.md).

## Other guides

You might want to read the [Guide to adding new content to CDDA for first time
contributors](https://github.com/CleverRaven/Cataclysm-DDA/wiki/Guide-to-adding-new-content-to-CDDA-for-first-time-contributors) on the CDDA wiki.

## The basics

### Creating a barebones mod
A mod is created by creating a folder within Cataclysm's `data/mods` directory. The mod's properties are set by the `modinfo.json` file that is present within that folder. In order for Cataclysm to recognize the folder as a mod, it **must** have a `modinfo.json` file present within it. <!--I know this isn't strictly true. A mod will function as long as there's a JSON file with a MOD_INFO structure in it. The file doesn't need to be called "modinfo.json"-->

### Modinfo.json
The modinfo.json file is a file that contains metadata for your mod. Every mod must have a `modinfo.json` file in order for Cataclysm to find it.
A barebones `modinfo.json` file looks like this:
```jsonc
[
  {
    "type": "MOD_INFO",
    "id": "Mod_ID",
    "name": "Mod's Display Name"
  }
]
```

This is the absolute bare minimum, but you may want to add `authors`(You!), a `description` and a `category`. See below for more possibilities.

### All MOD_INFO fields
Here is a full list of supported values for MOD_INFO:
```jsonc
[
  {
    "type": "MOD_INFO",                             // Mandatory, you're making one of these!
    "id": "Mod_ID",                                 // Mandatory, unique id for your mod. You should not use the same id as any other mod.  Cannot contain the '#' character.  Mod ids must also be valid folder pathing in order to support compatibility with other mods.
    "name": "Mod's Display Name",                   // Mandatory.
    "authors": [ "Me", "A really cool friend" ],    // Optional, but you probably want to put your name here. More than one entry can be added, as shown.
    "description": "The best mod ever!",            // Optional
    "category": "graphical",                        // Optional, see below for a full list of supported values. The category is just for information purposes, so don't worry if your mod might fit more than one category.
    "dependencies": [ "dda" ],                      // Optional. What other mods are required for this mod to function? If your mod depends on another one to work properly, adding that mod's `id` attribute to the array causes Cataclysm to force that mod to load before yours.
    "loading_images": [ "cool.png", "cool2.png" ],  // Optional. Filenames for any loading screen images the mod may have. Loading screens are only present on the graphical/"tiles" version. Only supports .png files. Actual loading screen image will be picked randomly from among all mod loading screens, including other loaded mods that have loading screens.
    "version": "1.3.bacon",                         // Optional. For informational purposes only. No versioning system is provided, so whatever you put here is up to you. Feel free to name your versions after ice cream.
    "core": false,                                  // Optional, default false. Core mods will be loaded before anything else. Used for DDA, third-party use will not be supported.
    "obsolete": false,                              // Optional, default false. Obsolete mods are loaded for legacy saves but cannot be used when starting new worlds
    "path": "my_mod_files/"                         // Optional, this directory relative to modinfo.json's location will be considered the mod's actual directory. e.g. if modinfo.json is located at C:\CDDA\my_mod\modinfo.json, then the mod files would be considered to be at C:\CDDA\my_mod\my_mod_files\. A file such as C:\CDDA\my_mod\some_other_file.json would be ignored, it isn't located inside the specified directory.
  }
]
```

The `category` attribute denotes where the mod will appear in the mod selection menu. These are the available categories to choose from, with some examples chosen from mods that existed when this document was written. Pick whichever one applies best to your mod when writing your modinfo file.
 - `content` - A mod that adds a lot of stuff. Typically reserved for large mods (eg: Core game files, Aftershock)
 - `total_conversion` - A mod that fundamentally changes the game.  In particular, the assumption is that a player should not use two total conversion mods at the same time, and so they will not be tested together.  However, nothing prevents players from using more than one if they wish. (eg: Dark Skies Above)
 - `items` - A mod that adds new items and recipes to the game (eg: More survival tools)
 - `creatures` - A mod that adds new creatures or NPCs to the game (eg: Modular turrets)
 - `misc_additions` - Miscellaneous content additions to the game (eg: Alternative map key, Crazy cataclysm)
 - `buildings` - New overmap locations and structures (eg: National Guard Camp). If you're blacklisting buildings from spawning, this is also a usable category (eg: No rail stations).
 - `vehicles` - New cars or vehicle parts (eg: Tanks and other vehicles)
 - `rebalance` - A mod designed to rebalance the game in some way (eg: Safe autodocs).
 - `magical` - A mod that adds something magic-related to the game (eg: Necromancy)
 - `item_exclude` - A mod that stops items from spawning in the world (eg: No survivor armor, No drugs)
 - `monster_exclude` - A mod that stops certain monster varieties from spawning in the world (eg: No fungal monsters, No monsters)
 - `graphical` - A mod that adjusts game graphics in some way (eg: Graphical overmap)

## Actually adding things to your mod
Now that you have a basic mod, you can get around to actually putting some stuff into it!

### File structure
It's a good idea to put different categories of additions into different json files. Any json files that are present in the mod folder or its subfolders will be detected and read by Cataclysm, but otherwise, there are no restrictions on what you can put where.

### JSON_INFO.md
It's worth reading [JSON/JSON_INFO.md](JSON/JSON_INFO.md) to get a comprehensive list of everything you can do with these mods. The rest of this document will have a few examples to copy and paste, but it is by no means comprehensive. The base game's data is also defined in the same way as any mod you write, so taking a look through the game's json files (in `data/json`) can also teach you a lot. If the game finds any issues in your JSON syntax when you try to load a game world, it will spit out an error message, and you won't be able to load that game until the issue is fixed.

### Adding a scenario
Scenarios are what the game uses to determine your general situation when you create a character. They determine when and where your character may spawn in the world, and what professions can be used. They are also used to determine whether those professions can have mutations starting out. Below you will find the JSON definition for the game's built-in `Large Building` scenario.
```jsonc
[
  {
    "type": "scenario",
    "id": "largebuilding",
    "name": "Large Building",
    "points": -2,
    "description": "Whether due to stubbornness, ignorance, or just plain bad luck, you missed the evacuation, and are stuck in a large building full of the risen dead.",
    "allowed_locs": [
      "mall_a_12",
      "mall_a_30",
      "apartments_con_tower_114",
      "apartments_con_tower_014",
      "apartments_con_tower_104",
      "apartments_con_tower_004",
      "hospital_1",
      "hospital_2",
      "hospital_3",
      "hospital_4",
      "hospital_5",
      "hospital_6",
      "hospital_7",
      "hospital_8",
      "hospital_9"
    ],
    "start_name": "In Large Building",
    "surround_groups": [ [ "GROUP_BLACK_ROAD", 70.0 ] ],
    "flags": [ "CITY_START", "LONE_START" ]
  }
]
```

### Adding a profession.
Professions are what the game calls the character classes you can choose from when the game starts. Professions can start with traits, skills, items, and even pets! Below is the definition for the Police Officer profession:
```jsonc
[
  {
    "type": "profession",
    "id": "cop",
    "name": "Police Officer",
    "description": "Just a small-town deputy when you got the call, you were still ready to come to the rescue.  Except that soon it was you who needed rescuing - you were lucky to escape with your life.  Who's going to respect your authority when the government this badge represents might not even exist anymore?",
    "points": 2,
    "skills": [ { "level": 3, "name": "gun" }, { "level": 3, "name": "pistol" } ],
    "traits": [ "PROF_POLICE" ],
    "items": {
      "both": {
        "items": [ "pants_army", "socks", "badge_deputy", "police_belt", "boots", "whistle", "wristwatch" ],
        "entries": [
          { "group": "charged_cell_phone" },
          { "group": "charged_two_way_radio" },
          { "item": "postman_shirt", "variant": "sheriff" },
          { "item": "ear_plugs", "custom-flags": [ "no_auto_equip" ] },
          { "item": "usp_45", "ammo-item": "45_acp", "charges": 12, "container-item": "holster" },
          { "item": "legpouch_large", "contents-group": "army_mags_usp45" }
        ]
      },
      "male": [ "boxer_shorts" ],
      "female": [ "bra", "boy_shorts" ]
    }
  }
]
```

### Adding an item
Items are where you really want to read the [JSON/JSON_INFO.md](JSON/JSON_INFO.md) file, just because there's so much that you can do with them, and every category of item is a little bit different.
<!--I chose this one because it's about as basic an item as I could find. Everything else does something.-->
```jsonc
[
  {
    "id": "family_photo",
    "type": "GENERIC",
    "//": "Unique mission item for the CITY_COP.",
    "category": "other",
    "name": "family photo",
    "description": "A photo of a smiling family on a camping trip.  One of the parents looks like a cleaner, happier version of the person you know.",
    "weight": "1 g",
    "volume": 0,
    "price": "8 USD",
    "material": [ "paper" ],
    "symbol": "*",
    "color": "light_gray"
  }
]
```

### Preventing monsters from spawning
This kind of mod is relatively simple, but very useful. If you don't want to deal with certain types of monsters in your world, this is how you do it. You can create blacklists and whitelists to define the allowed monsters individually, by species, or by category. In order to create these you'll need the relevant identifiers; look for a monster's `id`, `species`, and any `categories` in its JSON definition, which can be found in `data/json/monsters/` for the core game.

Below is an excerpt from the `Mythos` mod that shows how to blacklist monsters individually and by species. This will prevent all zombies, cyborgs, and robots from spawning in-game, with fungal zombies specified by `id`.
```jsonc
[
  {
    "type": "MONSTER_BLACKLIST",
    "monsters": [
      "mon_zombie_fungus",
      "mon_boomer_fungus",
      "mon_zombie_child_fungus",
      "mon_zombie_gasbag_fungus",
      "mon_zombie_smoker_fungus",
      "mon_skeleton_fungus",
      "mon_skeleton_brute_fungus",
      "mon_skeleton_hulk_fungus",
      "mon_chud"
    ]
  },
  {
    "type": "MONSTER_BLACKLIST",
    "species": [ "ZOMBIE", "ROBOT", "CYBORG" ]
  }
]
```
The following is an example of how to blacklist monsters by category. In this case, it will remove all classic zombie types from the game.
```jsonc
[
  {
    "type": "MONSTER_BLACKLIST",
    "categories": [ "CLASSIC" ]
  }
]
```
You can also define exclusions to a blacklist by combining it with a whitelist. Expanding on the previous example, this will remove all classic zombie types except zombie horses.
```jsonc
[
  {
    "type": "MONSTER_BLACKLIST",
    "categories": [ "CLASSIC" ]
  },
  {
    "type": "MONSTER_WHITELIST",
    "monsters": [
      "mon_zombie_horse"
    ]
  }
]
```
Alternatively, if you only want specific monsters or species to appear, you can define an exclusive whitelist. Note that this will override any defined blacklists. The example below is from the `No Monsters` mod, which prevents all monsters except wildlife from spawning.
```jsonc
[
  {
    "type": "MONSTER_WHITELIST",
    "mode": "EXCLUSIVE",
    "categories": [ "WILDLIFE" ]
  }
]
```
You can define a non-exclusive whitelist by itself, but they have no notable effect unless they're combined with blacklists or exclusive whitelists as shown above. This can still be useful because these lists are combined across all active mods, so you might include one to ensure certain monster types are present for your mod. For example, `Crazy Cataclysm` uses the list below to enable some monsters that the core game blacklists by default, allowing them to spawn regardless of any other mods that might try to disable them.
```jsonc
[
  {
    "type": "MONSTER_WHITELIST",
    "monsters": [
      "mon_zombie_dancer",
      "mon_zombie_jackson",
      "mon_shia",
      "mon_bear_smoky",
      "mon_zombie_skeltal",
      "mon_zombie_skeltal_minion"
    ]
  }
]
```

### Preventing locations from spawning
<!--I'm not especially happy with this section. Blacklisting things on the map works differently depending on what you're blacklisting. Overmap specials are different from overmap extras, city buildings, and building groups.-->
Preventing certain types of locations from spawning in-game is a little trickier depending on the type of thing you want to target. An overmap building can either be a standard building, or an overmap special. If you want to block things with a specific flag from spawning, you can blacklist those in a very similar manner to monsters. The example below is also from the `No Fungal Monsters` mod, and stops all fungal regions from spawning.
```jsonc
[
  {
    "type": "region_overlay",
    "regions": [ "all" ],
    "overmap_feature_flag_settings": { "blacklist": [ "FUNGAL" ] }
  }
]
```

If the location you want to blacklist is an overmap special, you'll likely have to copy it from its definition, and manually set it's `occurrences` attribute to read `[ 0, 0 ]`.

Finally, if you're trying to blacklist something that spawns inside of cities, you can do that with a region overlay. The below example is used in the `No rail stations` mod, and stops railroad stations from spawning inside of cities. It doesn't stop the railroad station overmap special from spawning, though.
```jsonc
[
  {
    "type": "region_overlay",
    "regions": [ "all" ],
    "city": { "houses": { "railroad_city": 0 } }
  }
]
```

### Disabling certain scenarios
The `SCENARIO_BLACKLIST` can be either a blacklist or a whitelist.
When it is a whitelist, it blacklists all scenarios but the ones specified.
No more than one blacklist can be specified at one time - this is in all json loaded for a particular game (all mods + base game), not just your specific mod.
The format is as follows:
```jsonc
[
  {
    "type": "SCENARIO_BLACKLIST",
    "subtype": "whitelist",
    "scenarios": [ "largebuilding" ]
  }
]
```
Valid values for `subtype` are `whitelist` and `blacklist`.
`scenarios` is an array of the scenario ids that you want to blacklist or whitelist.

### Disabling certain professions or hobbies
The `profession_blacklist` can be either a blacklist or a whitelist.
When it is a whitelist, only the professions/hobbies specified may be chosen.
No more than one blacklist can be specified at one time - this is in all json loaded for a particular game (all mods + base game), not just your specific mod.
The format is as follows:
```jsonc
[
  {
    "type": "profession_blacklist",
    "subtype": "blacklist",
    "professions": [ "caffiend", "unemployed" ]
  }
]
```
Valid values for `subtype` are `whitelist` and `blacklist`.
`professions` is an array of the profession/hobby ids that you want to blacklist or whitelist.

### Adding dialogue to existing NPCs

You can't edit existing dialog, but you can add new dialogue by adding a new response that can kick off new dialogue and missions. Here is a working example from DinoMod:

```jsonc
  {
    "type": "talk_topic",
    "id": "TALK_REFUGEE_BEGGAR_2_WEARING",
    "responses": [
      {
        "text": "Yes.  I ask because I noticed there are dinosaurs around.  Do you know anything about that?",
        "topic": "TALK_REFUGEE_BEGGAR_2_DINO2"
      }
    ]
  }
```
## Adjusting monster stats
Monster stats can be adjusted using the `monster_adjustment` JSON element.
```jsonc
  {
    "type": "monster_adjustment",
    "species": "ZOMBIE",
    "flag": { "name": "REVIVES", "value": false },
	  "stat": { "name": "speed", "modifier": 0.9 }
  }
```
Using this syntax allows modification of the following things:
**stat**: `speed` and `hp` are supported.  Modifier is a multiplier of the base speed or HP stat.
**flag**: add or remove a monster flag.
**special**: currently only supports `nightvision` which makes the specified monster species gain nightvision equal to its dayvision.

Currently, adjusting multiple stats or flags requires separate `monster_adjustment` entries.

## External options

External options control a variety of global settings not appropriate for region settings, from `SHOW_MUTATION_SELECTOR` that lets the
player choose mutations you get on mutation, to `ETERNAL_WEATHER` which let's you pick a type of weather to always be active.
All the external options available are located in `/core/external_options.json` along with comments explaining their purpose and their DDA values.
To change the values in a mod you just define an identical object to the dda one with the value changed.

You can also override any source defined option with an external option of the same name in the same way (Eg if the player has `AUTO_FEATURES` set to false
but you make an external option `AUTO_FEATURES` set to true when the player loads the mod their value will be changed to true).
**However currently on save this overrides the user's config so shouldn't be used unless necessary to make the mod work.**

## Important note on json files

The following characters: `[ { , } ] : "` are *very* important when adding or modifying JSON files. This means a single missing `,` or `[` or `}` can be the difference between a working file and a hanging game at startup.
If you want to include these characters in whatever you are doing (for instance, if you want to put a quote inside an item's description), you can do so by putting a backslash before the relevant character. This is known as "escaping" that character. For instance, I can make an item's description contain a quote if I want by doing this:
```jsonc
...
"description": "This is a shirt that says \"I wanna kill ALL the zombies\" on the front.",
...
```

In game, that appears like this:
`This is a shirt that says "I wanna kill ALL the zombies" on the front.`

Many editors have features that let you track `{ [` and `] }` to see if they're balanced (ie, have a matching opposite); These editors will also respect escaped characters properly. [Notepad++](https://notepad-plus-plus.org/) is a popular, free editor on Windows that contains this feature.  On Linux, there are a plethora of options, and you probably already have a preferred one 🙂

### That which cannot be modded

Almost everything in this game can be modded. Almost. This section is intended to chart those areas not supported for modding to save time and headaches.

The Names folder and contents (EN etcetera) confirmed 5/23/20
Construction recipes. Can be worked around by adding requirements and modding those, confirmed 7/4/22
