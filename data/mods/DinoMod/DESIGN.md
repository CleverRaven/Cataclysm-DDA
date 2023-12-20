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
North American dinos should be added to dinosaur and wilderness monster groups. Zombie variants should be added to the zinosaur monster groups. Dinos from other parts of the world should be added to the labs monster group and their region lists and/or get a new dedicated lab finale variant just for them, especially good for finales if they're very dangerous. Small dinos that can't zombify are good options for CBM dinos. Fungal zombie variants get added to fungal spawn lists.

# How to add a dinosaur
As of this writing, each dinosaur touches at least ten different JSON files, listed here by folder. Please put the new dino in the same order in all files, near similar dinos, organized by real world taxonomy. 

Main DinoMod folder: 

* monster_factions.json is where custom dino factions go. Plant eaters are pretty simple but predators have three each usually to manage famiies

items folder:

* egg.json is where you create the dino egg

monstergroups folder:

* dinosaur.json is where you add the dinosaur to spawn in special DinoMod locations
* fungi.json is where you add the fungal variant to spawn in fungal locations
* lab.json is where you add the dinosaur if it is not native to North America
* misc.json has all kinds of odd lists, especially for safe, scavenging, and underground dinos
* monstergroups_egg.json is where you add the hatchling to be spawned from its own egg, and from random eggs
* wilderness.json is where your dino will be spawned in natural settings. Forests should stay safe.
* zinosaur.json  adds the zombified version to zombie spawn lists and is where the zombie upgrade groups go for now

monsters folder:

* dinosaur.json is where you finally create the dino itself. copy-from can be a good move to keep things tidy if there is already a similar dino
* fungus.json is where you add the fungal variant
* hatchling.json is where freshly hatched dinos go. Tiny dinos grow to adults directly, but larger ones (15 kg or greater) grow into...
* juvenile.json is where juveniles go. They should be about half adult size and HP. They grow to be adults in adult weight in kg divided by six days or one year, whichever is shorter.
* zed-dinosaur.json is where new zombified dinos go. copy-from can be very helpful here.
* zinosaur_burned.json is where the burned zombie variant goes
* zinosaur_upgrade.json is where upgraded versions go

requirements folder:

* cooking_components.json is where you add the dinosaur egg to allow it to be cooked

# How to add a dinosaur nest
This is much easier! 

* Each dinosaur gets its own nest mapgen file in \mapgen\map_extras\ you can copy from a similar dino there
* You can define the map extra in \overmap\map_extras.json
* Finally you tell it to spawn in the right biome in regional_overlay.json . Predator nests go in swamps, everything else in forests
  
# Lore

* Dinosaurs came here through portals at the very beginning of the portal storm
* The Swampers expected this and were prepared.  They believe that the dinosaurs are here to restore our world to be back in harmony with the rest
* Before all this, some of the labs were already pulling dinos and dino material back here for study, including some with CBMs.  They got out.
* The Exodii don't know that this was a dino free world. That's very rare.

