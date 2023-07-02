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

It's deliberate that I use the word "powers" repeatedly in the above description instead of the code terminology of spells--even though game-style psionics were basically an excuse for science fiction authors to include magic into their works, Mind Over Matter powers should feel different than magic. It's very important that new powers fit the theme of using your mind to activate them rather than chanting and making gestures or studying musty tomes. I've tried to go for a moderate level of power--psionics are not subtle and a powerful pyrokinetic can blow up a house, but I've deliberately excluded powers that I felt were too close to magic: telekinetics cannot summon weapons made of telekinetic force, biokinetics cannot shapeshift, vitakinetics cannot instantly heal themselves, pyrokinetics cannot turn themselves into "living flame", and so on. 

# Intelligence Bonuses

Because of the cascading effects of increased intelligence on the effectiveness of psionic powers, there are deliberately no effects in Mind Over Matter that increase intelligence. 
