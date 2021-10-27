This document is under construction.

# Overview of game balance principles



**Summary**
The driving goal of balance in DDA is to support **versimilitude in the scenario**.  In other words we want the right *things* to be present to match reality, but also when those things are used to accomplish goals, they should be appoximately as effective at doing so as their real-world counterparts.  Of note, this is not the same as pursuing all aspects of realism.  New features added should have the goal of improving verissimilitude rather than complexity for complexity's sake.

This means that *solutions to problems* should come from the same angle.  If something is not working as it should, the answer is to assess how it should work in reality, and figure out a way to better simulate that.  We find that reality tends to be quite balanced, and having things work as one would expect makes for a fun game.  

This concept is backstopped by the concept that reality has a reasonable and interesting set of balance tradeoffs, and approaching this set of tradeoffs makes for a fun game.

Some examples:  
- Bows and crossbows have naturally poor armour penetration compared to guns.  The solution to this was not to artificially inflate their armour penetration, but to add gaps in armour for archers to hit.  This led to a robust and interesting weakpoints system instead of an eternal shuffling of stats.
- Modern vehicles are quite easy to operate and effective at their core competencies of moving people and items around on reasonably well-prepared surfaces, as such they are incredibly valuable and powerful tools in the game as well.  However, their use should be limited by the difficulty of getting a vehicle to operate without a key, congestion of roads in and out of population centers, the eventual expiration of common fuels, and the difficulty of repairing vehicle chassis as they take damage from impacts and use.
- Guns deal FAR more damage than elastic projectile launchers like bows and crossbows.  However, they are loud and require looted ammunition.  Nonetheless, bows and crossbows should always be a niche/challenge weapon.
- Melee weapons consume considerable effort to operate and are generally significantly more dangerous to use than ranged weapons.  However, anything can be repurposed to a melee weapon and they are often superior in close quarters.
- Some foods are healthy and plentiful, when those foods are in season survival becomes easy.  However, preserving food is a time and skill intensive proposition, and farming requires land and skill.



## Balance around general game concepts

### General combat balance

Combat in CDDA should always feel deadly.  In particular, the player should never feel safe being surrounded by multiple zombies, no matter how far in the game they are.  At first this means "regular" zombies; later, as the player approaches transhumanism, it may take multiple transhuman enemies to threaten the player, but at no time should a player be able to shrug off a crowd of zombies. 

- Zombies should have a "rend" mechanic, allowing groups of them to tear at armour and damage it more effectively than a single zombie might.  This would be enhanced if they can overwhelm and knock down a foe.
- Almost all players should be limited heavily by fatigue and stamina as they attempt to fight off hordes.
- Even with 'enhanced healing', wounds in CDDA should be severe and limiting.  We are not trying to model a power fantasy.
- Cities should represent the primary 'dungeons' of CDDA in a roguelike sense (we are not at all near this at the time of this writing).  It should not be possible to clear a city: putting down hundreds of thousands of zombies is not a feasible task.  Even if it were, that high a concentration of pulped zombie-meat should do something: pulping it prevents regeneration, but doesn't make it inert.

### Sound
Noise is a hugely environmental and immersion building mechanic, both in terms of what the player hears and what noises the player makes.  Game balance should strive to ensure noise always feels important to the player.

- Making loud noise in areas with a potential zombie presence should feel dangerous, verging on suicidal, at any point of the game.  This is the primary way we can balance elite level weapons such as guns and cars.  This ties back to 'general combat balance': If sound brings in the horde, and the horde is prepared to overwhelm your defences, firing your gun becomes a decision you'll have to weigh a little more carefully.

### Light
Like noise, light and shadow are powerful tools for tension building.  Despite the availability of artificial lights, night vision, and infrared, it is important to work to maintain a healthy fear of the dark in players.

### Time
Time is the primary currency of our game.  This is a dying world, and it should feel as if the game gets harder as time progresses.

- Evolution is a key mechanic for managing time.  More powerful enemies arise as time passes.  This requires us to keep evolution *slow enough to matter*.  If powerful zombies pop up in a week, then the principle engagement of the player is going to be with those zombies.
- We will need at some point to manage some biosphere collapse.  Initially this will be slow, but as time progresses in the game, extreme weather and storms should become more common and plants rarer.  Eventually this Earth will become uninhabitable, although that will still take a while. 

### Crafting vs Looting

### 

## Balance of specific gameplay elements

### Melee
The player can use *highly* suboptimal unarmed strikes with no equipment, which deals low damage per second and comes with a high stamina cost.  
Improvised melee weapons such as sticks or primitive spears greatly improve on unarmed, to the point where unarmed can be considered a dead end.  
Improvised weapons suffer from relative fragility, and should be replaced with a purpose built melee weapon as soon as possible.  
Repurposed tools such as hammers and machetes, as well as survivor-crafted equivalents represent an intermediate tier of melee weapon that can be relied on long-term, but are still outclassed by purpose-built melee weapons.  
At the pinnnacle of melee weapons are purpose-built melee weapons such as swords, robust spears, polearms and axes.  These weapons will reliably outperform all kinds of repurposed weapons, but will have some differentiation in how effective they are based on the situation.  
Martial arts training and skill can extend the usefullness of melee combat of all kinds, but not to the point where melee combat can become the player's sole form of combat.  
The intended niche for melee combat of all kinds is to conserve other resources when dealing with relatively low-threat opponents, or as a defense when caught unprepared.  
At no point is melee intended to be sufficient to deal with the entire range of enemies present in the game, even those who appear early in the game.

### Guns
Entry into guns is gated by access to guns themselves, magazines and ammunition.
Civilian style hunting and self defense weapons are far more numerous than higher performing military weapons.
Only a handful of poorly-performing guns and magazines are craftable by the player, and conventional gunpowder and casings are also not player craftable, though the player can reload existing cases with craftable bullets and gunpowder and primers scavenged from less desirable calibres of ammunition.
Guns provide massive firepower in a highly portable package, but can cause trouble by alerting enemies in a wide area to the player's presence.
Guns present a multi-tiered set of resource limitations.
A gun can fire more or less continuously until it's current magazine is depleted of rounds.
Spare magazines allow a player to swap in more ammunition to continue firing, and additional magazines provide longer and longer continuous combat capability.
Magazines are reloadable, but not in a timeframe reasonable for combat, so the number of loaded magazines a player has on their person places a cap on their sustained damage output.
Once a player is able to cease combat, they can relatively rapidly refill magazines, ammunition stores permitting.
The total weight of one or more guns, loaded magazines, and potentially spare ammunition for magazine reloads can be significant.
Ability to replinish ammunition supplies may vary wildly depending on the area.

### Armour

### Vehicles
Vehicles are one of the most powerful tools in the game.  However, their status (at the time of this writing) as a god-tier utility device should eventually be mitigated with some logical injection of balance.

- Pre-cataclysm vehicles should use construction techniques that the player cannot replicate.  Player-made vehicles should be significantly heavier for the weight they support, but may be more durable to impact.
- Pre-cataclysm vehicles are designed to crumple on impact.  A modified sedan might work OK to run over a few zombies, but over time this will lead to decay of its structure that cannot be restored to mint condition.  Replacing these parts should be possible, but the replacements should not be the same as the originals.
- The cataclysm should leave roads into and out of major centers completely gridlocked with vehicles.  Travel through cities by car should be next to impossible.
- Travelling off road should be substantially affected by wheel load weight on terrain, so taking your ten ton hand made deathmobile across a muddy field may be ill-advised.
- Designing custom vehicles from scratch should require significant engineering knowledge.
- Fuel should become an increasingly desperate resource as fuel sources from before the cataclysm age.  Wood gassification and biodiesel are the principle available replacement options.
- Vehicle batteries have a lifespan as well and should gradually decay over time.  For lead-acid batteries, there is an option to replace them.  Lithium-ion storage batteries will likely remain useful for the forseeable duration of the game, but their storage capacity may decline over years.


### Food and Food Preparation