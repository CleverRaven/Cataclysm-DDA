# Modding guide

Certain features of the game can be modified without rebuilding the game from source code. This includes professions, monsters, npcs, and more. Just modify the pertinent files and run the game to see your changes.

The majority of modding is done by editing JSON files. An in-depth review of all json files and their appropriate fields is available in [JSON_INFO.md](JSON_INFO.md).

## Adding a profession.

Let's say we want to add a "survivalist" profession.

We'll say this profession starts with archery, survival, traps, beef jerky and a few survival items.  We'll set the starting cost at 3 points since it modifies skills and items. We can do that with the following entries:

````json
{
  "ident" : "survivalist",
  "name": "Wilderness Survivalist",
  "description": "You live off the wild and wander the world; you've never
                  had a place to live or items other than what you can
                  scavenge off the land.  People called you crazy; now,
                  after the disaster, you're the sane one.",

  "points" : 3,
  "items": ["pants_cargo", "boots", "beltrig", "selfbow", "jerky", "arrow_field_point"],

 "skills" : [
     {
        "name": "archery",
        "level" : 1
     },
     {
        "name" : "survival",
        "level" : 1
     },
     {
        "name" : "traps",
        "level" : 1
     }
 ]
},
````

After adding this to the `data/json/professions` file, we should see the new profession show up in the character creation screen.

## Adding an item
1.  Open the appropriate json in `data/json/items` and add your item near similar types.

> **NOTE:** See `JSON_INFO` for a more in-depth review of each individual item .json file/flags

2.  Add the item to `data/json/itemgroups` to have it spawn in in-game locations

## Important note on json files

The following characters: `[ { , } ] : "` are *very* important when adding or modifying JSON files. This means a single missing `,` or `[` or `}` can be the difference between a working file and a hanging game at startup.

Many editors have features that let you track `{ [` and `] }` to see if they're balanced (ie, have a matching opposite); [Notepad++](https://notepad-plus-plus.org/) is a popular, free editor on Windows that contains this feature.  On Linux, there are a plethora of options, and you probably already have a preferred one 🙂
