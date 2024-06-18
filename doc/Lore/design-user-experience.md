# Design - User Experience

Explore, fight and survive in an expansive post-apocalyptic game world.

The core tenet of dark days ahead is if it works in reality, it works in the game.  Scavenge and craft makeshift weapons, improvise shelter, outmaneuver enemies, find, repair and drive vehicles.  Deal with monsters using your brains, melee weapons, firearms, traps, explosives, other monsters, fire, and if all else fails, a truck or two.

### User Experience Table of Contents
* [Overview](#overview)
* [Deployment](#deployment)
* [Background](#background)
* [Key Features](#key-features)
* [Depth](#depth)
* [Reward System](#reward-system)
* [Meaningfulness](#meaningfulness)
* [Collectibles, Easter Eggs & Unlockables](#collectibles)
* [World](#world)

## Overview

DDA is a turn-based survival RPG that utilizes a top-down, grid-based view.  The graphics are either character-based or use layered tiles.

DDA presents an uncompromising survival scenario to overcome, but also presents a huge variety of options for problem solving, both of which are deeply rooted in a reality-based view of the post-apocalyptic scenario.  This grounding results in sometimes unexpected inversions of gaming tropes, where storage capacity of clothing is more important than armor, or weight of highly effective gear outweighs its effectiveness.

## Deployment

DDA is built as a standalone executable for Linux, Windows, OSX, and Android.  Ports to other platforms are welcome.

The primary platform target is laptop or desktop computers, but affordances for other platforms, such as smartphones are welcome as long as they are not invasive.

## Background

DDA has two kinds of inspirations, games and game elements that it emulates, and games and game elements that it avoids.

### Games to emulate

#### Ultima, especially Ultima III and IV.

The player-guided exploration of the Ultima games was groundbreaking, as was the insight that the game doesn't have to be about defeating enemies and killing the big bad guy.

#### Fallout Series.

Surviving in a realistic setting while scavenging for resources as your dominant means of survival is the main contribution of Fallout to DDA.  Special mention to Fallout Tactics for fusing many of the concepts from Fallout and X-com games.

#### X-Com 1 and 2.

Early X-Com games made deep and punishing tactical turn based combat interspersed with long-term character building a reality.

#### Dwarf Fortress

DF is an inspiration in many ways, it takes itself seriously without *being* too serious, it doesn't suffer (much) from "chosen one syndrome", it shows that investment in game world system verisimilitude pays off once it reaches completion, and that players have a taste for punishing and deep gameplay.

### Games to avoid

#### JRPGs

Suffer from "chosen one syndrome" to a huge degree.  Limited ability to interact with the environment.  Game world is often barely deep enough to hold up if you play as expected, *any* deviation from the chosen path usually causes the player to encounter shortcomings.

#### Minecraft

Minecraft suffers from "Bang rocks together crafting" to a massive degree.  Crafting is evocative rather than literal, leading to absurdities like a wooden wood axe.

#### Dwarf Fortress

DF seems to over-invest in world building.  The progress made by the game is astounding, but the lack of pragmatism around some aspects of the game are worrying.  Additionally the lack of emphasis on UI is IMO a much greater barrier to players than the depth of the game, which is surmountable.

## Key Features

Explore a gigantic procedurally generated world.

Unique and deep focus on the complex implications of an apocalyptic scenario.

DDA uses a hybrid turn-based system.  Each action has a move cost, many of which are procedurally determined.  Once actions deplete an actor's move points, their turn ends and any other game entities act for a similar duration.  Game actors receive an allotment of move points each turn based on their stats and conditions.  All game entities, including the player character, monsters and NPCs use the same system.

The player character in DDA navigates a 3D grid resembling voxels.  Motion is predominately within the horizontal plane, but stairs, ladders, pits, and wall climbing actions can move the player or other entities vertically as well.

The player character can see other entities in a wide area around themselves, with vision mediated by both dynamic lighting and dynamic line of sight factored in.  The player is also notified when the player character can perceive noises.

The player character can attack adjacent (and sometimes nearby) enemies with melee weapons, and distant enemies with various ranged weapons.

A great deal of game progression is gated by acquisition of items.  The player can scavenge items from the procedurally generated towns and cities, as well as more exotic locations.

The player is able to supplement these scavenged goods by crafting a wide variety of items, ranging from mundane cooking to advanced chemistry.

Structures provide a valuable resource as well, which can be found and modified or constructed from scratch.

Vehicles in various states of repair litter the world, and can be driven, repaired and modified.

NPCs can form factions either independently or with the player.

Players can acquire and install bionic modules that give them abilities unheard of in unaugmented individuals.

Alternately, the player can more directly abandon their humanity and mutate into an entirely different kind of creature.

### Senses

The player character can perceive their surroundings by sight, hearing, smell and touch.

Sight is managed by dynamic field of view and lighting systems, and works over a relatively long range, ideally approaching real-world view distances.

The player receives a rich set of auditory cues from their environment, ranging from footsteps and combat noises to environmental sounds.

A normal humans sense of smell is remarkably poor, but can still alert the player to otherwise unperceived dangers.

As a last resort, a player robbed of their sense of sight might still navigate by their sense of touch.

Additionally, the player has a limited memory of the terrain they have explored.

### Combat

There are many options for causing damage in DDA, but unlike many games, DDA allows the use of a huge number of very suboptimal weapons, and most weapons have tradeoffs.

Firearms are highly effective and operate at range, but use limited ammunition, are universally uncraftable and cause significant noise to attract enemies.

Other ranged weapons like bows and crossbows trade a great deal of range and damage output for craftable ammunition and less noise when fired.

Melee weapons are often craftable and require no consumables, but have limited damage output, place the player in danger, and consume stamina rapidly when used.

Any item the player can pick up can be thrown, with wildly varying results based on player and item stats.

Unarmed combat is possible, but is even more dangerous than armed melee combat, and usually very ineffective.

### Scavenging

The player character is expected to obtain the vast majority of their possessions by scavenging items made before the cataclysm.  With the vast reduction in population, the surplus of durable goods available to each survivor is staggering.

In general, goods should be distributed in a way representative of how they would be distributed in reality, and if this negatively impacts game balance, other game aspects such as placement of enemies should be adjusted to bring things in balance rather than adjusting the placement of items.

This results in a glut of marginally useful resources, but scarcity of ideal resources.

For example cars are extraordinarily common, but robust and functional cars that are both in full working order and not surrounded by dangerous enemies are rare.

Likewise improvised melee weapons appear in vast numbers, but quality melee weapons and powerful firearms and ammunition are somewhat harder to come by.

Clothing is totally ubiquitous, but an article of clothing that improves on the player's current ensemble and fits properly is cause for celebration.

### Crafting

The player can accumulate raw materials and tools to craft a wide variety of items to supplement what they scavenge.

Skill,tool, raw material and time requirements to craft items reflect the parameters of crafting the corresponding item in reality.

This places many useful items out of reach of most survivors, in particular staples such as electronics  internal combustion engines, most firearms and gunpowder are out of reach.

### Structures

The player can also build and modify permanent structures, providing shelter from both the environment and marauding monsters.

### Vehicles

Scavenged or built, vehicles provide a trump card of sorts for the player, providing mobility, storage and shelter all at once, at the cost of significant upkeep.

Pre-cataclysm vehicles are modeled on existing real world vehicles.

The survivor can repair, modify and even build vehicles from scratch, but the quality is never the same as the original.

### Bionics

The player can acquire Compact Bionic Modules via various means and install them in their own bodies.  Once installed, these modules can provide capabilities unheard of in the unaugmented, but at the cost of pervasive tradeoffs due to side effects inflicted by the bionics.  Bionics must also be supplied with power by installing battery modules and some kind of charging module.  The most straightforward power source is a cable charging system that can rapidly refill the player's reserves as long as they arrange for an external source of power, but other modules can provide a trickle of power from various other sources.

### Mutations

Various mutagenic substances exist that can manipulate the player's own body structure, either helping or hindering the player's abilities in unpredictable ways.  Some variants of these substances can direct the progress of the mutation in a direction more or less advantageous to the player.

## Reward System

Cataclysms reward system is built around roleplay, survival and exploration.

The key concept of the game is, "what would you do in the Cataclysm?", and the game allows you to answer that question in a bewildering number of ways.

The goal of game difficulty is to start out with survival being difficult, and get steadily more difficult over time, requiring mastery of a succession of game systems in order to continue to survive and thrive.

Acquiring mastery of the game sufficient to overcome this difficulty curve is the primary form of advancement for a player.

Cataclysm also contains a vast amount of content to explore and discover, rewarding the player with a constant stream of this content, combined and re-combined over and over by procedural generation.

## Meaningfulness

The core message to Cataclysm is that we are all cogs in a massive, uncaring universe.

## World

The primary setting of the game is present-day (2019 as of the time of writing) Earth.  The current setting is New England in the United States, but that is not a core value, and there is interest in expanding the game to other regions.

Super-technology exists, but it is mostly restricted to secret lab facilities and military special forces.  See the dedicated [technology](./technology.md) page for more information on what is specifically appropriate to the setting.

That all changed during the cataclysm, a several day long event where horrors from other dimensions flooded into the world, destroying, killing and in some cases creating, rendering the world almost unrecognizable.

After the Cataclysm, the invasion ebbed, leaving humanity crippled but not completely destroyed.  Some of the invaders remain, carving out niches for themselves, but most retreated elsewhere.

The Cataclysm:DDA game world is very much modeled on the real world, with similar geography and demographics.  Balance is adjusted by placement of otherworldly or other fictional elements such as zombies and robots in addition to natural hazards.

### Locations

#### Cities

Dense urban centers present a great deal of risk, and a great deal of reward for survivors.

Zombies or other previously-human forces tend to congregate in these areas, rendering them extremely dangerous by sheer numbers if nothing else.

On the other hand, cities are where most of the resources of the previous civilization are, ready for the taking for anyone who can survive the horde.

#### Towns

Small towns represent a good risk vs reward for early game characters.  Zombies or other monsters are attracted to previous population centers, but the smaller ones will not attract very large populations.  On the other hand, the available loot is significant, but will be lacking in exotic and high-end gear such as technology, tactical clothing, and weapons.

The primary risk is attracting an entire town full of zombies by making too much noise.

Towns present ample opportunities to maneuver, break line of sight, hide, and set up ambushes, but present many opportunities for an unwary survivor to become cornered.

If a survivor can clear an area, food, water, tools, weapons and shelter are all readily available, even to a survivor with no particular survival skills.

Vehicular access to towns is ample due to the road network, and only significantly hampered by hordes of undead and the occasional abandoned or wrecked car.

#### Fields

Fields are wide open and contain sparse resources.

Threats are few and far between, but difficult to escape except by outrunning them.

Vehicles with basic off-roading capabilities can navigate fields with ease, but it requires more ability in inclement weather.

Fields are ideal building sites for shelters, but do not supply the resources for construction.

#### Hills

Hills present both opportunities and danger.  They provide both the player and enemies ample opportunity to hide, which is mostly to the player's benefit unless heavily outnumbered.

TODO: enemies specific to hills? Caves?

#### Forests

Forests present a different set of challenges for the player.  On one hand there is a huge amount of space to lose oneself in, and huge amounts of resources available for the taking if you have appropriate skills and equipment.  On the other hand forests are just as deadly to the unprepared as a city full of zombies due to exposure and starvation.

Tactically forests provide a great resource for evading pursuers by breaking line of sight by using the dense growths of trees and underbrush, but at the same time the player might be the one ambushed.

Enemies in forests are less pervasive than in towns and cities, but can still occur in dense pockets.

#### Rivers/Lakes

Without any form of water borne vehicle, rivers represent a hard barrier to player movement.  While an unencumbered player can likely swim across a river, that leaves them vulnerable once they reach the other side.

With a water-borne vehicle, the player may achieve relative safety, but at the cost of limiting their ability to access many resources.

Rivers provide a natural source of both water and some food in the form of fish, but no access to materials for crafting.