# List of Short Term Tasks

In order to keep Dark Skies Above moving and to not get overwhelmed, I'm creating lists of achievable, short term tasks that need to done.  These lists should be periodically updated as items are completed.

## JSON
1.  Alien light reconnaissance patrols (ALRPs): The aliens need a light reconnaissance unit.  Conceptually, these should be weak enough for a lone survivor with military grade terrestrial weapons to beat, so around 50-60 HP, light body armor, and energy weapons. A standard ALRP should be 3-4 aliens along with surveillance or seeker drones.

Ideally, ALRPs show up 30 minutes or so after a surveillance drone summons them after seeing a human wielding a firearm or carrying a rifle, patrol the area for a day or two, and return to base.  The first implementation is probably going to appear instantly after a drone is killed (part of the drone's death function) and stay indefinitely.

2.  ALRP gear: Each ALRP has an energy weapon of some kind and light body armor (much less weight/encumbrance than ESAPI, but somewhat less protection).  They possibly have alien grenades, alien surveillance gear, and other stuff that can be repurposed into homebrew devices.

3.  ALRP landing map extra (ALMX): ALRPs are deployed from orbit in drop capsules.  We'll need a map extra for this.  Incidentally, abandoned drop capsules are also good locations to scavenge minor alien parts for homebrew items.

4.  ALRP-army shootout map extra (AASMX): ALRPs should be killable, and obviously so. The AASMX has 2-3 dead aliens and 4-9 dead soldier corpses.  The avatar can loot military grade weapons and armor, as well as ALRP items for making homebrew and cracked items, but mostly homebrew.

5.  Initial homebrew recipes: We need some early recipes for homebrew using ALRP gear as primary components.

## Code
1.  Patrol patterns: This new code feature will allow monsters placed via mapgen to move around the map in a predictable pattern.  The initial implementation will only cover movement within the reality bubble, but an advanced version will include overmap movement.

2.  Expiration dates: This new code feature will also monsters placed via mapgen to silently disappear from the game after a specified period.  This means we can have limited duration alien patrols.

3.  Alien surveillance drone threat recognition: Alien drones will need code support to recognize that a Character is carrying/wearing a rifle or wielding a firearm.

4.  Deferred reinforcement summoning: ALRP drop pods should arrive 30-60 minutes after being summoned by a surveillance drone.  This will probably require code support.

# Mid Term Ideas

1.  Alien heavy patrols - upgrades for ALRPs, either more little aliens with better armors and guns, or stronger aliens summoned when ALRPs get killed.  JSON.

2.  The Resistance - human allies!  There's some stuff that can be done with time bounded conversations so that the Resistance base can naturally upgrade over time.  JSON.

3.  Time delayed map extras - Map extras should change over time, the same way that animal random spawns do.  Code/JSON but mostly code.

4.  Vehicle targeting - Monsters and NPCs should be able to recognize vehicles as threats and attack them directly without needing to see a passenger.  I'm going to do that anyway but it could be useful here.  Code.
