# Portal Storm Design
Portal storms are otherworldly collisions. Evil and malicious entities from inexplainable spaces are pouring into our world. However this doesn't mean it's a good way to immediately kill the character.

## Ideas
Portal Storms should consist of the inexplainable and the disturbing.  There should be less of a focus even on our usual nether monsters, instead leaning on even more disturbing creatures. 

## Reminders
Some things to keep in mind when working on Portal Storms:
* Portal storms are weather: You are not writing boss fights, or end game encounters, you are writing and implementing weather effects. Try not to lose track of that.
* Portal storms shouldn't be forced rest: It should be *possible* for a strong character to maneuver in a portal storm. It can be risky, but shouldn't be impossible.
* Portal storm encounters don't need to be rewarding: Some encounters will be, but not every encounter needs a carrot. The variance of portal storms means that traveling the storms is a gamble, sometimes you get lucky, other times you do not.
* Portal storms should only be *actively* dangerous with creatures hunting you when you go outside (more on this in balance).
* Portal storms are trying to destroy humans, and warp the world: Portal storms should also really only be dangerous to humans, creatures that can perceive the otherworldly nature. Animals and the dead should have little to no interest in the other side and vice versa.
* Don't be annoying: Nothing ruins ambience as fast as repetition. Don't spam effects, don't waste the players time. Portal Storms already *show* the player they are dangerous, we do not need to *tell* the player they are scary or weird.
* Less is more: As an extension of above, portal storm encounters should be disruptive, strange, and not overstay their welcome. Always remember there is something new coming to terrorize the player soon enough.
* Portal storms are compounding difficulty: A player sitting in their fortified base with doors and the blinds shut **should be safe from the storm**. Storms should compound problems the player is already facing, Cataclysm is a game of **compounding difficulty** and **losing to a mistake you made 5 minutes ago**. Don't build portal storm encounters to kill high powered characters in a 1v1, they should be an additional vector to put pressure on the player and force them adapt while in the field.

## Long Term Desires
A few of the additional goals for storms are as follows:

* Localized and trackable storms, similar to a game like Death Stranding where you can detect and avoid storm areas, or even drive out of them.
* Themed storms. Currently we have one eldritch themed storm with a mix of everything, having a number of themed storms with different effects and thematic voices would be great.

# Portal Storm Balance
There are a few separate concepts dictating portal storms.

## Ire
Being outside in a portal storm should draw the attention of the malicious entities being pulled into our realm by the storm. This is referred to in the code and this document as `ire`.

## Passive vs Active
Portal storms should represent two separate ideas.
1. *Passively* creatures and objects from other realities are pouring into ours, misshapen and incomprehensible.
2. *Actively* you are targeted by powerful otherworldly figures you have drawn the ire of that want you explicitly dead.

With this goal in mind, portal storms are split into two categories of event. 

### `Passive` 
These events represent goal 1 in the entry above and should reflect creatures that are more disturbing than dangerous, or behave more as obstacles than active threats. They should not be able to **actively** hunt the player. Passive effects happen whether or not the portal storm is aware of the Player. These are the effects of worlds colliding together, no intent, just chaos. 

Passive examples include stuff like:
* Inexplainable shapes, unmoving creatures that if you look on for too long make you go insane.
* Memories of the dead, monsters with no senses that when you get near grab you and scream drawing nearby monsters.
* Giant appendages, similarly creatures with no senses, however these ones are much more dangerous if you do get into melee with them.

### `Active`
These events represent goal 2 in the entry above. This is where the designs can get more awful. These entities should be actively trying to sabotage, disrupt, and destroy the player. This doesn't mean they should be strong combatants but it does mean they should be able to perceive, track and follow the player.  These effects should *only* happen if you have built up `ire` and should cost `ire` to generate.

Active examples include stuff like:
* The Person, a clairvoyant entity that attempts to follow you into buildings and on touching you causes nightmares and hallucinations
* The Swarm Structure, a mass of deadly blades that explodes into fragments on getting close to the player.

### Non conforming storms
This is relating to people looking to implement themed storms. All future storm concepts should at some level follow the above paradigm even if it isn't directly.

So you can:
  * have a storm that is *all passive effects* such as alien flora growing everywhere and rapidly dying.
  * have a storm that is *all active effects* such as extradimensional bounty hunters tracking anyone who is visible outside.
  * have a storm that doesn't directly use the concept of `ire`, but still follows the idea of **active danger only when outside** such as a storm that *passively* rains blood and has monsters that can only *actively* track characters soaked in blood.


## Implementation
In the code portal storm effects you wish to include should be added to one of two EOC's: 

### `EOC_PORTAL_EFFECTS_PASSIVE`
This is the EOC that triggers passive portal storm effects. These should *not* cost ire.

### `EOC_PORTAL_EFFECTS_ACTIVE`
This is the EOC that triggers active portal storm effects. **Any** EOC that is added to this list should subsequently reduce the characters `ire` u_val.
