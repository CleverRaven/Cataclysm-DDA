# Design Docs

These documents are for the lore basis and mechnical assumptions behind some of XEs systems. They contain heavy spoilers, obviously--if you're a player, don't read if you want to be surprised!

## Timeline Differences from vanilla CDDA

TBD

## Dream Magick

TBD

## Hedge Magick and Alchemy

TBD

## Vampires

Xedra Evolved vampires are not walking corpses who drink the blood of the living. At least, not at first. Vampirism is a progressive disease and many sufferers are more alive than they are dead. While they must drink blood and can derive power from it, they still need to eat, drink, and sleep just like mortals do. Becoming an actual undead monster requires a conscious decision on the part of the nascent vampire and can only be accomplished by committing heinous crimes (at the time of this writing, murdering a dear and trusting friend). Actual vampires are monsters without exception, predators who have extinguished all feeling from their hearts. 

Vampires have existed for a long time on the Earth of Xedra Evolved, and on some--though not all--extradimensional Earths as well. It is possible that humanity's first steps into agriculture and gathering into stationary cities were encouraged by vampires, so that they would have an easier time maintaining their herds. They use their `renfields`, brainwashed mortal agents, to manipulate human society and keep themselves safe from those who would destroy them. 

Vampires require human blood, or the blood of human-like species. They cannot survive on animal blood, so they are forced to prey on fellow sapients to survive. They also cannot drink vampire blood (though see below), becuase vampires have enough control over their own blood to prevent another from drinking it. A vampire who does not use any of their powers (called Blood Arts) is capable of surviving for quite a long time without needing to feed. 

### Blood Arts

Blood Arts is the term for vampires special abilities. With then, vampires can become stronger, faster, more resilient, have better senses, hypnotize or charm their prey, and eventually turn into wolf, bat, or mist.  More powerful vampires can also control darkness and shadows. The balance point for bLood arts is intentionally set so that a vampires who uses several of them will either have to feed soon after to avoid the `withering` (the status penalties that come from being low on blood), or will have to gorge beforehand and become `blood_quenched` in order to have enough blood to use all the Blood Arts they want during combat.

Principles of Blood Arts:
- Blood Arts cost blood. Any secondary costs (mana, stamina, sleepiness, etc) are unnecessary. Use of their powers should require vampires to drink more blood.
- Blood Arts should be "vampire-y" in theme and scope, with some latitude for interpretation. Creating tactile illusionary beasts that can harm the vampire's enemies is out, but creating animate shadows that can harm the vampire's enemies but die in light, or clouding their enemy's mind so they perceive a group of wolves approaching them, would both be acceptable.
- Blood Arts do not have levels, and are not subject to failure based on power or skill level.
- Blood Arts that turn on and remain on until turned off should be implemented as activatable mutations, so that there's a easy screen where the player can see what all is active and turn them on and off. Blood Arts which create an instant or temporary effect should be implemented as spells in the Supernatural Powers menu.
- It is currently possible for a player to eventually learn every single available Blood Art, so be careful of unexpected synergies. 

### Vampire Anathema

TBD

### Dhampirs

Dhampirs are the children of a lower-tier vampire, one who has not yet made the awful choice to become an undead monster, and a mortal (or, rarely, another lower-tier vampire). They inherit a portion of vampire power but also some vampiric weaknesses. Dhampirs are innately capable of sensing both vampires and renfields, and will sometimes become vampire hunters because they cannot help but be aware of the threat.

Dhampirs naturally regenerate the blood vitamin uses to power their Blood Arts, but intentionally do so very slowly. The dhampir gameplay loop is designed to always tempt to them drink blood for power, to facilitate the "am I a man, or a monster?" question that often plagues fictional dhampirs, but also punish them in turn if they do. Dhampirs gain two weaknesses from a set of thematically-appropriate ones, but if they've ever drunk blood--even once--this list is expanded and they become eligible for more total weaknesses.

Dhampirs can use tier 1, 2, and 3 Blood Arts, though they should not have any shapeshifting or Blood Arts that rely on the vampire being undead (such as falling off a ten-story building and taking no damage). 

Some dhampir-exclusive abilities, and some vampire Blood Arts when used by dhampirs, only function when they have `empowered blood`, which is when the blood vitamins is at 1 or higher. When `enervated` (at blood vitamin 0 or lower), they suffer minor but slow scaling penalties to hunger, thirst, sleepiness, and stamina. Dhampir do not suffer from the `withering`, and while they can become `blood_quenched`, they can only do so by drinking blood. 

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

## The Fair Folk

The Fair Folk are a multidimensional power on the level of the triffids, in that they are capable of exerting power across many dimensions simultaneously and they have nothing to fear from the blob. Unlike the triffids, they are not expansionist. They have a home dimension variously called `Under the Hill`, `the Bright Lands`, `the Elflands`, and similar names. If they have their own name for their dimension, they don't use it with mortals.

The Fair Folk are divided into the elemental fae, or Paraclesians, and more humanlike fae, or changelings. Note that despite being called changelings in XE, most changelings were born in the Elflands and not swapped for human babies.

Almost all Fair Folk are vulnerable to iron to varying degrees (though see below)

### Paraclesians

Paraclesians are based on the elements: Sylphs for air, Ierde for earth, Undines for water, Salamanders for fire, and Arvore for wood. A sixth type, Homullus is effectively an "elemental" of humanity. Other elemental types might exist related to other species such as gracken, but they generally do not visit Earth.

Paraclesians are the eldest and original Fair Folk, born from the "dreams" of natural features like mountains and rivers. They spring to life fully formed, but with little power, and their parents often place them outside the Bright Lands to grow up. Those Paraclesians who survive and manage to make it back home are considered adults. 

Paralecian magick is heavily themed and also grows much stronger when in an appropriate place of power (in the wilderness away from cities for Arvore, underground for Ierde, and so on) and much weaker when in an opposite place (underground for Sylph, in the cold for Salamanders, etc). At the moment it has levels and requires practice, but this is undesirable--the Fair Folk are magical beings and should not need to practice their spells like human wizards. All Paraclesians can transform into the embodiment of their element, and all of them transform into more pure elemental forms as they grow in power.

Each Paraclesian has their own design sensibilities:
- Arvore: Arvore powers relate to plants, nature, and growing things. Their magic allows them to become more plantlike, encourage plant growth, draw sustenance from soil and sunlight, and so on. While Arvore do have damaging spells, they mostly only affect plants and zombies, and then not generally in a magical bolt instant-damage fashion, since damage-over-time is much more similar to how nature works. Arvore deliberately have no powers that affect animals. The mortal need Arvore can overcome is requiring food and drink.
- Homullus: Homullus powers relate to humanity, tools, cities and knowledge.  They have abilities related to managing humans and formerly human creatures such as ferals, renfields, etc.  Currently not implemented but they should be incredibly effective at dealing with cyborgs and some abilities related to interacting with robots (as tools created by humans).  They have the ability to install CBMs or even choose to limit their Paraclesian abilities by pursuing a mutation path created by humans.  The mortal need that Homullus can overcome is anything that limits their ability to make choices such as mind control.
- Ierde: Ierde powers relate to resilience, preservation, and the manipulation of earth and stone. They are designed less to destroy their enemies with magick and more to slow them, redirect them, and simply outlast them (though since only the player has stamina this works less well than it could). The mortal need Ierde can overcome is sensitivity to pain and tiredness (stamina, not sleepiness). 
- Salamander: Salamanders are the most innately destructive of the Paraclesians, though their magick also invokes the way forests regrow after a forest fire. Their magick allows them to move quickly and set everything on fire (though they are deliberately designed without an "I cast fireball" equivalent), with some associatons with the heat of the forge and with light. The mortal need Salamander can overcome is need for water or sleep, though part of their design is that they need much more extra food (as fuel, so to speak)
- Sylph: Sylph powers relate to wind, lightning, and the weather (weather currently unimplemented). The mortal need Sylphs can overcome is being tied down, in both the literal gravity sense and the metaphorical snares or entanglements sense.
- Undine: Undine powers relate to water and acid, as well as the transformation of liquids.  Undines are slightly different because by default they do not need to drink water and heal when in water, but many of their powers have a thirst cost so they can become thirsty by using their magic. They can also regain thirst automatically when underwater, so the push-pull of Undine powers is making sure to never be so far from water that they can't top up when necessary.

### Changelings

## Lilin

## Gracken

Gracken, called `Species XE843` by XEDRA, are the inhabitants of a dimension of shadows. Those encountered on Earth are the immature juveniles of the species. (more TBD)

