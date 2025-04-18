# Design - Overview of game balance principles

**Summary**

The driving goal of balance in DDA is to support **verisimilitude in the scenario**.  In other words we want the right *things* to be present to match reality, but also when those things are used to accomplish goals, they should be appoximately as effective at doing so as their real-world counterparts.  Of note, this is not the same as pursuing all aspects of realism.  New features added should have the goal of improving verissimilitude rather than complexity for complexity's sake.

This means that *solutions to problems* should come from the same angle.  If something is not working as it should, the answer is to assess how it should work in reality, and figure out a way to better simulate that.  We find that reality tends to be quite balanced, and having things work as one would expect makes for a fun game.  

This concept is backstopped by the concept that reality has a reasonable and interesting set of balance tradeoffs, and approaching this set of tradeoffs makes for a fun game.

Some examples:  

- Bows and crossbows have naturally poor armour penetration compared to guns.  The solution to this was not to artificially inflate their armour penetration, but to add gaps in armour for archers to hit.  This led to a robust and interesting weakpoints system instead of an eternal shuffling of stats.
- Modern vehicles are quite easy to operate and effective at their core competencies of moving people and items around on reasonably well-prepared surfaces, as such they are incredibly valuable and powerful tools in the game as well.  However, their use should be limited by the difficulty of getting a vehicle to operate without a key, congestion of roads in and out of population centers, the eventual expiration of common fuels, and the difficulty of repairing vehicle chassis as they take damage from impacts and use.
- Guns deal FAR more damage than elastic projectile launchers like bows and crossbows.  However, they are loud and require looted ammunition.  Nonetheless, bows and crossbows should always be a niche/challenge weapon.
- Melee weapons consume considerable effort to operate and are generally significantly more dangerous to use than ranged weapons.  However, anything can be repurposed to a melee weapon and they are often superior in close quarters.
- Some foods are healthy and plentiful, when those foods are in season survival becomes easy.  However, preserving food is a time and skill intensive proposition, and farming requires land and skill.

## Balance around general game concepts

### Overall combat balance

Combat in CDDA should always feel deadly.  In particular, the player should never feel safe being surrounded by multiple zombies, no matter how far in the game they are.  At first this means "regular" zombies; later, as the player approaches transhumanism, it may take multiple transhuman enemies to threaten the player, but at no time should a player be able to shrug off a horde. 

- Zombies should have a "rend" mechanic, allowing groups of them to tear at armour and damage it more effectively than a single zombie might.  This would be enhanced if they can overwhelm and knock down a foe.
- Almost all players should be limited heavily by sleepiness and stamina as they attempt to fight off hordes.
- Even with 'enhanced healing', wounds in CDDA should be severe and limiting.  We are not trying to model a power fantasy.
- Cities should represent the primary 'dungeons' of CDDA in a roguelike sense (we are not at all near this at the time of this writing).  It should not be possible to clear a city: putting down hundreds of thousands of zombies is not a feasible task.  Even if it were, that high a concentration of pulped zombie-meat should do something: pulping it prevents regeneration, but doesn't make it inert.

### Sound
Noise is a hugely environmental and immersion building mechanic, both in terms of what the player hears and what noises the player makes.  Game balance should strive to ensure noise always feels important to the player.

- Making loud noise in areas with a potential zombie presence should feel dangerous, verging on suicidal, at any point of the game.  This is the primary way we can balance elite level weapons such as guns and cars.  This ties back to 'general combat balance': If sound brings in the horde, and the horde is prepared to overwhelm your defences, firing your gun becomes a decision you'll have to weigh a little more carefully.

### Light
Like noise, light and shadow are powerful tools for tension building.  Despite the availability of artificial lights, night vision, and infrared, it is important to work to maintain a healthy fear of the dark in players.

### Time
Time is the primary currency of our game.  This is a dying world, and it should feel as if the game gets harder as time progresses.

- pre-cataclysm loot should decay over time.  First food, then medicine and fuel, then wooden stuff will all age and crumble as time passes, requiring players to transition to making or repairing their own.
- Evolution is a key mechanic for managing time.  More powerful enemies arise as time passes.  This requires us to keep evolution *slow enough to matter*.  If powerful zombies pop up in a week, then the principle engagement of the player is going to be with those zombies.
- We will need at some point to manage some biosphere collapse.  Initially this will be slow, but as time progresses in the game, extreme weather and storms should become more common and plants rarer.  Eventually this Earth will become uninhabitable, although that will still take a while. 

### Crafting vs Looting
At its core, this is a post-apocalyptic simulation, and a key aspect of early post-apocalyptic stuff should be looting and repurposing existing gear.  One does not associate the apocalypse genre with smithing a fully functional suit of plate armour at a smithy a week after the world ends; one associates it with rolling a shopping cart through eerily empty city streets gathering dented tins of beans, cautiously glancing about through the visor of your dirty riot armour.

- Crafting new gear is generally an emergency measure at first.  Everything you want should be available to loot, if you know where to look.  If players look in the right place, they should find what they expect to find there: looking in ten kitchens to get a single frying pan doesn't feel fun nor real.
- Over time, NPC survivors and the free merchants should strip easily accessible resources from lootable areas. Looting will be replaced with trading or crafting as a viable option, unless one wants to go into the cities.  This requires adding some intelligent heuristics to simulate this behaviour.  Although it's acceptable to add some "looted" map variants at the start, these should represent what was looted in the pre-cataclysm riots, not what has been looted since the world ended.
- Cities should be the best source of well preserved loot, but should be teeming with uncountable zombies.  You have to brave the risk if you want to find more oxyacetalene.
- Much of the midgame crafting should be around repurposing and improving pre-cataclysm gear to suit the new post-cataclysm situation, rather than making new stuff from scratch.
- Into the later game, as pre-cataclysm stuff decays, we should see more newly-minted gear rise up, at a quality level in between medieval and early industrial era due to limited crafting abilities.
- Crafting gear should always be a very time consuming option, especially if lacking powered tools.

### NPC interactions and non-linear plotting
As an apocalypse-genre game, we expect interaction with survivors to become a crucial part of the game.  This could represent having traveling companions and a home faction, but it could also be through befriending and/or paying a static game faction.

- It is perfectly reasonable to balance certain gameplay elements, especially advanced ones, with the requirement of NPC interaction.  For example, a lone wolf survivor should not be able to do all types of vehicle repair and construction: some jobs just *need another pair of hands*.
- Not every gameplay element needs to be, nor should be, available in every playthrough.  Allying with the Exodii or with Hub01 should produce different results, locking off some elements of the game forever and opening up others.
- Similarly, not every gameplay element needs to be available through each faction.  While the Free Merchants or Hell's Raiders might be encoded to offer a version of the services a player faction could do, they don't have to have everything.
- NPCs have the potential to represent an enemy that scales in a way zombies just can't.  If you make enemies with the Hell's Raiders, they should be able to circumvent your static base defenses in ways a real person would, likely through a combination of scripting and AI.  No amount of concrete walls and stationary turret emplacements will make your base impregnable to concentrated mortar fire.
- As the game development progresses we should continue to remove features that were initially added as substitutes for NPC interaction, as we have been doing with various AI elements.  Examples include autodocs and vending machines.

### Monster design considerations
It is tempting to write an ever-increasing flow of monster evolution and difficulty, with perpetually rising armour and damage numbers.  While some of this is desirable, it is important not to overuse this as a gameplay principle: it just creates a pretty boring arms race.  Monsters should have tradeoffs just like the player.  This doesn't mean that every durable monster must be slow (quite the opposite) but it does mean that we do not need a heavily armoured hulk variant of every zombie type.  Rather than adding "more", particularly just bigger numbers, consider adding things that encourage new play mechanics.  Some examples of mechanics already in play that shift gameplay without just bumping numbers include:

- Hounds of Tindalos multiplying and teleporting all over the place
- Shady zombies vanishing into shadows and giving jumpscares
- Pupating zombies leaving trails that can affect terrain
- Necromancers reviving other zombies
- Ferals opening doors and using tools

## Balance of specific gameplay elements

### Melee
Melee combat should rarely be the best way to handle combat: rather, it's easily obtained, and often really fun.  At no point is melee intended to be sufficient to deal with the entire range of enemies present in the game, even those who appear early in the game.  The intended niche for melee combat of all kinds is to conserve other resources when dealing with relatively low-threat opponents, as a defense when caught unprepared, or in forced close quarters, particularly when silent combat is necessary.  

1. The player can use *highly* suboptimal unarmed strikes with no equipment, which deals low damage per second and comes with a high stamina cost.  Even with superb unarmed skills, this is *never* going to be a competitive way to fight zombies.  Sticking exclusively to unarmed is a challenge mode, and may be encouraged with achievements, but we have no desire to try to "balance" unarmed to armed combat against each other.
2. Improvised melee weapons such as sticks or primitive spears greatly improve on unarmed and can be scooped off the ground immediately.  These weapons suffer from relative fragility and poor balance, and should be replaced with a purpose built melee weapon as soon as possible.
3. Repurposed tools such as hammers and machetes, as well as survivor-crafted equivalents represent an intermediate tier of melee weapon that can be relied on long-term, but are still outclassed by purpose-built melee weapons.  
4. At the pinnnacle of melee weapons are purpose-built melee weapons such as swords, robust spears, polearms and axes.  These weapons will reliably outperform all kinds of repurposed weapons, but will have some differentiation in how effective they are based on the situation.  

- Martial arts training and skill can extend the usefullness of melee combat of all kinds, but not to the point where melee combat can become the player's most optimal form of combat.
- Melee combat will always place the player in harm's way, which greatly affects its utility.  A solid strike with a sufficiently weighted lever arm may hit nearly as hard as a gun or bow, but you cannot do it from safety.
- Reach weapons occupy a noteworthy spot in balance considerations.  Having reach is nearly always preferable to not having reach, except in specific close quarters situations.  There is a reason that the spear is the most ubiquitous weapon throughout human history.  Just as we have no desire to attempt to balance unarmed to be equal to armed combat, we have no intention of ensuring that using non-reach weapons is as good as using reach weapons.  They aren't.  Killing zombies that can't reach you is good.  That said, any actual considerations that would limit the use of reach weapons, such as cramped quarters or low ceilings, are perfectly reasonable measures to add to the game.
 
### Guns
There are several major reasons guns dominate modern warfare.  They are very effective weapons.  In pursuit of verissimilitude, we do not aim to change this in the game balance; if you have a shotgun, it should feel like it hugely outclasses a longsword for close quarters combat, because it does.

- Entry into guns is gated by access to guns themselves, and by availability of magazines and ammunition.
- Civilian style hunting and self defense weapons are far more numerous than higher performing military weapons.
- Only a handful of poorly-performing guns and magazines are craftable by the player, and conventional gunpowder and casings are also not player craftable, though the player can reload existing cases with craftable bullets and gunpowder and primers scavenged from less desirable calibres of ammunition.
- Guns provide massive firepower in a highly portable package, but can cause trouble by alerting enemies in a wide area to the player's presence.  One of the most vital ways of balancing guns has nothing to do with weapon stats, but instead with properly modeling the effects of **noise**, as described above.
- Guns present a multi-tiered set of resource limitations that can be adjusted to achieve a desired play balance.  This balance should be informed as much as possible by real life availability of these things.
- A gun can fire more or less continuously until its current magazine is depleted of rounds.
- Spare magazines allow a player to swap in more ammunition to continue firing, and additional magazines provide longer and longer continuous combat capability.
- Magazines are reloadable, but not in a timeframe reasonable for combat, so the number of loaded magazines a player has on their person places a cap on their sustained damage output.
- Once a player is able to cease combat, they can relatively rapidly refill magazines, ammunition stores permitting.
- The total weight of one or more guns, loaded magazines, and potentially spare ammunition for magazine reloads can be significant.
- Ability to replinish ammunition supplies may vary wildly depending on the area.
- Gunfire not only alerts the horde to your presence, but also reminds potentially hostile NPCs that there are other survivors out there.  Those NPCs may also have guns.

#### Enemies using guns
Fighting someone who might shoot you is extremely difficult to manage from an entertaining gameplay perspective.  Much of the time, if facing an at-all prepared attacker, gameplay would consist of hearing a bang, and receiving a "you have died" message. There are several important ways to mitigate this.  The general theme of these barriers is that there should be some thematic form of warning that a gunfight may ensue, to allow players to take cover or other appropriate methods of avoiding attack.  This is a gameplay consideration that, in many cases, may need to supercede realism concerns.  Being shot in the head by a well concealed bandit sniper with a scope, or having your car blown up by a TOW misile the moment you swing around a corner, is quite a realistic model of post-apocalyptic combat, but it's not going to make for a fun game.

Note that these principles don't just apply to guns, but other "gotcha" attacks that could, if not managed appropriately, lead to instant unwarned death.  Mines and grenades are other good examples.  These notes don't mean these attacks should *never* happen, but we should take steps to make players feel like it was their own foolishness that got them killed, not the unavoidable whims of the RNG.

- In general, world-spawning unintelligent monsters should not be equipped with guns.  Even reduced accuracy is not a solution to this: that just means you have a lower chance of being suddenly killed without warning.  "Stupid" enemies with guns should be kept restricted to special locations with hints alerting the players to their presence or layouts that ensure the player will see them first.
- Turrets and robots should only be instantly hostile in rare circumstances: similar to above, it should be quite obvious to the player when this is a possibility.  Automated turrets overall should be rare, mostly limited to XEDRA and Hub connected facilities; unless the player is in a highly restricted area, a turret is more likely to demand the player get down on the ground and not move than it is to open fire.  Until such time as this is an option we may need less realistic ways to prevent a turret from randomly shooting people from off-screen.
- Any world-spawning enemy with a gun should be intelligent, an NPC or above.  These enemies should attempt to get the player into a compromising position before opening fire as much as possible.  Most of the time these will be raiders and bandits, who will still prefer to steal what you have and let you live.  For the times when they don't, their presence should be clearly flagged with an ambush rather than heralded with a sniper shot from outside the reality bubble.  On rare occasions this would be unrealistic, but most of the time, bandits and raiders should still prefer to let another living human survive than to burn the world to the ground.

### Armour
Worn armour is one of the best defenses against zombies, and over the years, humans have developed some great armour.  Modern armour includes, notably, police riot armour and military body armour, as well as the fictional exoskeleton-based power armours in our game world.  These armours should be very good against zombies in general, which raises the complex problem of how to establish a reasonable armour progression and upgrade sequence that is not eclipsed by lootable modern armours.

- In general, all armour should be a tradeoff between protection, coverage, heat disspation, moisture dissipation, and encumbrance.  As armours become more covering, they will hold in more heat and moisture (becoming less comfortable the longer they are worn).  More protecting armours, by and large, should be more encumbering, especially at higher coverage.
- Optimal armour strategies should usually emphasize layering less-protective, more-covering lower layers with increasingly protective but less covering upper layers.
- As much as possible, highly protective armours should not have total coverage of the entire body.  All armours have gaps.
- Pre-cataclysm high-protection armours are generally not designed to be used with a backpack or to be worn for days at a time, with the exception of military armour.  Military armour is generally not designed to fend off a mob of biting, clawing zombies at point blank range.  
- All modern armours are designed to be discarded when damaged, not repaired with duct tape and glue: attrition will be a major balancing factor.
- Compared to medieval armours, modern armours have less perfect coverage and tend to have softer, more penetrable fabric in the less vital organs.  Many enemies will have weapons that can get through here.
- **special element damage** (electricity, acid, fire, gas/spores, etc) is intended to be a means of reducing the overall effectiveness of armour and preserving threat to higher levels.  It should be possible to circumvent or mitigate these, of course, but there should always be a tradeoff between special damage resistances and overall armour effectiveness.  Further, there should be a choice between close-to-total protection from a single special damage, or broad but less complete protection from multiple types.  As part of this, we should be judicious about overusing these attack types (and at the time of this writing, we definitely are not).

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
