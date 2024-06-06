# No Hope

The mod is based on the old lore where there was a full-scale war with China with many months of preceding marauding and civil disorder. Thus the world with the mod will be much more damaged overall, there will be much less loot etc.

The major goal is to make Cataclysm harder and harsher. To achieve this goal, I decreased loot spawn, made most houses spawn damaged, as well as most cars, and some other changes.


# List of features
- Returned some cut vanilla content, including, but not limited to: laser turret (rewrote description to mention it's working not on the solar panels, but rather on compact yet powerful storage batteries; updated drop list to reflect that), chickenbot, tankbot and tripod and made it spawn in some military locations.

- Returned old names and descriptions for turrets (they are manufactured by General Atomics and Leadworks) and security-bot (Northrop).

- Made ordinary walls hard to set on fire. Based on my old closed PR (https://github.com/CleverRaven/Cataclysm-DDA/pull/15537). Now you can't set it on fire using a simple lighter.

- Alters domestic mapgen palettes to increase damage and messiness.

- Item spawns are decreased by an amount set by a player chosen difficulty at the start of the game. See EoCs.md.

- Returned moose's legendary aggression. Now it's the good old machine of destruction as it used to be.

- Returned back flaming eye's ray of destruction. Removed flaming eye's STARE attack. Added almost identical monster - staring eye with STARE attack.

- Integrated "Cars to Wrecks" mod. Most vehicles spawn damaged and without fuel. Intact cars now are much, much harder to find, though not impossible.

- Integrated "Mutant NPCs" mod. I like the idea, it's fun, so why not?

- Integrated "Extra Bandits" mod. Extra bandits of all sorts everywhere is exactly the thing I'd expect to emerge after the apocalypse. TODO: Leaning further into bandit prevalence would be good for the mods direction imo. Bandits would also be a fitting source of working vehicles/fuel.

- Removed all turrets from military vehicles.

- Most LMOE shelters now have a rather high chance to spawn bandits inside. Empty LMOEs now has a lower chance (1 for every 3 occupied) to spawn.

- Slightly reduced stamina burn rate while walking. TODO: Why?

- Made zombies don't revive by default. Reviving has some serious issues that can't be easy fixed, so until these issues are to be resolved, I'm disabling zombie resurrection in my mod. TODO: Are any of these issues still present?

- Reduced chance to drop military gear from zombie soldiers. TODO: Could be handled by the item category spawn rates instead?

- Gave all mechas ability to protect their operators from melee and ranged damage.

- Returned CBM loot in most places it was removed from (electronic stores, toxic dumps, sewage treatment plants etc).

- Removed military base from the game as it has very serious balance issues.

# List of temporarily missing features

- Decreased amount of ammo drop from turrets: 5.56 - 120, 7.62 - 100, .50 - 90. TODO: Should be handled by the item category spawn rates instead.

- Made all gas stations have 0 - 5000 units of fuel instead of vanilla 40000 - 50000 units. Also made almost all locations in mapgen have the same 0 - 5000 units of fuel, including avgas. Also made almost all cars have zero fuel. There are places and cars where you could still find fuel, they are just very rare. TODO: Some of this should be handled by the item category spawn rates instead.