# Activities

Activities are long term actions, that can be interrupted and (optionally)
continued. This allows the avatar or an npc to react to events (like
monsters appearing, getting hurt) while doing something that takes more
than just one turn.

## Adding new activities

1. `player_activities.json` Define properties that apply to all instances
of the activity in question.

2. `activity_actor.h` Create a new subclass of `activity_actor`
(e.g. move_items_activity_actor) to store state and handle turns of the
new activity.

3. `activity_actor.cpp` Define the `start`, `do_turn`, and `finish`
functions needed for the new actor as well as the required serialization
functions. Don't forget to add the deserialization function of your new
activity actor to the `deserialize_functions` map towards the bottom of
`activity_actor.cpp`. Define `canceled` function if activity modifies
some complex state that should be restored upon cancellation / interruption.

4. If this activity is resumable, `override` 
`activity_actor::can_resume_with_internal`

5. Construct your activity actor and then pass it to the constructor for
`player_activity`. The newly constructed activity can then be assigned
to the character and started using `Character::assign_activity`.

## JSON Properties

* verb: A descriptive term to describe the activity to be used in the
query to stop the activity, and strings that describe it, for example:
`"verb": "mining"` or
`"verb": { "ctxt": "instrument", "str": "playing" }`.

* suspendable (true): If true, the activity can be continued without
starting from scratch again. This is only possible if `can_resume_with()`
returns true.

* rooted (false): If true, then during the activity, recoil is reduced,
and plant mutants sink their roots into the ground. Should be true if the
activity lasts longer than a few minutes, and can always be accomplished
without moving your feet.

* based_on: Can be 'time', 'speed', or 'neither'.

    * time: The amount that `player_activity::moves_left` is
    decremented by is independent from the character's speed.

    * speed: `player_activity::moves_left` may be decremented faster
    or slower, depending on the character's speed.

    * neither: `moves_left` will not be decremented. Thus you must
    define a do_turn function; otherwise the activity will never end!

* no_resume (false): Rather than resuming, you must always restart the
activity from scratch.

* multi_activity(false): This activity will repeat until it cannot do
any more work, used for NPC and avatar zone activities.

* refuel_fires( false ): If true, the character will automatically refuel
fires during the long activity.

* auto_needs( false ) : If true, the character will automatically eat and
drink from specific auto_consume zones during long activities.

## Termination

There are several ways an activity can be ended:

1. Call `player_activity::set_to_null()`

    This can be called if it finished early or something went wrong,
    such as corrupt data, disappearing items, etc. The activity will
    not be put into the backlog.

2. `moves_left` <= 0

    Once this condition is true, the finish function, if there is one,
    is called. The finish function should call `set_to_null()`. If
    there isn't a finish function, `set_to_null()` will be called
    for you (from activity_actor::do_turn).

3. `Character::cancel_activity`

    Canceling an activity prevents the `activity_actor::finish`
    function from running, and the activity does therefore not yield a
    result. Instead, `activity_actor::canceled` is called. If activity is
    suspendable, a copy of it is written to `Character::backlog`.

## Notes

While the character performs the activity,
`activity_actor::do_turn` is called on each turn. Depending on the
JSON properties, this will do some stuff. It will also call the do_turn
function, and if `moves_left` is non-positive, the finish function.

Some activities (like playing music on a mp3 player) don't have an end
result but instead do something each turn (playing music on the mp3
player decreases its batteries and gives a morale bonus).

If the activity needs any information during its execution or when
it's finished (like *where* to do something, *which* item to use to do
something, ...), simply add data members to your activity actor as well
as serialization functions for the data so that the activity can persist
across save/load cycles.

Be careful when storing coordinates as the activity may be carried out
by NPCS. If its, the coordinates must be absolute not local as local
coordinates are based on the avatars position.

### `activity_actor::start`

This function is called exactly once when the activity
is assigned to a character. It is useful for setting
`player_activity::moves_left`/`player_activity::moves_total` in the case
of an activity based on time or speed.

### `activity_actor::do_turn`

To prevent an infinite loop, ensure that one of the following is
satisfied:

- The `based_on` JSON property is `speed` or `time`

- The `player_activity::moves_left` is decreased in `do_turn`

- The the activity is stopped in `do_turn`  (see 'Termination' above)

For example, `move_items_activity_actor::do_turn` will either move as
many items as possible given the character's current moves or, if there
are no target items remaining, terminate the activity.

### `activity_actor::finish`

This function is called when the activity has been completed
(`moves_left` <= 0). It must call `player_activity::set_to_null()` or
assign a new activity. One should not call `Character::cancel_activity`
for the finished activity, as this could make a copy of the activity in
`Character::backlog`, which allows resuming an already finished activity.
