# Creating missions

NPCs can assign missions to the player.  There is a fairly regular structure for this:

```jsonc
  {
    "id": "MISSION_GET_BLACK_BOX_TRANSCRIPT",
    "type": "mission_definition",
    "name": "Retrieve Black Box Transcript",
    "description": "Decrypt the contents of the black box using a terminal from a nearby lab.",
    "goal": "MGOAL_FIND_ITEM",
    "deadline": [ "16 hours", "math": [ "time(' 16 h') * 2" ] ],
    "difficulty": 2,
    "value": 150000,
    "item": "black_box_transcript",
    "invisible_on_complete": false,
    "start": {
       "effect": { "u_buy_item": "black_box" },
       "assign_mission_target": { "om_terrain": "lab", "reveal_radius": 3 }
    },
    "urgent": false,
    "has_generic_rewards": true,
    "item": "pencil",
    "item_group": "pencil_box_with_pencil",
    "count": 6,
    "required_container": "pencil_box",
    "remove_container": true,
    "empty_container": "can_drink",
    "origins": [ "ORIGIN_SECONDARY" ],
    "followup": "MISSION_EXPLORE_SARCOPHAGUS",
    "dialogue": {
      "describe": "With the black box in hand, we need to find a lab.",
      "offer": "Thanks to your searching we've got the black box but now we need to have a look inside her.  Now, most buildings don't have power anymore but there are a few that might be of use.  Have you ever seen one of those science labs that have popped up in the middle of nowhere?  Them suckers have a glowing terminal out front so I know they have power somewhere inside 'em.  If you can get inside and find a computer lab that still works you ought to be able to find out what's in the black box.",
      "accepted": "Fuck yeah, America!",
      "rejected": "Do you have any better ideas?",
      "advice": "When I was playin' with the terminal for the one I ran into it kept asking for an ID card.  Finding one would be the first order of business.",
      "inquire": "How 'bout that black box?",
      "success": "America, fuck yeah!  I was in the guard a few years back so I'm confident I can make heads-or-tails of these transmissions.",
      "success_lie": "What?!  I oughta whip your ass.",
      "failure": "Damn, I maybe we can find an egg-head to crack the terminal."
    }
  }
```

### type
Must always be there and must always be "mission_definition".

### id
The mission id is required, but for new missions, it can be arbitrary.  Convention is to start
it with "MISSION" and to use a fairly descriptive name.

### name
The name is also required, and is displayed to the user in the 'm'issions menu.

### deadline
How long after being assigned this mission before it will automatically fail (if not already completed). Can be a pair of values, in which case a random value between the two is picked. If only a single value is given, always uses that value.

Supports variable objects and math expressions.

### description
Not required, but it's strongly recommended that you summarize all relevant info for the mission.
You may refer to mission end effects of the "u_buy_item" type, as long as they do not come at a
cost to the player. See the example below:
```jsonc
    "id": "MISSION_EXAMPLE_TOKENS",
    "type": "mission_definition",
    "name": "Murder Money",
    "description": "Whack the target in exchange for <reward_item:FMCNote> c-notes and <reward_item:cig> cigarettes.",
    "goal": "MGOAL_ASSASSINATE",
    "end": {
      "effect": [
        { "u_buy_item": "FMCNote", "count": 999 },
        { "u_buy_item": "cig", "count": 666 } ]
    }
```
This system may be expanded in the future to allow referring to other mission parameters and effects.

### urgent
If true, the NPC giving this mission will refuse to speak on any other topics besides completing this mission while it is active.

### goal
Must be included, and must be one of these strings:

Goal string               | Goal conditions
---                       | ---
`MGOAL_GO_TO`             | Reach a specific overmap tile
`MGOAL_GO_TO_TYPE`        | Reach any instance of a specified overmap tile type
`MGOAL_COMPUTER_TOGGLE`   | Activating the correct terminal will complete the mission
`MGOAL_FIND_ITEM`         | Find 1 or more items of a given type
`MGOAL_FIND_ANY_ITEM`     | Find 1 or more items of a given type, tagged for this mission
`MGOAL_FIND_MONSTER`      | Find and retrieve a friendly monster
`MGOAL_FIND_NPC`          | Find a specific NPC
`MGOAL_TALK_TO_NPC`       | Talk to a specific NPC
`MGOAL_RECRUIT_NPC`       | Recruit a specific NPC
`MGOAL_RECRUIT_NPC_CLASS` | Recruit an NPC of a specific class
`MGOAL_ASSASSINATE`       | Kill a specific NPC
`MGOAL_KILL_MONSTER`      | Kill a specific hostile monster
`MGOAL_KILL_MONSTERS`     | Kill a number of specific hostile monsters
`MGOAL_KILL_MONSTER_TYPE` | Kill some number of a specific monster type
`MGOAL_KILL_MONSTER_SPEC` | Kill some number of monsters from a specific species
`MGOAL_CONDITION`         | Satisfy the dynamically created condition and talk to the mission giver

Missions with goals `MGOAL_GO_TO` and `MGOAL_GO_TO_TYPE` behave differently depending on whether the player gets them from an NPC or from another source (e.g. from a starting scenario):
* When given by an NPC the mission is an escort quest: to complete it the player has to talk to the quest giver while standing at the destination. Note: make sure to set the quest giver to follow the player or the mission will be impossible to complete.
* Otherwise the mission is simple traversal - to complete it the player only has to reach the destination

### monster_species
For "MGOAL_KILL_MONSTER_SPEC", sets the target monster species.

### monster_type
For "MGOAL_KILL_MONSTER_TYPE", sets the target monster type.

### monster_kill_goal
For "MGOAL_KILL_MONSTER_SPEC" and "MGOAL_KILL_MONSTER_TYPE", sets the number of monsters above
the player's current kill count that must be killed to complete the mission.

### goal_condition
For "MGOAL_CONDITION", defines the condition that must be satisfied for the mission to be considered complete.
Conditions are explained in more detail in [NPCs.md](./NPCs.md), and are used here in exactly the same way.

### invisible_on_complete
Optional bool, defaults to false.  If true when this mission is finished it won't show up on the missions completed or missions failed lists.

### dialogue
This is a dictionary of strings.  The NPC says these exact strings in response to the player
inquiring about the mission or reporting its completion.  All these strings are required, even if they may not be used in the mission.

String ID            | Usage
---                  | ---
`describe`           | The NPC's overall description of the mission
`offer`              | The specifics of the mission given when the player selects that mission for consideration
`accepted`           | The NPC's response if the player accepts the mission
`rejected`           | The NPC's response if the player refuses the mission
`advice`             | If the player asks for advice on how to complete the mission, they hear this
`inquire`            | This is used if the NPC asks the player how the mission is going
`success`            | The NPC's response to a report that the mission was successful
`success_lie`        | The NPC's response if they catch the player lying about a mission's success
`failure`            | The NPC's response if the player reports a failed mission

### start
Optional field.  If it is present and a string, it must be name of a function in mission_start::
that takes a mission * and performs the start code for the mission.  This allows missions other
than the standard mission types to be run.  A hardcoded function is currently necessary to set
up missions with "MGOAL_COMPUTER_TOGGLE".

Alternately, if present, it can be an object as described below.

### start / end / fail effects
If any of these optional fields are present they can be objects with the following fields contained:

### origin
This determines how the player can be given this mission. There are a number of different options for this as follows.

String ID             | Usage
---                   | ---
`ORIGIN_GAME_START`   | Given when the game starts
`ORIGIN_OPENER_NPC`   | NPC comes up to you when the game starts
`ORIGIN_ANY_NPC`      | Any NPC
`ORIGIN_SECONDARY`    | Given at the end of another mission
`ORIGIN_COMPUTER`     | Taken after reading investigation provoking entries in computer terminal

#### effect
This is an effects array, exactly as defined in [NPCs.md](./NPCs.md), and can use any of the values from
effects.  In all cases, the NPC involved is the quest giver.

#### reveal_om_ter
This can be a string or a list of strings, all of which must be overmap terrain ids.  A randomly
selected overmap terrain tile with that id - or one of the ids from list, randomly selected -
will be revealed, and there is a 1 in 3 chance that the road route to the map tile will also
be revealed.

#### assign_mission_target

The `assign_mission_target` object specifies the criteria for finding (or creating if
necessary) a particular overmap terrain and designating it as the mission target. Its parameters
allow control over how it is picked and how some effects (such as revealing the surrounding area)
are applied afterwards. The `om_terrain` is the only required field.

Identifier             | Description
---                    | ---
`om_terrain`           | ID of overmap terrain which will be selected as the target. Mandatory.  String or or [variable object](#variable-object)
`om_terrain_match_type`| Matching rule to use with `om_terrain`. Defaults to TYPE. Details below.
`om_special`           | ID of overmap special containing the overmap terrain.  String or or [variable object](#variable-object)
`om_terrain_replace`   | ID of overmap terrain to be found and replaced if `om_terrain` cannot be found. String or or [variable object](#variable-object)
`reveal_radius`        | Radius in overmap terrain coordinates to reveal.  Int or or [variable object](#variable-object)
`must_see`             | If true, the `om_terrain` must have been seen already.
`cant_see`             | If true, the `om_terrain` must not have been seen already.
`random`               | If true, a random matching `om_terrain` is used. If false, the closest is used.
`search_range`         | Maximum range in overmap terrain coordinates to look for a matching `om_terrain`.  Int or or [variable object](#variable-object). Default 2520. Should only be used to limit maximum range when multiple valid destinations are possible, and only works to do that when random is also set to true.
`min_distance`         | Range in overmap terrain coordinates.  Instances of `om_terrain` in this range will be ignored.  Int or or [variable object](#variable-object)
`origin_npc`           | Start the search at the NPC's, rather than the player's, current position.
`z`                    | If specified, will be used rather than the player or NPC's z when searching.  Int or or [variable object](#variable-object)
`var`                  | A variable_object ( see `variable_object` in [doc](NPCs.md#variable-object) ), if set this variable's value will be used.
`offset_x`,<br\>`offset_y`,<br\>`offset_z` | After finding or creating `om_terrain`, offset the mission target terrain by the offsets in overmap terrain coordinates.

**example**
```jsonc
{
  "assign_mission_target": {
    "om_terrain": "necropolis_c_44",
    "om_special": "Necropolis",
    "reveal_radius": 1,
    "must_see": false,
    "random": true,
    "search_range": 180,
    "z": -2
  }
}
```

If the `om_terrain` is part of an overmap special, it's essential to specify the `om_special`
value as well--otherwise, the game will not know how to spawn the entire special. If a multitile `om_special` is used it is important to specify the exact `om_terrain` that you would like the target to appear in.

`om_terrain_match_type` defaults to TYPE if unspecified, and has the following possible values:

* `EXACT` - The provided string must completely match the overmap terrain id,
  including linear direction suffixes for linear terrain types or rotation
  suffixes for rotated terrain types.

* `TYPE` - The provided string must completely match the base type id of the
  overmap terrain id, which means that suffixes for rotation and linear terrain
  types are ignored.

* `PREFIX` - The provided string must be a complete prefix (with additional
  parts delimited by an underscore) of the overmap terrain id. For example,
  "forest" will match "forest" or "forest_thick" but not "forestcabin".

* `CONTAINS` - The provided string must be contained within the overmap terrain
  id, but may occur at the beginning, end, or middle and does not have any rules
  about underscore delimiting.

If an `om_special` must be placed, it will follow the same placement rules as defined in its
overmap special definition, respecting allowed terrains, distance from cities, road connections,
and so on. Consequently, the more restrictive the rules, the less likely this placement will
succeed (as it is competing for space with already-spawned specials).

`om_terrain_replace` is only relevant if the `om_terrain` is not part of an overmap special.
This value is used if the `om_terrain` cannot be found, and will be used as an alternative target
which will then be replaced with the `om_terrain` value.

If `must_see` is set to true, the game will not create the terrain if it can't be found. It tries
to avoid having new terrains magically appear in areas where the player has already been, and this
is a consequence.

`reveal_radius`, `min_distance`, and `search_range` are all in overmap terrain coordinates (an
overmap is currently 180 x 180 OMT units).  The search is centered on the player unless
`origin_npc` is set, in which case it centered on the NPC.  Currently, there is rarely a
difference between the two, but it is possible to assign missions over a radio.  The search gives
preference to existing overmaps (and will only spawn new overmaps if the terrain can't be found
on existing overmaps), and only executes on the player's current z-level. The `z` attribute can
be used to override this and search on a different z-level than the player--this is an absolute
value rather than relative.

`offset_x`, `offset_y`, and `offset_z` change the final location of the mission target by their
values.  This can change the mission target's overmap terrain type away from `om_terrain`.


#### Variable Object
Can be several differnet types of thing.  See the [NPCS](NPCS.md) document, section `Variable Object` for full details.

#### update_mapgen
The `update_mapgen` object or array provides a way to modify existing overmap tiles (including the ones created by "assign_mission_target") to add mission specific monsters, NPCs, computers, or items.

As an array, `update_mapgen` consists of two or more `update_mapgen` objects.

As an object, `update_mapgen` contains any valid JSON mapgen objects.  The objects are placed on the mission target terrain from "assign_mission_target" or optionally the closest overmap terrain specified by the `om_terrain` and `om_special` fields.  If "mapgen_update_id" is specified, the "mapge_update" object with the matching "mapgen_update_id" will be executed.

See [MAPGEN.md](MAPGEN.md) for more details on JSON mapgen and `update_mapgen`.

An NPC, monster, or computer placed using `update_mapgen` will be the target of a mission if it has the `target` boolean set to `true` in its `place` object in `update_mapgen`.

## Adding new missions to NPC dialogue
In order to assign missions to NPCs, the first step is to find that NPC's definition.  For unique NPCs this is usually at the top of the npc's JSON file and looks something like this:
```jsonc
{
  "type": "npc",
  "id": "refugee_beggar2",
  "//": "Schizophrenic beggar in the refugee center.",
  "name_unique": "Dino Dave",
  "gender": "male",
  "name_suffix": "beggar",
  "class": "NC_BEGGAR_2",
  "attitude": 0,
  "mission": 7,
  "chat": "TALK_REFUGEE_BEGGAR_2",
  "faction": "lobby_beggars"
},
```
Add a new line that defines the NPC's starting mission, eg:
```
"mission_offered": "MISSION_BEGGAR_2_BOX_SMALL"
```

Any NPC that has missions needs to have a dialogue option that leads to TALK_MISSION_LIST, to get the player
started on their first mission for the NPC, and either:

* Add one of their talk_topic IDs to the list of generic mission response IDs in the first
talk_topic of data/json/npcs/TALK_COMMON_MISSION.json, or
* Have a similar talk_topic with responses that lead to TALK_MISSION_INQUIRE and
TALK_MISSION_LIST_ASSIGNED.

Either of these options will allow the player to do normal mission management dialogue with the NPC.

This is an example of how a custom mission inquiry might appear.  This will only appear in the NPC's dialogue
options if the player has already been assigned a mission.
```jsonc
{
  "type": "talk_topic",
  "//": "Generic responses for Old Guard Necropolis NPCs that can have missions",
  "id": [ "TALK_OLD_GUARD_NEC_CPT", "TALK_OLD_GUARD_NEC_COMMO" ],
  "responses": [
    {
      "text": "About the mission...",
      "topic": "TALK_MISSION_INQUIRE",
      "condition": { "and": [ "has_assigned_mission", { "u_is_wearing": "badge_marshal" } ] }
    },
    {
      "text": "About one of those missions...",
      "topic": "TALK_MISSION_LIST_ASSIGNED",
      "condition": { "and": [ "has_many_assigned_missions", { "u_is_wearing": "badge_marshal" } ] }
    }
  ]
},
```
