# Beginner's Guide to Making a Map

## Introduction
Okay, so, let's get started.  You want to add something to Cataclysm, some kind of cool content.  There are thousands of items, more than enough monsters, what is there to add?  The answer is obvious: places to explore.  Mapmaking in Cataclysm is suprisingly easy, fun, and rewarding... let's get going.

### In this tutorial:
I'm going to walk you through the construction of a map that takes up two **overmap tiles**, has three **z-levels**, and uses a couple **chunks** to create some variability, so it's not the same every time it spawns. We'll learn about a couple different ways to handle **monster spawns** and **item spawns**, and put it onto the overmap.

What I conspicuously *won't* do is add an NPC. We might get to that in another tutorial.

### Terminology 
The big map, that you get pressing (m), is called the **overmap**.   One square on the overmap is an **overmap tile**.  

The map you walk around on and play the game in is called **the map**.  A single overmap tile produces a 24x24 square of individual tiles on the map.  

The file that explains to the game which tiles to put down on the map is called a **mapgen file**. It's coded in a format called **JSON**. That's the format we'll be using. We won't be touching c++ so put the textbook away. I don't even have a c++ compiler, stop being so damn gung ho.  
  

> *Historical note: in the dark times, all maps in cdda were coded in c++, very hard to read, and impossible to alter without recompiling. Legends say that to this day, certain map types are hard coded in c++, until the One True Hero returns to convert the last few remnants to JSON. If you want to edit a map that's currently coded in c++, change your mind and do a different map instead. That's not a beginner level project.*

  
Mapgen files build the map in two layers, **terrain** and **furniture**, in a grid that basically looks like an ASCII art version of the map. (The way these layers are defined takes a bit of getting used to, but we'll get to that). Terrain types are defined in **terrain.json** and furniture types are defined in **furniture.json**. I keep these files open for reference when I start a map, because I need to make sure I enter the right **ID**'s, that's the name the game knows the terrain by.
  
There is no standardized character key in mapbuilding to say that, say, "T" means "table": there can't be, for reasons you'll soon see (not the least of which is that there are more terrain types than possible mapgen characters). The meaning of each letter or symbol on the map is defined in an entry called a **palette**. Your palette can be set up in its own JSON entry, or in a list right inside the mapgen file, or both. I usually use a separate palette entry if I'm doing something where multiple maps will have the same, or almost the same files. (I will show you some dirty tricks that let you use the same palette in multiple maps, but change just a few characters, this is very handy for z-levels).  
  
When we get into advanced stuff, like **chunks**, I'll explain those. For now you've got enough to process. If you really want to learn about chunks now, skip to that section.  
  
# Tutorial 1: Single overmap tile.  
  
Okay, so first, we're going to make a very simple, boring map, that uses terrain definitions in a single overmap tile. This is going to be a map that appears in fields (ie. the default field overmap terrain), a single patch of tall grass with a few extra rocks in.  
  
This demo will show you how to define terrain types, how to make a basic map, and how to have the game select a random terrain from a list. Also I'll show you how to make your spawn appear in game, first manually for playtest and then as part of natural spawns. And finally, I'll show you how I add it to GitHub as a Pull Request. Note: I barely understand GitHub, so I'm not going to explain much about it, just walk you through the basics. Likewise, I will be using JSON and while I'll explain what I'm doing, I won't go into a lot of detail on JSON formatting. If you want more information check out (mark tutorial)

## Getting Started
Open up a cool kid text editor like Notepad++, Programmer's Notepad, or Sublime Text.  Don't use plain notepad unless you hate yourself.

Navigate to your local copy of your git repository, if you already know how to do that, or otherwise just make a folder somewhere you can find it and create a new file called, I dunno, maptutorial.json.  
> Lemme learn you something big here: the game doesn't care at all what you name your json files, or where you put them as long as they're *somewhere* in the cataclysm/data/json/ folder.  *I* care a lot because I want to find your damn file if I need to change anything, so give it a descriptive name if you plan to actually submit it, and make sure it's in the right folder.

Okay. Let's start writing our map. We're going to start easy here.
```
[
]
```
There, now we have the start of a JSON file. Now let's put some shit in it. I'm about to give you a **blank template** for a map file.  Don't spend it all in one place, kid.  Anywhere I have left the letters "TK", that means you need to fill that information in later.

### Template!
```
[
  {
    "type": "mapgen",
    "method": "json",
    "om_terrain": [ "TK" ],
    "object": {
      "fill_ter": "TK",
      "rows": [
        "                        ",
        "                        ",
        "                        ",
        "                        ",
        "                        ",
        "                        ",
        "                        ",
        "                        ",
        "                        ",
        "                        ",
        "                        ",
        "                        ",
        "                        ",
        "                        ",
        "                        ",
        "                        ",
        "                        ",
        "                        ",
        "                        ",
        "                        ",
        "                        ",
        "                        ",
        "                        ",
        "                        "
      ],
      "terrain": { "TK": "TK_id" },
      "furniture": { "TK": "TK_id" }
    }
  }
]

```
Let's go through that line-by-line.
> I'm not going to help you too much with the JSON, I'm assuming you can follow along or read another guide. Basically, everything but numbers is enclosed in quotes, and you need a comma at the end of each item unless you're going to put in a closing } or ]. From there you can either blunder your way through like I did or get some help elsewhere. Clean up your code in http://dev.narc.ro/cataclysm/format.html to get it looking pretty, and if that says something useless like "syntax error" you could try www.jsonlint.com to tell you what error you actually have.

- **"type": "mapgen", "method": "json",** = This is a mapgen file in JSON. Tells the game how to read it. Critical.
- **"om_terrain":** = In this space you'll tell the game what this map is called on the overmap. Later on I'll show you how to make the overmap spawn list call on the map, based on the name you give it here.
- **"object":** = Everything between the upcoming { and closing } is going to be the map file itself. Everything before this entry is collectively all metadata about the map, ie. ways to tell the game how to handle it.
- **"fill_ter": "TK",** = "fill terrain". This is a huge timesaving measure that makes your maps much more robust. This tells the game "if I didn't tell you what kind of terrain to put in this tile, use the fill terrain". If you have a map with interiors, make sure this is whatever the most common floor you use is. Otherwise, just make it whatever the most common terrain type on your map is.
- **"rows": [** = This is the map itself. Currently our map is a 24x24 blank area. Everything between those [ ] is going to be a map.
- **"terrain": { "TK": "TK_id" }, "furniture": { "TK": "TK_id" }** = These are going to tell the game what our letters and numbers and ~s mean. Let's define those now.

### Defining Terrain and Furniture
We're going to be making a map that goes out in the field, so our main terrain types are going to be t_dirt, t_grass, t_grass_long, and t_grass_tall. We're going to make a rough circle of long and tall grass surrounded by dirt and short grass, with scattered rocks here and there.

Before we get going let's name our map "tall_grass_patch".
```
"om_terrain": [ "tall_grass_patch" ],
```

Let's start with some easy definitions.
```
"terrain": {
  " ": "t_dirt",
  ".": "t_grass",
  ",": "t_grass_long",
  "|": "t_grass_tall"
}
```
There. Now when I put a space on the mapgen file, it will make a patch of dirt. A period will make regular grass, a comma will make long grass, and a bar will make tall grass.

The only thing is, I want to make this map have some organic randomness to it. I don't want to have to draw a cool irregular shape, and have it always look identical when it appears in mapgen. Luckily, there's a crazy-ass way to do it in the game already! Check this shiznit out.

```
"terrain": {
  " ": [ "t_dirt", "t_grass", "t_grass_dead" ],
  ".": [ "t_grass", "t_grass", "t_grass_dead" ],
  ",": [ "t_grass", "t_grass_long" ],
  ";": [ "t_grass_long", "t_grass_tall" ],
  "|": "t_grass_tall"
}
```

Ah, I hear you now. "Erk, what horrible voodoo have you wrought?" Well, we all ftaghn a little sometimes. Each entry within the [ ] has an equal chance of appearing. So now, a space has a 1/3 chance of being dirt, a 1/3 chance of being grass, and a 1/3 chance of being dead grass. A period has a 2/3 chance of being grass, a 1/3 chance of being dead grass. Comma and semicolon are fifty fifty chances, and bars still are tall grass. We'll come back to this later - the exact balance sometimes needs to be filled out - but for now let's mess with our fill_ter. It uses the same formatting.
```
"fill_ter": [ "t_dirt", "t_grass", "t_grass" ],
```
So this says to the game "when in doubt, throw down some dirt or regular grass".
Okay, how about furniture? It uses exactly the same mechanic. Here's a good furniture type for our grassy field:
```
"furniture": {
  "b": "f_boulder_small",
  "B": [ "f_boulder_medium", "f_boulder_medium", "f_boulder_large" ]
}
```
Tricksy warning: So, if I plop down a letter "b" in our map now, it will go in just fine. However, remember back when I told you furniture was on another layer? The reason the 'b' will work is because I defined the fill_ter. Really, when I put a 'b' on the mapgen file, I will be putting in a boulder on top of "undefined terrain". The game now knows that when I haven't defined a bit of terrain, it should place either dirt or grass, so it will put my boulder on top of either dirt or grass. I am fine with that - it's exactly what I want, actually - but let's say I also had a small cabin, and I'd defined my fill_ter as t_floor. If that were the case, then my boulder would now sit in the field on top of a nice wood floor. If I want the boulder to sit on anything other than the fill_ter, I need a parallel entry for it in the terrain entry. Let's do one for the large boulder type as an example. We'll make it sometimes spawn with tall grass growing around it. I'll add this to my terrain entry:
```
"B": [ "t_dirt", "t_grass", "t_grass", "t_grass_tall" ],
```

OKAY! That's enough terrain defining for now. We could refine it later, but let's see what we have.
Our template now looks like this.
```
[
  {
    "type": "mapgen",
    "method": "json",
    "om_terrain": [ "tall_grass_patch" ],
    "object": {
      "fill_ter": [ "t_dirt", "t_grass", "t_grass" ],
      "rows": [
------(snip)------
      ],
      "terrain": {
        " ": [ "t_dirt", "t_grass", "t_grass_dead" ],
        ".": [ "t_grass", "t_grass", "t_grass_dead" ],
        ",": [ "t_grass", "t_grass_long" ],
        ";": [ "t_grass_long", "t_grass_tall" ],
        "|": "t_grass_tall",
        "B": [ "t_dirt", "t_grass", "t_grass", "t_grass_tall" ]
      },
      "furniture": { "b": "f_boulder_small", "B": [ "f_boulder_medium", "f_boulder_medium", "f_boulder_large" ] }
    }
  }
]

```
Sweet. Now let's get mapping.
Note that this is now currently a totally functionally mapgen file. I could put this in game. It would just be a large square made up of equal parts dirt, grass, and dead grass, but it would load and run. In fact, let's see how it looks!

TK: add image here
Needs work.

### Let's actually do some mapping
I want a blobby shape in the middle made of bars, dithering out with semicolons, then commas, then periods, then spaces. I want to intersperse some boulders at random. Let's check this out. I'm going to load up my map in my text editor, change my cursor to "insert" mode, and just type kinda randomly on top of the map.
```
        "         ...            ",
        "     .....,,...         ",
        "   ...,,,;;;;,....      ",
        "   .,,,;;;|||;;;,,,.    ",
        "   ...,,;;||||;,,,,,..  ",
        "  ..,,,;;;|||;;;;,,,,.. ",
        "  ....,,,;;;;;;,,,,,..  ",
        "   ......,,;;;;,,,,..   ",
        "   ..,,,,,,,;;,,,...    ",
        "   ....,,,;;,,,......   ",
        "  ..,,,,;;||;;;,,,,,... ",
        "  ....,,,,;;;;,,,,..... ",
        "   .....,,,;;,,,.....   ",
        "   ...,,,,;;||;;,,,...  ",
        "    ....,;;||||;;,..... ",
        "    ..,,,;;||||;;,,,,,. ",
        "   ....,,;;||||;;,,,,.. ",
        "  ...,,,,;;;||;;;,,,,.. ",
        "   ...,,,,;;;;;,,,,,... ",
        "  ......,,,,,,,,,...... ",
        "  .........,,,.......   ",
        "     .............      ",
        "                        ",
        "                        "
```

As you can see, I settled on three central bits where tall grass is guaranteed, each surrounded by a margin that will be mixed tall and long grass, then mixed long and short grass, et cetera.  What I haven't done yet is boulders, and I'll explain why in a sec. First let's see how this renders up.

TK: Image here.
Not bad, I'm cool with that.

Now, boulders. When I showed you how to design boulders, what I wasn't thinking of was that I really would like boulders to appear randomly throughout the map, not in pre-placed locations. So, I'm going to go back and do something crazy. I'm going to delete my "b" and "B" entries in furniture, and change my furniture to look like this. (brace yourselves, this will be ugly).

```
"furniture": {
  " ": [ "f_boulder_small", "f_boulder_medium", "f_boulder_large", "f_null", "f_null", "f_null", "f_null", "f_null", "f_null", "f_null", "f_null", "f_null", "f_null", "f_null", "f_null", "f_null", "f_null", "f_null", "f_null", "f_null", "f_null", "f_null", "f_null", "f_null", "f_null", "f_null", "f_null", "f_null", "f_null", "f_null" ],
  ".": [ -exactly the same stuff- ],
  ",":  [ -exactly the same stuff- ],
  ";":  [ -exactly the same stuff- ],
  "|":  [ -exactly the same stuff- ]
}
```

This horrifying mess is a temporary solution to a problem we're working on, which is that I can't define frequency of a terrain type appearing. Ugh. In the meantime I have three entries for the different boulder types, and twenty-seven entries for null, ie. no furniture. I've defined this for every terrain type I've defined, so my map will randomly place a boulder in 3/30 terrain tiles all over.  I suspect that will be too many boulders. There are 576 overmap tiles, so that will produce around 60 boulders in this scene. Well, let's see how it looks first.

TK: Screenshot and commentary.

All right. We're now done that map. The question is, how do we get it in game?

### Adding your map to the overmap spawn list
Let's navigate our way over to /data/json/overmap_terrain.json and open that file in our text editor. You'll see a whole bunch overmap terrain entries (a shock, I know).  Use your search function to find the phrase: 
```
"id": "field",
```
We're going to copy that entry, I'll explain what's in it, and then we'll add our new entry in.

```
  {
    "type": "overmap_terrain",
    "id": "field",
    "name": "field",
    "sym": 46,
    "color": "brown",
    "see_cost": 2,
    "extras": "field",
    "spawns": { "group": "GROUP_FOREST", "population": [ 0, 1 ], "chance": 13 },
    "flags": [ "NO_ROTATE" ]
  },
```
- **"type": "overmap_terrain",** = Same as the similar entry in our mapgen file, this tells the game we're making an overmap terrain entry. Necessary.
- **"id": "field",** = This is the unique ID of the terrain entry from our mapgen file! Well, *this* isn't. This is "field". We don't want to make another field, so we'll replace that.
- **"name": "field",** = This is the NON unique name that will be displayed to the player. In this case, we are going to keep the name "field", because we want our new map extra to appear to be just another normal field tile.
- **"sym": 46,** = This is the ASCII tile the map will use. http://www.asciitable.com/ is a good quick reference for which tiles are which. 46 is a period. That's why field tiles are represented in game with a period.
- **"color": "brown",** = I'm gonna get technical here: this is the colour that the game uses to display the tile. I don't know if there's a master list of colour choices, but most common colour names work. If there is I will link it here for you some time.
- **"see_cost": 2,** = This tells the game how hard it is to see past this overmap tile. It helps determine how far you can explore. Don't just set this to some arbitrary number, copy an existing value from a similar overmap type - buildings for buildings, forest for treeish things, or 2 for an open field like we're about to add.
- **"extras": "field",** = This tells the game that any map extras that belong in "fields" can spawn in our map tile type. We want that, because we want stuff like crashed helicopters to be able to crash in our grassy field too.
- **"spawns": { "group": "GROUP_FOREST", "population": [ 0, 1 ], "chance": 13 },** = This is a cool one. This tells the game what monsters can spawn in our tile. GROUP_FOREST is probably defined in monstergroup.json at the time of this writing, but that file is a damned mess so it might not be there when you're doing this. Check out monstergroup.json, and the subfolder json/monstergroups/ for a file that might apply to your region. Or use the "search multiple files" option in your fancy text editor for "id": "GROUP_FOREST" to find the file wherever it is. Chances are that if you want a monstergroup, there already is a good spawn group for your needs. Gonna be honest with you here, I don't know exactly how "population" and "chance" work. At some point I'll get a pro to break that down for us, but for our purposes here we'll do what I usually do: copy the spawn information from a location that is about what we want. This is generally a great idea anyway, because it keeps your location from being suddenly monsterrific compared to other similar areas.
- **"flags": [ "NO_ROTATE" ]** = Sometimes you don't want your tile to rotate. We don't care if our tile rotates, so we'll delete that.
 
 So now we have:
 ```
   {
    "type": "overmap_terrain",
    "id": "tall_grass_patch",
    "name": "field",
    "sym": 46,
    "color": "brown",
    "see_cost": 2,
    "extras": "field",
    "spawns": { "group": "GROUP_FOREST", "population": [ 0, 1 ], "chance": 13 },
  },
  ```

This is all we would need ot add to the overmap_terrain file to make it possible to spawn our new field type in the game. For debug purposes when I'm testing, I make the name something I can work with in the terrain editor (something searchable) so for my WIP file here I've had that line set to:
```
"name": "field_erkerk"
```
and then when I want to check it out, I go to the debug menu, edit overmap terrain, press 't', press '/' and type 'erke' and see my new terrain.

TK: Writing ongoing

# Tutorial 2: a complex multitile map.
Okay, you have the basics now, I hope. Let's get going on something real. We're going to make a map that spans two overmap tiles and three z-levels. We'll go into detail on item spawns and monster spawns, and I'll show you a few tricks for each. I'll also talk about good design and making your map look cool. We will use nested mapgen to place a chunk -
## What's a chunk?
What do you mean "what's a chunk"? We talked about this back in the introduction.
### No we didn't, you said you'd explain later.
Oh... I guess I did. OK. Chunks and nested mapgen.

Nested mapgen is kinda what it sounds like, but I know that's not a very good explanation. Basically, you can tell the mapgen file to call up *another* mapgen file, typically a smaller one, inside of it. That smaller map is called a **chunk**. Why do this? Oh, only because it's *awesome*.

When you call a chunk, you call it by its mapgen ID. If you have more than one chunk with the same ID, the game *selects a random one from the list*. Pause for a second and let that sink in: you can make buildings that are hollow shells of walls, put each room in a nested mapgen file, and have it mix and match rooms! This is a slick, easy way to create simple procedurally generated maps. Not only that, but each chunk has its own mapgen file, so it can use a different palette too. That's a surprisingly helpful trick to keep reusing common letters ... W can mean washing machine in the laundry room, water tank in the garage, and wardrobe in the bedroom.

You can use chunks to make identical variants of maps with a chance for a rare spawn. Make a safe room that spawns with an NPC sometimes, an active turret others. Make a wall that is intact in some versions of the building, blown apart in the cataclysm in others. The possibilities are totally endless.

## Starting a large map from template

Here's a cool thing to know: when you make a map that spans two or more overmap tiles, you can use a single "rows" [] entry for them, and the game will understand to break them apart into two files. We could do this vertically or horizontally, but it's easier to copy and paste it vertically so that's what I'm going to go with. I'll pull up my template from before, edit it for two map files (named tut_map_erk1 and tut_map_erk2) and post it here. I'll post my standard "work in progress" map in its entirety. It has some extra stuff that we'll discuss.

```
[
  {
    "type": "mapgen",
    "method": "json",
    "om_terrain": [ [ "tut_map_erk1" ], [ "tut_map_erk2" ] ],
    "object": {
      "fill_ter": "TK",
      "rows": [
        "                        ",
        "                        ",
        "                        ",
        "                        ",
        "                        ",
        "                        ",
        "                        ",
        "                        ",
        "                        ",
        "                        ",
        "                        ",
        "                        ",
        "                        ",
        "                        ",
        "                        ",
        "                        ",
        "                        ",
        "                        ",
        "                        ",
        "                        ",
        "                        ",
        "                        ",
        "                        ",
        "                        ",
        
        "                        ",
        "                        ",
        "                        ",
        "                        ",
        "                        ",
        "                        ",
        "                        ",
        "                        ",
        "                        ",
        "                        ",
        "                        ",
        "                        ",
        "                        ",
        "                        ",
        "                        ",
        "                        ",
        "                        ",
        "                        ",
        "                        ",
        "                        ",
        "                        ",
        "                        ",
        "                        ",
        "                        "
      ],
      "terrain": { "TK": "TK_id" },
      "furniture": { "TK": "TK_id" },
      "palettes": "TK"
    }
  },
  {
    "type": "overmap_terrain",
    "id": "tut_map_erk1",
    "name": "erk tutorial map 1",
    "sym": 66,
    "color": "red",
    "see_cost": 4
  },
  {
    "type": "overmap_terrain",
    "id": "tut_map_erk2",
    "name": "erk tutorial map 2",
    "sym": 67,
    "color": "red",
    "see_cost": 4
  }
]
```

As you can see, I've added the overmap definitions right there in the file. They're only there while I'm testing. Keeping it all in one file lets me quickly copy it back and forth between my GitHub working folder and the copy of the game where I'm playtesting. When I'm done my final draft I'll split those out to the appropriate file. Also note that I left a convenient line break between the two maps. I'll delete that at some point, but for now I want to know where the seam between the two maps is, because certain things can't cross that seam.

Look up at the om_terrain entry for the mapgen file. It has two entries. Each square bracketed entry (each "array" if you talk the talk, which I don't normally) represents a different map ID vertically. If I had laid them out horizontally it would instead read:
```
"om_terrain": [ "tut_map_erk1", "tut_map_erk2" ],
```
and if I had four maps arranged in a 48x48 square, it would look like this:
```
"om_terrain": [
  [ "tut_map_erk1", "tut_map_erk2" ],
  [ "tut_map_erk3", "tut_map_erk4" ]
] 
```
and on and on for as many map IDs as you need.

OK, that's all I think we need to know about the mapgen file except for one new addition since the last time: 
```
"palettes": "TK"
```
Since I plan to add a nested map this time, I would like a palette. Otherwise, I'd need to redefine my terrain and furniture every time I went into a nested mapgen file. So let's go on to that.

### Working with a palette
Palettes just contain the furniture and terrain definitions we talked about in tutorial 1, but in a portable format, so you can summon up the palette whenever you need and not have to redefine that stuff.

> Sneaky trick: If you define specific furniture and terrain entries within the mapgen object *before you call the palette*, they will override any palette entries that might use . This can be handy on occasion, like when you want a nested mapgen file to be *almost* the same as the source but you're going to hijack that precious letter "W" for a different, more nefarious purpose.

So, let's create a palette for our new map. We're going to be building a 3-floor office building with a downstairs lobby, so we'll need regular gyprock walls as well as concrete walls, windows, a linoleum floor, a bit of red carpet, and some office furnishings. I don't generally do this from scratch, I usually borrow entries from a similar file... but for today, let's make one from scratch. We'll make the fill_ter "t_linoleum_white" for this one, seems pretty officey to me. I'll keep this palette at the top of my map file, although if I start using it in more than one file I'll move it to data/json/mapgen_palettes and put it into its own file.

```
  {
    "type": "palette",
    "id": "office_city",
    "terrain": {
      ".": [ "t_dirt", "t_grass", "t_grass" ],
      "*": "t_shrub",
      "#": "t_sconc_wall",
      "|": "t_wall",
      " ": "t_linoleum_white",
      "_": "t_carpet_red",
      "+": "t_door_c",
      "-": "t_door_locked_interior",
      "=": "t_door_glass_gray_c",
      "]": "t_sidewalk",
      "0": "t_window_alarm",
      "1": "t_window",
      "2": "t_window_domestic",
      ":": "t_door_glass_c",
      ";": "t_door_locked",
      "m": "t_door_metal",
      "M": "t_door_metal_locked",
      "X": "t_console_broken",
      "<": "t_stairs_up",
      ">": "t_stairs_down"
    },
    "furniture": {
      "$": "f_safe_l",
      "A": "f_air_conditioner",
      "b": "f_bench",
      "B": "f_bookcase",
      "c": "f_chair",
      "C": "f_armchair",
      "d": "f_desk",
      "D": "f_counter",
      "f": "f_fridge",
      "g": "f_glass_cabinet",
      "i": "f_filing_cabinet",
      "l": "f_locker",
      "o": "f_bookcase",
      "r": "f_trashcan",
      "s": "f_stool",
      "S": "f_sink",
      "U": "f_utility_shelf",
      "W": "f_water_heater",
      "t": "f_table",      
      "y": [ "f_indoor_plant_y", "f_indoor_plant_x" ]
    },
    "toilets": { "T": {  } }
  }
```
Now I'm going to start mapping. I will start by drawing the sidewalk around the building, and outlining the exterior wall in concrete. Then I'll put a couple doors in the middle of the building and create a double-wide hallway running up the middle, using that convenient 
```
[
  {
    "type": "mapgen",
    "method": "json",
    "om_terrain": [ [ "tut_map_erk1" ], [ "tut_map_erk2" ] ],
    "object": {
      "fill_ter": "t_linoleum_white",
      "rows": [
        "]]]]]]]]]]]]]]]]]]]]]]]]",
        "]######################]",
        "]#                    #]",
        "]#                    #]",
        "]#                    #]",
        "]#                    #]",
        "]#                    #]",
        "]#                    #]",
        "]#                    #]",
        "]#                    #]",
        "]#                    #]",
        "]#                    #]",
        "]#                    #]",
        "]#                    #]",
        "]#                    #]",
        "]#                    #]",
        "]#                    #]",
        "]#                    #]",
        "]#                    #]",
        "]#||||||              #]",
        "]#     |              #]",
        "]#     |              #]",
        "##     |||||||||||||||#]",
        "=      =             y#]",
        "=      =             <#]",
        "##     |||||||||||||||#]",
        "]#     |              #]",
        "]#     |              #]",
        "]#||||||              #]",
        "]#                    #]",
        "]#                    #]",
        "]#                    #]",
        "]#                    #]",
        "]#                    #]",
        "]#                    #]",
        "]#                    #]",
        "]#                    #]",
        "]#                    #]",
        "]#                    #]",
        "]#                    #]",
        "]#                    #]",
        "]#                    #]",
        "]#                    #]",
        "]#                    #]",
        "]#                    #]",
        "]#                    #]",
        "]######################]",
        "]]]]]]]]]]]]]]]]]]]]]]]]"
      ],
      "palettes": "office_city"
    }
  },
```

Now I'm going to lay out some rooms in the north half. I'm going to make that area into a few enclosed rooms. I want to make these rooms into nested map chunks, and nested maps need to be squares, so I'll make sure all the rooms are square. I don't *have* to do it this way - I could make a 5x4 room and just make sure all the chunks I used in it had a wall along the bottom or placed "t_null" there.

I'm also going to make sure to stick a bathroom in there. Never forget bathrooms.

```
        "]]]]]]]]]]]]]]]]]]]]]]]]",
        "]######################]",
        "]#     |ST|TS|8       #]",
        "]#     |  |  |7       #]",
        "]#     |r | r|6       #]",
        "]#     |||||||5       #]",
        "]#     |     |4       #]",
        "]#||||||     |3       #]",
        "]#     |     |2       #]",
        "]#     |     |12345678#]",
        "]#     |  ||||||||||||#]",
        "]#     |  |1          #]",
        "]#     |  |0          #]",
        "]#||||||  |9          #]",
        "]#    1|  |8          #]",
        "]#    2|  |7          #]",
        "]#    3|  |6          #]",
        "]#    4|  |5          #]",
        "]#12345|  |4          #]",
        "]#||||||  |3          #]",
        "]#     |  |2          #]",
        "]#     |  |12345678901#]",
        "##     |==||||||||||||#]",
        "=      =             y#]",
```