existing hordes live in overmap::zg;
hordes move with calls to overmap::move_hordes()
These should both be removed, specific monster instances should be created instead of hordes, probably.

monsters outside the reality bubble live in overmap::monster_map
methods for spawning and despawning monsters live in overmapbuffer::{spawn,despawn}_monster()
need an act method, overmapbuffer::move_hordes()
In overmapbuffer since they will need to cross overmap borders.
Store references to monsters on a queue keyed by turns, after acting monsters insert themselves in the queue a number of turns in the future based on their speed.
Treat each submap of monsters as a group that gets processed at once, that would be slightly less turn-accurate, but have MUCH better memory coherence.

Use cases:
Steady-state processing:
The overmap is triggered to do a turn, it grabs the monster group at the head of monster_turn_sequence and checks if they are due for processing, if they are, it processes them, moving to new locations in monster_map if necessary, and re-inserts them in the appropriate monster_turn_sequence (might be on a new overmap).
Needs O(1) lookup for next group to process (will likely iterate on it).
O(1) insertion into a new monster_group iff next update for it is sooner than the next update for the monster being inserted (likely, it just took a turn), O(logn) insertion if new monster group updates later than the new monster, or does not exist, requiring an insertion into monster_turn_sequence.
O(logn) re-insertion into the current monster_group, since its turn count is definitely changing.
Optimizations: Batch insertions into neighbor monster groups to minimize number of insertions of the group.  Process monsters slightly ahead of schedule (based on distance from player?) to reduce number of updates.

Reacting to stimuli: (sound or visible stimuli such as flashing lightsor movement)
Iterate over monster_map locations in range of the event (O(n^2) in range) and check for replacing existing event (loudest sound or most visible stimuli wins).  Weird special case, underground gets extra extra attenuation.  At a minimum queue these up and process them once per turn, with only loudest sound per submap (or possibly loudest per map? being applied to all monster groups.
For flavor it would be nice if there were some random-ish noises on the overmap as well.

For map data (navigability, vision, and scent), monster AI can have a persistent "oracle" injected into it, which it can query for map data such as nearest visible enemy, nearest strong scent, and pathing information.  This means the monster AI doesn't have a dependency on the overmap, and creates an injection point for testing monster AI.

Save/load:
When an overmap is savesd, the monster groups can be written out in no particular order.
When an overmap is loaded, re-inserting the monster groups should repopulate the data structures properly.

API on overmap (or overmapbuffer?):
despawn_monsters() // Move monsters from map to an overmap.
spawn_monsters()
move_hordes()
add_monster() // Will probably want it for missions and mapgen

API on monster_map
add_monster(monster new_monster)
add_monster_group(monster_group new_monsters)
save_monsters()
load_monsters()?
monster_group_info get_monster_group_info(tripoint location) // For overmap display
move()

API on monster_group
add_monster(monster new_monster)
merge_monster_group(monster_group new_monsters)



Movement:
Each OMT has a load factor determined as a low estimate of the number of navigable tiles for a tile of its kind.
OMTs backing generated submaps should be updated with the actual load factor.
Monsters attempting to move into a submap at capacity should lose a turn.
OMTs should have a move cost derived in a similar way to the load factor.
Monster pays move cost to exit an OMT
Eventually monster groups should fight each other if they encounter opposition.
Dense hordes should mark map tiles for destruction as they cross over them, at some point becoming completely wrecked.

Spawning:
When a monster group encounters the reality bubble or vice versa, it spawns on the border of the reality bubble.
When a monster moves onto the reality bubble, it tries to spawn in the row adjacent to the edge of the bubble, if that fails it might try to push or trample another monster, or bash something as per normal movement.
When the reality bubble moves onto a monster group (loads the submap it is on), each monster attempts to spawn in the corresponding submap.  If it is an already generated submap, the capacity is known and there should be only enough monsters to fill it.  If it is a newly generated submap we can pass an argument to mapgen to force enough room for the monsters.