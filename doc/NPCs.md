TODO: document the "npc" structure, used to load NPC template

# Writing dialogues
Dialogues work like state machines. They start with a certain topic (the NPC says something), the player character can respond (choosing one of several responses), and that response sets the new talk topic. This goes on until the dialogue is finished, or the NPC turns hostile.

Note that it is perfectly fine to have a response that switches the topic back to itself.

NPC missions are controlled by a separate but related JSON structure and are documented in
[the missions docs](MISSIONS_JSON.md).

Two topics are special:
- `TALK_DONE` ends the dialogue immediately.
- `TALK_NONE` goes to the previously talked about topic.


### Validating Dialogues
Keeping track of talk topics and making sure that all the topics referenced in responses are
defined, and all defined topics are referenced in a response or an NPC's chat, is very tricky.
There is a python script in `tools/dialogue_validator.py` that will map all topics to responses
and vice versa.  Invoke it with
```sh
python tools/dialogue_validator.py data/json/npcs/* data/json/npcs/Backgrounds/* data/json/npcs/refugee_center/*
```

If you are writing a mod with dialogue, you can add the paths to the mod's dialogue files.

## Talk topics

Each topic consists of:
1. a topic id (e.g. `TALK_ARSONIST`)
2. a dynamic line, spoken by the NPC.
3. an optional list of effects that occur when the NPC speaks the dynamic line
4. a list of responses that can be spoken by the player character.
5. a list of repeated responses that can be spoken by the player character, automatically generated if the player or NPC has items in a list of items.

One can specify new topics in json. It is currently not possible to define the starting topic, so you have to add a response to some of the default topics (e.g. `TALK_STRANGER_FRIENDLY` or `TALK_STRANGER_NEUTRAL`) or to topics that can be reached somehow.

Format:
```json
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
Must always be there and must always be `"talk_topic"`.

### id
The topic id can be one of the built-in topics or a new id. However, if several talk topics *in json* have the same id, the last topic definition will override the previous ones.

The topic id can also be an array of strings. This is loaded as if several topics with the exact same content have been given in json, each associated with an id from the `id`, array. Note that loading from json will append responses and, if defined in json, override the `dynamic_line` and the `replace_built_in_responses` setting. This allows adding responses to several topics at once.

This example adds the "I'm going now!" response to all the listed topics.
```C++
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
The `dynamic_line` is the line spoken by the NPC.  It is optional.  If it is not defined and the topic has the same id as a built-in topic, the `dynamic_line` from that built-in topic will be used.  Otherwise the NPC will say nothing.  See the chapter about `dynamic_line` below for more details.

### speaker_effect
The `speaker_effect` is an object or array of effects that will occur after the NPC speaks the `dynamic_line`, no matter which response the player chooses.  See the chapter about Speaker Effects below for more details.

### response
The `responses` entry is an array with possible responses.  It must not be empty.  Each entry must be a response object. See the chapter about Responses below for more details.

### replace_built_in_responses
`replace_built_in_responses` is an optional boolean that defines whether to dismiss the built-in responses for that topic (default is `false`). If there are no built-in responses, this won't do anything. If `true`, the built-in responses are ignored and only those from this definition in the current json are used. If `false`, the responses from the current json are used along with the built-in responses (if any).

---

## dynamic_line
A dynamic line can either be a simple string, or an complex object, or an array with `dynamic_line` entries.  If it's an array, an entry will be chosen randomly every time the NPC needs it.  Each entry has the same probability.

Example:
```json
"dynamic_line": [
    "generic text",
    {
        "npc_female": [ "text1", "text2", "text3" ],
        "npc_male": { "u_female": "text a", "u_male": "text b" }
    }
]
```

A complex `dynamic_line` usually contains several `dynamic_line` entry and some condition that determines which is used.  If dynamic lines are not nested, they are processed in the order of the entries below.  The possible types of lines follow.

In all cases, `npc_` refers to the NPC, and `u_` refers to the player.  Optional lines do not have to be defined, but the NPC should always have something to say.  Entries are always parsed as `dynamic_line` and can be nested.

#### Several lines joined together
The dynamic line is a list of dynamic lines, all of which are displayed.  The dynamic lines in the list are processed normally.
```json
{
    "and": [
        {
            "npc_male": true,
            "yes": "I'm a man.",
            "no": "I'm a woman."
        },
        "  ",
        {
            "u_female": true,
            "no": "You're a man.",
            "yes": "You're a woman."
        }
    ]
}
```

#### A randomly selected hint
The dynamic line will be randomly chosen from the hints snippets.

```json
{
    "give_hint": true
}
```

#### Based on a previously generated reason
The dynamic line will be chosen from a reason generated by an earlier effect.  The reason will be cleared.  Use of it should be gated on the `"has_reason"` condition.

```json
{
    "has_reason": { "use_reason": true },
    "no": "What is it, boss?"
}
```

#### Based on any Dialogue condition
The dynamic line will be chosen based on whether a single dialogue condition is true or false.  Dialogue conditions cannot be chained via `"and"`, `"or"`, or `"not"`.  If the condition is true, the `"yes"` response will be chosen and otherwise the `"no"` response will be chosen.  Both the `'"yes"` and `"no"` reponses are optional.  Simple string conditions may be followed by `"true"` to make them fields in the dynamic line dictionary, or they can be followed by the response that will be chosen if the condition is true and the `"yes"` response can be omitted.

```json
{
    "npc_need": "fatigue",
    "level": "TIRED",
    "no": "Just few minutes more...",
    "yes": "Make it quick, I want to go back to sleep."
}
{
    "npc_aim_rule": "AIM_PRECISE",
    "no": "*will not bother to aim at all.",
    "yes": "*will take time and aim carefully."
}
{
    "u_has_item": "india_pale_ale",
    "yes": "<noticedbooze>",
    "no": "<neutralchitchat>"
}
{
    "days_since_cataclysm": 30,
    "yes": "Now, we've got a moment, I was just thinking it's been a month or so since... since all this, how are you coping with it all?",
    "no": "<neutralchitchat>"
}
{
    "is_day": "Sure is bright out.",
    "no": {
        "u_male": true,
        "yes": "Want a beer?",
        "no": "Want a cocktail?"
    }
}
```
---

## Speaker Effects
The `speaker_effect` entry contains dialogue effects that occur after the NPC speaks the `dynamic_line` but before the player responds and regardless of the player response.  Each effect can have an optional condition, and will only be applied if the condition is true.  Each `speaker_effect` can also have an optional `sentinel`, which guarantees the effect will only run once.

Format:
```json
"speaker_effect": {
  "sentinel": "...",
  "condition": "...",
  "effect": "..."
}
```
or:
```json
"speaker_effect": [
  {
    "sentinel": "...",
    "condition": "...",
    "effect": "..."
  },
  {
    "sentinel": "...",
    "condition": "...",
    "effect": "..."
  }
]
```

The `sentinel` can be any string, but sentinels are unique to each `TALK_TOPIC`.  If there are multiple `speaker_effect`s within the `TALK_TOPIC`, they should have different sentinels.  Sentinels are not required, but since the `speaker_effect` will run every time the dialogue returns to the `TALK_TOPIC`, they are highly encouraged to avoid inadvertantly repeating the same effects.

The `effect` can be any legal effect, as described below.  The effect can be a simple string, object, or an array of strings and objects, as normal for objects.

The optional `condition` can be any legal condition, as described below.  If a `condition` is present, the `effect` will only occur if the `condition` is true.

Speaker effects are useful for setting status variables to indicate that player has talked to the NPC without complicating the responses with multiple effect variables.  They can also be used, with a sentinel, to run a `mapgen_update` effect the first time the player hears some dialogue from the NPC.

---
## Responses
A response contains at least a text, which is display to the user and "spoken" by the player character (its content has no meaning for the game) and a topic to which the dialogue will switch to. It can also have a trial object which can be used to either lie, persuade or intimidate the NPC, see below for details. There can be different results, used either when the trial succeeds and when it fails.

Format:
```json
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
```json
{
    "text": "I, the player, say to you...",
    "effect": "...",
    "topic": "TALK_WHATEVER"
}
```
The short format is equivalent to (an unconditional switching of the topic, `effect` is optional):
```json
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

The optional boolean keys "switch" and "default" are false by default.  Only the first response with `"switch": true`, `"default": false`, and a valid condition will be displayed, and no other responses with `"switch": true` will be displayed.  If no responses with `"switch": true` and `"default":  false` are displayed, then any and all responses with `"switch": true` and `"default": true` will be displayed.  In either case, all responses that have `"switch": false` (whether or not they have `"default": true` is set) will be displayed as long their conditions are satisfied.

#### switch and default Example
```json
"responses": [
  { "text": "You know what, never mind.", "topic": "TALK_NONE" },
  { "text": "How does 5 Ben Franklins sound?",
    "topic": "TALK_BIG_BRIBE", "condition": { "u_has_cash": 500 }, "switch": true },
   { "text": "I could give you a big Grant.",
    "topic": "TALK_BRIBE", "condition": { "u_has_cash": 50 }, "switch": true },
  { "text": "Lincoln liberated the slaves, what can he do for me?",
    "topic": "TALK_TINY_BRIBE", "condition": { "u_has_cash": 5 }, "switch": true, "default": true },
  { "text": "Maybe we can work something else out?", "topic": "TALK_BRIBE_OTHER",
    "switch": true, "default": true },
  { "text": "Gotta go!", "topic": "TALK_DONE" }
]
```
The player will always have the option to return to a previous topic or end the conversation, and
will otherwise have the option to give a $500, $50, or $5 bribe if they have the funds.  If they
don't have at least $50, they will also have the option to provide some other bribe.

### truefalsetext
The player will have one response text if a condition is true, and another if it is false, but the same trial for either line.  `condition`, `true`, and `false` are all mandatory.

```json
{
    "truefalsetext": {
        "condition": { "u_has_cash": 800 },
        "true": "I may have the money, I'm not giving you any.",
        "false": "I don't have that money."
    },
    "topic": "TALK_WONT_PAY"
}
```

### text
Will be shown to the user, no further meaning.

### trial
Optional, if not defined, `"NONE"` is used. Otherwise one of `"NONE"`, `"LIE"`, `"PERSUADE"`, `"INTIMIDATE"`, or `"CONDITION"`. If `"NONE"` is used, the `failure` object is not read, otherwise it's mandatory.

The `difficulty` is only required if type is not `"NONE"` or `"CONDITION"` and specifies the success chance in percent (it is however modified by various things like mutations).  Higher difficulties are easier to pass.

An optional `mod` array takes any of the following modifiers and increases the difficulty by the NPC's opinion of your character or personality trait for that modifier multiplied by the value: `"ANGER"`, `"FEAR"`, `"TRUST"`, `"VALUE"`, `"AGRESSION"`, `"ALTRUISM"`, `"BRAVERY"`, `"COLLECTOR"`. The special `"POS_FEAR"` modifier treats NPC's fear of your character below 0 as though it were 0.  The special `"TOTAL"` modifier sums all previous modifiers and then multiplies the result by its value and is used when setting the owed value.

`"CONDITION"` trials take a mandatory `condition` instead of `difficulty`.  The `success` object is chosen if the `condition` is true and the `failure` is chosen otherwise.

### success and failure
Both objects have the same structure. `topic` defines which topic the dialogue will switch to. `opinion` is optional, if given it defines how the opinion of the NPC will change. The given values are *added* to the opinion of the NPC, they are all optional and default to 0. `effect` is a function that is executed after choosing the response, see below.

The opinion of the NPC affects several aspects of the interaction with NPCs:
- Higher trust makes it easier to lie and persuade, and it usually a good thing.
- Higher fear makes it easier to intimidate, but the NPC may flee from you (and will not talk to you).
- Higher value makes it easier to persuade them and to give them orders, it's a kind of a friendship indicator.
- High anger value (about 20 points more than fear, but this also depends on the NPCs personality) makes the NPC hostile and is usually a bad thing.
The combination of fear and trust decide together with the personality of the NPC the initial talk topic (`"TALK_MUG"`, `"TALK_STRANGER_AGGRESSIVE"`, `"TALK_STRANGER_SCARED"`, `"TALK_STRANGER_WARY"`, `"TALK_STRANGER_FRIENDLY"`, or `"TALK_STRANGER_NEUTRAL"`).

For the actual usage of that data, search the source code for `"op_of_u"`.

The `failure` object is used if the trial fails, the `success` object is used otherwise.

### Sample trials
```josn
"trial": { "type": "PERSUADE", "difficulty": 0, "mod": [ [ "TRUST", 3 ], [ "VALUE", 3 ], [ "ANGER", -3 ] ] }
"trial": { "type": "INTIMIDATE", "difficulty": 20, "mod": [ [ "FEAR", 8 ], [ "VALUE", 2 ], [ "TRUST", 2 ], [ "BRAVERY", -2 ] ] }
"trial": { "type": "CONDITION", "condition": { "npc_has_trait": "FARMER" } }
```

`topic` can also be a single topic object (the `type` member is not required here):

```json
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
This is an optional condition which can be used to prevent the response under certain circumstances. If not defined, it defaults to always `true`. If the condition is not met, the response is not included in the list of possible responses. For possible content see Dialogue Conditions below.

---

## Repeat Responses
Repeat responses are responses that should be added to the response list multiple times, once for each instance of an item.

A repeat response has the following format:
```json
{
  "for_item": [
    "jerky", "meat_smoked", "fish_smoked", "cooking_oil", "cooking_oil2", "cornmeal", "flour",
    "fruit_wine", "beer", "sugar"
  ],
  "response": { "text": "Delivering <topic_item>.", "topic": "TALK_DELIVER_ASK" }
}
```

`"response"` is mandatory and must be a standard dialogue response, as described above.  `"switch"` is allowed in repeat responses and works normally.

One of `"for_item"` or `"for_category"`, and each can either be a single string or list of items or item categories.  The `response` is generated for each item in the list in the player or NPC's inventory.

`"is_npc"` is an optioanl bool value, and if it is present, the NPC's inventory list is checked.  By default, the player's inventory list is checked.

`"include_containers"` is an optional bool value, and if it is present, items containing an item will generate seperate responses from the item itself.

---

## Dialogue Effects
The `effect` field of `speaker_effect` or a `response` can be any of the following effects. Multiple effects should be arranged in a list and are processed in the order listed.

#### Missions

Effect | Description
---|---
`assign_mission` | Assigns a previously selected mission to your character.
`mission_success` | Resolves the current mission successfully.
`mission_failure` | Resolves the current mission as a failure.
`clear_mission` | Clears the mission from the your character's assigned missions.
`mission_reward` | Gives the player the mission's reward.

#### Stats / Morale

Effect | Description
---|---
`give_aid` | Removes all bites, infection, and bleeding from your character's body and heals 10-25 HP of injury on each of your character's body parts.
`give_aid_all` | Performs `give_aid` on each of your character's NPC allies in range.
`buy_haircut` | Gives your character a haircut morale boost for 12 hours.
`buy_shave` | Gives your character a shave morale boost for 6 hours.
`morale_chat` | Gives your character a pleasant conversation morale boost for 6 hours.
`player_weapon_away` | Makes your character put away (unwield) their weapon.
`player_weapon_drop` | Makes your character drop their weapon.

#### Character effects / Mutations

Effect | Description
---|---
`u_add_effect: effect_string`, (*one of* `duration: duration_string`, `duration: duration_int`)<br/>`npc_add_effect: effect_string`, (*one of* `duration: duration_string`, `duration: duration_int`) | Your character or the NPC will gain the effect for `duration_string` or `duration_int` turns.  If `duration_string` is `"PERMANENT"`, the effect will be added permanently.
`u_add_trait: trait_string`<br/>`npc_add_trait: trait_string` | Your character or the NPC will gain the trait.
`u_lose_effect: effect_string`<br/>`npc_lose_effect: effect_string` | Your character or the NPC will lose the effect if they have it.
`u_lose_trait: trait_string`<br/>`npc_lose_trait: trait_string` | Your character or the NPC will lose the trait.
`u_add_var, npc_add_var`: `var_name, type: type_str`, `context: context_str`, `value: value_str` | Your character or the NPC will store `value_str` as a variable that can be later retrieved by `u_has_var` or `npc_has_var`.  `npc_add_var` can be used to store arbitary local variables, and `u_add_var` can be used to store arbitrary "global" variables, and should be used in preference to setting effects.
`u_lose_var`, `npc_lose_var`: `var_name`, `type: type_str`, `context: context_str` | Your character or the NPC will clear any stored variable that has the same `var_name`, `type_str`, and `context_str`.

#### Trade / Items

Effect | Description
---|---
`start_trade` | Opens the trade screen and allows trading with the NPC.
`buy_10_logs` | Places 10 logs in the ranch garage, and makes the NPC unavailable for 1 day.
`buy_100_logs` | Places 100 logs in the ranch garage, and makes the NPC unavailable for 7 days.
`give_equipment` | Allows your character to select items from the NPC's inventory and transfer them to your inventory.
`npc_gets_item` | Allows your character to select an item from your character's inventory and transfer it to the NPC's inventory.  The NPC will not accept it if they do not have space or weight to carry it, and will set a reason that can be referenced in a future dynamic line with `"use_reason"`.
`npc_gets_item_to_use` | Allow your character to select an item from your character's inventory and transfer it to the NPC's inventory.  The NPC will attempt to wield it and will not accept it if it too heavy or is an inferior weapon to what they are currently using, and will set a reason that can be referenced in a future dynamic line with `"use_reason"`.
`u_buy_item: item_string`, (*optional* `cost: cost_num`, *optional* `count: count_num`, *optional* `container: container_string`) | The NPC will give your character the item or `count_num` copies of the item, contained in container, and will remove `cost_num` from your character's cash if specified.<br/>If cost isn't present, the NPC gives your character the item at no charge.
`u_sell_item: item_string`, (*optional* `cost: cost_num`, *optional* `count: count_num`) | Your character will give the NPC the item or `count_num` copies of the item, and will add `cost_num` to your character's cash if specified.<br/>If cost isn't present, the your character gives the NPC the item at no charge.<br/>This effect will fail if you do not have at least `count_num` copies of the item, so it should be checked with `u_has_items`.
`u_bulk_trade_accept`<br/>`npc_bulk_trade_accept` | Only valid after a `repeat_response`.  The player trades all instances of the item from the `repeat_response` with the NPC.  For `u_bulk_trade_accept`, the player loses the items from their inventory and gains cash; for `npc_bulk_trade_accept`, the player gains the items from the NPC's inventory and loses cash.
`u_bulk_donate`<br/>`npc_bulk_donate` | Only valid after a `repeat_response`.  The player or NPC transfers all instances of the item from the `repeat_response`.  For `u_bulk_donate`, the player loses the items from their inventory and the NPC gains them; for `npc_bulk_donate`, the player gains the items from the NPC's inventory and the NPC loses them.
`u_spend_cash: cost_num` | Remove `cost_num` from your character's cash.  Negative values means your character gains cash.
`add_debt: mod_list` | Increases the NPC's debt to the player by the values in the `mod_list`.<br/>The following would increase the NPC's debt to the player by 1500x the NPC's altruism and 1000x the NPC's opinion of the player's value: `{ "effect": { "add_debt": [ [ "ALTRUISM", 3 ], [ "VALUE", 2 ], [ "TOTAL", 500 ] ] } }`
`u_consume_item`, `npc_consume_item: item_string`, (*optional* `count: count_num`) | You or the NPC will delete the item or `count_num` copies of the item from their inventory.<br/>This effect will fail if the you or NPC does not have at least `count_num` copies of the item, so it should be checked with `u_has_items` or `npc_has_items`.
`u_remove_item_with`, `npc_remove_item_with: item_string` | You or the NPC will delete any instances of item in inventory.<br/>This is an unconditional remove and will not fail if you or the NPC does not have the item.

#### Behaviour / AI

Effect | Description
---|---
`assign_guard` | Makes the NPC into a guard.  If allied and at a camp, they will be assigned to that camp.
`stop_guard` | Releases the NPC from their guard duty (also see `assign_guard`).  Friendly NPCs will return to following.
`start_camp` | The NPC will start a faction camp with the player.
`recover_camp` | Makes the NPC the overseer of an existing camp that doesn't have an overseer.
`remove_overseer` | Makes the NPC stop being an overseer, abandoning the faction camp.
`wake_up` | Wakes up sleeping, but not sedated, NPCs.
`reveal_stats` | Reveals the NPC's stats, based on the player's skill at assessing them.
`end_conversation` | Ends the conversation and makes the NPC ignore you from now on.
`insult_combat` | Ends the conversation and makes the NPC hostile, adds a message that character starts a fight with the NPC.
`hostile` | Make the NPC hostile and end the conversation.
`flee` | Makes the NPC flee from your character.
`follow` | Makes the NPC follow your character, joining the "Your Followers faction".
`leave` | Makes the NPC leave the "Your Followers" faction and stop following your character.
`follow_only` | Makes the NPC follow your character without changing factions.
`stop_following` | Makes the NPC stop following your character without changing  factions.
`npc_thankful` | Makes the NPC positively inclined toward your character.
`drop_weapon` | Make the NPC drop their weapon.
`stranger_neutral` | Changes the NPC's attitude to neutral.
`start_mugging` | The NPC will approach your character and steal from your character, attacking if your character resists.
`lead_to_safety` | The NPC will gain the LEAD attitude and give your character the mission of reaching safety.
`start_training` | The NPC will train your character in a skill or martial art.
`companion_mission: role_string` | The NPC will offer you a list of missions for your allied NPCs, depending on the NPC's role.
`basecamp_mission` | The NPC will offer you a list of missions for your allied NPCs, depending on the local basecamp.
`bionic_install` | The NPC installs a bionic from your character's inventory onto your character, using very high skill, and charging you according to the operation's difficulty.
`bionic_remove` | The NPC removes a bionic from your character, using very high skill, and charging you according to the operation's difficulty.
`npc_class_change: class_string` | Change the NPC's faction to `class_string`.
`npc_faction_change: faction_string` | Change the NPC's faction membership to `faction_string`.
`u_faction_rep: rep_num` | Increases your reputation with the NPC's current faction, or decreases it if `rep_num` is negative.
`toggle_npc_rule: rule_string` | Toggles the value of a boolean NPC follower AI rule such as `"use_silent"` or `"allow_bash"`
`set_npc_rule: rule_string` | Sets the value of a boolean NPC follower AI rule such as `"use_silent"` or `"allow_bash"`
`clear_npc_rule: rule_string` | Clears the value of a boolean NPC follower AI rule such as `"use_silent"` or `"allow_bash"`
`set_npc_engagement_rule: rule_string` | Sets the NPC follower AI rule for engagement distance to the value of `rule_string`.
`set_npc_aim_rule: rule_string` | Sets the NPC follower AI rule for aiming speed to the value of `rule_string`.
`npc_die` | The NPC will die at the end of the conversation.

#### Map Updates
`mapgen_update: mapgen_update_id_string`<br/>`mapgen_update:` *list of `mapgen_update_id_string`s*, (optional `assign_mission_target` parameters) | With no other parameters, updates the overmap tile at the player's current location with the changes described in `mapgen_update_id` (or for each `mapgen_update_id` in the list).  The `assign_mission_target` parameters can be used to change the location of the overmap tile that gets updated.  See [the missions docs](MISSIONS_JSON.md) for `assign_mission_target` parameters and [the mapgen docs](MAPGEN.md) for `mapgen_update`.

#### Deprecated

Effect | Description
---|---
`deny_follow`<br/>`deny_lead`<br/>`deny_train`<br/>`deny_personal_info` | Sets the appropriate effect on the NPC for a few hours.<br/>These are *deprecated* in favor of the more flexible `npc_add_effect` described above.

#### Sample effects
```json
{ "topic": "TALK_EVAC_GUARD3_HOSTILE", "effect": [ { "u_faction_rep": -15 }, { "npc_change_faction": "hells_raiders" } ] }
{ "text": "Let's trade then.", "effect": "start_trade", "topic": "TALK_EVAC_MERCHANT" },
{ "text": "What needs to be done?", "topic": "TALK_CAMP_OVERSEER", "effect": { "companion_mission": "FACTION_CAMP" } }
{ "text": "Do you like it?", "topic": "TALK_CAMP_OVERSEER", "effect": [ { "u_add_effect": "concerned", "duration": 600 }, { "npc_add_effect": "touched", "duration": "3600" }, { "u_add_effect": "empathetic", "duration": "PERMANENT" } ] }
```

---
### opinion changes
As a special effect, an NPC's opinion of your character can change. Use the following:

#### opinion: { }
trust, value, fear, and anger are optional keywords inside the opinion object. Each keyword must be followed by a numeric value. The NPC's opinion is modified by the value.

#### Sample opinions
```json
{ "effect": "follow", "opinion": { "trust": 1, "value": 1 }, "topic": "TALK_DONE" }
{ "topic": "TALK_DENY_FOLLOW", "effect": "deny_follow", "opinion": { "fear": -1, "value": -1, "anger": 1 } }
```

#### mission_opinion: { }
trust, value, fear, and anger are optional keywords inside the `mission_opinion` object. Each keyword must be followed by a numeric value. The NPC's opinion is modified by the value.

---

## Dialogue conditions
Conditions can be a simple string with no other values, a key and an int, a key and a string, a key and an array, or a key and an object. Arrays and objects can nest with each other and can contain any other condition.

The following keys and simple strings are available:

#### Boolean logic

Condition | Type | Description
--- | --- | ---
`"and"` | array | `true` if every condition in the array is true. Can be used to create complex condition tests, like `"[INTELLIGENCE 10+][PERCEPTION 12+] Your jacket is torn. Did you leave that scrap of fabric behind?"`
`"or"` | array | `true` if every condition in the array is true. Can be used to create complex condition tests, like `"[STRENGTH 9+] or [DEXTERITY 9+] I'm sure I can handle one zombie."`
`"not"` | object | `true` if the condition in the object or string is false. Can be used to create complex conditions test by negating other conditions, for text such as<br/>`"[INTELLIGENCE 7-] Hitting the reactor with a hammer should shut it off safely, right?"`

#### Player or NPC conditions
These conditions can be tested for the player using the `"u_"` form, and for the NPC using the `"npc_"` form.

Condition | Type | Description
--- | --- | ---
`"u_male"`<br/>`"npc_male"` | simple string | `true` if the player character or NPC is male.
`"u_female"`<br/>`"npc_female"` | simple string | `true` if the player character or NPC is female.
`"u_at_om_location"`<br/>`"npc_at_om_location"` | string | `true` if the player character or NPC is standing on an overmap tile with `u_at_om_location`'s id.  The special string `"FACTION_CAMP_ANY"` changes it to return true of the player or NPC is standing on a faction camp overmap tile.
`"u_has_trait"`<br/>`"npc_has_trait"` | string | `true` if the player character or NPC has a specific trait.  Simpler versions of `u_has_any_trait` and `npc_has_any_trait` that only checks for one trait.
`"u_has_trait_flag"`<br/>`"npc_has_trait_flag"` | string | `true` if the player character or NPC has any traits with the specific trait flag.  More robust versions of `u_has_any_trait` and `npc_has_any_trait`.  The special trait flag `"MUTATION_THRESHOLD"` checks to see if the player or NPC has crossed a mutation threshold.
`"u_has_any_trait"`<br/>`"npc_has_any_trait"` | array | `true` if the player character or NPC has any trait or mutation in the array. Used to check multiple specific traits.
`"u_has_var"`, `"npc_has_var"` | string | `"type": type_str`, `"context": context_str`, and `"value": value_str` are required fields in the same dictionary as `"u_has_var"` or `"npc_has_var"`.<br/>`true` is the player character or NPC has a variable set by `"u_add_var"` or `"npc_add_var"` with the string, `type_str`, `context_str`, and `value_str`.
`"u_has_strength"`<br/>`"npc_has_strength"` | int | `true` if the player character's or NPC's strength is at least the value of `u_has_strength` or `npc_has_strength`.
`"u_has_dexterity"`<br/>`"npc_has_dexterity"` | int | `true` if the player character's or NPC's dexterity is at least the value of `u_has_dexterity` or `npc_has_dexterity`.
`"u_has_intelligence"`<br/>`"npc_has_intelligence"` | int | `true` if the player character's or NPC's intelligence is at least the value of `u_has_intelligence` or `npc_has_intelligence`.
`"u_has_perception"`<br/>`"npc_has_perception"` | int | `true` if the player character's or NPC's perception is at least the value of `u_has_perception` or `npc_has_perception`.
`"u_has_item"`<br/>`"npc_has_item"` | string | `true` if the player character or NPC has something with `u_has_item`'s or `npc_has_item`'s `item_id` in their inventory.
`"u_has_items"`<br/>`"npc_has_item"` | dictionary | `u_has_items` or `npc_has_items` must be a dictionary with an `item` string and a `count` int.<br/>`true` if the player character or NPC has at least `count` charges or counts of `item` in their inventory.
`"u_has_item_category"`<br/>`"npc_has_item_category"` | string | `"count": item_count` is an optional field that must be in the same dictionary and defaults to 1 if not specified.  `true` if the player or NPC has `item_count` items with the same category as `u_has_item_category` or `npc_has_item_category`.
`"u_has_bionics"`<br/>`"npc_has_bionics"` | string | `true` if the player or NPC has an installed bionic with an `bionic_id` matching `"u_has_bionics"` or `"npc_has_bionics"`.  The special string "ANY" returns true if the player or NPC has any installed bionics.
`"u_has_effect"`<br/>`"npc_has_effect"` | string | `true` if the player character or NPC is under the effect with `u_has_effect` or `npc_has_effect`'s `effect_id`.
`"u_can_stow_weapon"`<br/>`"npc_can_stow_weapon"` | simple string | `true` if the player character or NPC is wielding a weapon and has enough space to put it away.
`"u_has_weapon"`<br/>`"npc_has_weapon"` | simple string | `true` if the player character or NPC is wielding a weapon.
`"u_driving"`<br/>`"npc_driving"` | simple string | `true` if the player character or NPC is operating a vehicle.  <b>Note</b> NPCs cannot currently operate vehicles.

#### Player Only conditions

`"u_has_mission"` | string | `true` if the mission is assigned to the player character.
`"u_has_cash"` | int | `true` if the player character has at least `u_has_cash` cash available.  Used to check if the PC can buy something.
`"u_has_camp"` | simple string | `true` is the player has one or more active base camps.

#### Player and NPC interaction conditions

Condition | Type | Description
--- | --- | ---
`"at_safe_space"` | simple string | `true` if the NPC's current overmap location passes the `is_safe()` test.
`"has_assigned_mission"` | simple string | `true` if the player character has exactly one mission from the NPC. Can be used for texts like "About that job...".
`"has_many_assigned_missions"` | simple string | `true` if the player character has several mission from the NPC (more than one). Can be used for texts like "About one of those jobs..." and to switch to the `"TALK_MISSION_LIST_ASSIGNED"` topic.
`"has_no_available_mission"` | simple string | `true` if the NPC has no jobs available for the player character.
`"has_available_mission"` | simple string | `true` if the NPC has one job available for the player character.
`"has_many_available_missions"` | simple string | `true` if the NPC has several jobs available for the player character.
`"mission_goal"` | string | `true` if the NPC's current mission has the same goal as `mission_goal`.
`"mission_complete"` | simple string | `true` if the player has completed the NPC's current mission.
`"mission_incomplete"` | simple string | `true` if the player hasn't completed the NPC's current mission.
`"npc_service"` | int | `true` if the NPC does not have the `"currently_busy"` effect and the player character has at least `npc_service` cash available.  Useful to check if the player character can hire an NPC to perform a task that would take time to complete.  Functionally, this is identical to `"and": [ { "not": { "npc_has_effect": "currently_busy" } }, { "u_has_cash": service_cost } ]`
`"npc_allies"` | int | `true` if the player character has at least `npc_allies` number of NPC allies.
`"npc_following"` | simple string | `true` if the NPC is following the player character.
`"is_by_radio"` | simple string | `true` if the player is talking to the NPC over a radio.

#### NPC only conditions

Condition | Type | Description
--- | --- | ---
`"npc_available"` | simple string | `true` if the NPC does not have effect `"currently_busy"`.
`"npc_following"` | simple string | `true` if the NPC is following the player character.
`"npc_friend"` | simple string | `true` if the NPC is friendly to the player character.
`"npc_hostile"` | simple string | `true` if the NPC is an enemy of the player character.
`"npc_train_skills"` | simple string | `true` if the NPC has one or more skills with more levels than the player.
`"npc_train_styles"` | simple string | `true` if the NPC knows one or more martial arts styles that the player does not know.
`"npc_has_class"` | array | `true` if the NPC is a member of an NPC class.
`"npc_role_nearby"` | string | `true` if there is an NPC with the same companion mission role as `npc_role_nearby` within 100 tiles.
`"has_reason"` | simple string | `true` if a previous effect set a reason for why an effect could not be completed.

#### NPC Follower AI rules
Condition | Type | Description
--- | --- | ---
`"npc_aim_rule"` | string | `true` if the NPC follower AI rule for aiming matches the string.
`"npc_engagement_rule"` | string | `true` if the NPC follower AI rule for engagement matches the string.
`"npc_rule"` | string | `true` if the NPC follower AI rule for that matches string is set.

#### Environment

Condition | Type | Description
--- | --- | ---
`"days_since_cataclysm"` | int | `true` if at least `days_since_cataclysm` days have passed since the Cataclysm.
`"is_season"` | string | `true` if the current season matches `is_season`, which must be one of "`spring"`, `"summer"`, `"autumn"`, or `"winter"`.
`"is_day"` | simple string | `true` if it is currently daytime.
`"is_outside"` | simple string | `true` if the NPC is on a tile without a roof.


#### Sample responses with conditions and effects
```json
{
  "text": "Understood.  I'll get those antibiotics.",
  "topic": "TALK_NONE",
  "condition": { "npc_has_effect": "infected" }
},
{
  "text": "I'm sorry for offending you.  I predict you will feel better in exactly one hour.",
  "topic": "TALK_NONE",
  "effect": { "npc_add_effect": "deeply_offended", "duration": 600 }
},
{
  "text": "Nice to meet you too.",
  "topic": "TALK_NONE",
  "effect": { "u_add_effect": "has_met_example_NPC", "duration": "PERMANENT" }
},
{
  "text": "Nice to meet you too.",
  "topic": "TALK_NONE",
  "condition": {
    "not": {
      "npc_has_var": "has_met_PC", "type": "general", "context": "examples", "value": "yes"
    }
  },
  "effect": {
    "npc_add_var": "has_met_PC", "type": "general", "context": "examples", "value": "yes" }
  }
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
},
{
    "text": "I killed them.  All of them.",
    "topic": "TALK_MISSION_SUCCESS",
    "condition": {
        "and": [ { "or": [ { "mission_goal": "KILL_MONSTER_SPEC" }, { "mission_goal": "KILL_MONSTER_TYPE" } ] }, "mission_complete" ]
    },
    "switch": true
},
{
    "text": "Glad to help.  I need no payment.",
    "topic": "TALK_NONE",
    "effect": "clear_mission",
    "mission_opinion": { "trust": 4, "value": 3 },
    "opinion": { "fear": -1, "anger": -1 }
},
{
    "text": "Maybe you can teach me something as payment?",
    "topic": "TALK_TRAIN",
    "condition": { "or": [ "npc_train_skills", "npc_train_styles" ] },
    "effect": "mission_reward"
},
{
    "truefalsetext": {
        "true": "I killed him.",
        "false": "I killed it.",
        "condition": { "mission_goal": "ASSASSINATE" }
    },
    "condition": {
        "and": [
            "mission_incomplete",
            {
                "or": [
                    { "mission_goal": "ASSASSINATE" },
                    { "mission_goal": "KILL_MONSTER" },
                    { "mission_goal": "KILL_MONSTER_SPEC" },
                    { "mission_goal": "KILL_MONSTER_TYPE" }
                ]
            }
        ]
    },
    "trial": { "type": "LIE", "difficulty": 10, "mod": [ [ "TRUST", 3 ] ] },
    "success": { "topic": "TALK_NONE" },
    "failure": { "topic": "TALK_MISSION_FAILURE" }
},
{
  "text": "Didn't you say you knew where the Vault was?",
  "topic": "TALK_VAULT_INFO",
  "condition": { "not": { "u_has_var": "asked_about_vault", "value": "yes", "type": "sentinel", "context": "old_guard_rep" } },
  "effect": [
      { "u_add_var": "asked_about_vault", "value": "yes", "type": "sentinel", "context": "old_guard" },
      { "mapgen_update": "hulk_hairstyling", "om_terrain": "necropolis_a_13", "om_special": "Necropolis", "om_terrain_replace": "field", "z": 0 }
  ]
},
{
  "text": "Why do zombies keep attacking every time I talk to you?",
  "topic": "TALK_RUN_AWAY_MORE_ZOMBIES",
  "condition": { "u_has_var": "even_more_zombies", "value": "yes", "type": "trigger", "context": "learning_experience" },
  "effect": [
    { "mapgen_update": [ "even_more_zombies", "more zombies" ], "origin_npc": true },
    { "mapgen_update": "more zombies", "origin_npc": true, "offset_x": 1 },
    { "mapgen_update": "more zombies", "origin_npc": true, "offset_x": -1 },
    { "mapgen_update": "more zombies", "origin_npc": true, "offset_y": 1 },
    { "mapgen_update": "more zombies", "origin_npc": true, "offset_y": -1 }
  ]
}
```
