# Activities

Activities are long term actions, that can be interrupted and (optionally) continued. This allows the player to react to events (like monsters appearing, getting hurt) while doing something that takes more than just one turn.

## Adding new activities

1. `player_activity.h`: Add an entry to `activity_type`, right before `NUM_ACTIVITIES`.
2. `player_activity.cpp`: Add a stop phrase in `player_activity::get_stop_phrase` for the new activity. Use the existing phrases as example.
3. `activity_handlers.h`, `activity_handlers.cpp`: Write a `<activity>_do_turn()` function and/or a `<activity>_finish` function
4. `player_activity.cpp`: Add a case statement in `player_activity::do_turn` and/or `player_activity::finish` to call the appropriate `activity_handlers` function.
5. Start the activity by calling `u.assign_activity(ACT_<activity>, ...)`

One might also add a case for the new activity in `player_activity::is_suspendable`, `player_activity::is_abortable` (see documentation of those functions for what they do).

## Notes

An activity is considered finished when `player_activity::moves_left` is at or below 0. One can reach this state either by explicitly setting the value to 0, or over time by decreasing the value each turn.

Activities can be canceled by calling `game::cancel_activity` or similar. Canceling an activity prevents the `player_activity::finish` function from running, and the activity does therefore not yield a result. `player_activity::finish` is called if the activity is finished normally (`player_activity::moves_left <= 0`).

Canceling the activity with `game::cancel_activity` allows for resuming the activity as a copy of the activity is written to `player::backlog` (only if `player_activity::is_suspendable()` returns `true`).

An activity can be stopped by setting `player_activity::type` to `ACT_NULL`, but this does not allow resuming the task. It is usually used to signal that the activity has been finished early.

While the player character performs the activity, `player_activity::do_turn` is called on each turn. This functions calls some activity specific functions and decrements `player_activity::moves_left`, so it will eventually reach 0.

Some activities (like playing on a PDA) don't have an end result but instead do something each turn (playing on the PDA decreases its batteries and gives a morale bonus).

If the activity needs any information during its execution or when it's finished (like *where* to do something, *which* item to use to do something, ...), use any of:

- `player_activity::index` - what to do (which recipe to craft, ...)
- `player_activity::position` - item position in player's inventory (`player::i_at`)
- `player_activity::name` - name of something to do (skill to be trained)
- `player_activity::values` - vector of any `int` values
- `player_activity::str_values` - vector of any `std::string` values
- `player_activity::placement` - where to do something

Those values are automatically saved and restored when loading a game. Other than that they are not changed/examined by any common code. Different types of activities can use them however they need to.

### `activity_handlers::<activity>_do_turn` function

Make sure that either:

- `player_activity::moves_left` is decreased (in `<activity>_do_turn` or directly in `player_activity::do_turn`),
- Or that the activity is stopped there

For example the pulp activity finishes itself when there are no more unpulped corpses on the map. It therefore does not need to decrease `player_activity::moves_left` on each turn.

One can decrease `player_activity::moves_left` either by `player::moves`, which makes the duration of the activity dependent on the player character's **speed**, or one can decrease it by a fixed value, which makes the activity take a specific amount of **time**, independent of the character's speed. This is done for example with the wait activity.

### `activity_handlers::<activity>_finish` function

This function is called when the activity has been completed. It must set the `player_activity::type` to `ACT_NULL` (or assign a new activity). One should not call `cancel_activity` for the finished activity, as this could make a copy of the activity in `player::backlog`, which allows resuming an already finished activity.
