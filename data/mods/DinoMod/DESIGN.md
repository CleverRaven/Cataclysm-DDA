# Design Document SPOILERS AHEAD
The core idea of DinoMod is this: https://tvtropes.org/pmwiki/pmwiki.php/Main/EverythingsBetterWithDinosaurs
The purpose of this mod is to make Cataclysm a more fun game to play and develop for, showing off new mechanics and bringing life to parts of the game that aren't as well-developed.

# What belongs in DinoMod?
Dinosaurs and content based around dinosaurs that for whatever reason can't just go into the vanilla game.

# Realism vs. Fun
There is more space in mods for fun. In general the vanilla living dinos should be as realistic as possible to the science we know but the zombie, CBM, and fungal dinos can go anywhere as long as it's fun.

# Adding, replacing, removing
This mod is about adding content. In general it should have as light a touch on the vanilla game as possible. This helps us keep it working by itself and with other mods.

# How to contribute
This mod is distributed with the base game, so any content contributed will have to be submitted to the Github site. You can talk with the mod maintainers about your ideas on Discord.

# Where should new dinosaurs spawn?
North American dinos should be added to dinosaur and wilderness monster groups. Zombie variants should be added to the zinosaur monster groups. Dinos from other parts of the world should be added to the labs monster group and/or get a new dedicated lab finale variant just for them, especially good for finales if they're very dangerous. Small dinos that can't zombify are good options for CBM dinos. Fungal zombie variants get added to fungal spawn lists.

# How to add a dinosaur
As of this writing, each dinosaur touches at least ten different JSON files, listed here by folder. Please put the new dino in the same order in all files, near similar dinos, organized by real world taxonomy. 

Main DinoMod folder: 

* cooking_components.json is where you add the dinosaur egg to allow it to be cooked, 
* egg.json is where you create the dino egg

monstergroups folder:

* dinosaur.json is where you add the dinosaur to spawn in special DinoMod locations
* monstergroups_egg.json is where you add the hatchling to be spawned from its own egg, and from random eggs
* wilderness.json is where your dino will be spawned in natural settings. Forests should stay safe.
* zinosaur.json  adds the zombified version to zombie spawn lists

monsters folder:

* dinosaur.json is where you finally create the dino itself. copy-from can be a good move to keep things tidy if there is already a similar dino
* hatchling.json is where freshly hatched dinos go. Tiny dinos grow to adults directly, but larger ones (15 kg or greater) grow into...
* juvenile.json is where juveniles go. They're five times bigger but still pretty tiny by dino standards. They grow to be adults in adult weight in kg divided by six days or one year, whichever is shorter.  Sauropods are they exception, they should be 1000 kg and L so they can defend themselves some.
* zed-dinosaur.json is where new zombified dinos go. copy-from can be very helpful here.
