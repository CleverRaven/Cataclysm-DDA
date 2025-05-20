# Developer FAQ

## Adding stuff

## Adding a monster

1. Edit `data/json/monsters.json` or create a new json file and insert the definition of your new monster there (probably copy an existing entry).
2. Make sure the id value is unique among all other monster types.
3. Your monster type is now valid, but won't be spawned.  If you want it to be spawned among similar monsters, edit monstergroups.json.  Find the appropriate array, and insert the identifier for your monster (e.g, `mon_zombie`).  `cost_multiplier`, makes it more expensive to spawn. The higher the cost, the more 'slots' it takes up, and freq is how frequent they spawn.  See `mongroupdef.cpp`
4. If you want your monster to drop items, edit `monster_drops.json`.  Make a new array for your monster type with all the map item groups it may carry, and a chance value for each.
5. Your monster may have a special attack, a `monattack::function` reference.  Edit `monattack.h` and include the function in the class definition; edit `monstergenerator.cpp` and add the translation, then edit `monattack.cpp` and define your function.  Functions may be shared among different monster types.  Be aware that the function should contain a statement that the monster uses to decide whether or not to use the attack, and if they do, should reset the monster's attack timer.
6. Just like attacks, some monsters may have a special function called when they die.  This works the same as attacks, but the relevant files are `mondeath.h` and `mondeath.cpp`.
7. If you add flags, document them in `JSON_FLAGS.md`, and `mtype.h`. Please. Or we will replace your blood with acid in the night.

## Adding structures to the map

Most "regular" buildings are spawned in cities (large clusters of buildings which are located rather close to each other).

In file `omdata.h` in the enum `oter_id` structure define names (code identifiers) for your building.

If you want your building to be displayed at overmap in different orientations, you should add 4 identifiers for each orientation (`south`, `east`, `west` and `north` correspondingly).

In the same file in structure `const oter_t oterlist[num_ter_types]` we should define how these buildings will be displayed, how much they obscure vision and which extras set they have. For example:

```cpp
{"mil. surplus", '^', c_white, 5, build_extras, false, false},
{"mil. surplus", '>', c_white, 5, build_extras, false, false},
{"mil. surplus", 'v', c_white, 5, build_extras, false, false},
{"mil. surplus", '<', c_white, 5, build_extras, false, false}
```

Comments at the beginning of this structure are rather useful. In the file `mapgen.cpp` find the subroutine called `draw_map(...);` where you should find a huge variant operator ("switch"). This is where you should put your code defining the new building by adding new case-statement.

It should be mentioned that most buildings are built on the square `SEEX*2 x SEEY*2` tiles.

If you want your building to be spawned not only in city limits you should refer to structures in file `omdata.h` (starting from the line `#define OMSPEC_FREQ 7`).

These structures are also commented in source code. Add new identifier in enum `omspec_id` structure before `NUM_OMSPECS` and then add a record in `const overmap_special overmap_specials[NUM_OMSPECS]` array. For example:

```cpp
{ot_toxic_dump,   0,  5, 15, -1, mcat_null, 0, 0, 0, 0, &omspec_place::wilderness,0}
```

The comments given in source code to structure `struct overmap_special` explain the meaning of these constants in the example above.

## Adding a bionic

1. Edit `data/json/bionics.json` and add your bionic near similar types. See `JSON_INFO.md` for a more in-depth review of the individual fields.
2. If you want the bionic to be available in the game world as an item, add it to `item_groups.json`, and add a bionic item to `data/json/items/bionics.json`.
3. Manually code in effects into the appropriate files, for activated bionics edit the `player::activate_bionic` function in `bionics.cpp`.
4. For bionic ranged weapons add the bionic weapon counterparts to `ranged.json`. Give them the `BIONIC_WEAPON` flag if you want to prevent the weapon from being wielded if not attached to a bionic.
5. For bionic close combat weapons add the bionic weapon to `data/json/items/melee.json`. Give them the `BIONIC_WEAPON` flag if you want to prevent the weapon from being wielded if not attached to a bionic.

## How armor protection is calculated

1. When the player is hit at a specific body part, first a sublocation on the limb is generated from the weighted list of sublimbs
2. For the torso location an additional secondary sublimb is chosen for items hanging off the player (like backpacks)
3. Armor coverage determines whether any armor is hit, or an uncovered part of the player is hit (roll a single `1d100` against coverage).
4. Go through all worn armor from the most outward clothing piece inwards. Reducing the total damage as you apply the defense of each item.
5. If the armor doesn't cover the chosen sub body part skip it.
6. Depending on the attack the armors melee, ranged or general coverage (which is not scaled to the overall body part and instead just how much of the sublimb it covers) is compared to the roll from above.
7. In the future Vitals Coverage will be used to scale incoming critical damage, reducing crit multipliers but it is not implemented yet.
8. If the above roll fails (i.e. roll value is above coverage), then the armor does not absorb any damage from the blow, neither does it become damaged.
9. If the above roll succeeds, the armor is hit, possibly absorbing some damage and possibly getting damaged in the process.
10. If the armor has ABLATIVE pockets (like ballistic vests) at this point the coverage of those plates is rolled to see if they possibly absorb some damage and possibly get damaged in the process. At most one ablative pocket on an item can apply to an attack and the coverage of the plates is scaled based on the coverage of the armor they are in. This is because if a plate covers 45% of the torso but the clothing its in only covers 50% of the torso, you already know the attack hit the jacket and the jacket is 90% (45/50) plate.
11. Armor protects against damage.  The values are determined by multiplying the armor thickness by the materials individual damage type resistance factor respectively, given in `materials.json`.
12. For simple definition armors: If the armor is made from more than 1 material types, then it divides the armors overall thickness based on proportions each is (assumes equal proportions by default). Giving a single protection value per type of damage.
13. For complex definition armors: Each material on each limb/sublimb can have a specific thickness and also an amount of the overall armored portion that it covers (proportional_coverage). Each individual material is rolled against its proportional_coverage vs `1d100` to see if it applies to any given attack. That is why some armors have a red (worst case) percent, green (best case) percent and a median protection value now because an armor itself can have variable protection for any given attack. This is to simulate armor with padding or plating that don't fully cover it.

Specifically to ablative plates, some transform when damaged instead of damaging normally. The chance of transforming scales based on damage taken.

## Adding an iuse function.

1. Add the new item use code to `iuse.cpp` and `iuse.h`.
2. Add a new json_flag to `flags.json`, and add the flag to your item(s) and to an `item_actions.json` entry. 
3. Link the flag to the iuse function in `item_factory.cpp`.
4. Document the new flag in `JSON_FLAGS.md`.

## Acid resistance

This determines how items react to acid fields.  Item acid resistances are a weighted average of the materials acid resistance (see `item::acid_resist`):

- An item acid resistance of zero means it will get corroded every turn;
- An item acid resistance above zero means it has a chance of getting corroded every turn;
- An item acid resistance above 9 means completely acidproof.

Acid resistance values are in `materials.json`, and defined as such:

- 0 - zero resistance to acid (metals)
- 1 - partly resistant to acid
- 2 - very resistant to acid
- 3 - complete acid resistance

# FAQ

**Q: How do I prevent an item from showing up in NPC inventory?**

A: Add the `TRADER_AVOID` flag to it.

**Q: What the heck is up with the map objects?**

A: The pertinent map objects are submap, mapbuffer, map, and overmap.  The submap contains the actual map data, in SEEXxSEEY chunks.  Vehicles and spawns are stored in vectors because they are relatively sparse.  Submaps live in the single global mapbuffer, MAPBUFFER. Map encapsulates the area around the player that is currently active.  It contains an array of MAPSIZE * MAPSIZE submap pointers called grid.  This is a 2D array, but is mapped to a 1D array for (questionable) indexing purposes.  When the player moves from one submap to another, `map::shift()` is called.  It moves the pointers in grid to keep the player centered.  The leading edge of grid is populated by `map::load()`.  If the submap has been visited before, it's loaded from MAPBUFFER.  Otherwise a 2x2 chunk of submaps is generated based on the corresponding overmap square type.  An overmap encapsulates the large-scale structure of a map (such as location and layout of cities, forests, rivers, roads, etc...).  There are an arbitrary number of overmaps.  Additional overmaps are generated as the player enters new areas.

**Q: What is the relation of NPCs/Monsters vs maps. How are they stored, where are they stored? Are they stored?**

A: All npcs are now stored in the overmap. Active npcs only contain the currently active npcs. There is a difference between npc active npc coordinates, and overmap coordinates. So these will need to be changed when saving the npcs. Or else the npcs will be saved at the wrong location. And yes they are stored.
