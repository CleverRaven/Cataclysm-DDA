# Modding guide

Certain features of the game can be modified without rebuilding the game from source code. This includes professions, monsters, npcs, and more. Just modify the pertinent files and run the game to see your changes.

The majority of modding is done by editing JSON files. An in-depth review of all json files and their appropriate fields is available in [JSON_INFO.md](JSON_INFO.md).

## The basics

### Creating a barebones mod
A mod is created by creating a folder within Cataclysm's `data/mods` directory. The mod's properties are set by the `modinfo.json` file that is present within that folder. In order for Cataclysm to recognize the folder as a mod, it **must** have a `modinfo.json` file present within it. <!--I know this isn't strictly true. A mod will function as long as there's a JSON file with a MOD_INFO structure in it. The file doesn't need to be called "modinfo.json"-->

### Modinfo.json
The modinfo.json file is a file that contains metadata for your mod. Every mod must have a `modinfo.json` file in order for Cataclysm to find it.
A barebones `modinfo.json` file looks like this:
````json
  {
    "type": "MOD_INFO",
    "ident": "Mod_ID",
    "name": "Mod's Display Name",
    "authors": [ "Your name here", "Your friend's name if you want" ],
    "description": "Your description here",
    "category": "content",
    "dependencies": [ "dda" ]
  }
````
The `category` attribute denotes where the mod will appear in the mod selection menu. These are the available categories to choose from, with some examples chosen from mods that existed when this document was written. Pick whichever one applies best to your mod when writing your modinfo file.
 - `content` - A mod that adds a lot of stuff. Typically reserved for very large mods or complete game overhauls (eg: Core game files, Aftershock)
 - `items` - A mod that adds new items and recipes to the game (eg: More survival tools)
 - `creatures` - A mod that adds new creatures or NPCs to the game (eg: Modular turrets)
 - `misc_additions` - Miscellaneous content additions to the game (eg: Alternative map key, Crazy cataclysm)
 - `buildings` - New overmap locations and structures (eg: Fuji's more buildings). If you're blacklisting buildings from spawning, this is also a usable category (eg: No rail stations).
 - `vehicles` - New cars or vehicle parts (eg: Tanks and other vehicles)
 - `rebalance` - A mod designed to rebalance the game in some way (eg: Safe autodocs).
 - `magical` - A mod that adds something magic-related to the game (eg: Necromancy)
 - `item_exclude` - A mod that stops items from spawning in the world (eg: No survivor armor, No drugs)
 - `monster_exclude` - A mod that stops certain monster varieties from spawning in the world (eg: No fungal monsters, No ants)
 - `graphical` - A mod that adjusts game graphics in some way (eg: Graphical overmap)

The `dependencies` attribute is used to tell Cataclysm that your mod is dependent on something present in another mod. If you have no dependencies outside of the core game, then just including `dda` in the list is good enough. If your mod depends on another one to work properly, adding that mod's `ident` attribute to the array causes Cataclysm to force that mod to load before yours.

## Actually adding things to your mod
Now that you have a basic mod, you can get around to actually putting some stuff into it!

### File structure
It's a good idea to put different categories of additions into different json files. Any json files that are present in the mod folder or its subfolders will be detected and read by Cataclysm, but otherwise, there are no restrictions on what you can put where.

### JSON_INFO.md
It's worth reading [JSON_INFO.md](JSON_INFO.md) to get a comprehensive list of everything you can do with these mods. The rest of this document will have a few examples to copy and paste, but it is by no means comprehensive. The base game's data is also defined in the same way as any mod you write, so taking a look through the game's json files (in `data/json`) can also teach you a lot. If the game finds any issues in your JSON syntax when you try to load a game world, it will spit out an error message, and you won't be able to load that game until the issue is fixed.

### Adding a scenario
Scenarios are what the game uses to determine your general situation when you create a character. They determine when and where your character may spawn in the world, and what professions can be used. They are also used to determine whether those professions can have mutations starting out. Below you will find the JSON definition for the game's built-in `Large Building` scenario.
````json
[
  {
    "type": "scenario",
    "ident": "largebuilding",
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
    "flags": [ "SUR_START", "CITY_START", "LONE_START" ]
  }
]
````

### Adding a profession.
Professions are what the game calls the character classes you can choose from when the game starts. Professions can start with traits, skills, items, and even pets! Below is the definition for the Police Officer profession:
````json
[
  {
    "type": "profession",
    "ident": "cop",
    "name": "Police Officer",
    "description": "Just a small-town deputy when you got the call, you were still ready to come to the rescue.  Except that soon it was you who needed rescuing - you were lucky to escape with your life.  Who's going to respect your authority when the government this badge represents might not even exist anymore?",
    "points": 2,
    "skills": [ { "level": 3, "name": "gun" }, { "level": 3, "name": "pistol" } ],
    "traits": [ "PROF_POLICE" ],
    "items": {
      "both": {
        "items": [ "pants_army", "socks", "badge_deputy", "sheriffshirt", "police_belt", "boots", "whistle", "wristwatch" ],
        "entries": [
          { "group": "charged_cell_phone" },
          { "group": "charged_two_way_radio" },
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
````

### Adding an item
Items are where you really want to read the [JSON_INFO.md](JSON_INFO.md) file, just because there's so much that you can do with them, and every category of item is a little bit different. 
<!--I chose this one because it's about as basic an item as I could find. Everything else does something.-->
````json
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
    "price": 800,
    "material": [ "paper" ],
    "symbol": "*",
    "color": "light_gray"
  }
]
````

### Preventing monsters from spawning
This kind of mod is relatively simple, but very useful. If you don't want to deal with certain types of monsters in your world, this is how you do it. There are two ways to go about this, and both will be detailed below. You can blacklist entire monster groups, or you can blacklist certain monsters. In order to do either of those things, you need that monster's ID. These can be found in the relevant data files. For the core game, these are in the `data/json/monsters` directory.
The example below is from the `No Ants` mod, and will stop any kind of ant from spawning in-game.
````json
[
  {
    "type": "MONSTER_BLACKLIST",
    "categories": [ "GROUP_ANT", "GROUP_ANT_ACID" ]
  },
  {
    "type": "MONSTER_BLACKLIST",
    "monsters": [
      "mon_ant_acid_larva",
      "mon_ant_acid_soldier",
      "mon_ant_acid_queen",
      "mon_ant_larva",
      "mon_ant_soldier",
      "mon_ant_queen",
      "mon_ant_acid",
      "mon_ant"
    ]
  }
]
````
### Preventing locations from spawning
<!--I'm not especially happy with this section. Blacklisting things on the map works differently depending on what you're blacklisting. Overmap specials are different from overmap extras, city buildings, and building groups.-->
Preventing certain types of locations from spawning in-game is a little trickier depending on the type of thing you want to target. An overmap building can either be a standard building, or an overmap special. If you want to block things with a specific flag from spawning, you can blacklist those in a very similar manner to monsters. The example below is also from the `No Ants` mod, and stops all anthills from spawning.
````json
[
  {
    "type": "region_overlay",
    "regions": [ "all" ],
    "overmap_feature_flag_settings": { "blacklist": [ "ANT" ] }
  }
]
````

If the location you want to blacklist is an overmap special, you'll likely have to copy it from its definition, and manually set it's `occurrences` attribute to read `[ 0, 0 ]`.

Finally, if you're trying to blacklist something that spawns inside of cities, you can do that with a region overlay. The below example is used in the `No rail stations` mod, and stops railroad stations from spawning inside of cities. It doesn't stop the railroad station overmap special from spawning, though.
````json
[
  {
    "type": "region_overlay",
    "regions": [ "all" ],
    "city": { "houses": { "railroad_city": 0 } }
  }
]
````
## Important note on json files

The following characters: `[ { , } ] : "` are *very* important when adding or modifying JSON files. This means a single missing `,` or `[` or `}` can be the difference between a working file and a hanging game at startup.
If you want to include these characters in whatever you are doing (for instance, if you want to put a quote inside an item's description), you can do so by putting a backslash before the relevant character. This is known as "escaping" that character. For instance, I can make an item's description contain a quote if I want by doing this:
````json
...
"description": "This is a shirt that says \"I wanna kill ALL the zombies\" on the front.",
...
````

In game, that appears like this:
`This is a shirt that says "I wanna kill ALL the zombies" on the front.`

Many editors have features that let you track `{ [` and `] }` to see if they're balanced (ie, have a matching opposite); These editors will also respect escaped characters properly. [Notepad++](https://notepad-plus-plus.org/) is a popular, free editor on Windows that contains this feature.  On Linux, there are a plethora of options, and you probably already have a preferred one 🙂

## Addendum
<!-- I really don't know if this should be here or not. Please let me know. -->
### No Zombie Revival
This mod is very simple, but it's worth a special section because it does things a little differently than other mods, and documentation on it is tricky to find.

The entire mod can fit into fifteen lines of JSON, and it's presented below. Just copy/paste that into a modinfo.json file, and put it in a new folder in your mods directory.
````json
[
  {
    "type": "MOD_INFO",
    "ident": "no_reviving_zombies",
    "name": "Prevent Zombie Revivication",
    "description": "Disables zombie revival.",
    "category": "rebalance",
    "dependencies": [ "dda" ]
  },
  {
    "type": "monster_adjustment",
    "species": "ZOMBIE",
    "flag": { "name": "REVIVES", "value": false }
  }
]
````
