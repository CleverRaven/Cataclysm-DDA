# Activities

Activities are long term actions, that can be interrupted and (optionally) continued. This allows the player to react to events (like monsters appearing, getting hurt) while doing something that takes more than just one turn.

## Adding new activities

1. `player_activities.json` Define properties that apply to all instances of the activity in question.
2. `activity_handlers.h` Optionally declare a do_turn and/or finish function.
3. `player_activity.cpp` Optionally update `can_resume_with`.
4. `activity_handlers.cpp` Add the functions from #2 into the do_turn and finish maps, and define the functions.
5. Start the activity by calling `player::assign_activity`.

## JSON Properties

* verb: A descriptive term to describe the activity to be used in the query to stop the activity, and strings that describe it, example : `"verb": "mining"` or `"verb": { "ctxt": "instrument", "str": "playing" }`.
* suspendable (true): If true, the activity can be continued without starting from scratch again. This is only possible if `can_resume_with()` returns true.
* rooted (false): If true, then during the activity, recoil is reduced, and plant mutants sink their roots into the ground. Should be true if the activity lasts longer than a few minutes, and can always be accomplished without moving your feet.
* based_on: Can be 'time', 'speed', or 'neither'.
	* time: The amount that `player_activity::moves_left` is decremented by is independent from the character's speed.
	* speed: `player_activity::moves_left` may be decremented faster or slower, depending on the character's speed.
	* neither: `moves_left` will not be decremented. Thus you must define a do_turn function; otherwise the activity will never end!
* no_resume (false): Rather than resuming, you must always restart the activity from scratch.
* multi_activity(false): This activity will repeat until it cannot do any more work, used for NPC and player zone 		  activities.

## Termination

There are several ways an activity can be ended:

1. Call `player_activity::set_to_null()`
	This can be called if it finished early or something went wrong, such as corrupt data, disappearing items, etc. The activity will not be put into the backlog.
2. `moves_left` <= 0
	Once this condition is true, the finish function, if there is one, is called. The finish function should call `set_to_null()`. If there isn't a finish function, `set_to_null()` will be called for you (from player_activity::do_turn).
3. `game::cancel_activity` or `player::cancel_activity`
	Canceling an activity prevents the `player_activity::finish` function from running, and the activity does therefore not yield a result. A copy of the activity is written to `player::backlog` if it's suspendable.

## Notes

While the player character performs the activity, `player_activity::do_turn` is called on each turn. Depending on the JSON properties, this will do some stuff. It will also call the do_turn function, and if `moves_left` is non-positive, the finish function.

Some activities (like playing music on a mp3 player) don't have an end result but instead do something each turn (playing music on the mp3 player decreases its batteries and gives a morale bonus).

If the activity needs any information during its execution or when it's finished (like *where* to do something, *which* item to use to do something, ...), use any of:

- `player_activity::index` - what to do (which recipe to craft, ...)
- `player_activity::position` - item position in player's inventory (`player::i_at`)
- `player_activity::name` - name of something to do (skill to be trained)
- `player_activity::values` - vector of any `int` values
- `player_activity::str_values` - vector of any `std::string` values
- `player_activity::placement` - where to do something

Those values are automatically saved and restored when loading a game. Other than that they are not changed/examined by any common code. Different types of activities can use them however they need to.

If you are adding an activity that may be possible for NPCs to perform, then please make the activity placement in absolute coords.
As the local coordinate system is based on the avatar position.

### `activity_handlers::<activity>_do_turn` function

To prevent an infinite loop, ensure that one of the following is satisfied:

- The `based_on` JSON property is `speed` or `time`,
- Or `player_activity::moves_left` is decreased (in `<activity>_do_turn`,
- Or that the activity is stopped there (see 'Termination', above).

For example the pulp activity finishes itself when there are no more unpulped corpses on the map. It therefore does not need to decrease `player_activity::moves_left` on each turn.

### `activity_handlers::<activity>_finish` function

This function is called when the activity has been completed (`moves_left` <= 0). It must call `set_to_null()` or assign a new activity. One should not call `cancel_activity` for the finished activity, as this could make a copy of the activity in `player::backlog`, which allows resuming an already finished activity.
