#ADDING STUFF
##Adding a monster
1. Edit mtype.h.  Change the enum mon_id and insert a unique identifier for
    your new monster type.  Be sure to put it among similar monsters.
	Add or remove the appropriate entry to the monster_names array in tile_id_data.h
	Add the monster to init_translation in mongroupdef.cpp.
2. Edit monsters.json  It is pretty straightforward (Any of this ring a bell?).
    Be ABSOLUTELY sure that you insert the macro at the same point in the list as 
    your inserted the identifier in mon_id!
3. Your monster type is now valid, but won't be spawned.  If you want it to be
    spawned among similar monsters, edit monstergroups.json.  Find the appropriate
    array, and insert the identifier for your monster (e.g, mon_zombie).  Make
    sure it comes in before the NULL at the end of the list.
    Cost_multiplier, makes it more expensive to spawn. The higher the cost, the 
    more 'slots' it takes up, and freq is how frequent they spawn.
    See mongroupdef.cpp:line:116 and up.
4. If you want your monster to drop items, edit monster_drops.json.  Make a new
    array for your monster type with all the map item groups it may carry, and a
    chance value for each.
5. Your monster may have a special attack, a monattack::function reference.
    Edit monattack.h and include the function in the class definition; then
    edit monattack.cpp and define your function.  Functions may be shared among
    different monster types.  Be aware that the function should contain a
    statement that the monster uses to decide whether or not to use the attack,
    and if they do, should reset the monster's attack timer.
6. Just like attacks, some monsters may have a special function called when
    they die.  This works the same as attacks, but the relevant files are
    mondeath.h and mondeath.cpp.

##Adding structures to the map
Most "regular" buildings are spawned in cities (large clusters of buildings which are located rather close to each other).
In file omdata.h in the enum oter_id structure define names (code identifiers) for your building.
If you want your building to be displayed at overmap in different orientations, you should add 4 identifiers for each orientation (south, east, west and north correspondingly).
In the same file in `structure const oter_t oterlist[num_ter_types]`
we should define how these buildings will be displayed, how much they obscure vision and which extras set they have. For example:
```C++
{"mil. surplus",  '^',	c_white,	5, build_extras, false, false},
{"mil. surplus",  '>',	c_white,	5, build_extras, false, false},
{"mil. surplus",  'v',	c_white,	5, build_extras, false, false},
{"mil. surplus",  '<',	c_white,	5, build_extras, false, false}
```
Comments at the beginning of this structure are rather useful.
In the file `mapgen.cpp` find the subroutine called `draw_map(...);`
where you should find a huge variant operator ("switch"). This is where you should
put your code defining the new building by adding new case-statement.

It should be mentioned that most buildings are built on the square `SEEX*2 x SEEY*2` tiles.

If you want your building to be spawned not only in city limits you should refer to structures in file `omdata.h` (starting from the line `#define OMSPEC_FREQ 7`).
These structures are also commented in source code. Add new identifier in enum omspec_id structure before
   `NUM_OMSPECS` and then add a record in `const overmap_special overmap_specials[NUM_OMSPECS]` array. For example:
```C++
       {ot_toxic_dump,	   0,  5, 15, -1, mcat_null, 0, 0, 0, 0, &omspec_place::wilderness,0}
```
  The comments given in source code to `structure struct overmap_special` explain the meaning of these constants in the example above.

##Adding a bionic
1. Edit data/raw/bionics.json and add your bionic near similar types. See JSON_INFO for a more in-depth review of the individual fields.
2. If you want the bionic to be available in the gameworld as an item, add it to item_groups.json
3. Manually code in effects into the appropriate files


##How armour protection is calculated
1. When the player is hit at a specific body part, armour coverage determines whether the armour is hit, or an uncovered part of the player is hit (roll 1d100 against coverage)
2. If the above roll fails (ie roll value is above coverage), then the armour does not absorb any damage from the blow, neither does it become damaged
3. If the above roll succeeds, the armour is hit, possibly absorbing some damage and possibly getting damaged in the process
4. The above steps are repeated for each layer of armour on the body part
5. Armour protects against bash and cut damage.  These are determined by multiplying the armour thickness by the material bash/cut resistance factor respectively, given in materials.json
6. If the armour is made from 2 materials types, then it takes a weighted average of the primary material (66%) and secondary material (33%).
7. Materials resistance factors are given relative to PAPER as a material (this probably needs some fine-tuning for balance).

##Acid resistance
This determines how items react to acid fields.  Item acid resistances are a weighted
average of the materials acid resistance (see item::acid_resist).
An item acid resistance of zero means it will get corroded every turn
An item acid resistance above zero means it has a chance of getting corroded every turn
An item acid resistance above 9 means completely acidproof

Acid resistance values are in materials.json, and defined as such:
  0 - zero resistance to acid (metals)
  1 - partly resistant to acid
  2 - very resistant to acid
  3 - complete acid resistance


#FAQ
**Q: What the heck is up with the map objects?**

A: The pertinent map objects are submap, mapbuffer, map, and overmap.
The submap contains the actual map data, in SEEXxSEEY chunks.
   Vehicles and Spawns are stored in vectors because they're relatively sparse.
Submaps live in the single global mapbuffer, MAPBUFFER.
Map encapsulates the area around the player that is currently active.
   It contains an array of MAPSIZE * MAPSIZE submap pointers called grid.
      This is a 2D array, but is mapped to a 1D array for (questionable) indexing purposes.
   When the player moves from one submap to another, map::shift() is called.
      It moves the pointers in grid to keep the player centered.
      The leading edge of grid is populated by map::load()
         If the submap has been visited before, it's loaded from MAPBUFFER
         Otherwise a 2x2 chunk of submaps is generated based on the corresponding overmap square type.
An overmap encapsulates the large-scale structure of a map
   Such as location and layout of cities, forests, rivers, roads, etc...
   There are an arbitrary number of overmaps
   Additional overmaps are generated as the player enters new areas.

**Q: what is the relation of NPCS/Monsters vs maps. How are they stored, where are they stored? Are they stored?**
A: All npcs are now stored in the overmap. Active npcs only contain the currently active npcs. There is a difference between npc active npc coordinates, and overmap coordinates. So these will need to be changed when saving the npcs. Or else the npcs will be saved at the wrong location. And yes they are stored.
