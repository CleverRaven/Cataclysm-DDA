# Adding New Powers

When adding powers, keep the following in mind:

1) Choose a Difficulty, which is important for determining how much Drain the power causes.
2) Make sure to use the "extra_effects" field to apply drain
3) Make sure the power has some element of randomness--unlike magical spells, psionic powers are not completely predictable. The standard formula I've used is generally { "math": [ "( ( (u_val('spell_level', 'spell: [NAME]') * [LEVELED_VALUE]) + [BASE_VALUE]) * ( ( u_val('intelligence') + 10) / 20 ) )" ] }. This makes sure that the power scales appropriately with its level and also that its effects are modified by intelligence: +5% effectiveness for every point of intelligence above 10, -5% for every point below 10. Generally damage, duration, and range are all scaled this way, while maximum level is a simple 1.5 * intelligence.
4) Make sure the power has connections to other powers, either teaching them or being learned by them.
5) If the power is low-level enough that it should be a starting power, add it in to the EOC_Matrix_Awakening and EOC_Portal_Awakening EoCs to make sure that it's learned when awakening, as well as the appropriate professions.
6) Write a practice recipe for the power and add it to the appropriate EOC_learn_recipes EoC. Psionic practice recipes are found in recipes/practice. The numbers within are drawn from spellbook reading XP rates and teach Difficulty 1 2 3 and powers up to level 12, Difficulty 4 and 5 powers up to level 10, and Difficulty 6 and 7 powers up to level 7.
7) Make sure the power is random in some way--existing powers make heavy use of the RANDOM_DAMAGE and RANDOM_DURATION flags so that they're never completely predictable. 

I've tried to keep each particular psionic path balanced in the amount of powers, though the nature of the powers makes any kind of real balance impossible--teleportation is simply better than most other paths simply because it lets you always choose your battles. 

# Maintaining the Theme

It's deliberate that I use the word "powers" repeatedly in the above description instead of the code terminology of spells--even though game-style psionics were basically an excuse for science fiction authors to include magic into their works, Mind Over Matter powers should feel different than magic. It's very important that new powers fit the theme of using your mind to activate them rather than chanting and making gestures or studying musty tomes. I've tried to go for a moderate level of power--psionics are not subtle and a powerful pyrokinetic can blow up a house, but I've deliberately excluded powers that I felt were too close to magic: see below for limitations.

# Intelligence Bonuses

Because of the cascading effects of increased intelligence on the effectiveness of psionic powers, there are deliberately no effects in Mind Over Matter that increase intelligence. 

# Path Capabilities

Each path should have well-defined capabilities and limitations to prevent thematic dilution:

Biokinesis Can: Improve base human capabilities, making the psion stronger, faster, tougher, and better in combat.  Allow superhuman but understandable feats, such as Sealed System's holding one's breath while fighting in melee combat for minutes at a time or Hardened Skin's turning aside blades. Allow minor temporary alteration of appearance by moving muscles and bones around, just enough to work as a disguise.  Replace very simple tools such as hammers or wrenches.  Allow the psions to maintain good motor function and vigor into very old age. 
Biokinesis Can't: Allow the psion to perform feats that are impossible for human biology, such as breathing chlorine gas without injury, flying, or seeing into the infrared spectrum.  Allow the psion to shapeshift in any capacity or use their body parts as replacements for tools such as mining picks or wood saws.  Heal the psion.

Clairsentience Can:  See a few seconds into the future.  Enhance the psion's mundane senses to see, hear, smell, touch, or taste as well or better than as an animal.  Perform feats impossible with mundane senses, such as seeing in complete darkness, or seeing through walls, or hearing radio transmissions, or smelling carbon monoxide.  Allow the psion to remove any impediment to the clarity of their senses, such as intoxication, poison or disease, or psychosis. 
Clairsentience Can't: Make prophecies or other long-term future predictions.  Affect the senses of others.  Restore the psion's natural senses if they've been damaged (though the psion can use Clairsentience to overcome whatever natural damage they've suffered).  Cure blob psychosis.

Photokinesis Can: Create light out of nothing.  Control existing light or remove it from an area. Protect the psion from dangerous/disruptive amounts of light radiation.  Manipulate other forms of electromagnetic radiation such as gamma rays or radio waves.  Manipulate light to create lasers.  Bend light to render people or objects invisible while still allowing the target to perceive their environment.  Create illusions by changing the movement of light.
Photokinesis Can't: Alter the biology of the psion to use light in any new way, such as becoming photosynthetic.  Turn any part of the psion's body into "living light."  Solidify and give mass to light to make "hardlight."

Pyrokinesis Can: Create fire out of nothing, including on nonflammable surfaces. Protect the psion from excessive heat or flames.  Create light or glowing patches in the air or around objects or people.  Direct fire to move in unnatural ways, or to go through places with no flammable medium to transfer it.  Heat the air around the psion to maintain a comfortable temperature. 
Pyrokinesis Can't: Make fire solid or flow like liquid.  Transform part of the psion's body into "living flame."  Create or manipulate cold.  Create fire inside objects or people. 

Telekinesis Can: Lift objects or beings and throw them with enough force to cause damage.  Speed or slow the movement of objects near the psion enough to protect them from damage (e.g. Inertial Barrier or Aegis).  Allow the psion to fly.  Perform simple manipulation such as picking up a quarter off a table or taking a gun out of holster.  Force another person to move in crude ways, such as dropping a weapon or falling to the floor.
Telekinesis Can't: Create tangible objects or platforms that the psion or another person can pick up and use.  Perform complex manipulation such as picking a lock or programming a computer.  Force another person to move in natural ways, such as walking smoothly or picking up a coffee cup. 

Telepathy Can: Read the surface thoughts of others.  Disrupt their thinking or brain function, possibly leading to permanent injury or death.  Improve concentration and thinking efficiency. Take direct control of others' minds.  Read the minds of sapient alien species (mi-go, yrax, triffids).  Learn information deeper than surface thoughts, in the form of images or dream-like impressions.  Make brainwashing or cult recruitment much easier.  Protect the psion from telepathic attacks or being noticed by otherworldly entities during a portal storm.  Directly edit the psion's own thoughts or memories.  Make another person forget the last few minutes (short-term memory only).  Influence the attitude of others in small ways, such as changing indifference into willingness to listen or readiness to attack into veiled hostility. 
Telepathy Can't: Create permanent personality changes that do not stem from brain damage caused by telepathic attack.  Overcome language barriers.  Make the psion more intelligent.  Read or affect the mind of beings which don't have a conventional mind (zombies, robots, yrax constructs, mycus, some Nether inhabitants).  Cure blob psychosis or restore ferals to their former selves.  Directly edit another's thoughts or long-term memories.

Teleportation Can: Allow the psion to move without crossing the intervening distance.  "Warp space" such that distances take longer or shorter to traverse than they appear that they should.  Teleport objects out of or into people's hands.  Teleport the psion to other realities or to the Nether (possible endgame--extremely powerful teleporter just leaves?).  Teleport safely to unseen nearby locations.
Teleportation Can't: Teleport only part of a creature, such as teleporting a zombie's head off or a feral's heart out of their chest.  Teleport other people, or the psion themselves, into walls or solid rock (the nature of their powers prevents unsafe teleporting).  Travel through time.  Teleport long distances to places the psion has never seen. 

Vitakinesis Can: Speed up healing to preternatural levels, healing broken bones in days and grievous wounds in hours.  Resist disease and poisoning.  Cure some deleterious traits such as deafness or blindness, with an appropriate channeling time and recovery period.  Regrow lost limbs.  Obviate the need for sleep. Transfer diseases to or from other people.  Affect the healing rates of others, speeding them up or slowing them down.  Extend the psion's lifespan, possibly indefinitely. 
Vitakinesis Can't: Instantly close wounds.  Cure mutations that change the fundamental shape of the body, such as a tail or paws.  Cause serious wounds in others.  Extend others' lifespans.  Cure blob psychosis or restore ferals to their former selves. 

Matrix technology can overcome a few of the above-listed limitations.
