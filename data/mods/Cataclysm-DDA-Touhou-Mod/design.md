# Design (last updated 2022-12-21)

An attempt to explain how balance is done, or rather why is missing, from the mod and the Expansion
WARNING: contains spoilers

## Character balance 
	
Lore-wise, all 2hus are completely broken in one way or another.  Usually it's either due their ability or another of their characteristics.  Transforming these from qualitative (2hu can manipulate wavelengths) to quantitative power (Udonge's aura spells) is not simple, if it can be done at all (2hu can manipulate fate… and cdda has no LCK stat)
	
Balancing thus, is more of a nerf from the canon standpoint, but done in such a way each 2hu feels strong and can feel stronger in-game, while having some depth in how to play, so to speak.  Some anchor points to set their power are explained below


### Species

The initial component for a 2hu's power is her species.  In Touhou there is a large number of species, each 2hu being her own species is a good estimate.  Each species would be the base level of a 2hu's raw stats and some passive abilities (e.g. those that can be added via mutations and enchantments).  Some examples are:
	* Youkai: (more of a Genus, or even Family) would gain or lose no raw stats, so this is more of a tag
	* Human: average everything, the standard
	* Gyokuto: reduced STR, increased AGI + PER, MANA would cap before a human's
	* Vampire: subtype of Youkai, increased everything, including lots of weaknesses


### Character background

The second component is the specific background data, according to the canon (games + written works).  The main source for both passive and active abilities.  Some examples are:
	* Kaenbyou (subtype Kasha, a feline youkai) would have high AGI.  She is actually cosplaying as a human, and smells like corpses
	* Sakuya (human, yet mysterious) would have ??? MANA and high AGI (due her ability).  She is also perfect and elegant
	* Youmu (half-human half-ghost) would have below-average MANA + STR (as she's just a little girl).  She's a master swordswoman who works as a gardener.  She also should have below-average INT, but that's mean
	* Mokou (subtype Person of Hourai) would have high MANA and average stats elsewhere.  She literally can not die, and has been tired of being alive for the last… 800 years or so
	* Udonge (Gyokuto) would have below-average MANA, slightly increased INT, and high AGI + PER.  A former soldier of the Lunarian Defense Force, she exiled herself.  Ironically she's one of the most serious/determinate 2hus
	* Remi (subtype Vampire) has high everything.  She is *very* strong, even by vampire standards, somewhat childish
	
This is the point where narrative liberties require to be taken.  First, to not ignore qualitative power, such as Sakuya disliking humans, or Remi being a 'high tier' (read: maxed stats) 2hu.  Second, because not even Touhou Project is free from either retcons or speculation (in fact, speculation **is** encouraged by ZUN).  Third, as abilities enter the game


### Abilities

Abilities are basically *designed* to not be translated into numbers.  A JoJo reference, if you will, abilities are an independent dimension to a 2hu's total power.  See, Sakuya may be a strong human (capable of fighting against gods and other powerful enemies), but now she can also stop time.  Thankfully, this is (relatively) easy to port to cdda, in the form of movement speed modifiers and stuns.  In the case of Remi however, how can fate manipulation be translated into anything?  Not only that, but each 2hu is able to use their ability as easy as they breathe, which requires to be toned down
	
In most cases, abilities would be ported as an active, like an spell or ability, so the player can *use* and feel the weight in-game


### Limitations

The fourth is more of a constraint: what I can't, or don't know how to do
One example are Youkai, who naturally prey on humans, specifically their emotions (mostly by causing fear), but also their flesh (perhaps because that's scary).  Thus, applying the `PSYCHOPATH` and `CANNIBAL` mutations is the closest approximation, but not the correct one to make: Youkai should get no bonuses nor demerits from eating human flesh, while getting small demerits *when not* eating human flesh
A second example is Youmu having her phantom half floating around all the time (or even to materialize it as a clone, for a short time), which would be interesting to manage, like duplicate her attacks, or enable new abilities
	
Some features, like those mentioned above, are thus half-baked into the mod

	
## Combat
	
The baseline combat scenario is when the PC fights a group of enemies over a very short period of time, 1 - 5 ingame min.  After that, the player reconsiders if they want to keep going, or go recover from their wounds.  Yet it is pain, not HP, what actually stops players from reentering combat: pain slows the PC down and makes it prone to fail attacks and dodges, which quickly results in a spiral of avatar death
	
Pain is thus, the first limit players have to overcome for subsequent fights.  Two groups of game decisions dealing with that are:
1. Enabling players to stall being physically reached by their enemies as much as they can, or
2. Ignoring the mechanic of pain
	
Things like looting better weapons/armor, sniping from a distance, stealth and spellcasting fall under the first category


### Spells

Unlike Magiclysm, where players are able to build the PCs as they want, here different 2hus fulfill different roles/fantasies.  This means that, when picking Sakuya, the player automatically gains knowledge in spells related to time manipulation, but they're also locked to use spells only related to that.  Thus, 2hus not using external sources to learn new spells is a deliberate design choice
	
Another difference is mana recovery time, which is increased x4 - x12.  This enables a PC who can exit the combat with relatively minor wounds to keep going, which encourages the player to play more risky.  Additionally, I think it feels more lore accurate, and more fun


#### The XP system

The XP system for spells is less than comfortable.  Basically, the PC has to cast spells, **repeatedly**, in order to level up and increase its numbers.  However, this process is **very** repetitive, boring, takes a long time, and can't be customized.  This ends in three different outcomes for virtually all spells:
1. The spell starts weak, and becomes strong
2. The spells starts strong, and becomes slightly stronger
3. The spell is at a fixed level


#### How to balance spells around the XP system, part 1

This is evidently a problem for the most common type of spell: the one that deals damage.  If the spell starts weak, then it feels weak (not fun) unless there's heavy investment from the player (not fun).  If the spell starts strong, then it is immediately better than the previous case (unless heavy unfun restrictions apply).  If the spell is at a fixed level, then it's either case but with no reward, which is worse

Setting the initial dmg values of ST spells at 50-100% HP of the average zombie, and AoE spells around half of that, partially fixes spells not feeling powerful.  The damage cap goes from double to triple the initial damage, so they're guaranteed to feel powerful in the late-game.  The PC is also able to cast 6 spells per fight, on average.  These two assumptions mean the PC is able to clear a group of 3 - 6 enemies then rest and continue, or dispatch that number from a larger group and take the rest from melee


#### How to balance spells around the XP system, part 2

A fundamental problem emerges from the previous scenario: damage powercreep.  Why bother casting a weak spell, when it's objectively inferior?  The keyword there is "objectively", enabling three different approaches to alleviate, or even nullify damage powercreep:
	
1. Add depth as utility/features as a bonus over damage.  Remi's "Spear the Gungnir" will always create the same 2h lance, which works as a summonable melee weapon **and** she can also throw (cast) it, as a directed proyectile with maximum accuracy.  This is the most difficult from a design standpoint due how spells currently work in cdda
2. Design spells as tools, having the utility/features as the core of the spell.  For example, Tenko's Keystone spells do initial damage, but they're mainly intended to modify the terrain by spawning furniture, with different characteristics.  I believe this is the most fun for the player, but the hardest to do from a lore-accurate standpoint
3. Add small variations that give a different feeling to the same core spell.  Seiga and Kaenbyou share the same dot spell, yet in the case of Seiga it deals bio damage and heals her, whereas for Kaenbyou it deals fire damage (and should make the target shine… which currently doesn't work).  This is the easiest one to do and thus potentially the most boring one


## Story (Updated 2022-12-21)

### The story so far

(I haven't played the end-game of C:DDA, probably not even the mid-game, so I'm at a crossroads where I don't want to spoil myself, but also I don't want to misunderstand the story and ruin the vision I have for the mod and put anachronistic stuff)
The Cataclysm happened and everything went to shit, except for Gensokyo.  However, Yukari (and few independent factions such as the Lunarians) knew about it and planned things accordingly


#### The Yukari-side

In the case of Yukari, it's implied she warned each faction leader to either escape Gensokyo, or made them prepare for the Barrier deactivation.  All faction leaders (and thus the PC) understood her message as a warning to prepare for the new situation.  In reality, the message was no warning, as 2hus in general would have zero issues surviving outside the Barrier, but to make them disperse *after* the event.  The Cataclysm would enable supernatural entities showing up alongside C:DDA monsters, thus allowing for domain of superstition and fear to expand upon the world once again.  The reason she exhorted everyone just before the Cataclysm, was to deceive them: make it sound as if it was outside her control, to damage the image of the idyllic status quo in Gensokyo, to facilitate them turn feral once again.  At the same time, this façade would give false data to the Lunar Capital (*) (this would cause X event instead of Y which would benefiting Yukari, and also giving some sort of coincidental benefit to the PC)


#### The Lunarian-side
	
The Lunarians knew about the Blob, but they chosed to not act immediately due what could be better described as 'bureaucracy' and 'still being pissed at humans'.  Instead, their plan was to let the Blob (and everyone else under the Cataclysm) to run rampant, show up, cleanse everything and take Earth (including the surviving humans) as property of the Moon, in a sort of planet-wide Gensokyo of sorts.  They deployed a first wave, the current LDC faction, as reconnaissance team (regardless, their technology makes them appear otherwise).  The second, real invading force would be deployed two-three seasons after the PC making contact with the first wave, these would go from hostile to neutral depending on the PC's final status with the first wave.  All of the LDC forces would retreat some point, shortly before the third, purifying wave.  This would be equivalent to a bad ending for the PC


#### The Hecatia-side
	
Hecatia is mysteriously silent while all of this is happening.  Some Hell sub-factions opposing her showed up to wreak havoc during the Cataclysm (or whatever Hell denizens would do), yet bizarrely enough, Hecatia-aligned forces (such as the chthounds) randomly show to protect seemingly random humans from whatever tries to kill them


#### Reimu and Marisa?

I have no idea how/where Reimu and Marisa would go.  Completely removing them from the picture would imply this is happening some decades after the events from the mainline games, where both are out of comission or protected by Yukari/SDM/their closest friends.  Making them playable characters would be somewhat boring (for me, not for the player), not to mention I feel story potential will be lost if they're playable, instead of NPCs


## Random stuff that occurs to me when I'm inspired

### Lunar Weapons
	
All Lunar weapons are made from extremely high technology relative to human tech, not excluding actual magical components.  As the Moon is expected to be a peaceful land free of death, some questions immediately arise: What were the Lunarians preparing against?  Why do they need such advanced firepower?  Why are they bringing such advanced firepower here on Earth?
Their arms tech can be categorized into tiers:
* Tier 1: weapons based on human shapes and blueprints, remade with Lunarian technology.  Basically amazingly improved versions of what would otherwise be obsolete human weapons, that end up being miles better than modern human examples.  Examples: Service rifle, Service smg (?)
* Tier 1.5: Iterations of tier 1 weapons, each time less compatible with human specs (like gunmods, tools) but still fully compatible with human handling (after all, these are designed for Gyokuto).  The truly outlandish and alien part starts from this point on.  Examples: Service handgun (Udonge's rabbit-eared handgun), Lunar 'VSS', 'Prototype' modular weapon (smg, assault rifle or sniper depending on how it's armed, think of a better idw)
* Tier 2: Weaponized Lunarian tech.  These use tier 1 and 1.5 models as 'containers', so these retain a gun-like feeling, and while able to be handled by humans, much of their functionality and potential is lost.  Asking a Gyokuto to show the PC is probably the only way to learn how to properly use them.  Examples: Time warp rifle (no projectile, instead clips the space where the aim is at where the trigger is pulled).  Around half of the anti-horror weaponry such as the anti-shoggoth 'caltrop gun', the totally not Yithian lightning gun, etc. go here
* Tier 3: Weaponized Lunarian tech.  At this point, the weapon is the intended concept given shape.  Fully Lunarian in nature, humans can't use these at all, or they do the equivalent of using a railgun as club.  Example: Anti-impurity mines, 'fist gun' (similar to Halo's plasma rifle dealing 'gravity' damage), the other half of the anti-horror weaponry
* Tier 4: Weaponized Lunarian tech.  Same as the previous tier, except this time these can't be used at all due their effects, think of 'reality crashing' 'bullets' being shot.  Could not be deployed (or even used) in the field at all, just be mentioned.  Examples: Toyohime's nuclear fan, the stereotypical black hole generator, etc.


### Disorganized misc stuff

* Two battle types: 'world' combat (the existing one) and 'spellcard duels'
	* Spellcard duels would be similar to how they're in the games/lore: use of large, fancy declared spells to dodge and catch the opponent on your danmaku pattern
	* A new 'instance' for the spellcard duel would have to be created, where everything (except story items) is consumed/destroyed/moved/affected as normal, with the exception there's no 'death' by reaching 0 HP, and after the duel full health and half mana are restored to both sides
	* Spellcard duels would happen when meeting/talking/confronting another 2hu, with the rules automatically triggered by specific dialogue options (such as, the more you piss them off the harder the duel is).  This is obviously a complex task so it's more of a long term goal
	* Several win conditions are possible: health reaching 0, race mode, gain X of certain item, not going OOM, time, etc.
	* There is a story reason why the spellcard rules are now applying outside Gensokyo (spoilers)
		* "We are definitely not in Gensokyo. Why are the rules still applying?"

* Reisen Alter (heavily based on KKHTA) shows up as hostile NPC when playing any SDM member or any Gyokuto:
	* She would be extremely difficult to fight because she's there to kill, not just fight, the PC.  Normally, she would win 90-95% of cases by using underhanded tactics over the course of the fight (not by granting her immediate victory but by giving her more and more advantage until the player can't keep up).  At some point she would try to deal the killing blow, but the spellcard rules would already be active, so she exchanges some dialogue and runs away
		* This would be the only, or the second only time the spellcard rules activate outside of normal conditions
	* She would show up a second time, and do something that implicitly tells the player she overcame the rules, like killing/severely injuring someone during a spellcard duel between two different people, so the player realizes they CAN die this time
		* If the player wins, she runs away again
	
* Yorihime shows up using a patch
	* Not related to Reisen Alter, it would be an unrelated teaser/bait


### Quest ideas
	
* Find random items with messages or signs the PC can recognized (without examining the object!).  Example: Sakuya finding Reimu's discarded ofuda, which enables the quest "Find the Shrine Maiden"
* Find random items that are being tracked by more than one faction.  Example: McGuffin that the LDC and Tengu are interested in, the player has to decide which faction they help, with different consequences
	* If the PC has some connection with either faction, they may get additional benefits by siding with them
	* Additionally, the PC can choose to steal the object, ignore it, or let them fight for it
* Visiting reverse-spirited away locations.  Example: the PC was traveling between cities and sees the Hakurei shrine, goes to visit finds a NPC Sanae
	* This would allow the player to know the 'Sanae' side of the story, potentially opening several other quests
* Meeting a NPC that was just fighting "something".  Example: the PC was exploring a city and meets an NPC Sanae who is finishing a previously unseen enemy.  Sanae stops the PC, exchanges some dialogue saying "these things did not exist in Gensokyo, nor the Outside World.  Where are they coming from?"
	* This would enable the previously unseen enemy to start spawning
	* The unseen enemy are linked to the activity of different factions, for example a horror to the LDC
