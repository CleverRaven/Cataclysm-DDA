# Creating missions

NPCs can assign missions to the player.  There is a fairly regular structure for this:

```JSON
  {
    "id": "MISSION_GET_BLACK_BOX_TRANSCRIPT",
    "type": "mission_definition",
    "name": "Retrieve Black Box Transcript",
    "goal": "MGOAL_FIND_ITEM",
    "difficulty": 2,
    "value": 150000,
    "item": "black_box_transcript",
    "start": {
       "effect": { "u_buy_item": "black_box" },
       "target_om_ter": { "om_ter": "lab", "reveal_rad": 3 }
    },
    "origins": [ "ORIGIN_SECONDARY" ],
    "followup": "MISSION_EXPLORE_SARCOPHAGUS",
    "dialogue": {
      "describe": "With the black box in hand, we need to find a lab.",
      "offer": "Thanks to your searching we've got the black box but now we need to have a look'n-side her.  Now, most buildings don't have power anymore but there are a few that might be of use.  Have you ever seen one of those science labs that have popped up in the middle of nowhere?  Them suckers have a glowing terminal out front so I know they have power somewhere inside'em.  If you can get inside and find a computer lab that still works you ought to be able to find out what's in the black box.",
      "accepted": "Fuck ya, America!",
      "rejected": "Do you have any better ideas?",
      "advice": "When I was play'n with the terminal for the one I ran into it kept asking for an ID card.  Finding one would be the first order of business.",
      "inquire": "How 'bout that black box?",
      "success": "America, fuck ya!  I was in the guard a few years back so I'm confident I can make heads-or-tails of these transmissions.",
      "success_lie": "What?!  I out'ta whip you're ass.",
      "failure": "Damn, I maybe we can find an egg-head to crack the terminal."
    }
  }
```

### type
Must always be there and must always be "mission_definition".

### id
The mission id is required, but for new missions, it can be arbitrary.  Convention is to start
it with "MISSION" and to use a fairly desriptive name.

### name
The name is also required, and the name is what is displayed in the 'm'issions menu, so please
make it descriptive.

### goal
Must be included, and must be one of these strings:
"MGOAL_GO_TO"             - Reach a specific overmap tile
"MGOAL_GO_TO_TYPE"        - Reach any instance of a specified overmap tile type
"MGOAL_COMPUTER_TOGGLE"   - Activating the correct terminal will complete the mission
"MGOAL_FIND_ITEM"         - Find 1 or more items of a given type
"MGOAL_FIND_ANY_ITEM"     - Find 1 or more items of a given type, tagged for this mission
"MGOAL_FIND_MONSTER"      - Find and retrieve a friendly monster
"MGOAL_FIND_NPC"          - Find a specific NPC
"MGOAL_RECRUIT_NPC"       - Recruit a specific NPC
"MGOAL_RECRUIT_NPC_CLASS" - Recruit an NPC of a specific class
"MGOAL_ASSASSINATE"       - Kill a specific NPC
"MGOAL_KILL_MONSTER"      - Kill a specific hostile monster
"MGOAL_KILL_MONSTER_TYPE" - Kill some number of a specific monster type
"MGOAL_KILL_MONSTER_SPEC" -  Kill some number of monsters from a specific species

Currently, only "MGOAL_FIND_ITEM", "MGOAL_KILL_MONSTER_TYPE", and "MGOAL_KILL_MONSTER_SPEC"
missions can be fully defined in JSON without requiring C++ code support.

### start
Optional field.  If it is present and a string, it must be name of a function in mission_start::
that takes a mission * and performs the start code for the mission.  A hardcoded function is
currently necessary to set up any mission other than MGOAL_FIND_ITEM", "MGOAL_KILL_MONSTER_TYPE",
or "MGOAL_KILL_MONSTER_SPEC".

Alternately, if present, it can be an object with any of the following optional fields:

#### effects
This is an effects array, exactly as defined in NPCs.md, and can use any of the values from
effects.  In all cases, the NPC involved is the quest giver.

#### reveal_om_ter
This can be a string or a list of strings, all of which must be overmap terrain ids.  A randomly
selected overmap terrain tile with that id - or one of the ids from list, randomly selected -
will be revealed, and there is a 1 in 3 chance that the road route to the map tile will also
be revealed.

#### target_om_ter or target_om_ter_random or target_om_ter_random_or_create
Each of these is an object, with a mandatory field of "om_ter" that must be an overmap terrain
id.  "target_om_ter_random_or_create" also has mandatory fields of "om_spec" and "om_replace_ter",
which must be an overmap special id and an overmap terrain id, respectively.

All three also have the optional fields of "reveal_rad" which is an number, and "must_see"
which is a boolean.  "target_om_ter" has a third optional field of "zlevel", which again is a
number, while the other two have "range", which is also a number.

"target_om_ter" sets the mission target to be the closet terrain of om_ter, optionally the closest
that the player has already seen if "must_see" is true.  If "reveal_rad" is 1 or more, it also
reveals the overmap around the target for reveal_rad tiles.  The terrain can be at a different
z-level by passing a non-zero value into "zlevel".

"target_om_ter_random" finds all om_ter terrains in "range", defaulting to 5, and sets and
reveals the target to a randomly chosen terrain.  "must_see" and "reveal_rad" have the same
meaning.

"target_om_ter_random_or_create" works like "target_om_ter_random", except that if there are
no instances of "om_ter" in range, it creates an instance of "om_spec" on some unseen territory
of type "om_replace_ter" and then reveals and targets that.  "om_spec" must include "om_ter".

### monster_species
For "MGOAL_KILL_MONSTER_SPEC", sets the target monster species.

### monster_type
For "MGOAL_KILL_MONSTER_TYPE", sets the target monster type.

### monster_kill_goal
For "MGOAL_KILL_MONSTER_SPEC" and "MGOAL_KILL_MONSTER_TYPE", sets the number of monsters above
the player's current kill count that must be killed to complete the mission.

### dialogue
This is a dictionary of strings.  The NPC says these exact strings in response to the player
inquiring about the mission or reporting its completion.

## Adding new missions to NPC dialogue
Any NPC that has missions needs to either:
* Add one of their talk_topic IDs to the list of generic mission reponse IDs in the first
talk_topic of data/json/npcs/TALK_COMMON_MISSION.json
* Have a similar talk_topic with responses that lead to TALK_MISSION_INQUIRE and
TALK_MISSION_LIST_ASSIGNED.
