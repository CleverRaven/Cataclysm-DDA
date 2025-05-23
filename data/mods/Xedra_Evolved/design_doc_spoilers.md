# Design Docs

These documents are for the lore basis and mechnical assumptions behind some of XEs systems. They contain heavy spoilers, obviously--if you're a player, don't read if you want to be surprised!

## The Principle of Supernatural Exclusivity

Xedra Evolved has more types of playable supernatural with their own magic than any other CDDA mod, and it's a general principle that these are all exclusive. You cannot have a werewolf chronomancer, or a changeling mad genius, or a a gracken lilit. Most magickal powers require starting as having that power to develop it, and those who start with non-human power sources cannot learn human dream magick.

The exception to this is vampirism. Since vampirism is an infection, anyone can theoretically develop it (barring lilin or dhampirs who are immune), but becoming a vampire locks out previous supernatural powers. The lower tiers merely cannot learn new powers, but making the choice to become a full vampire removes all previous supernatural powers as the blood overwhelms everything.

## Timeline Differences from vanilla CDDA

TBD

## Dream Magick

TBD

## Cryptids

Cryptids are an assortment of unique beings and creatures that share a single thing: how hard they are to encounter.  Their abilities and motives are as unique as they are, and their origin is unclear.

There are three ways to encounter a cryptid:
- Find and examinate their tracks to locate that cryptid's lair.
- Start as a cryptid hunter to get a map to a random cryptid.
- Stumble upon their lair by pure chance.

Guidelines to add a new cryptid:
- All cryptids must be unique.  The player cannot encounter the same cryptid twice in the same world.
- Adding a cryptid includes the addition of a lair terrain where they can be found, and a track terrain where the player can get directions to the lair by examinating a special tile.
- The cryptid's lair is globally unique, while the track overmap cannot be found more than once per overmap.
- Each cryptid have their own track and lair.  They do not share them.
- There is no need for them to have loot.  You can make your new cryptid drop something, but it is not required.
- Add a map item leading to where your cryptid can be found and a small note with information about them.  The information can be as vague as you want, as cryptids are defined by being hard to find, but it should at least give a clue about what to expect from that cryptid.
- Remember to edit the random_cryptid_map item group to include the note and map you added, or else cryptid hunters won't be able to start with directions to your cryptid.

## Hedge Magick and Alchemy

Hedge Magick is designed to represent minor tricks and simple charms, things like starting a fire, calling a rainstorm to water crops, or putting luck more on your side.

Principles of Hedge Magick:
- Anyone can learn hedge magick. 
- Hedge magick has no levels and does not fail. If you have the ingredients and appropriate conditions it succeeds, and if you don't it can't be attempted at all.
- Hedge magick requires either components, particular conditions like a time of day or location, extensive periods of time to enact, and occasionally all three.
- Hedge magick requires extensive study to learn. Most hedge magick the survivor is likely to find developed from first principles by recluses and weirdos, and the survivor has to interpret those notes into a decipherable magickal formula.
- Existing hedge magick was often developed by looking at real-world occultism, but this is less important than keeping hedge magick's power level low--real-world occultists make some absolutely wild claims of their power.

Alchemy: TBD

## Vampires

Xedra Evolved vampires are not walking corpses who drink the blood of the living. At least, not at first. Vampirism is a progressive disease and many sufferers are more alive than they are dead. While they must drink blood and can derive power from it, they still need to eat, drink, and sleep just like mortals do. Becoming an actual undead monster requires a conscious decision on the part of the nascent vampire and can only be accomplished by committing heinous crimes (at the time of this writing, murdering a dear and trusting friend). Actual vampires are monsters without exception, predators who have extinguished all feeling from their hearts. 

Vampires have existed for a long time on the Earth of Xedra Evolved, and on some--though not all--extradimensional Earths as well. It is possible that humanity's first steps into agriculture and gathering into stationary cities were encouraged by vampires, so that they would have an easier time maintaining their herds. They use their `renfields`, brainwashed mortal agents, to manipulate human society and keep themselves safe from those who would destroy them. 

Vampires require human blood, or the blood of human-like species. They cannot survive on animal blood, so they are forced to prey on fellow sapients to survive. They also cannot drink vampire blood (though see below), because vampires have enough control over their own blood to prevent another from drinking it. A vampire who does not use any of their powers (called Blood Arts) is capable of surviving for quite a long time without needing to feed. 

Vampires gain new powers with time, intended to mimic the progression of a disease.

### Blood Arts

Blood Arts is the term for vampires' special abilities. With them, vampires can become stronger, faster, more resilient, have better senses, hypnotize or charm their prey, and eventually turn into wolf, bat, or mist.  More powerful vampires can also control darkness and shadows. The balance point for Blood Arts is intentionally set so that a vampire who uses several of them will either have to feed soon after to avoid the `withering` (the status penalties that come from being low on blood), or will have to gorge beforehand and become `blood_quenched` in order to have enough blood to use all the Blood Arts they want during combat.

Principles of Blood Arts:
- Blood Arts cost blood. Any secondary costs (mana, stamina, sleepiness, etc) are unnecessary. Use of their powers should require vampires to drink more blood.
- Blood Arts should be "vampire-y" in theme and scope, with some latitude for interpretation. Creating tactile illusionary beasts that can harm the vampire's enemies is out, but creating animate shadows that can harm the vampire's enemies but die in light, or clouding their enemy's mind so they perceive a group of wolves approaching them, would both be acceptable.
- Blood Arts do not have levels, and are not subject to failure based on power or skill level.
- Blood Arts that turn on and remain on until turned off should be implemented as activatable mutations, so that there's a easy screen where the player can see what all is active and turn them on and off. Blood Arts which create an instant or temporary effect should be implemented as spells in the Supernatural Powers menu.
- It is currently possible for a player to eventually learn every single available Blood Art, so be careful of unexpected synergies. 
- New vampire powers need to be added to the vampire virus EoC (\Xedra_Evolved\effects\vampvirus.json) in order for vampires to learn them. If it's a power that a dhampir could also learn, they should also be added to the dhampir learning EoC (\Xedra_Evolved\mutations\dhampir_eocs.json)

### Vampire Anathema

There exists a way to push vampirism beyond its limits.  This isn't something the mentors can teach, as almost none of them know about this method.  The few who know that this empowerment is possible keep it a complete secret, for its method is directly opposed to their plan of creating as many strong and willing vampires as possible.

Long ago, a vampire hunter decided to fight fire with fire and sought a way to consume other vampires to gain the power to eradicate them all.  They succeeded, but won't ever share how they did to gain such power.  They relentlessly roam the world, searching for vampire-blooded to kill and drain.  Once the player reaches the final tier of vampirism, they too will eventually get a visit from the Anathema.

The Vampiric Anathema is intended to be almost impossible to kill without a plan, for they are far stronger than regular vampires.  Proper planning, such as exploiting their vampire weaknesses, is the key to kill them.  They must always be nigh-invincible head-on, assuming a well-geared player with high skills and every single vampire power, but be challenging yet beatable in a well-prepared fight.

Should the Anathema be slain, something incredibly easier said than done, their notes will allow the player to use the same method, pushing their own vampirism to levels previously only seen in the Anathema.  The player will then gain the ability to consume vampire blood to gain the Anathema-specific traits, in a way similar to a mutation path.  On average, three vampires contain enough blood to grant one such trait.

If a trait should be given, but the player already have them all, it will increment an effect counting as a tier 1 power for power calculation purposes.  This is so the player never stops wanting vampire blood, the same way vampires never stop wanting regular blood.

Powers guidelines:
-Amplifying vampiric abilities (such as no-maintenance Gleaming Eyes, stronger Sanguine Ecstasy and lower-cost Master the Mortal Mind)
-Becoming more of a vampire than vampires are (such as gaining nourishment from vampire blood and detecting nearby vampires)
-Powers of the creatures of the night (such as being able to turn invisible in the dark or quickly healing while in the dark)

If a power amplifies a base vampire trait, said trait is a prerequisite.  Gaining the boost before the boosted results in gaining the base vampire trait instead.

Drawbacks guidelines:
-The high blood drain is an integral part of being an Anathema.  Amplifying vampirism also amplifies the thirst for blood and drives them to kill and drain as many as they can as their self-devouring vampire blood causes heightened depletion.  
-Player Anathema that have yet to reach the Anathema threshold have a total of six vampiric weaknesses.  A post-threshold player Anathema has a total of nine.  They can use the mentors' recipe to reshuffle them, but they cannot reduce how many weaknesses they have.
-Gaining this threshold also grants one and only one of the Anathema-exclusive weaknesses.  If adding new ones, it is important that their gameplay impact is similar.  This weakness is permanent and cannot be reshuffled, so there mustn't be a weakness that is overall better or worse than the others.

This path is incompatible with regular mutation thresholds.  The player cannot gain access to the anathema mutations if they already have a threshold, and the trait granting this access prevents gaining any other threshold.  Mutating is still possible, but the player will have to pick between regular thresholds and boosting their vampirism through this method.

### Dhampirs

Dhampirs are the children of a lower-tier vampire, one who has not yet made the awful choice to become an undead monster, and a mortal (or, rarely, another lower-tier vampire). They inherit a portion of vampire power but also some vampiric weaknesses. Dhampirs are innately capable of sensing both vampires and renfields, and will sometimes become vampire hunters because they cannot help but be aware of the threat.

Dhampirs naturally regenerate the blood vitamin used to power their Blood Arts, but intentionally do so very slowly. The dhampir gameplay loop is designed to always tempt to them drink blood for power, to facilitate the "am I a man, or a monster?" question that often plagues fictional dhampirs, but also punish them in turn if they do. Dhampirs gain two weaknesses from a set of thematically-appropriate ones, but if they've ever drunk blood--even once--this list is expanded and they become eligible for more total weaknesses.

Dhampirs can use tier 1, 2, and 3 Blood Arts, though they should not have any shapeshifting or Blood Arts that rely on the vampire being undead (such as falling off a ten-story building and taking no damage). 

Some dhampir-exclusive abilities, and some vampire Blood Arts when used by dhampirs, only function when they have `empowered blood`, which is when the blood vitamin is at 1 or higher. When `enervated` (at blood vitamin 0 or lower), they suffer minor but slow scaling penalties to hunger, thirst, sleepiness, and stamina. Dhampir do not suffer from the `withering`, and while they can become `blood_quenched`, they can only do so by drinking blood. 

Dhampirs gain new powers by spending blood. Every point of blood vitamin they spend goes into a pool that is checked once an hour. This method is designed to further tempt them to drink blood; by drinking blood, they'll have more blood to spend, and thus will advance faster.

## Shapeshifters

At the moment, no shapeshifters appear as monsters or NPCs.  Werewolves are playable but barebones. Most aspects of werewolves, or shapeshifters in general, are TBD.

### Werewolves

Werewolves, who call themselves "The People of the Moon", function as defenders of Earth and nature against extra-dimensional or spiritual threats, like beings from the Nether, possessing spirits, and so on. Their supernatural powers revolve around this concept. 

Werewolves have three forms: their human form, their wolf form, and their hybrid war form, a three-meter-tall mass of muscle with enormous claws and teeth. The war form has a very high metabolism, so staying in it for long periods of time is discouraged.

Werewolves are vulnerable to silver. This is currently unimplemented.

Principles of Werewolf powers:
- Werewolf powers are fueled by mana, but their mana is proportional to their physical prowess, not their intelligence.
- Werewolf powers have a chance to fail based entirely on the pain the werewolf is suffering from. Since werewolves regenerate, this means that while they can be in a very difficult situation if they're at high pain, they merely need to wait a few minutes to return to 0 chance to fail. 
- Werewolf powers revolve around changing form and hunting prey. They generally cannot bring their metaphysical might to bear in situations that require stealth or diplomacy, though some exceptions exist. 

Werewolves gain new powers by killing enemies in barehanded combat. The wolf must hunt.

## The Fair Folk

The Fair Folk are a multidimensional power on the level of the triffids, in that they are capable of exerting power across many dimensions simultaneously and they have nothing to fear from the blob. Unlike the triffids, they are not expansionist. They have a home dimension variously called `Under the Hill`, `the Bright Lands`, `the Elflands`, and similar names. If they have their own name for their dimension, they don't use it with mortals.

The Fair Folk are divided into the elemental fae, or Paraclesians, and more humanlike fae, or changelings. Note that despite being called changelings in XE, most changelings were born in the Elflands and not swapped for human babies.

Almost all Fair Folk are vulnerable to iron to varying degrees, with Arvore, Sylphs, and Undines being the most vulnerable. Homullus are, uniquely, not vulnerable to it at all.

### Paraclesians

Paraclesians are based on the elements: Sylphs for air, Ierde for earth, Undines for water, Salamanders for fire, and Arvore for wood. A sixth type, Homullus is effectively an "elemental" of humanity. Other elemental types might exist related to other species such as gracken, but if so, they generally do not visit Earth.

Paraclesians are the eldest and original Fair Folk, born from the "dreams" of natural features like mountains and rivers. They spring to life fully formed, but with little power, and their parents often place them outside the Bright Lands to grow up. Those Paraclesians who survive and manage to make it back home are considered adults. 

Paralecian magick is heavily themed and also grows much stronger when in an appropriate place of power (in the wilderness away from cities for Arvore, underground for Ierde, and so on) and much weaker when in an opposite place (underground for Sylph, in the cold for Salamanders, etc). At the moment it has levels and requires practice, but this is undesirable--the Fair Folk are magical beings and should not need to practice their spells like human wizards. All Paraclesians can transform into the embodiment of their element, and all of them transform into more pure elemental forms as they grow in power.

Each Paraclesian has their own design sensibilities:
- Arvore: Arvore powers relate to plants, nature, and growing things. Their magic allows them to become more plantlike, encourage plant growth, draw sustenance from soil and sunlight, and so on. While Arvore do have damaging spells, they mostly only affect plants and zombies, and then not generally in a magical bolt instant-damage fashion, since damage-over-time is much more similar to how nature works. Arvore deliberately have no powers that affect animals. The mortal need Arvore can overcome is requiring food and drink.
- Homullus: Homullus powers relate to humanity, tools, cities and knowledge.  They have abilities related to managing humans and formerly human creatures such as ferals, renfields, etc.  Currently not implemented but they should be incredibly effective at dealing with cyborgs and some abilities related to interacting with robots (as tools created by humans).  They have the ability to install CBMs or even choose to limit their Paraclesian abilities by pursuing a mutation path created by humans.  The mortal need that Homullus can overcome is anything that limits their ability to make choices such as mind control. Homullus are also, uniquely, not bothered by iron.
- Ierde: Ierde powers relate to resilience, preservation, and the manipulation of earth and stone. They are designed less to destroy their enemies with magick and more to slow them, redirect them, and simply outlast them (though since only the player has stamina this works less well than it could). The mortal need Ierde can overcome is sensitivity to pain and tiredness (stamina, not sleepiness). 
- Salamander: Salamanders are the most innately destructive of the Paraclesians, though their magick also invokes the way forests regrow after a forest fire. Their magick allows them to move quickly and set everything on fire (though they are deliberately designed without an "I cast fireball" equivalent), with some associatons with the heat of the forge and with light. The mortal need Salamander can overcome is need for water or sleep, though part of their design is that they need much more extra food (as fuel, so to speak)
- Sylph: Sylph powers relate to wind, lightning, and the weather (weather currently unimplemented). Their powers allow them to breathe without air, affect the morale of enemies and friends, conjure up blasts of lightning, and control the air around them. The mortal need Sylphs can overcome is being tied down, in both the literal gravity sense and the metaphorical snares or entanglements sense.
- Undine: Undine powers relate to water and acid, as well as the transformation of liquids.  Undines are slightly different because by default they do not need to drink water and heal when in water, but many of their powers have a thirst cost so they can become thirsty by using their magic. They can also regain thirst automatically when underwater, so the push-pull of Undine powers is making sure to never be so far from water that they can't top up when necessary.

Paraclesians gain their powers by spending time in their native environments, though this is very slow (up to a month to gain one power), or by hunting down other elemental fae and making `destiny draughts` that function like mutagen.

### Changelings

Changelings are originally the offspring of pairings between mortals and elemental fae in the unremembered past, which occurred often enough that their descendants formed a new stable population. While they do sometimes still swap mortal babies for fae babies, the name "changeling" as a collective is used for convenience and most changelings are born and grow up Under the Hill.

The Fair Folk make a habit of making offers to mortals to come back to the Elflands with them, especially on worlds that have undergone or are about to undergo a Cataclysm. The reason for this is that changeling magic relies on mortal dreams for power, which means they need mortals on hand to dream for them. The degree to which they are willing to coerce mortals into this arragement varies based on the individual changeling but the temptation is always there. They will not bring anyone to the Elflands who has not sworn some sort of agreement with them, and for this reason the Exodii are wary to antagonistic towards them. 

Changelings are divided into two great courts, the Summer (or Seelie) Court and the Winter (or Unseelie) Court. The Summer Court is more benevolent to mortals (Lady Boann is Summer Court), but that does not mean they are nice. Changelings are also divided into two broad types, nobles and commoners. Nobles are more magically powerful, but commoners have individual tricks that the nobles cannot copy with their magic, such as the selkies' facility with weather control or brownies' ability to speed crafting when unobserved.

The following types of changelings exist or are planned:

- Brownies: Based on various kinds of helpful household spirits. Their abilities are based on crafting and remaining unseen.
- Ghillie Dhu (currently unimplemented): Based on spirits of the forest. Their abilities are based on hiding in the forest and leading enemies astray.
- Nobles: Based on various royal faeries like King Oberon or the Queen of the Fairies from Tamlin. They can command mortals and other faeries, but their main ability is that they can learn two seasons of seasonal magick (q.v.) instead of one. 
- Pooka: Shapeshifters, based on legends of talking animals. They can assume various animal forms, either wholly or partially, and talk to animals.
- Redcaps (currently unimplemented): Based on the legend of the predator fae who soaked his cap in his victims' blood. Their abilities are based mostly on combat and eating.
- Selkies: Based on the seal-folk of Northern Europe. They can control the weather and exist without penalty in water, and are beguiling to mortals.
- Trow: Based on coblyns and knockers. Their abilities relate to the earth and mining.

Currently, changelings gain their powers in a steady drip of 1 per 5-7 days. It is intended that they accomplish various deeds to gain their powers, but it is currently unimplemeneted.

#### Seasonal Magick

Changeling magic is based on the seasons. Each changeling (except nobles) can pick one path, with their initial choice determined by their court (Spring or summer for the Summer Court, autumn or winter for the Winter Court). Nobles can pick a second season after their first, and this second season does not have to respect their court choice. Each season has its own themes for its glamours:

- Autumn: Fear, decay, the harvest, preservation, fog, colors, home and the hearth
- Spring: Plants, growth, healing, rain, creativity, and love
- Summer: Heat, light, athleticism, passionate/strong emotions, illusion, plenty
- Winter: Darkness, silence, sleep, stasis, ice, snow, cold, wind, death

Seasonal magick requires mana, but it also requires dreamdross to research and the higher-level glamours require dreamdross (refined into "dreamsparks") to weave. This is to provide some resource scarcity, since changelings will have a very large mana pool, and also reinforce that they require access to human dreams to weave their most powerful glamours. No glamour should cost more than 5 dreamsparks to weave, since the absolute maximum capacity is 50 dreamsparks. 

Principles of seasonal magick:
- Seasonal magick can fail. The chance is highest in opposing seasons (in winter if a summer glamour etc), and is increased by the glamour's Difficulty. It is reduced by deduction skill and by the total number of glamours of that season the changeling knows.
- Seasonal magic does not have levels. A glamour's power is determined by the changeling's deduction skill and the number of glamours of that season the changeling knows, like with the failure chance.
- Direct damage should be very rare and usually the result of some natural process caused by the seasonal magic (calling a lightning bolt out of a thunderstorm, starting a wildfire, etc). Folklore has the Fair Folk cursing people and turning them into animals but rarely blowing them up and leaving nothing but smoking boots behind.
- Seasonal magick needs to be added to the appropriate research system EoC (\Xedra_Evolved\mutations\playable_changeling_seasonal_magic_research_eocs.json) in order to allow it to be discovered. There is one for each season. 

### Iron Vulnerability

The Fair Folk (except Homullus) are all vulnerable to iron, suffering pain when they wear or wield items made of it. This can be merely problematic up to actively crippling, depending on the particular Fair Folk. This is intended to be a core balancing point of the Fair Folk and should not be easily worked around or suppressed.

## Lilin

Lilin (singular lilit) are based on the lilin from Hebrew folklore, plague spirits who haunted the wilderness and abandoned buildings. Like vampires, lilin need to feed on others to survive, but unlike vampires they feed in a more metaphysical sense. They require their target's ruach (Heb רוח, "breath" or "spirit") to survive. This is much more subtle than a vampire's feeding because it requires only touch, or for more powerful lilin, merely close proximity. Being drained of ruach leaves the target listless and enervated, and can eventually kill them. Unlike vampires, lilin can feed on a wide variety of targets, though humans and human-adjacent beings provide the most ruach.

Lilin powers are based around darkness, silence, owls, disease, and the moon. 

Principles of lilin powers:

- Lilin powers should, where possible, have their names taken from passages from the Hebrew Bible or related literature (The Book of Enoch, The Book of Tobit, etc)
- Lilin should not have any powers that make them physically tougher. Their strengths are in stealth and evasion. 
- Lilin should not have any damaging power with big up-front damage. Like a disease, they rely on enervating their targets until they're so weak they are easy to kill. However, they can increase the severity of existing diseases. 
- Lilin can use their ruach to weave beautiful illusions around themselves (at night), but they need to use their own words to persuade.
- Like Blood Arts, lilin powers that turn on and remain on until turned off should be implemented as activatable mutations, and powers which create an instant or temporary effect should be implemented as spells in the Supernatural Powers menu.

Lilin gain new powers by dedicating some ruach to power reseach. They can only possibly gain new powers when outside under the moonlight (currently implemented as just outside at night), where periodic power-gaining rolls are made. 

## Gracken

Gracken, called `Species XE843` by XEDRA, are the inhabitants of a dimension of shadows. Those encountered on Earth are the immature juveniles of the species. (more TBD)

