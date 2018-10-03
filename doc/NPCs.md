TODO: document the "npc" structure, used to load NPC template

# Writing dialogues
Dialogues work like state machines. They start with a certain topic (the NPC says something), the player character can respond (choosing one of several responses), and that response sets the new talk topic. This goes on until the dialogue is finished, or the NPC turns hostile.

Note that it is perfectly fine to have a response that switches the topic back to itself.

Two topics are special:
- "TALK_DONE" ends the dialogue immediately.
- "TALK_NONE" goes to the previously talked about topic.

## Talk topics

Each topic consists of:
1. a topic id (e.g. "TALK_ARSONIST")
2. a dynamic line, spoken by the NPC.
3. a list of responses that can be spoken by the player character.

One can specify new topics in json. It is currently not possible to define the starting topic, so you have to add a response to some of the default topics (e.g. "TALK_STRANGER_FRIENDLY" or "TALK_STRANGER_NEUTRAL") or to topics that can be reached somehow.

Format:
```JSON
{
    "type": "talk_topic",
    "id": "TALK_ARSONIST",
    "dynamic_line": "What now?",
    "responses": [
        {
            "text": "I don't know either",
            "topic": "TALK_DONE"
        }
    ],
    "replace_built_in_responses": true
}
```
### type
Must always be there and must always be "talk_topic".

### id
The topic id can be one of the built-in topics or a new id. However, if several talk topics *in json* have the same id, the last topic definition will override the previous ones.

The topic id can also be an array of strings. This is loaded as if several topics with the exact same content have been given in json, each associated with an id from the `id`, array. Note that loading from json will append responses and, if defined in json, override the `dynamic_line` and the `replace_built_in_responses` setting. This allows adding responses to several topics at once.

This example adds the "I'm going now!" response to all the listed topics.
```JSON
{
    "type": "talk_topic",
    "id": [ "TALK_ARSONIST", "TALK_STRANGER_FRIENDLY", "TALK_STRANGER_NEUTRAL" ],
    "dynamic_line": "What now?",
    "responses": [
        {
            "text": "I'm going now.",
            "topic": "TALK_DONE"
        }
    ]
}
```

### dynamic_line
The `dynamic_line` is the line spoken by the NPC. It is optional. If it is not defined and the topic has the same id as a built-in topic, the `dynamic_line` from that built-in topic will be used. Otherwise the NPC will say nothing. See the chapter about dynamic_line below for more details.

### response
The `responses` entry is an array with possible responses. It must not be empty. Each entry must be a response object. See the chapter about Responses below for more details.

### replace_built_in_responses
`replace_built_in_responses` is an optional boolean that defines whether to dismiss the built-in responses for that topic (default is `false`). If there are no built-in responses, this won't do anything. If `true`, the built-in responses are ignored and only those from this definition in the current json are used. If `false`, the responses from the current json are used along with the built-in responses (if any).

---

## dynamic_line
A dynamic line can either be a simple string, or an complex object, or an array with `dynamic_line` entries. If it's an array, an entry will be chosen randomly every time the NPC needs it. Each entry has the same probability.

Example:
```JSON
"dynamic_line": [
    "generic text",
    {
        "npc_female": [ "text1", "text2", "text3" ],
        "npc_male": { "u_female": "text a", "u_male": "text b" }
    }
]
```

A complex `dynamic_line` usually contains several `dynamic_line` entry and some condition that determines which is used.  If dynamic lines are not nested, they are processed in the order of the entries below.  The lines can be defined like this:

### Based on the gender of the NPC / NPC
The dynamic line is chosen based on the gender of the NPC, both entries must exists. Both entries are parsed as `dynamic_line`.

```JSON
{
    "npc_male": "I'm a man.",
    "npc_female": "I'm a woman."
}
```

### Based on the gender of the player character
The dynamic line is chosen based on the gender of the player character, both entries must exists. Both entries are parsed as `dynamic_line`. `u` is the player character.

```JSON
{
    "u_male": "You're a man.",
    "u_female": "You're a woman."
}
```

### Based on whether the player character is armed or unarmed
The dynamic line is chosen based on whether the character has a weapon in hand or not.  Both entries must exit.  Both entries are parsed as `dynamic_line`.  `u` is the player character.

```JSON
{
    "u_has_weapon": "Drop your weapon!",
    "u_unarmed": "Put your hands in air!"
}
```

### Based on items worn by the player character
The dynamic line is chosen based on whether the player character wears a specific item, both entries are optional, but you should make sure the NPC says something all the time. Both entries are parsed as `dynamic_line`. `u` is the player character. The `u_is_wearing` string should be a valid item id. The line from `yes` will be shown if the character wears the item, otherwise the line from `no`.

```JSON
{
    "u_is_wearing": "fur_cat_ears",
    "yes": "Hello, I like your ears.",
    "no": "Hello."
}
```

### Based on mutation (trait) possessed by the player character
The dynamic line is chosen based on whether the player character has any of an array of traits. Both entries are optional, but you should make sure the NPC says something all the time. Both entries are parsed as `dynamic_line`. `u` is the player character. The `u_has_any_trait` string should be one or more valid mutation IDs. The line from `yes` will be shown if the character has one of the traits, otherwise the line from `no`.

```JSON
{
    "u_has_any_trait": [ "CANINE_EARS", "LUPINE_EARS", "FELINE_EARS", "URSINE_EARS", "ELFA_EARS" ],
    "yes": "Hello, I like your ears.",
    "no": "Hello."
}
```

### Based on effect possessed by the NPC
The dynamic line is chosen based on whether the NPC is currently is under the efffect.  Both the yes and no entries are mandatory.  The line from `yes` will be shown in the NPC has the effect, otherwise the line from `no`.

```JSON
{
    "npc_has_effect": "infected",
    "yes": "I need antibiotics.",
    "no": "What's going on?"
}
```

### Based on whether the NPC has missions available
The dynamic line is chosen based on whether the NPC has any missions to give out.  All three entries are mandatory.  The line from `many` will be shown in the NPC has two or more missions to assign to the player, the line from `one` will be shown if the NPC has one mission available, and otherwise the line from `none` will be shown.

```JSON
{
    "npc_has_mission": true,
    "many": "There are lots of things you could do to help.",
    "one": "There's one thing you could do to help.",
    "none": "You've cleaned this place up!"
}
```

### Based on whether the player character is performing missions for the NPC
The dynamic line is chosen based on whether the player character is performing any missions for the NPC.  All entries are mandatory.  The line from `many` if the player character is performing two or more missions for the NPC, the line from `one` will be shown if the player is performing one mission, and otherwise the line from `none` will be shown.

```JSON
{
    "u_has_mission": true,
    "many": "You're doing so much for this town!",
    "one": "It looks like you have some spare time.",
    "none": "Since you're not doing anything important, I have a favor to ask."
}
```

### A randomly selected hint
The dynamic line will be randomly chosen from the hints snippets.

```JSON
{
    "give_hint": true
}

---

## Responses
A response contains at least a text, which is display to the user and "spoken" by the player character (its content has no meaning for the game) and a topic to which the dialogue will switch to. It can also have a trial object which can be used to either lie, persuade or intimidate the NPC, see `npctalk.cpp` for details. There can be different results, used either when the trial succeeds and when it fails.

Format:
```JSON
{
    "text": "I, the player, say to you...",
    "condition": "...something...",
    "trial": {
        "type": "PERSUADE",
        "difficulty": 10
    },
    "success": {
        "topic": "TALK_DONE",
        "effect": "...",
        "opinion": {
            "trust": 0,
            "fear": 0,
            "value": 0,
            "anger": 0,
            "owed": 0,
            "favors": 0
        }
    },
    "failure": {
        "topic": "TALK_DONE"
    }
}
```

Alternatively a short format:
```JSON
{
    "text": "I, the player, say to you...",
    "effect": "...",
    "topic": "TALK_WHATEVER"
}
```
The short format is equivalent to (an unconditional switching of the topic, `effect` is optional):
```JSON
{
    "text": "I, the player, say to you...",
    "trial": {
        "type": "NONE"
    },
    "success": {
        "effect": "...",
        "topic": "TALK_WHATEVER"
    }
}
```


### text
Will be shown to the user, no further meaning.

### trial
Optional, if not defined, "NONE" is used. Otherwise one of "NONE", "LIE", "PERSUADE" or "INTIMIDATE". If "NONE" is used, the `failure` object is not read, otherwise it's mandatory.
The `difficulty` is only required if type is not "NONE" and specifies the success chance in percent (it is however modified by various things like mutations).

### success and failure
Both objects have the same structure. `topic` defines which topic the dialogue will switch to. `opinion` is optional, if given it defines how the opinion of the NPC will change. The given values are *added* to the opinion of the NPC, they are all optional and default to 0. `effect` is a function that is executed after choosing the response, see below.

The opinion of the NPC affects several aspects of the interaction with NPCs:
- Higher trust makes it easier to lie and persuade, and it usually a good thing.
- Higher fear makes it easier to intimidate, but the NPC may flee from you (and will not talk to you).
- Higher value makes it easier to persuade them and to give them orders, it's a kind of a friendship indicator.
- High anger value (about 20 points more than fear, but this also depends on the NPCs personality) makes the NPC hostile and is usually a bad thing.
The combination of fear and trust decide together with the personality of the NPC the initial talk topic ("TALK_MUG", "TALK_STRANGER_AGGRESSIVE", "TALK_STRANGER_SCARED", "TALK_STRANGER_WARY", "TALK_STRANGER_FRIENDLY" or "TALK_STRANGER_NEUTRAL").

For the actual usage of that data, search the source code for "op_of_u".

The `failure` object is used if the trial fails, the `success` object is used otherwise.

`topic` can also be a single topic object (the `type` member is not required here):
```JSON
"success": {
    "topic": {
        "id": "TALK_NEXT",
        "dynamic_line": "...",
        "responses": [
        ]
    }
}
```

### condition
This is an optional condition which can be used to prevent the response under certain circumstances. If not defined, it defaults to always `true`. If the condition is not met, the response is not included in the list of possible responses. For possible content see below.

---

## response effect
The `effect` function ca be one the following:

### start_trade
Opens the trade screen and allows trading with the NPC.

### hostile
Make the NPC hostile and end the conversation.

### flee
Makes the NPC flee from your character.

### follow
Makes the NPC follow your character.

### leave
Makes the NPC not follow your character anymore.

### stop_guard
Releases the NPC from their guard duty (also see "assign_guard").

### assign_guard
Makes the NPC into a guard, which will defend the current location.

### end_conversation
Ends the conversation and makes the NPC ignore you from now on.

### insult_combat
Ends the conversation and makes the NPC hostile, adds a message that character starts a fight with the NPC.

### drop_weapon
Make the NPC drop their weapon.

### player_weapon_away
Makes the player character put away (unwield) their weapon.

### player_weapon_drop
Makes the player character drop their weapon.

---

## response conditions
Conditions can be a simple string, a key and an int, a key and a string, a key and an array, or a key and an object. Arrays and objects can nest with each other and can contain any other condition.

The following keys and simple strings are available:

### "and" (array)
`true` if every condition in the array is true. Can be used to create complex condition tests, like "[INTELLIGENCE 10+][PERCEPTION 12+] Your jacket is torn. Did you leave that scrap of fabric behind?"

### "or" (array)
`true` if every condition in the array is true. Can be used to create complex condition tests, like
"[STRENGTH 9+] or [DEXTERITY 9+] I'm sure I can handle one zombie."

### "not" (object)
`true` if the condition in the object or string is false. Can be used to create complex conditions test by negating other conditions, for text such as "[INTELLIGENCE 7-] Hitting the reactor with a hammer should shut it off safely, right?"

### "u_has_any_trait" (array)
`true` if the player character has any trait or mutation in the array. Used to check multiple traits.

### "u_has_strength" (int)
`true` if the player character's strength is at least the value of u_has_strength.

### "u_has_dexterity" (int)
`true` if the player character's dexterity is at least the value of u_has_dexterity.

### "u_has_intelligence" (int)
`true` if the player character's intelligence is at least the value of u_has_intelligence.

### "u_has_perception" (int)
`true` if the player character's perception is at least the value of u_has_perception.

### "u_is_wearing" (string)
`true` if the player character is wearing something with u_is_wearing's item_id.

### "npc_has_effect" (string)
`true` if the NPC is under the effect with npc_has_effect's effect_id.

### "u_has_effect" (string)
`true` if the player character is under the effect with u_has_effect's effect_id.

### "u_has_mission" (string)
`true` if the mission is assigned to the player character.

### "npc_service" (int)
`true` if the NPC does not have the "currently_busy" effect and the player character has at least
npc_service cash available.  Useful to check if the player character can hire an NPC to perform a task that would take time to complete.  Functionally, this is identical to "and": [ { "not": { "npc_has_effect": "currently_busy" } }, { "u_has_cash": service_cost } ]

### "u_has_cash" (int)
`true` if the player character has at least u_has_cash cash available.  Used to check if the PC can buy something.

### "has_assigned_mission" (simple string)
`true` if the player character has exactly one mission from the NPC. Can be used for texts like "About that job..."

### "has_many_assigned_missions" (simple string)
`true` if the player character has several mission from the NPC (more than one). Can be used for texts like "About one of those jobs..." and to switch to the "TALK_MISSION_LIST_ASSIGNED" topic.

### "has_no_available_mission" (simple string)
`true` if the NPC has no jobs available for the player character.

### "has_available_mission" (simple string)
`true` if the NPC has one job available for the player character.

### "has_many_available_missions" (simple string)
`true` if the NPC has several jobs available for the player character.

### "npc_available" (simple string)
`true` if the NPC does not have effect "currently_busy".

### "npc_following" (simple string)
`true` if the NPC is following the player character.

### "at_safe_space" (simple string)
`true` if the NPC's currrent overmap location passes the is_safe() test.

### "u_can_stow_weapon" (simple string)
`true` if the player character is wielding a weapon and has enough space to put it away.

### "u_has_weapon" (simple string)
`true` if the player character is wielding a weapon.

### "npc_has_weapon" (simple string)
`true` if the NPC is wielding a weapon.

#### Sample responses with conditions
```JSON
{
  "text": "Understood.  I'll get those antibiotics.",
  "topic": "TALK_NONE",
  "condition": { "npc_has_effect": "infected" }
},
{
  "text": "[INT 11] I'm sure I can organize salvage operations to increase the bounty scavengers bring in!",
  "topic": "TALK_EVAC_MERCHANT_NO",
  "condition": { "u_has_intelligence": 11 }
},
{
  "text": "[STR 11] I punch things in face real good!",
  "topic": "TALK_EVAC_MERCHANT_NO",
  "condition": { "and": [ { "not": { "u_has_intelligence": 7 } }, { "u_has_strength": 11 } ] }
},
{ "text": "[$2000, 1d] 10 logs", "topic": "TALK_DONE", "effect": "buy_10_logs", "condition": 
{ "npc_service": 2000 } },
{ "text": "Maybe later.", "topic": "TALK_RANCH_WOODCUTTER", "condition": "npc_available" },
{
  "text": "[$8] I'll take a beer",
  "topic": "TALK_DONE",
  "condition": { "u_has_cash": 800 },
  "effect": { "u_buy_item": "beer", "container": "bottle_glass", "count": 2, "cost": 800 }
},
{
  "text": "Okay.  Lead the way.",
  "topic": "TALK_DONE",
  "condition": { "not": "at_safe_space" },
  "effect": "lead_to_safety"
},
{
  "text": "About one of those missions...",
  "topic": "TALK_MISSION_LIST_ASSIGNED",
  "condition": { "and": [ "has_many_assigned_missions", { "u_is_wearing": "badge_marshal" } ] }
},
{
  "text": "[MISSION] The captain sent me to get a frequency list from you.",
  "topic": "TALK_OLD_GUARD_NEC_COMMO_FREQ",
  "condition": {
    "and": [
      { "u_is_wearing": "badge_marshal" },
      { "u_has_mission": "MISSION_OLD_GUARD_NEC_1" },
      { "not": { "u_has_effect": "has_og_comm_freq" } }
    ]
  }
}



```

