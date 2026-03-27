# Sorcerer

A different way to play Magiclysm.<br>
Instead of reading books, gain spells and spell levels when you level up, in much the same way as Bombasticperks.<br>
Each level, you can learn a limited number of spells.<br>
Your caster level is determined by your sorcerer level and is boosted by your intelligence; (int/2)-3, so +0 at 6, +1 at 8, +2 at 10, and so on.<br>
You are not limited to what spells you can pick by classes.<br>
You may pick a bloodline that alters how the sorcerer works. For example, the spelldancer gets increased caster level from their effective dodge, rather than from intelligence. The default bloodline uses intelligence and lets you boost one of your known spells to level 3.<br>
Each time you level up, you can swap one spell you know with a spell of the same or lower level, so you are not locked in to your choices forever.<br>
Inspired by Pathfinder and Dungeons and Dragons.

## Contributing

Please note that the code inside the folder generated_code is auto generated and should not be edited manually. Instead, use the python script in tools. You might have to adjust the paths used before you run them.<br>
generated_code/spell_data_dump.txt contains data for all spells found and generated. You can copy that data into tools/spell_data.txt to edit properties of spells, disable spells, register sorcerer-specific spells, or manually alter the spell level of the spell.<br>
I have not yet figured out a way to auto format the generated code, so you will have to deal with that yourself.<br>
Note that while tools/spell_data.txt contains json data, it is a .txt file. This is to make sure CDDA doesn't confuse it for something it should be reading. The json is read by tools/generate_spell_levels.py.<br>
Bloodline-specific stuff should if practical go into the bloodlines folders, either in an .json named after the bloodline, or in a folder with the same name. The exception is code for when you first pick the bloodline, which is currently located in pick_bloodline.json.

## Balancing

It seems that sorcerers will end up being more powerful than traditional Magiclysm wizards. I'm not yet sure if that is good or not. The mod might need a fair bit of nerfing.<br>
One path forwards would be to give traditional Magiclysm wizards more toys depending on their school, like the school weapons, the caster level boosting items, or the druid's wildshape.<br>
Bloodlines should feel roughly equally powerful to each other, preferably.<br>
My thinking is that if you are playing as a traditional Magiclysm wizard, you would level your main combat spell to around level 5 before you try to use it in combat, so a sorcerer should start with their main combat spell's effective spell level being around there as well. A sorcerer will likely gain levels in less in-game time than a wizard would, but would have to do more dangerous activities to do so.<br>
The leveling up speed is supposed to be equal to the default settings for BombasticPerks.

## Other things the mod contains

Counterfeit Rune: This spell creates a small pebble infused with a sorcerer's power. It can trick spells that require runes to cast to think that you have the rune required. Does not work for crafting, but lets you cast animist spells.

# Future plans and loose ideas

Some more rebalancing and nerfing is probably needed. I'm open to suggestions. Perhaps your sorcerer level should only give half its level to your caster level? Perhaps higher level spells should have a reduced caster level bonus so that you start using them with a low level?

Figure out a more elegant way to lock the player out of reading all magic books, including classless.

Add some milestone experience bonuses to Innawood so that sorcerers can level up there too, since there is not going to be much combat in that mod.

Eventually remove the default bloodline and force the player to choose another one, once more bloodlines are implemented?

## Integrate Mind over Matter

I've considered adding powers from MoM to the pool of spells you can pick if MoM is installed. But I don't know enough about the mod to be able to rate the powers on a scale from 0 to 9 (inclusive). I'm also not sure it would be easy to loop over and create code to learn and un-learn all the posers, as some of them seem to be 'learned' in special ways.

## New spells

### Metamagic spells

Works similar to the metamagic perks from bombastic perks, but are spells, might cost mana or another resource, and have a duration. Only available if BombasticPerks is **not** installed.

Meta-Metamagic: Lingering Metamagic - Makes metamagic linger for their full duration instead of being consumed on the next spell cast. It’s So Meta Even This Acronym

Mnemonic Trick: Lets you learn one spell from a scroll or book you own. Casting this spell again replaces the last spell learned. Perhaps a level 3 spell? Switching the spell might have a cooldown of a day or a week?

## Spelldancer changes

Spelldancer should have their own martial art (Spelldancing) that is similar to Aikido. The martial art focuses on dodge. When you dodge, the next spell you cast has a shorter cast time (and maybe cost?). When you cast a spell you get a temporary bonus to unarmed damage you deal (Might scale with amount of mana spent). Maybe some bonus to armed combat if you are fighting with a spell focus (an item that lets you cast spells while wielded) as well.

# Some random ideas for other bloodlines

## Tarot bloodline

You have a tarot deck that is the source of your power. Each time you wake up you draw a hand, assuming the last time you did so is more than 8 hours ago.<br>
Randomly draw spells equal to the spells known by a normal sorcerer, divided up in levels the same way. You can cast the spells you drew as many times as you want.<br>
Optionally, have specific tarot cards that lets you cast a spell once before it is consumed, like a Tarot Card: Black Lotus, which gives you a bunch of mana instantly, or Tarot Card: The Blacksmith, which summons a forge for half a day or so.<br>
Mulligan your cards. Discard x cards and draw x-1 new cards. Fill up your highest leveled spell slot first, and the specific card slots last. Or perhaps let the new cards be completely random?<br>
As you level up, you gain more harrowing-specific card slots, and a couple free mulligans.

## Shifter

Gains the druid shapeshifting ability at level 1, gains more forms as they level up. Maybe lock them out of domination classes as a drawback? (Biomancer, Kelvinist, Magus, Technomancer)

## Sylvan bloodline

You can summon an animal once a day for maybe 8 hours. The animal will follow you and grow in power as you grow in caster level. Might also gain tree-flavored spells.

## Cult leader

Gain power based on number of followers instead of intelligence. Might be inconvenient given the state of the AI. Perhaps unlock a basecamp structure where the followers can grant you power remotely? Take inspiration from Cult of the Lamb? (haven’t played it)

## Spellslinger

Spellcasting level bonus is somewhat based on perception instead of intelligence. Perhaps after you have fired a ranged weapon, you gain a bonus to your caster level based on distance shot multiplied by your to-hit chance? Gain the ability to cast spells from further away, and to imbue your bullets with magical properties. Gain bonus spells focused on staying far away and using guns. Despite the name of the class, you can also use bows and other ranged weapons.<br>
Perhaps instead they can craft a spellcasting focus item that lets you cast spells from further way when wielded, and your spells become more powerful the further away you are?

## Attuned bloodlines

Based on the Magiclysm attunements. Gives powers that eventually scales up to those given by the attunements, and also eventually gives the spells from that attunement, as well as other thematically fitting spells.

## Mutation Master

Gain caster levels depending on the number of purifiable mutations. Can mutate easier

## Blessed Machine

Gain max mana from stored energy instead of intelligence. Gain caster levels from the number of installed bionics.

## Alchemist

Gain significantly fewer spells (Maybe only crystalize mana), but can easily make magical potions to compensate. An attempt to make a ´prepared spellcaster´: You have to prepare your spells ahead of time.
