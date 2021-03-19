# Design Document
* Aftershock is being developed into a full conversion mod set in a mostly-abandoned extrasolar colony named Salus IV.  The player has arrived on Salus for various reasons (depending on their profession).  Eventually, we hope to have multiple potential end game situations, ranging from finding what you came here for and leaving the planet with enough money to retire, to more-secret endings.  There will no longer be zombies; instead, there are a variety of alien and human-based enemies.  Toe-to-toe combat will likely be more deadly in Aftershock, with more emphasis on picking your battles and only fighting when absolutely necessary.  Special locations may have a different combat aesthetic; for example, you may enter a dungeon-like zone with hordes of small weak enemies that can't be avoided.  As part of this concept, in which different situations require different solutions, we have implemented bionic slots but made the process of changing out CBMs much easier with less chance of failure.  This is to encourage players to change their loadout based on where they are going and what they are doing.
* As it becomes a total conversion mod, it will look very different from the base game of Cataclysm: Dark Days Ahead and may not interact kindly with other mods.  If you've been away for a while, you may wonder where some parts of Aftershock have gone.  In the process of becoming a total conversion mod, we are eliminating items that no longer fit the theme.  For example, all the Mad Max aesthetics have been removed.  Any additions must either fit the general theme or have story-driven reasons for the divergence, such as NPC quests to special locations.

### Areas seeking contributors
1. Faction interactions and new factions.  Currently there are two new factions in Aftershock: PrepNet and the Whately Clan.  PrepNet is seeking to build an independent colony and the Whatelys are mad scientists.
2. Uplifted animal mobs and mutation lines are always wanted.  We'd love to see someone make a Snow Hare or Yeti line.  Currently we have Mi-Go, Mastodon, and Cecaelian mutation trees.
3. Hi-tech item recipes.  Aftershock has its own special crafting for ultratech and we'd love to see what you come up with.
4. Missions, locations, and lore snippets.  We want this to feel like a world just as real as Cataclysm Prime.  Please feel free to reach out to us about ideas and implementations.
5. Alien world basics would be especially desirable at this time.  Flora, fauna, terrain, and furniture that make it clear we are no longer on earth.

# Size as of 0.F Stable :  31,084 Lines

# Here be dragons!

These are the files for Aftershock.  Whatever you're looking for has been sorted into subfolders for ease of access:

### itemgroups

Contains json data for itemgroups.

### items

Contains json data for all item types.

### maps

Contains json data for map generation, chunks, terrain, map specials, and furniture.

### mobs

Contains json data for new monsters and monster spawn groups.

### monsterdrops

Contains the json which controls what monsters drop on death.

### mutations

Contains json for mutations and threshold dreams.

### player

Contains json data for things that affect the player; bionics, professions, techniques, status effects, etc.

### recipes

Contains json data for all recipes, sorted by category or niche.

### vehicles

Contains json data for vehicles, vehicle parts, and vehicle spawn groups.

### npcs

Contains json data for npcs and factions; the second is about the new factions PrepPhyle and Whately Clan.  (A faction design document is in progress.)  The PrepPhyle are a group of frontier squatters taking advantage of decaying colony infrastructure to set up an independent colony separate from Corporate and Earthgov influences.  The Whately Clan are a possibly-exiled branch of a corporate executive family.  They are mad, bad, and dangerous to know.  Mutant NPCs are also located in this folder.

### spells

Contains json data for all spells that are used to create unique effects in game.
