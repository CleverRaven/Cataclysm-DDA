TODO: document the "npc" structure, used to load NPC template
# Creating new NPCs
Often you will want to create new NPCs to either add quests, flavor, or to introduce little known activities to the larger player base. New NPCs are written entirely in JSON, so they are one of the easier places to begin your contributions.

There are two parts to creating a new NPC, apart from any dialogue you may want to add.
#### NPC Class
First there is the `npc_class` which follows the following template.
Format:
```json
{
  "type": "npc_class",
  "id": "NC_EXAMPLE",
  "name": { "str": "Example NPC" },
  "job_description": "I'm helping you learn the game.",
  "common": false,
  "sells_belongings": false,
  "bonus_str": { "rng": [ -4, 0 ] },
  "bonus_dex": { "rng": [ -2, 0 ] },
  "bonus_int": { "rng": [ 1, 5 ] },
  "skills": [
    {
      "skill": "ALL",
      "level": { "mul": [ { "one_in": 3 }, { "sum": [ { "dice": [ 2, 2 ] }, { "constant": -2 }, { "one_in": 4 } ] } ] }
    }
  ],
  "worn_override": "NC_EXAMPLE_worn",
  "carry_override": "NC_EXAMPLE_carried",
  "weapon_override": "NC_EXAMPLE_weapon",
  "shopkeeper_item_group": [
    { "group": "example_shopkeeper_itemgroup1" },
    { "group": "example_shopkeeper_itemgroup2", "trust": 10 },
    { "group": "example_shopkeeper_itemgroup3", "trust": 20, "rigid": true }
    { "group": "example_shopkeeper_itemgroup3", "trust": 40, "strict": true }
  ],
  "traits": [ { "group": "BG_survival_story_EVACUEE" }, { "group": "NPC_starting_traits" }, { "group": "Appearance_demographics" } ]
}
```
There are a couple of items in the above template that may not be self explanatory:
* `"common": false` means that this NPC class will not spawn randomly. It defaults to `true` if not specified.
* `"sells_belongings": false` means that this NPC's worn or held items will strictly be excluded from their shopkeeper list; otherwise, they'll be happy to sell things like their pants. It defaults to `true` if not specified.
*`"shopkeeper_item_group"` is only needed if the planned NPC will be a shopkeeper with a revolving stock of items that change every three in-game days. All of the item overrides will ensure that any NPC of this class spawns with specific items.

##### Shopkeeper item groups
`"shopkeeper_item_group"` entries have the following fields:
- `"group"` : Identifies an item group to include in the possible shop rotation
- `"trust"` : (_optional_) If the faction's trust with the player is below this value, items in this group will not be available for sale (Defaults to 0)
- `"strict"` : (_optional_) If true, items in this group cannot be traded back to the player if traded to the NPC. (Defaults to false)
- `"rigid"` : (_optional_) By default, item groups will be continually iterated until they reach a certain value or size threshold for the NPC. Rigid groups are instead guaranteed to populate a single time if they can, and will not include duplicate reruns. (Defaults to false)

#### NPC
There is a second template required for a new NPC. It looks like this:
Format:
```json
{
  "type": "npc",
  "id": "examplicious",
  "//": "The luckiest NPC to never experience the Cataclysm.",
  "name_suffix": "examplar",
  "class": "NC_EXAMPLE",
  "attitude": 0,
  "mission": 7,
  "chat": "TALK_EXAMPLE",
  "faction": "no_faction",
  "death_eocs": [ "EOC_DEATH_NPC_TEST" ]
}
```
This is the JSON that creates the NPC ID that is used to spawn an NPC in "mapgen" (map generation).
Attitude is based on the enum in npc.h. The important ones are 0=NPCATT_NULL, 1=NPCATT_TALK", 3=NPCATT_FOLLOW, 7=NPCATT_DEFEND, 10=NPCATT_KILL, and 11=NPCATT_FLEE.
Mission is based on the enum in npc.h.  The important ones are 0=NPC_MISSION_NUL, 3=NPC_MISSION_SHOPKEEP", and 7=NPC_MISSION_GUARD", 8 = NPC_MISSION_GUARD_PATROL will actively investigate noises".
Chat is covered in the dialogue examples below.
Faction determines what faction, if any, the NPC belongs to.  Some examples are the Free Traders, Old Guard, Marloss Evangelists, and Hell's raiders but could include a brand new faction you create!
death_eocs are string effect_on_condition ids and or inline effect_on_conditions (see [EFFECT_ON_CONDITION.md](EFFECT_ON_CONDITION.md)).  When the npc dies all of these eocs are run with the victim as u and the killer as npc.

# Age and Height
You can define the age and height of the NPC in the `age` or `height` fields in `"type": "npc"`.

Field | Description
---|---
`age` | NPC age
`height` | NPC height (cm)


# Writing dialogues
Dialogues work like state machines. They start with a certain topic (the NPC says something), the player character can then respond (choosing one of several responses), and that response sets the new talk topic. This goes on until the dialogue is finished, or the NPC turns hostile.

Note that it is perfectly fine to have a response that switches the topic back to itself.

NPC missions are controlled by a separate but related JSON structure and are documented in
[the missions docs](MISSIONS_JSON.md).

Two topics are special:
- `TALK_DONE` ends the dialogue immediately.
- `TALK_NONE` goes to the previously talked about topic.

If `npc` has the follows fields, the game will display a dialogue with the indicated topic instead of default topic.

Field | Default topic ID  | Uses for...
---|---|---
`talk_friend` | `TALK_FRIEND` | Talk to a follower NPC
`talk_radio` | `TALK_RADIO` | Talk to a follower NPC with two way radios
`talk_leader` | `TALK_LEADER` | Talk to an NPC that have 5=NPCATT_LEAD
`talk_stole_item` | `TALK_STOLE_ITEM` | Talk to an NPC that have 18=NPCATT_RECOVER_GOODS
`talk_wake_up` | `TALK_WAKE_UP` | Talk to a sleeping NPC
`talk_friend_guard` | `TALK_FRIEND_GUARD` | Faction camp guard
`talk_mug` | `TALK_MUG` | see "success and failure" section
`talk_stranger_aggressive` | `TALK_STRANGER_AGGRESSIVE` | see "success and failure" section
`talk_stranger_scared` | `TALK_STRANGER_SCARED` | see "success and failure" section
`talk_stranger_wary` | `TALK_STRANGER_WARY` | see "success and failure" section
`talk_stranger_friendly` | `TALK_STRANGER_FRIENDLY` | see "success and failure" section
`talk_stranger_neutral` | `TALK_STRANGER_NEUTRAL` | see "success and failure" section

### Customize NPC speech
NPCs have dialogue depending on the situation.
This dialogue can be customized in `"type": "npc"` json entries.

If `npc` has one of the following fields, the NPCs will speak the indicated message or snippet instead of the default message.

All messages can be used with snippets.
Any `%s` included is automatically replaced by the game, with words depending on the message.

For further information on snippets, see [New Contributor Guide: Dialogue](https://github.com/CleverRaven/Cataclysm-DDA/wiki/New-Contributor-Guide-Dialogue)

Field | Default messages/snippets | Used for...
---|---|---
`<acknowledged>` | `<acknowledged>` | see data/json/npcs/talk_tags.json
`<camp_food_thanks>` | `<camp_food_thanks>` | see data/json/npcs/talk_tags.json
`<camp_larder_empty>` | `<camp_larder_empty>` | see data/json/npcs/talk_tags.json
`<camp_water_thanks>` | `<camp_water_thanks>` | see data/json/npcs/talk_tags.json
`<cant_flee>` | `<cant_flee>` | see data/json/npcs/talk_tags.json
`<close_distance>` | `<close_distance>` | see data/json/npcs/talk_tags.json
`<combat_noise_warning>` | `<combat_noise_warning>` | see data/json/npcs/talk_tags.json
`<danger_close_distance>` | `<danger_close_distance>` | see data/json/npcs/talk_tags.json
`<done_mugging>` | `<done_mugging>` | see data/json/npcs/talk_tags.json
`<far_distance>` | `<far_distance>` | see data/json/npcs/talk_tags.json
`<fire_bad>` | `<fire_bad>` | see data/json/npcs/talk_tags.json
`<fire_in_the_hole_h>` | `<fire_in_the_hole_h>` | see data/json/npcs/talk_tags.json
`<fire_in_the_hole>` | `<fire_in_the_hole>` | see data/json/npcs/talk_tags.json
`<general_danger_h>` | `<general_danger_h>` | see data/json/npcs/talk_tags.json
`<general_danger>` | `<general_danger>` | see data/json/npcs/talk_tags.json
`<heal_self>` | `<heal_self>` | see data/json/npcs/talk_tags.json
`<hungry>` | `<hungry>` | see data/json/npcs/talk_tags.json
`<im_leaving_you>` | `<im_leaving_you>` | see data/json/npcs/talk_tags.json
`<its_safe_h>` | `<its_safe_h>` | see data/json/npcs/talk_tags.json
`<its_safe>` | `<its_safe>` | see data/json/npcs/talk_tags.json
`<keep_up>` | `<keep_up>` | see data/json/npcs/talk_tags.json
`<kill_npc_h>` | `<kill_npc_h>` | see data/json/npcs/talk_tags.json
`<kill_npc>` | `<kill_npc>` | see data/json/npcs/talk_tags.json
`<kill_player_h>` | `<kill_player_h>` | see data/json/npcs/talk_tags.json
`<let_me_pass>` | `<let_me_pass>` | see data/json/npcs/talk_tags.json
`<lets_talk>` | `<lets_talk>` | see data/json/npcs/talk_tags.json
`<medium_distance>` | `<medium_distance>` | see data/json/npcs/talk_tags.json
`<monster_warning_h>` | `<monster_warning_h>` | see data/json/npcs/talk_tags.json
`<monster_warning>` | `<monster_warning>` | see data/json/npcs/talk_tags.json
`<movement_noise_warning>` | `<movement_noise_warning>` | see data/json/npcs/talk_tags.json
`<need_batteries>` | `<need_batteries>` | see data/json/npcs/talk_tags.json
`<need_booze>` | `<need_booze>` | see data/json/npcs/talk_tags.json
`<need_fuel>` | `<need_fuel>` | see data/json/npcs/talk_tags.json
`<no_to_thorazine>` | `<no_to_thorazine>` | see data/json/npcs/talk_tags.json
`<run_away>` | `<run_away>` | see data/json/npcs/talk_tags.json
`<speech_warning>` | `<speech_warning>` | see data/json/npcs/talk_tags.json
`<thirsty>` | `<thirsty>` | see data/json/npcs/talk_tags.json
`<wait>` | `<wait>` | see data/json/npcs/talk_tags.json
`<warn_sleep>` | `<warn_sleep>` | see data/json/npcs/talk_tags.json
`<yawn>` | `<yawn>` | see data/json/npcs/talk_tags.json
`<yes_to_lsd>` | `<yes_to_lsd>` | see data/json/npcs/talk_tags.json
`snip_bleeding` | `My %s is bleeding!` | The NPC is bleeding from their %s.
`snip_bleeding_badly` | `My %s is bleeding badly!` | The NPC is bleeding badly from their %s.
`snip_wound_bite` | `The bite wound on my %s looks bad.` | The NPC's %s was badly bitten.
`snip_wound_infected` | `My %s wound is infected…` | The NPC's wound on their %s is infected.
`snip_lost_blood` | `I've lost lot of blood.` | The NPC has lost lot of blood.
`snip_bye` | `Bye.` | Say at the end of a conversation.
`snip_consume_eat` | `Thanks, that hit the spot.` | You gave them some food and the NPC ate it.
`snip_consume_cant_accept` | `I don't <swear> trust you enough to eat THIS…` | You gave them some food but the NPC didn't accept it.
`snip_consume_cant_consume` | `It doesn't look like a good idea to consume this…` | You gave them some food but the NPC wouldn't eat it.
`snip_consume_rotten` | `This is rotten!  I won't eat that.` | You gave them some food but the NPC wouldn't eat it because it's rotten.
`snip_consume_med` | `Thanks, I feel better already.` | You gave them a drug and the NPC swallowed it.
`snip_consume_use_med` | `Thanks, I used it.` | You gave them a drug and the NPC used it.
`snip_consume_need_item` | `I need a %s to consume that!` | You gave them a drug but the NPC couldn't use it without a %s (e.g., a syringe).
`snip_consume_nocharge` | `It doesn't look like a good idea to consume this…` | You gave them a drug but the NPC couldn't use it because of a lack of charges.
`snip_give_cancel` | `Changed your mind?` | You started to give them an item but hit the escape key.
`snip_give_carry` | `Thanks, I'll carry that now.` | You gave them an item and the NPC took it.
`snip_give_nope` | `Nope.` | You gave them an item but the NPC couldn't take it. First says.
`snip_give_carry_cant` | `I have no space to store it.` | You gave an item but NPC couldn't take because no space.
`snip_give_carry_cant_few_space` | `I can only store %s %s more.` | You gave an item but NPC couldn't take because few spaces.
`snip_give_carry_cant_no_space` | `…or to store anything else for that matter.` | You gave an item but NPC couldn't take because no spaces.
`snip_give_carry_too_heavy` | `It is too heavy for me to carry.` | You gave an item but NPC couldn't take because too heavy.
`snip_give_dangerous` | `Are you <swear> insane!?` | You gave a dangerous item like active grenade.
`snip_give_to_hallucination` | `No thanks, I'm good.` | You gave an item but NPC couldn't take because he/she was in hallucination.
`snip_give_wield` | `Thanks, I'll wield that now.` | You gave a weapon and NPC wield it.
`snip_give_weapon_weak` | `My current weapon is better than this.\n` | You gave a weapon but NPC didn't take it because it was weaker than current weapons.
`snip_heal_player` | `Hold still %s, I'm coming to help you.` | NPC try to heal you.
`snip_pulp_zombie` | `Hold on, I want to pulp that %s.` | NPC try to pulp %s.
`snip_radiation_sickness` | `I'm suffering from radiation sickness…` | NPC are in radiation sickness.
`snip_wear` | `Thanks, I'll wear that now.` | You gave a clothes or armor and NPC wear it.

### Special Custom Entries

Certain entries like the snippets above are taken from the game state as opposed to JSON; they are found in the npctalk function parse_tags. They are as follows:
`<yrwp>` | displays avatar's wielded item
`<mywp>` | displays npc's wielded item
`<u_name>` | displays avatar's name
`<npc_name>` | displays npc's name
`<ammo>` | displays avatar's ammo
`<current_activity>` | displays npc's current activity
`<punc>` | displays a random punctuation from: `.`, `…`, `!`
`<mypronoun>` | displays npc's pronoun
`<topic_item>` | referenced item
`<topic_item_price>` | referenced item unit price
`<topic_item_my_total_price>` | TODO Add
`<topic_item_your_total_price>` | TODO Add
`<u_val:VAR>` | The user variable VAR
`<npc_val:VAR>` | The npc variable VAR
`<global_val:VAR>` | The global variable VAR


### Validating Dialogues
Keeping track of talk topics and making sure that all the topics referenced in responses are
defined, and all defined topics are referenced in a response or an NPC's chat, is very tricky.
There is a python script in `tools/dialogue_validator.py` that will map all topics to responses
and vice versa.  Invoke it with
```sh
python3 tools/dialogue_validator.py data/json/npcs/* data/json/npcs/Backgrounds/* data/json/npcs/refugee_center/*
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

#### A line to be translated with gender context
The line is to be given a gender context for the NPC, player, or both, to aid
translation in languages where that matters. For example:
```json
{
    "gendered_line": "Thank you.",
    "relevant_genders": [ "npc" ]
}
```
("Thank you" is different for male and female speakers in e.g. Portuguese).

Valid choices for entries in the `"relevant_genders"` list are `"npc"` and
`"u"`.

#### A randomly selected hint
The dynamic line will be randomly chosen from the hints snippets.

```json
{
    "give_hint": true
}
```

#### A list of potential faction camp sites
The dynamic line will list all of the possible starting sites for faction camps.

#### Based on a previously generated reason
The dynamic line will be chosen from a reason generated by an earlier effect.  The reason will be cleared.  Use of it should be gated on the `"has_reason"` condition.

```json
{
    "has_reason": { "use_reason": true },
    "no": "What is it, boss?"
}
```

#### Based on any conditional statement
The dynamic line will be chosen based on whether a single conditional statement (see [Conditionals](./CONDITIONALS.md) ) is true or false.  Conditions cannot be chained via `"and"`, `"or"`, or `"not"`.  If the condition is true, the `"yes"` response will be chosen and otherwise the `"no"` response will be chosen.  Both the `'"yes"` and `"no"` responses are optional.  Simple string conditions may be followed by `"true"` to make them fields in the dynamic line dictionary, or they can be followed by the response that will be chosen if the condition is true and the `"yes"` response can be omitted.

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
The `speaker_effect` entry contains effects that occur after the NPC speaks the `dynamic_line` but before the player responds and regardless of the player response.  Each effect can have an optional condition, and will only be applied if the condition is true.  Each `speaker_effect` can also have an optional `sentinel`, which guarantees the effect will only run once.

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

The `sentinel` can be any string, but sentinels are unique to each `TALK_TOPIC`.  If there are multiple `speaker_effect`s within the `TALK_TOPIC`, they should have different sentinels.  Sentinels are not required, but since the `speaker_effect` will run every time the dialogue returns to the `TALK_TOPIC`, they are highly encouraged to avoid inadvertently repeating the same effects.

The `effect` can be any legal [scripted effect](./script_effects.md).  The effect can be a simple string, object, or an array of strings and objects, as normal for objects.

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
The player will have one response text if a [condition](./CONDITIONALS.md) is true, and another if it is false, but the same trial for either line.  `condition`, `true`, and `false` are all mandatory.

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

The `difficulty` is only required if type is not `"NONE"` or `"CONDITION"` and, for most trials, specifies the success chance in percent (it is however modified by various things like mutations).  Higher difficulties are easier to pass. `"SKILL_CHECK"` trials are unique, and use the difficulty as a flat comparison.

An optional `mod` array takes any of the following modifiers and increases the difficulty by the NPC's opinion of your character or personality trait for that modifier multiplied by the value: `"ANGER"`, `"FEAR"`, `"TRUST"`, `"VALUE"`, `"AGGRESSION"`, `"ALTRUISM"`, `"BRAVERY"`, `"COLLECTOR"`. The special `"POS_FEAR"` modifier treats NPC's fear of your character below 0 as though it were 0.  The special `"TOTAL"` modifier sums all previous modifiers and then multiplies the result by its value and is used when setting the owed value.

`"CONDITION"` trials take a mandatory `condition` instead of `difficulty`.  The `success` object is chosen if the `condition` is true and the `failure` is chosen otherwise.

`"SKILL_CHECK"` trials check the user's level in a skill, whose ID is read from the string object `skill_required`. The `success` object is chosen if the skill level is equal to or greater than `difficulty`, and `failure` is chosen otherwise.

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
```json
"trial": { "type": "PERSUADE", "difficulty": 0, "mod": [ [ "TRUST", 3 ], [ "VALUE", 3 ], [ "ANGER", -3 ] ] }
"trial": { "type": "INTIMIDATE", "difficulty": 20, "mod": [ [ "FEAR", 8 ], [ "VALUE", 2 ], [ "TRUST", 2 ], [ "BRAVERY", -2 ] ] }
"trial": { "type": "CONDITION", "condition": { "npc_has_trait": "FARMER" } }
"trial": { "type": "SKILL_CHECK", "difficulty": 3, "skill_required": "swimming" }
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
This is an optional [condition](./CONDITIONALS.md) which can be used to prevent the response under certain circumstances. If not defined, it defaults to always `true`. If the condition is not met, the response is not included in the list of possible responses.

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

`"is_npc"` is an optional bool value, and if it is present, the NPC's inventory list is checked.  By default, the player's inventory list is checked.

`"include_containers"` is an optional bool value, and if it is present, items containing an item will generate separate responses from the item itself.

---

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
