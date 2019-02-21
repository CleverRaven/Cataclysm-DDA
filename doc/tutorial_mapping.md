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

**"type": "mapgen",
"method": "json",** = This is a mapgen file in JSON. Tells the game how to read it. Critical.
**"om_terrain":** = In this space you'll tell the game what this map is called on the overmap. Later on I'll show you how to make the overmap spawn list call on the map, based on the name you give it here.
**"object":** = Everything between the upcoming { and closing } is going to be the map file itself. Everything before this entry is collectively all metadata about the map, ie. ways to tell the game how to handle it.
**"fill_ter": "TK",** = "fill terrain". This is a huge timesaving measure that makes your maps much more robust. This tells the game "if I didn't tell you what kind of terrain to put in this tile, use the fill terrain". If you have a map with interiors, make sure this is whatever the most common floor you use is. Otherwise, just make it whatever the most common terrain type on your map is.
**"rows": [** = This is the map itself. Currently our map is a 24x24 blank area. Everything between those [ ] is going to be a map.
**"terrain": { "TK": "TK_id" },
"furniture": { "TK": "TK_id" }** = These are going to tell the game what our letters and numbers and ~s mean. Let's define those now.

### Defining Terrain and Furniture
We're going to be making a map that goes out in the field, so our main terrain types are going to be t_dirt, t_grass, t_grass_long, and t_grass_tall. We're going to make a rough circle of long and tall grass surrounded by dirt and short grass, with scattered rocks here and there.

Before we get going let's name our map "tall_grass_patch".
```"om_terrain": [ "tall_grass_patch" ],```


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
        "     ..........         ",
        "   ...............      ",
        "   .................    ",
        "   ...................  ",
        "  ..................... ",
        "  ....................  ",
        "   ..................   ",
        "   .................    ",
        "   ..................   ",
        "  ..................... ",
        "  ..................... ",
        "   ..................   ",
        "   ...................  ",
        "    ................... ",
        "    ................... ",
        "   .................... ",
        "  ..................... ",
        "   .................... ",
        "  ..................... ",
        "  ...................   ",
        "     .............      ",
        "                        ",
        "                        "
```

TK: Out of typing time, back soon
