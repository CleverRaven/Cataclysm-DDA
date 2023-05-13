# Contents

- [Creating new NPCs](#creating-new-npcs)
  - [NPC Class definition](#npc-class-definition)
    - [Shopkeeper NPC configuration](#shopkeeper-npc-configuration)
  - [NPC instance definition](#npc-instance-definition)
- [Writing dialogues](#writing-dialogues)
  - [Validating Dialogues](#validating-dialogues)
  - [Customizing NPC speech](#customizing-npc-speech)
  - [Talk Topics](#talk-topics)
  - [Dynamic Lines](#dynamic-lines)
  - [Speaker Effects](#speaker-effects)
  - [Responses](#responses)
  - [Dialogue Effects](#dialogue-effects)
  - [Dialogue Conditions](#dialogue-conditions)
  - [Sample responses with conditions and effects](#sample-responses-with-conditions-and-effects)
  - [Utility Structures](#utility-structures)

---

# Creating new NPCs
Often you will want to create new NPCs to either add quests, flavor, or to introduce little known activities to the larger player base. New NPCs are written entirely in JSON, so they are one of the easier places to begin your contributions.

There are two parts to creating a new NPC, apart from any dialogue you may want to add.

## NPC Class definition
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
    { "group": "example_shopkeeper_itemgroup3", "trust": 40, "strict": true },
    {
      "group": "example_shopkeeper_itemgroup4",
      "condition": { "u_has_var": "VIP", "type": "general", "context": "examples", "value": "yes" }
    }
  ],
  "shopkeeper_consumption_rates": "basic_shop_rates",
  "shopkeeper_price_rules": [
    { "item": "scrap", "price": 10000 },
  ],
  "shopkeeper_blacklist": "test_blacklist",
  "restock_interval": "6 days",
  "traits": [ { "group": "BG_survival_story_EVACUEE" }, { "group": "NPC_starting_traits" }, { "group": "Appearance_demographics" } ]
}
```
There are some items in the above template that may not be self explanatory:
* `"common": false` means that this NPC class will not spawn randomly. It defaults to `true` if not specified.
* See also [Shopkeeper NPC configuration](#shopkeeper-npc-configuration) below.

### Shopkeeper NPC configuration
`npc_class` supports several properties for configuring the behavior of NPCs that behave as shopkeepers:
* `"sells_belongings": false` means that this NPC's worn or held items will strictly be excluded from their shopkeeper list; otherwise, they'll be happy to sell things like their pants. It defaults to `true` if not specified.
* `"shopkeeper_item_group"` is only needed if the planned NPC will be a shopkeeper with a revolving stock of items that change every three in-game days. All of the item overrides will ensure that any NPC of this class spawns with specific items.
* `"shopkeeper_consumption_rates"` optional to define item consumption rates for this shopkeeper. Default is to consume all items before restocking
* `"shopkeeper_price_rules"` optional to define personal price rules with the same format as faction price rules (see [FACTIONS.md](FACTIONS.md)). These take priority over faction rules
* `"shopkeeper_blacklist"` optional to define blacklists for this shopkeeper
* `"restock_interval"`: optional. Default is 6 days

#### Shopkeeper item groups
`"shopkeeper_item_group"` entries have the following fields:
- `"group"` : Identifies an item group to include in the possible shop rotation
- `"trust"` : (_optional_) If the faction's trust with the player is below this value, items in this group will not be available for sale (Defaults to 0)
- `"condition"` : (_optional_) Checked alongside trust with the avatar as alpha and the evaluating NPC as beta. See [Player or NPC conditions](#player-or-npc-conditions).
- `"strict"` : (_optional_) If true, items in this group will not be available for restocking unless the conditions are met. (Defaults to false)
- `"rigid"` : (_optional_) By default, item groups will be continually iterated until they reach a certain value or size threshold for the NPC. Rigid groups are instead guaranteed to populate a single time if they can, and will not include duplicate reruns. (Defaults to false)
- `"refusal"` : (_optional_) message to display in UIs (ex: trade UI) when conditions are not met. Defaults to `"<npcname> does not trust you enough"`

#### Shopkeeper consumption rates
Controls consumption of shopkeeper's stock of items (simulates purchase by other people besides they player).
```JSON
  "type": "shopkeeper_consumption_rates",
  "id": "basic_shop_rates",
  "default_rate": 5, // defined as units/day since last restock
  "junk_threshold": "10 cent", // items below this price will be consumed completely regardless of matches below
  "rates": [ // lower entries override higher ones
    { "item": "hammer", "rate": 1 },
    {
      "item": "hammer",
      "rate": 10,
      "condition": { "npc_has_var": "hammer_eater", "type": "bool", "context": "dinner", "value": "yes" }
    },
    { "category": "ammo", "rate": 10 },
    { "group": "EXODII_basic_trade", "rate": 100 }
    { "group": "EXODII_basic_trade", "category": "ammo", "rate": 200 }
  ]
```
`condition` is checked with avatar as alpha and npc as beta. See [Player or NPC conditions](#player-or-npc-conditions).

#### Shopkeeper blacklists
Specifies blacklist of items that shopkeeper will not accept for trade.  Format is similar to `shopkeeper_consumption_rates`.

```JSON
  "type": "shopkeeper_blacklist",
  "id": "basic_blacklist",
  "entries": [
    {
      "item": "hammer",
      "condition": { "npc_has_var": "hammer_hater", "type": "bool", "context": "test", "value": "yes" },
      "message": "<npcname> hates this item"
    },
    { "category": "ammo" },
    { "group": "EXODII_basic_trade" }
  ]
```

#### Shop restocking
NPCs with at least one `shopkeeper_item_group` will (re)stock their shop in nearby loot zones (within `PICKUP_RANGE` = 6 tiles) owned by their faction and will ignore all other items. If there isn't at least one `LOOT_UNSORTED` zone nearby, fallback zones will be automatically placed on all nearby, reachable, unsealed furniture with either the `CONTAINER` flag or a max volume higher than the floor. If there is no suitable furniture around, a 3x3 zone centered on the NPC will be created instead.

Before restocking, items owned by the NPC's faction within these zones will be consumed according to `shopkeeper_consumption_rates`.

The shop restocks every `restock_interval` regardless of interactions with the avatar.

NOTE: do not place items within these loot zones in mapgen definitions as these will be consumed during the first restock. Add them to the item groups instead.

## NPC instance definition
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

Attitude is based on the enum in `npc.h`. The important ones are `0=NPCATT_NULL`, `1=NPCATT_TALK`, `3=NPCATT_FOLLOW`, `10=NPCATT_KILL`, and `11=NPCATT_FLEE`.

Mission is based on the enum in `npc.h`.  The important ones are `0=NPC_MISSION_NULL`, `3=NPC_MISSION_SHOPKEEP`, `7=NPC_MISSION_GUARD`, and `8=NPC_MISSION_GUARD_PATROL`.

Chat is covered in the dialogue examples below.

Faction determines what faction, if any, the NPC belongs to.  Some examples are the Free Traders, Old Guard, Marloss Evangelists, and Hell's raiders but could include a brand new faction you create!

`death_eocs` are string `effect_on_condition` ids and/or inline `effect_on_condition`s (see [EFFECT_ON_CONDITION.md](EFFECT_ON_CONDITION.md)).  When the npc dies all of these `eoc`s are run with the victim as u and the killer as npc.

`age` and `height` are optional fields that can be used to define the age and height (in cm) of the NPC respectively.

---

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

---

## Validating Dialogues
Keeping track of talk topics and making sure that all the topics referenced in responses are
defined, and all defined topics are referenced in a response or an NPC's chat, is very tricky.
There is a python script in `tools/dialogue_validator.py` that will map all topics to responses
and vice versa.  Invoke it with
```sh
python3 tools/dialogue_validator.py data/json/npcs/* data/json/npcs/Backgrounds/* data/json/npcs/refugee_center/*
```

If you are writing a mod with dialogue, you can add the paths to the mod's dialogue files.

---

## Customizing NPC speech
NPCs have dialogue depending on the situation.
This dialogue can be customized in `"type": "npc"` json entries.

If `npc` has one of the following fields, the NPCs will speak the indicated message or snippet instead of the default message.

All messages can be used with snippets.
Any `%s` included is automatically replaced by the game, with words depending on the message.

Case use example:

```json
{
  "type":"npc",
  "...": "rest of fields go here",
  "<acknowledged>": "I gotcha fam",
  "<camp_food_thanks>": "<food_thanks_custom>"
},
{
  "type":"snippet",
  "category":"<food_thanks_custom>",
  "text": [
    "thanks for the grub",
    "thanks for the food!",
    "itadakimasu!"
  ]
}
```

For further information on snippets, see [New Contributor Guide: Dialogue](https://github.com/CleverRaven/Cataclysm-DDA/wiki/New-Contributor-Guide-Dialogue)

### Custom Entries
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

Field | Used for...
---|---
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
`<item_name:ID>` | The name of the item with ID
`<item_description:ID>` | The description of the item with ID
`<trait_name:ID>` | The name of the trait with ID
`<trait_description:ID>` | The description of the trait with ID

---

## Talk Topics

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

#### `type`
Must always be there and must always be `"talk_topic"`.

#### `id`
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

#### `dynamic_line`
The `dynamic_line` is the line spoken by the NPC.  It is optional.  If it is not defined and the topic has the same id as a built-in topic, the `dynamic_line` from that built-in topic will be used.  Otherwise the NPC will say nothing.  [See the chapter about Dynamic Lines below](#dynamic-lines) for more details.

#### `speaker_effect`
The `speaker_effect` is an object or array of effects that will occur after the NPC speaks the `dynamic_line`, no matter which response the player chooses.  [See the chapter about Speaker Effects below](#speaker-effects)) for more details.

#### `response`
The `responses` entry is an array with possible responses.  It must not be empty.  Each entry must be a response object. [See the chapter about Responses below](#responses) for more details.

#### `replace_built_in_responses`
`replace_built_in_responses` is an optional boolean that defines whether to dismiss the built-in responses for that topic (default is `false`). If there are no built-in responses, this won't do anything. If `true`, the built-in responses are ignored and only those from this definition in the current json are used. If `false`, the responses from the current json are used along with the built-in responses (if any).

---

## Dynamic Lines
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
  "concatenate": [
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

```json
{
  "list_faction_camp_sites": true
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

#### Based on any dialogue condition
The dynamic line will be chosen based on whether a single dialogue condition is true or false.  Dialogue conditions cannot be chained via `"and"`, `"or"`, or `"not"`.  If the condition is true, the `"yes"` response will be chosen and otherwise the `"no"` response will be chosen.  Both the `'"yes"` and `"no"` responses are optional.  Simple string conditions may be followed by `"true"` to make them fields in the dynamic line dictionary, or they can be followed by the response that will be chosen if the condition is true and the `"yes"` response can be omitted.

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

The `sentinel` can be any string, but sentinels are unique to each `TALK_TOPIC`.  If there are multiple `speaker_effect`s within the `TALK_TOPIC`, they should have different sentinels.  Sentinels are not required, but since the `speaker_effect` will run every time the dialogue returns to the `TALK_TOPIC`, they are highly encouraged to avoid inadvertently repeating the same effects.

The `effect` can be any legal effect, as described below.  The effect can be a simple string, object, or an array of strings and objects, as normal for objects.

The optional `condition` can be any legal condition, as described below.  If a `condition` is present, the `effect` will only occur if the `condition` is true.

Speaker effects are useful for setting status variables to indicate that player has talked to the NPC without complicating the responses with multiple effect variables.  They can also be used, with a sentinel, to run a `mapgen_update` effect the first time the player hears some dialogue from the NPC.

---

## Responses
A response contains at least a text, which is display to the user and "spoken" by the player character (its content has no meaning for the game) and a topic to which the dialogue will switch to. It can also have a trial object which can be used to either lie, persuade or intimidate the NPC, [see below](#trials) for details. There can be different results, used either when the trial succeeds and when it fails.

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

When using a conditional you can specify the response to still appear but be marked as unavaiable. This can be done by adding a `failure_explanation` or `failure_topic` in the bellow example if the condition fails `*Didn't have enough: I, the player, say to you...` will be what appears in the responses, and if selected it will instead go to `TALK_EXPLAIN_FAILURE` and wont trigger the other effects:
```json
{
  "condition": "...something...",
  "failure_explanation": "Didn't have enough",
  "failure_topic": "TALK_EXPLAIN_FAILURE",
  "text": "I, the player, say to you...",
  "effect": "...",
  "topic": "TALK_WHATEVER"
}
```

#### `text`
Will be shown to the user, no further meaning.

Text boxes; dialogue in general is a convenient space to sprinkle in descriptive text, something that isn't necessarily being said by any interlocutor
but something the player character, npc or speaking entity express, do or generally interact with given a context
there are many ways to present this, ultimately is up to the writer, and their preferred style.

Currently you may add a `&` as the first character in dialogue, this deletes quotation round the output text, denotes the descriptive nature of the displayed
text, use `\"` escaped double quotes to indicate the start of actual dialogue.

#### `truefalsetext`
May be used in place of text.  The player will have one response text if a condition is true, and another if it is false, but the same trial for either line.  `condition`, `true`, and `false` are all mandatory.

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

#### `topic`
`topic` defines which topic the dialogue will switch to, usually specified by giving its id.

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
#### `effect`
`effect` is a function that is executed after choosing the response, [see Dialogue Effects below](#dialogue-effects) for details.

### Trials
A trial object can be used to attempt to lie to, persuade or intimidate the NPC. Different outcomes can be defined for use depending on whether the trial succeeds or fails.

#### `trial`

Optional, if not defined, `"NONE"` is used. Otherwise one of `"NONE"`, `"LIE"`, `"PERSUADE"`, `"INTIMIDATE"`, or `"CONDITION"`. If `"NONE"` is used, the `failure` object is not read, otherwise it's mandatory.

The `difficulty` is only required if type is not `"NONE"` or `"CONDITION"` and, for most trials, specifies the success chance in percent (it is however modified by various things like mutations).  Higher difficulties are easier to pass. `"SKILL_CHECK"` trials are unique, and use the difficulty as a flat comparison.

An optional `mod` array takes any of the following modifiers and increases the difficulty by the NPC's opinion of your character or personality trait for that modifier multiplied by the value: `"ANGER"`, `"FEAR"`, `"TRUST"`, `"VALUE"`, `"AGGRESSION"`, `"ALTRUISM"`, `"BRAVERY"`, `"COLLECTOR"`. The special `"POS_FEAR"` modifier treats NPC's fear of your character below 0 as though it were 0.  The special `"TOTAL"` modifier sums all previous modifiers and then multiplies the result by its value and is used when setting the owed value.

`"CONDITION"` trials take a mandatory `condition` instead of `difficulty`.  The `success` object is chosen if the `condition` is true and the `failure` is chosen otherwise.

`"SKILL_CHECK"` trials check the user's level in a skill, whose ID is read from the string object `skill_required`. The `success` object is chosen if the skill level is equal to or greater than `difficulty`, and `failure` is chosen otherwise.

Sample trials:
```json
"trial": { "type": "PERSUADE", "difficulty": 0, "mod": [ [ "TRUST", 3 ], [ "VALUE", 3 ], [ "ANGER", -3 ] ] }
"trial": { "type": "INTIMIDATE", "difficulty": 20, "mod": [ [ "FEAR", 8 ], [ "VALUE", 2 ], [ "TRUST", 2 ], [ "BRAVERY", -2 ] ] }
"trial": { "type": "CONDITION", "condition": { "npc_has_trait": "FARMER" } }
"trial": { "type": "SKILL_CHECK", "difficulty": 3, "skill_required": "swimming" }
```

#### `success` and `failure`
The `success` and `failure` objects define the outcome, depending on the result of the trial.  Both objects have the same structure; the `failure` object is used if the trial fails, the `success` object is used otherwise.

### Opinion Changes

#### `opinion`
`opinion` is optional, if given it defines how the NPC's opinion of your character will change.

trust, value, fear, and anger are optional fields inside the opinion object, each specifying a numeric value (defaults to 0). The given values are *added* to the opinion of the NPC.

The opinion of the NPC affects several aspects of the interaction with NPCs:
- Higher trust makes it easier to lie and persuade, and it usually a good thing.
- Higher fear makes it easier to intimidate, but the NPC may flee from you (and will not talk to you).
- Higher value makes it easier to persuade them and to give them orders, it's a kind of a friendship indicator.
- High anger value (about 20 points more than fear, but this also depends on the NPCs personality) makes the NPC hostile and is usually a bad thing.
The combination of fear and trust decide together with the personality of the NPC the initial talk topic (`"TALK_MUG"`, `"TALK_STRANGER_AGGRESSIVE"`, `"TALK_STRANGER_SCARED"`, `"TALK_STRANGER_WARY"`, `"TALK_STRANGER_FRIENDLY"`, or `"TALK_STRANGER_NEUTRAL"`).

For the actual usage of that data, search the source code for `"op_of_u"`.

Example opinions
```json
{ "effect": "follow", "opinion": { "trust": 1, "value": 1 }, "topic": "TALK_DONE" }
{ "topic": "TALK_DENY_FOLLOW", "effect": "deny_follow", "opinion": { "fear": -1, "value": -1, "anger": 1 } }
```

#### `mission_opinion`
Similar to `opinion`, but adjusts the NPC's opinion of your character according to the mission value. The NPC's opinion is modified by the value of the current mission divided by the value of the keyword.

### Response Availability

#### condition
This is an optional condition which can be used to prevent the response under certain circumstances. If not defined, it defaults to always `true`. If the condition is not met, the response is not included in the list of possible responses. For possible content, [see Dialogue Conditions below](#dialogue-conditions) for details.

#### switch and default
The optional boolean keys "switch" and "default" are false by default.  Only the first response with `"switch": true`, `"default": false`, and a valid condition will be displayed, and no other responses with `"switch": true` will be displayed.  If no responses with `"switch": true` and `"default":  false` are displayed, then any and all responses with `"switch": true` and `"default": true` will be displayed.  In either case, all responses that have `"switch": false` (whether or not they have `"default": true` is set) will be displayed as long their conditions are satisfied.

Example:
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

### Repeat Responses
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

## Dialogue State
Variables and information relevant to the current dialogue can be tracked using `context variables`. Accessing these is discussed further in [variable object](#variable-object).  The main thing that makes context variables special however is that they are only relevant to the current dialogue and any child dialogue / effects. When the dialogue or effect ends any context variables defined inside go out of scope (stop existing).

---

## Dialogue Effects
The `effect` field of `speaker_effect` or a `response` can be any of the following effects. Multiple effects should be arranged in a list and are processed in the order listed.

#### Missions

| Effect            | Description                                                     |
| ----------------- | --------------------------------------------------------------- |
| `assign_mission`  | Assigns a previously selected mission to your character.        |
| `mission_success` | Resolves the current mission successfully.                      |
| `mission_failure` | Resolves the current mission as a failure.                      |
| `clear_mission`   | Clears the mission from the your character's assigned missions. |
| `mission_reward`  | Gives the player the mission's reward.                          |

#### Stats / Morale

| Effect                | Description |
| --------------------- | ----------- |
| `lesser_give_aid`     | Removes bleeding from your character's body and heals 5-15 HP of injury on each of your character's body parts. |
| `lesser_give_aid_all` | Performs `lesser_give_aid` on each of your character's NPC allies in range. |
| `give_aid`            | Removes all bites, infection, and bleeding from your character's body and heals 10-25 HP of injury on each of your character's body parts. |
| `give_aid_all`        | Performs `give_aid` on each of your character's NPC allies in range.        |
| `buy_haircut`         | Gives your character a haircut morale boost for 12 hours.                   |
| `buy_shave`           | Gives your character a shave morale boost for 6 hours.                      |
| `morale_chat`         | Gives your character a pleasant conversation morale boost for 6 hours.      |
| `player_weapon_away`  | Makes your character put away (unwield) their weapon.                       |
| `player_weapon_drop`  | Makes your character drop their weapon.                                     |

#### Character effects / Mutations

Effect | Description
---|---
`u_mutate`, `npc_mutate`: int or [variable object](#variable-object), (*optional* `use_vitamins: vitamin_bool`) | Your character or the NPC will attempt to mutate, with a one in int chance of using the highest category, with 0 never using the highest category, requiring vitamins if `vitamin_bool` is true(defaults true)
`u_mutate_category`, `npc_mutate_category`: string or [variable object](#variable-object), (*optional* `use_vitamins: vitamin_bool`) | Your character or the NPC will attempt to mutate in the category, requiring vitamins if `vitamin_bool` is true(defaults true)
`u_add_effect, npc_add_effect: ` string or [variable object](#variable-object), (`duration: ` duration or [variable object](#variable-object)),(*optional* `target_part: ` [string or variable object](#variable-object) ) , (*optional* `intensity: ` int or [variable object](#variable-object) )<br/> | Your character or the NPC will gain the effect for `duration`, turns at intensity `intensity` or 0 if it was not supplied. If `force_bool` is true(defaults false) immunity will be ignored. If `target_part` is supplied that part will get the effect otherwise its a whole body effect. If `target_part` is `RANDOM` a random body part will be used. If `duration` is `"PERMANENT"`, the effect will be added permanently.
`u_add_bionic, npc_add_bionic: ` string or [variable object](#variable-object) | Your character or the NPC will gain the bionic.
`u_lose_bionic, npc_lose_bionic: ` string or [variable object](#variable-object) | Your character or the NPC will lose the bionic.
`u_add_trait, npc_add_trait: `string or [variable object](#variable-object) | Your character or the NPC will gain the trait.
`u_lose_effect, npc_lose_effect: `string or [variable object](#variable-object) | Your character or the NPC will lose the effect if they have it.
`u_lose_trait, npc_lose_trait: `string or [variable object](#variable-object) | Your character or the NPC will lose the trait.
`u_learn_martial_art, npc_learn_martial_art: `string or [variable object](#variable-object) | Your character or the NPC will learn the martial art style.
`u_forget_martial_art, npc_forget_martial_art: `string or [variable object](#variable-object) | Your character or the NPC will forget the martial art style.
`u_add_var, npc_add_var`: `var_name, type: type_str`, `context: context_str`, either `value: value_str` or `time: true` or `possible_values: string_array` | Your character or the NPC will store `value_str` as a variable that can be later retrieved by `u_has_var` or `npc_has_var`.  `npc_add_var` can be used to store arbitrary local variables, and `u_add_var` can be used to store arbitrary "global" variables, and should be used in preference to setting effects.  If `time` is used instead of `value_str`, then the current turn of the game is stored. If `possible_values` is used one of the values given at random will be used.
`u_lose_var`, `npc_lose_var`: `var_name`, `type: type_str`, `context: context_str` | Your character or the NPC will clear any stored variable that has the same `var_name`, `type_str`, and `context_str`.
`u_adjust_var, npc_adjust_var`: `var_name, type: type_str`, `context: context_str`, `adjustment: `int or [variable object](#variable-object) | Your character or the NPC will adjust the stored variable by `adjustment`.
`set_string_var`: string or [variable object](#variable-object) or array of either, `target_var: ` [variable object](#variable-object) | Store string from `set_string_var` in the variable object `target_var`. If an array is provided a random element will be used.
`set_condition`: string or [variable object](#variable-object), `condition`: a dialogue condition object. | Store the dialogue condition object `condition` as the provided name. Set conditions have the same scope as context variables and can be gotten in nested EOCs.
`u_location_variable, npc_location_variable`: `target_var`, (*optional* `min_radius: `int or [variable object](#variable-object)) , (*optional* `max_radius: ` int or [variable object](#variable-object)), (*optional* `outdoor_only: outdoor_only_bool`), (*optional* `target_params: assign_mission_target` parameters), (*optional* `z_adjust: ` int or [variable object](#variable-object)), (*optional* `x_adjust: `string or [variable object](#variable-object)), (*optional* `y_adjust: `int or [variable object](#variable-object)), (*optional* `z_override: bool`), (*optional* `terrain, furniture, trap, monster, zone, or npc: `string or [variable object](#variable-object)), (*optional* `target_min_radius: `int or [variable object](#variable-object)), (*optional* `target_max_radius: `int or [variable object](#variable-object)) | If `target_params` is defined it will be used to find a tile any of the following commands will search from that tile rather than the player or npc location. See [the missions docs](MISSIONS_JSON.md) for `assign_mission_target` parameters.  If `terrain`, `furniture`, `trap`, `monster`, or `npc` are defined then an entity with the corresponding id will be found between `target_min_radius`(defaults to 0) and `target_max_radius`(defaults to 0) before the following alterations are made to it.  If an empty string is passed for `furniture`, `trap`, `monster`, `zone`, or `npc` than any will be found.  Otherwise targets a point between `min_radius_int`( or `min_radius_variable_object`)(defaults to 0) and `max_radius_int`( or `max_radius_variable_object`)(defaults to 0) spaces of the target and if `outdoor_only_bool` is true(defaults to false) will only choose outdoor spaces. The chosen point will be saved to `target_var` which is a `variable_object`.  `z_adjust` will be used as the Z value if `z_override`(defaults false) is true or added to the current z value otherwise. x_adjust and y_adjust are added to the final position.
`location_variable_adjust`: `target_var`, (*optional* `z_adjust: ` int or [variable object](#variable-object)), (*optional* `x_adjust: `string or [variable object](#variable-object)), (*optional* `y_adjust: `int or [variable object](#variable-object)), (*optional* `z_override: bool`), (*optional* `output_var: `[variable object](#variable-object)), (*optional* `overmap_tile: bool`) | Adjusts `target_var` which is a `variable_object` most likely made using `u_location_variable`.  `z_adjust` will be used as the Z value if `z_override`(defaults false) is true or added to the current z value otherwise.  x_adjust and y_adjust are added to the final position.  If `output_var` is provided the adjusted value will be save to it instead of `target_var`.  If `overmap_tile`(defaults false) is true, the adjustments will be made in overmap tiles rather than map tiles.
`barber_hair` | Opens a menu allowing the player to choose a new hair style.
`barber_beard` | Opens a menu allowing the player to choose a new beard style.
`u_learn_recipe: `string or [variable object](#variable-object)  | Your character will learn and memorize the recipe.
`npc_first_topic: `string or [variable object](#variable-object) | Changes the initial talk_topic of the NPC in all future dialogues.
`u_add_wet, npc_add_wet: `int or [variable object](#variable-object) | Your character or the NPC will be wet as if they were in the rain.
`u_make_sound, npc_make_sound:`string or [variable object](#variable-object), `volume: `int or [variable object](#variable-object), `type: `string or [variable object](#variable-object), (*optional* `target_var: `[variable object](#variable-object)), (*optional* `snippet: snippet_bool`), (*optional* `same_snippet: same_snippet_bool`)  | A sound of description `message_string` will be made at your character or the NPC's location of volume `volume` and type `type_`. Possible types are: background, weather, music, movement, speech, electronic_speech, activity, destructive_activity, alarm, combat, alert, or order. If `target_var` is set this effect will be centered on a location saved to a variable with its name.  If `snippet_bool` is true(defaults to false) it will instead display a random snippet from `message_string` category, if `same_snippet_bool` is true(defaults to false) it will always use the same snippet and will set a variable that can be used for custom item names(this requires the snippets to have id's set)
`u_mod_healthy, npc_mod_healthy : `int or [variable object](#variable-object), `cap: ` int or [variable object](#variable-object) | Your character or the NPC will have `amount_int` added or subtracted from its health value, but not beyond `cap_int` or `cap_variable_object`.
`u_add_morale, npc_add_morale: `string or [variable object](#variable-object), (*optional* `bonus: `int or [variable object](#variable-object)), (*optional* `max_bonus: `int or [variable object](#variable-object)), (*optional* `duration: `duration or [variable object](#variable-object)), (*optional* `decay_start: `duration or [variable object](#variable-object) ), (*optional* `capped`: `capped_bool`)| Your character or the NPC will gain a morale bonus of type `morale_string`. Morale is changed by `bonus`(default 1), with a maximum of up to `max_bonus_int`(default 1). It will last for `duration` time (default 1 hour). It will begin to decay after `decay_start` time (default 30 minutes). `capped_bool` Whether this morale is capped or not, defaults to false.
`u_lose_morale, npc_lose_morale: `string or [variable object](#variable-object) | Your character or the NPC will lose any morale of type `morale_string`.
`u_add_faction_trust: `int or [variable object](#variable-object) | Your character gains trust with the speaking NPC's faction, which affects which items become available for trading from shopkeepers of that faction.
`u_lose_faction_trust: `int or [variable object](#variable-object) | Your character loses trust with the speaking NPC's faction, which affects which items become available for trading from shopkeepers of that faction.
`u_message, npc_message: `string or [variable object](#variable-object), (*optional* `sound: sound_bool`), (*optional* `outdoor_only: outdoor_only_bool`), (*optional* `snippet: snippet_bool`), (*optional* `same_snippet: snippet_bool`, (*optional* `type: `string or [variable object](#variable-object)), (*optional* `popup: popup_bool`) | Displays a message to either the player or the npc of `message_string`.  Will not display unless the player or npc is the actual player.  If `snippet_bool` is true(defaults to false) it will instead display a random snippet from `message_string` category, if `same_snippet_bool` is true(defaults to false) it will always use the same snippet and will set a variable that can be used for custom item names(this requires the snippets to have id's set).  If `sound` is true (defaults to false) it will only display the message if the player is not deaf.  `outdoor_only`(defaults to false) only matters when `sound` is true and will make the message less likely to be heard if the player is underground. Message will display as type of `type`. Type affects the color of message and can be any of the following values: good, neutral, bad, mixed, warning, info, debug, headshot, critical, grazing.  enums.h has more info on each types use. If `popup_bool` is true the message will be in a modal popup the user has to dismiss to continue.  You can use any of the  Special Custom Entries(defined above).
`u_cast_spell, npc_cast_spell : fake_spell_data`, (*optional* `true_eocs: eocs_array`), (*optional* `false_eocs: eocs_array`) | The spell described by fake_spell_data will be cast with u or the npc as the caster and u or the npc's location as the target.  Fake spell data can have the following attributes: `id:string`: the id of the spell to cast, (*optional* `hit_self`: bool ( defaults to false ) if true can hit the caster, `trigger_message`: string to display on trigger, `npc_message`: string for message if npc uses, `max_level` int max level of the spell, `min_level` int min level of the spell ).  If the spell is cast, then all of the effect_on_conditions in `true_eocs` are run, otherwise all the effect_on_conditions in `false_eocs` are run.
`u_assign_activity, npc_assign_activity: `string or [variable object](#variable-object), `duration: `duration or [variable object](#variable-object) | Your character or the NPC will start activity `u_assign_activity`. It will last for `duration` time.
`u_teleport, npc_teleport: `[variable object](#variable-object), (*optional* `success_message: `string or [variable object](#variable-object)), (*optional* `fail_message: `string or [variable object](#variable-object)), (*optional* `force: force_bool`) | u or npc are teleported to the destination stored in the variable named by `target_var`.  If the teleport succeeds and `success_message` is defined it will be displayed, if it fails and `fail_message` is defined it will be displayed.  If `force` is true any creatures at the destination will be killed and if blocked a nearby spot will be chosen to teleport to instead.
`u_set_hp, npc_set_hp : `int or [variable object](#variable-object), (*optional* `target_part: `string or [variable object](#variable-object)), (*optional* `only_increase: bool`), (*optional* `main_only: bool`), (*optional* `minor_only: bool`), (*optional* `max: bool`) | Your character or the NPC will have the hp of `target_part`(or all parts if it was not used) set to `amount`.  If `only_increase` is true (defaults to false) this will only happen if it increases the parts hp.  If `major_only` is true (defaults to false) only major body parts will be affected, if `minor_only` (defaults to false) instead only minor parts will be affected.  If `max` (defaults to false) is true `amount` will be ignored and the part will be set to its max hp.

#### Trade / Items

Effect | Description
---|---
`start_trade` | Opens the trade screen and allows trading with the NPC.
`give_equipment` | Allows your character to select items from the NPC's inventory and transfer them to your inventory.
`npc_gets_item` | Allows your character to select an item from your character's inventory and transfer it to the NPC's inventory.  The NPC will not accept it if they do not have space or weight to carry it, and will set a reason that can be referenced in a future dynamic line with `"use_reason"`.
`npc_gets_item_to_use` | Allow your character to select an item from your character's inventory and transfer it to the NPC's inventory.  The NPC will attempt to wield it and will not accept it if it is too heavy or is an inferior weapon to what they are currently using, and will set a reason that can be referenced in a future dynamic line with `"use_reason"`.
`u_spawn_item: `string or [variable object](#variable-object), (*optional* `count: `int or [variable object](#variable-object)), (*optional* `container: `string or [variable object](#variable-object)), (*optional* `use_item_group: `bool), (*optional* `suppress_message: `bool) | Your character gains the item or `count` copies of the item, contained in container if specified. If used in an NPC conversation the items are said to be given by the NPC.  If a variable item is passed for the name an item of the type contained in it will be used.  If `use_item_group` is true (defaults to false) it will instead pull an item from the item group given.  If `suppress_message` is true (defaults to false) no message will be shown.
`u_buy_item: `string or [variable object](#variable-object), `cost: `int or [variable object](#variable-object), (*optional* `count: `int or [variable object](#variable-object)), (*optional* `container: `string or [variable object](#variable-object)), (*optional* `true_eocs: eocs_array`), (*optional* `false_eocs: eocs_array`), (*optional* `use_item_group: `bool), (*optional* `suppress_message: `bool) | The NPC will sell your character the item or `count` copies of the item, contained in `container`, and will subtract `cost` from `op_of_u.owed`.  If the `op_o_u.owed` is less than `cost`, the trade window will open and the player will have to trade to make up the difference; the NPC will not give the player the item unless `cost` is satisfied.  If `use_item_group` is true (defaults to false) it will instead pull an item from the item group given.  If `suppress_message` is true (defaults to false) no message will be shown
`u_sell_item: `string or [variable object](#variable-object), (*optional* `cost: `int or [variable object](#variable-object)), (*optional* `count: `string or [variable object](#variable-object)), (*optional* `true_eocs: eocs_array`), (*optional* `false_eocs: eocs_array`) | Your character will give the NPC the item or `count` copies of the item, and will add `cost` to the NPC's `op_of_u.owed` if specified.<br/>If cost isn't present, the your character gives the NPC the item at no charge.<br/>This effect will fail if you do not have at least `count` copies of the item, so it should be checked with.  If the item is sold, then all of the effect_on_conditions in `true_eocs` are run, otherwise all the effect_on_conditions in `false_eocs` are run.
`u_bulk_trade_accept, npc_bulk_trade_accept, u_bulk_trade_accept, npc_bulk_trade_accept: `int or [variable object](#variable-object)  | Only valid after a `repeat_response`.  The player trades all instances of the item from the `repeat_response` with the NPC.  For `u_bulk_trade_accept`, the player loses the items from their inventory and gains the same value of the NPC's faction currency; for `npc_bulk_trade_accept`, the player gains the items from the NPC's inventory and loses the same value of the NPC's faction currency.  If there is remaining value, or the NPC doesn't have a faction currency, the remainder goes into the NPC's `op_of_u.owed`. If `quantity` is specified only that many items/charges will be moved.
`u_bulk_donate, npc_bulk_donate` or  `u_bulk_donate, npc_bulk_donate: `int or [variable object](#variable-object)  | Only valid after a `repeat_response`.  The player or NPC transfers all instances of the item from the `repeat_response`.  For `u_bulk_donate`, the player loses the items from their inventory and the NPC gains them; for `npc_bulk_donate`, the player gains the items from the NPC's inventory and the NPC loses them. If a value is specified only that many items/charges will be moved.
`u_spend_cash: `int or [variable object](#variable-object), (*optional* `true_eocs: eocs_array`), (*optional* `false_eocs: eocs_array`) | Remove an amount from your character's cash.  Negative values means your character gains cash.  *deprecated* NPCs should not deal in e-cash anymore, only personal debts and items. If the cash is spent, then all of the effect_on_conditions in `true_eocs` are run, otherwise all the effect_on_conditions in `false_eocs` are run.
`add_debt: mod_list` | Increases the NPC's debt to the player by the values in the `mod_list`.<br/>The following would increase the NPC's debt to the player by 1500x the NPC's altruism and 1000x the NPC's opinion of the player's value: `{ "effect": { "add_debt": [ [ "ALTRUISM", 3 ], [ "VALUE", 2 ], [ "TOTAL", 500 ] ] } }`
`u_consume_item`, `npc_consume_item: `string or [variable object](#variable-object), (*optional* `count: `int or [variable object](#variable-object)), (*optional* `charges: `int or [variable object](#variable-object)), (*optional* `popup: popup_bool`) | You or the NPC will delete the item or `count` copies of the item or `charges` charges of the item from their inventory.<br/>This effect will fail if the you or NPC does not have at least `count` copies of the item or `charges` charges of the item, so it should be checked with `u_has_items` or `npc_has_items`.<br/>If `popup_bool` is `true`, `u_consume_item` will show a message displaying the character giving the items to the NPC. It defaults to `false` if not defined, and has no effect when used in `npc_consume_item`.
`u_remove_item_with`, `npc_remove_item_with: `string or [variable object](#variable-object) | You or the NPC will delete any instances of item in inventory.<br/>This is an unconditional remove and will not fail if you or the NPC does not have the item.
`u_buy_monster: `string or [variable object](#variable-object), (*optional* `cost: `int or [variable object](#variable-object)), (*optional* `count: `int or [variable object](#variable-object)), (*optional* `name: `string or [variable object](#variable-object)), (*optional* `pacified: pacified_bool`), (*optional* `true_eocs: eocs_array`), (*optional* `false_eocs: eocs_array`) | The NPC will give your character `count` (default 1) instances of the monster as pets and will subtract `cost` from `op_of_u.owed` if specified.  If the `op_o_u.owed` is less than `cost`, the trade window will open and the player will have to trade to make up the difference; the NPC will not give the player the item unless `cost_num` is satisfied.<br/>If cost isn't present, the NPC gives your character the item at no charge.<br/>If `name` is specified the monster(s) will have the specified name. If `pacified_bool` is set to true, the monster will have the pacified effect applied.  If the monster is sold, then all of the effect_on_conditions in `true_eocs` are run, otherwise all the effect_on_conditions in `false_eocs` are run.

#### Behavior / AI

Effect | Description
---|---
`assign_guard` | Makes the NPC into a guard.  If allied and at a camp, they will be assigned to that camp.
`stop_guard` | Releases the NPC from their guard duty (also see `assign_guard`).  Friendly NPCs will return to following.
`start_camp` | The NPC will start a faction camp with the player.
`wake_up` | Wakes up sleeping, but not sedated, NPCs.
`reveal_stats` | Reveals the NPC's stats, based on the player's skill at assessing them.
`end_conversation` | Ends the conversation and makes the NPC ignore you from now on.
`insult_combat` | Ends the conversation and makes the NPC hostile, adds a message that character starts a fight with the NPC.
`hostile` | Makes the NPC hostile and ends the conversation.
`flee` | Makes the NPC flee from your character.
`follow` | Makes the NPC follow your character, joining the "Your Followers" faction.
`leave` | Makes the NPC leave the "Your Followers" faction and stop following your character.
`follow_only` | Makes the NPC follow your character without changing factions.
`stop_following` | Makes the NPC stop following your character without changing factions.
`npc_thankful` | Makes the NPC positively inclined toward your character.
`drop_weapon` | Makes the NPC drop their weapon.
`stranger_neutral` | Changes the NPC's attitude to neutral.
`start_mugging` | The NPC will approach your character and steal from your character, attacking if your character resists.
`lead_to_safety` | The NPC will gain the LEAD attitude and give your character the mission of reaching safety.
`start_training` | The NPC will train your character in a skill or martial art.  NOTE: the code currently requires that you initiate training by directing the player through `"topic": "TALK_TRAIN"` where the thing to be trained is selected.  Initiating training outside of "TALK_TRAIN" will give an error.
`start_training_npc` | The NPC will accept training from the player in a skill or martial art.
`start_training_seminar` | Opens a dialog to select which characters will participate in the training seminar hosted by this NPC.
`companion_mission: role_string` | The NPC will offer you a list of missions for your allied NPCs, depending on the NPC's role.
`basecamp_mission` | The NPC will offer you a list of missions for your allied NPCs, depending on the local basecamp.
`bionic_install` | The NPC installs a bionic from your character's inventory onto your character, using very high skill, and charging you according to the operation's difficulty.
`bionic_remove` | The NPC removes a bionic from your character, using very high skill, and charging you according to the operation's difficulty.
`npc_class_change: `string or [variable object](#variable-object) | Change the NPC's class to the new value.
`npc_faction_change: `string or [variable object](#variable-object) | Change the NPC's faction membership to the new value.
`u_faction_rep: `int or [variable object](#variable-object) | Increases your reputation with the NPC's current faction, or decreases it if the value is negative.
`toggle_npc_rule: `string or [variable object](#variable-object) | Toggles the value of a boolean NPC follower AI rule such as `"use_silent"` or `"allow_bash"`
`set_npc_rule: `string or [variable object](#variable-object) | Sets the value of a boolean NPC follower AI rule such as `"use_silent"` or `"allow_bash"`
`clear_npc_rule: `string or [variable object](#variable-object) | Clears the value of a boolean NPC follower AI rule such as `"use_silent"` or `"allow_bash"`
`set_npc_engagement_rule: `string or [variable object](#variable-object) | Sets the NPC follower AI rule for engagement distance to the value of `rule_string`.
`set_npc_aim_rule: `string or [variable object](#variable-object) | Sets the NPC follower AI rule for aiming speed to the value of `rule_string`.
`npc_die` | The NPC will die at the end of the conversation.
`u_set_goal, npc_set_goal:assign_mission_target_object`, (*optional* `true_eocs: eocs_array`), (*optional* `false_eocs: eocs_array`) | The NPC will walk to `assign_mission_target_object`. See [the missions docs](MISSIONS_JSON.md) for `assign_mission_target` parameters.  If the goal is assigned, then all of the effect_on_conditions in `true_eocs` are run, otherwise all the effect_on_conditions in `false_eocs` are run.
`u_set_guard_pos,npc_set_guard_pos` : [variable object](#variable-object), (*optional* `unique_id`: bool) | Set the NPC's guard pos to the contents of `_set_guard_pos`.  If the NPC has the `RETURN_TO_START_POS` trait then when they are idle they will attempt to move to this position.  If `unique_id`(defaults to false) is true then the NPC's `unique_id` will be added as a prefix to the variables name.  For example a guard with `unique_id` = `GUARD1` would check the variable `GUARD1_First` in the following json statement:  `{ "u_set_guard_pos": { "global_val": "_First" }, "unique_id": true }`

#### Map Updates
Effect | Description
---|---
`mapgen_update: `string or [variable object](#variable-object)<br/>`mapgen_update:` *array of string or [variable objects](#variable-object)*, (optional `assign_mission_target` parameters), (optional `target_var: `[variable object](#variable-object)), (*optional* `time_in_future: `duration or [variable object](#variable-object)), (*optional* `key: `string or [variable object](#variable-object)) | With no other parameters, updates the overmap tile at the player's current location with the changes described in `mapgen_update` (or for each `mapgen_update` entry). If `time_in_future` is set the update will happen that far in the future, in this case however the target location will be determined now and not changed even if its variables update.  The `assign_mission_target` parameters can be used to change the location of the overmap tile that gets updated.  See [the missions docs](MISSIONS_JSON.md) for `assign_mission_target` parameters and [the mapgen docs](MAPGEN.md) for `mapgen_update`. If `target_var` is set this effect will be centered on a location saved to a variable with its name instead.
`revert_location: `[variable object](#variable-object), `time_in_future: `duration or [variable object](#variable-object) | `revert_location` is a variable object of the location.  The map tile at that location will be saved (terrain,furniture and traps) and restored at `time_in_future`.  If `key` is provided it can be used with `alter_timed_events` to force it to occur early.
`alter_timed_events: `string or [variable object](#variable-object), (*optional* `time_in_future: `duration or [variable object](#variable-object)) | Will cause all future events associated with the title string as a key to occur `time_in_future` (defaults to 0) in the future.
`lightning` | Allows supercharging monster in electrical fields, legacy command for lightning weather.
`next_weather` | Forces a check for what weather it should be.
`custom_light_level: `int or [variable object](#variable-object), `length: `duration or [variable object](#variable-object), (*optional* `key: `string or [variable object](#variable-object)) | Sets the ambient light from the sun/moon to be `custom_light_level`.  This can vary naturally between 0 and 125 depending on the sun to give a scale. This lasts `length`.  If `key` is provided it can be used with `alter_timed_events` to force it to occur early.
`u_transform_radius, npc_transform_radius: `int or [variable object](#variable-object), `ter_furn_transform: `string or [variable object](#variable-object), (*optional* `target_var: `[variable object](#variable-object)), (*optional* `time_in_future: `duration or [variable object](#variable-object)), (*optional* `key: `string or [variable object](#variable-object)) | Applies the ter_furn_transform of id `ter_furn_transform` (See [the transform docs](TER_FURN_TRANSFORM.md)) in radius `transform_radius`. If `target_var` is set this effect will be centered on a location saved to a variable with its name.  If `time_in_future` is set the transform will that far in the future, in this case however the target location and radius will be determined now and not changed even if their variables update.  If `key` is provided it can be used with `alter_timed_events` to force it to occur early.
`transform_line: `string or [variable object](#variable-object), `first: `[variable object](#variable-object), `second: `[variable object](#variable-object) | Applies the ter_furn_transform of id `transform_line` (See [the transform docs](TER_FURN_TRANSFORM.md)) on a line between points `first` and `second`.
`place_override: `string or [variable object](#variable-object), `length: `duration or [variable object](#variable-object), (*optional* `key: `string or [variable object](#variable-object)) | Overrides the location name in the sidebar to instead be the title string.  Also disables map and map memory.  If `length` is set the effect will last that long.  If `key` is provided it can be used with `alter_timed_events` to force it to end early.
`u_spawn_monster, npc_spawn_monster: `string or [variable object](#variable-object), (*optional* `group: group_bool`), (*optional* `hallucination_count: `int or [variable object](#variable-object)), (*optional* `real_count: `int or [variable object](#variable-object)),(*optional* `min_radius: `int or [variable object](#variable-object)), (*optional* `max_radius: `int or [variable object](#variable-object)), (*optional* `outdoor_only: outdoor_only_bool`), (*optional* `indoor_only: indoor_only_bool`),(*optional* `open_air_allowed: open_air_allowed_bool`), (*optional* `target_range : `int or [variable object](#variable-object)), (*optional* `lifespan: `duration or [variable object](#variable-object)), (*optional* `target_var: `[variable object](#variable-object)), (*optional* `spawn_message: `string or [variable object](#variable-object)), (*optional* `spawn_message_plural: `string or [variable object](#variable-object)), (*optional* `true_eocs: eocs_array`), (*optional* `false_eocs: eocs_array`) | Spawns `real_count`(defaults to 0) monsters and `hallucination_count`(defaults to 0) hallucinations near you or the npc. The spawn will be of type `_spawn_monster`, if `group_bool` is false(defaults to false, if it is true a random monster from monster_group `_spawn_monster` will be used), if this is an empty string it will instead be a random monster within `target_range` spaces of you. The spawns will happen between `min_radius`(defaults to 1) and `max_radius`(defaults to 10) spaces of the target and if `outdoor_only` is true(defaults to false) will only choose outdoor spaces, if `indoor_only` is true(defaults to false) it will only choose indoor locations. If `open_air_allowed` is true(defaults to false) monsters can be spawned on open air. If `lifespan` is provided the monster or hallucination will only that long. If `target_var` is set this effect will be centered on a location saved to a variable with its name.  If at least one spawned creature is visible `spawn_message` will be displayed.  If `spawn_message_plural` is defined and more than one spawned creature is visible it will be used instead.  If at least one monster is spawned, then all of the effect_on_conditions in `true_eocs` are run, otherwise all the effect_on_conditions in `false_eocs` are run.
`u_spawn_npc, npc_spawn_npc: `string or [variable object](#variable-object), (*optional* `unique_id: `string or [variable object](#variable-object)), (*optional* `traits: `array of string or [variable object](#variable-object)), (*optional* `hallucination_count: `int or [variable object](#variable-object)), (*optional* `real_count: `int or [variable object](#variable-object)),(*optional* `min_radius: `int or [variable object](#variable-object)), (*optional* `max_radius: `int or [variable object](#variable-object)), (*optional* `outdoor_only: outdoor_only_bool`), (*optional* `indoor_only: indoor_only_bool`),(*optional* `open_air_allowed: open_air_allowed_bool`), (*optional* `target_range : `int or [variable object](#variable-object)), (*optional* `lifespan: `duration or [variable object](#variable-object)), (*optional* `target_var: `[variable object](#variable-object)), (*optional* `spawn_message: `string or [variable object](#variable-object)), (*optional* `spawn_message_plural: `string or [variable object](#variable-object)), (*optional* `true_eocs: eocs_array`), (*optional* `false_eocs: eocs_array`) | Spawns `real_count`(defaults to 0) npcs and `hallucination_count`(defaults to 0) hallucinations near you or the npc. The npc will be of class `_spawn_npc`, and have any members of `traits` as additional traits. If `unique_id` is defined any non hallucination npcs will have it as their unique id, if an npc with that unique id exists it will not spawn.  The spawns will happen between `min_radius`(defaults to 1) and `max_radius`(defaults to 10) spaces of the target and if `outdoor_only` is true(defaults to false) will only choose outdoor spaces, if `indoor_only` is true(defaults to false) it will only choose indoor locations. If `open_air_allowed` is true(defaults to false) monsters can be spawned on open air. If `lifespan` is provided the monster or hallucination will only that long. If `target_var` is set this effect will be centered on a location saved to a variable with its name.  If at least one spawned creature is visible `spawn_message` will be displayed.  If `spawn_message_plural` is defined and more than one spawned creature is visible it will be used instead.  If at least one monster is spawned, then all of the effect_on_conditions in `true_eocs` are run, otherwise all the effect_on_conditions in `false_eocs` are run.
`u_set_field, npc_set_field: `string or [variable object](#variable-object),(*optional* `intensity: `int or [variable object](#variable-object)), (*optional* `age: `int or [variable object](#variable-object)), (*optional* `radius: `int or [variable object](#variable-object)), (*optional* `outdoor_only: outdoor_only_bool`), (*optional* `indoor_only: indoor_only_bool`), (*optional* `hit_player : hit_player_bool`), (*optional* `target_var: `[variable object](#variable-object)) | Add a field centered on you or the npc of type `_set_field`, of intensity `intensity`( defaults to 1,) of radius `radius`(defaults to 10000000) and age `age` (defaults 1). It will only happen outdoors if `outdoor_only` is true, it defaults to false. `indoor_only` is the opposite. It will hit the player as if they entered it if `hit_player` is true, it defaults to true. If `target_var` is set this effect will be centered on a location saved to a variable with its name.

#### General
Effect | Description
---|---
`sound_effect: `string or [variable object](#variable-object), (*optional* `sound_effect_variant: `string or [variable object](#variable-object)), (*optional* `outdoor_event: outdoor_event`), (*optional* `volume: `int or [variable object](#variable-object))  | Will play a sound effect of id `sound_effect` and variant `sound_effect_variant`. If `volume` is defined it will be used otherwise 80 is the default. If `outdoor_event`(defaults to false) is true this will be less likely to play if the player is underground.
`open_dialogue`, (*optional* `topic: `string or [variable object](#variable-object)), (*optional* `true_eocs: eocs_array`), (*optional* `false_eocs: eocs_array`). | Opens up a dialog between the participants. This should only be used in effect_on_conditions. If the dialog is opened, then all of the effect_on_conditions in `true_eocs` are run, otherwise all the effect_on_conditions in `false_eocs` are run. If `topic` is supplied than a conversation with an empty talker starting with the topic `topic` will be opened.
`take_control`, (*optional* `true_eocs: eocs_array`), (*optional* `false_eocs: eocs_array`) | If the npc is a character then take control of them and then all of the effect_on_conditions in `true_eocs` are run, otherwise all the effect_on_conditions in `false_eocs` are run.
`give_achievement` string or [variable object](#variable-object), Marks the given achievement as complete.
`take_control_menu` | Opens up a menu to choose a follower to take control of.
`assign_mission: `string or [variable object](#variable-object) | Will assign mission to the player.
`remove_active_mission: `string or [variable object](#variable-object) | Will remove mission from the player's active mission list without failing it.
`finish_mission: `string or [variable object](#variable-object), (*optional* `success: success_bool` ), (*optional* `step: step_int`)  | Will complete mission to the player as a success if `success` is true, as a failure otherwise. If a `step` is provided that step of the mission will be completed.
`offer_mission: `string or [variable object](#variable-object) or array of them | Adds mission_type_id(s) to the npc's missions that they offer.  Assumes if there is no beta talker that the alpha is an NPC.
`run_eocs :` effect_on_condition_array or single effect_condition_object | Will run up all members of the `effect_on_condition_array`. Members should either be the id of an effect_on_condition or an inline effect_on_condition.
`run_eoc_with :` single effect_condition_object, `variables :` Object with variable names and values as pairs | Runs the given EOC with the provided variables as context variables.  EOC should either be the id of an effect_on_condition or an inline effect_on_condition.
`queue_eocs : effect_on_condition_array or single effect_condition_object`, `time_in_future: `duration or [variable object](#variable-object) | Will queue up all members of the `effect_on_condition_array`. Members should either be the id of an effect_on_condition or an inline effect_on_condition. Members will be run `time_in_future` in the future.  If the eoc is global the avatar will be u and npc will be invalid. Otherwise it will be queued for the current alpha if they are a character and not be queued otherwise.
`queue_eoc_with :` single effect_condition_object, `variables :` Object with variable names and values as pairs, `time_in_future: `duration or [variable object](#variable-object) | Queues the given EOC with the provided variables as context variables.  EOC should either be the id of an effect_on_condition or an inline effect_on_condition.  EOC will be run `time_in_future` in the future.  If the eoc is global the avatar will be u and npc will be invalid. Otherwise it will be queued for the current alpha if they are a character and not be queued otherwise.
`u_roll_remainder, npc_roll_remainder : `array of strings and/or [variable objects](#variable-object), `type: `string or [variable object](#variable-object), (*optional* `true_eocs: eocs_array`), (*optional* `false_eocs: eocs_array`), (*optional* `message: ` string or [variable object](#variable-object) ) | Type must be either `bionic`, `mutation`, `spell` or `recipe`.  If the u or npc does not have all of the listed bionics, mutations, spells, or recipes they will be given one randomly and and then all of the effect_on_conditions in `true_eocs` are run, otherwise all the effect_on_conditions in `false_eocs` are run.  If `message` is provided and a result is given then the `message` will be displayed as a message with the first instance of `%s` in it replaced with the name of the result selected.  
`switch : arithmetic/math_expression`, `cases: effect_array` | Will calculate value of `switch` and then run member of `cases` with the highest `case` that the `switch` is higher or equal to. `cases` is an array of objects with an int_or_var `case` and an `effect` field which is a dialog effect.
`u_run_npc_eocs or npc_run_npc_eocs : effect_on_condition_array`, (*optional* `unique_ids: `array of strings and/or [variable objects](#variable-object)), (*optional* `npcs_must_see: npcs_must_see_bool`), (*optional* `npc_range: `int or [variable object](#variable-object)), (*optional* `local: local_bool`) | Will run all members of the `effect_on_condition_array` on npcs. Members should either be the id of an effect_on_condition or an inline effect_on_condition.  If `local`(default: false) is false, then regardless of location all npcs with unique ids in the array `unique_ids` will be affected.  If `local` is true, only unique_ids listed in `unique_ids` will be affected, if it is empty all npcs in range will be effected. If a value is given for `npc_range` the npc must be that close to the source and if `npcs_must_see`(defaults to false) is true the npc must be able to see the source. For `u_run_npc_eocs` u is the source for `npc_run_npc_eocs` it is the npc.
`weighted_list_eocs: array_array` | Will choose one of a list of eocs to activate based on weight. Members should be an array of first the id of an effect_on_condition or an inline effect_on_condition and second an object that resolves to an integer weight.<br/><br/>Example: This will cause "EOC_SLEEP" 1/10 as often as it makes a test message appear.<pre>    "effect": [<br/>      {<br/>        "weighted_list_eocs": [<br/>          [ "EOC_SLEEP", { "const": 1 } ],<br/>          [ {<br/>              "id": "eoc_test2",<br/>              "effect": [ { "u_message": "A test message appears!", "type": "bad" } ]<br/>            },<br/>            { "const": 10 }<br/>          ]<br/>        ]<br/>      }<br/>    ]</pre>

#### Deprecated
Effect | Description
---|---
`deny_follow`<br/>`deny_lead`<br/>`deny_train`<br/>`deny_personal_info` | Sets the appropriate effect on the NPC for a few hours.<br/>These are *deprecated* in favor of the more flexible `npc_add_effect` described above.

#### Sample effects
```json
{ "topic": "TALK_EVAC_GUARD3_HOSTILE", "effect": [ { "u_faction_rep": -15 }, { "npc_change_faction": "hells_raiders" } ] }
{ "text": "Let's trade then.", "effect": "start_trade", "topic": "TALK_EVAC_MERCHANT" }
{ "text": "Show me what needs to be done at the camp.", "topic": "TALK_DONE", "effect": "basecamp_mission", "condition": { "npc_at_om_location": "FACTION_CAMP_ANY" } }
{ "text": "Do you like it?", "topic": "TALK_EXAMPLE", "effect": [ { "u_add_effect": "concerned", "duration": 600 }, { "npc_add_effect": "touched", "duration": 3600 }, { "u_add_effect": "empathetic", "duration": "PERMANENT" } ] }
```

---

## Dialogue Conditions
Conditions can be a simple string with no other values, a key and an int, a key and a string, a key and an array, or a key and an object. Arrays and objects can nest with each other and can contain any other condition.

The following keys and simple strings are available:

#### Boolean logic

Condition | Type | Description
--- | --- | ---
`"and"` | array | `true` if every condition in the array is true. Can be used to create complex condition tests, like `"[INTELLIGENCE 10+][PERCEPTION 12+] Your jacket is torn. Did you leave that scrap of fabric behind?"`
`"or"` | array | `true` if any condition in the array is true. Can be used to create complex condition tests, like `"[STRENGTH 9+] or [DEXTERITY 9+] I'm sure I can handle one zombie."`
`"not"` | object | `true` if the condition in the object or string is false. Can be used to create complex conditions test by negating other conditions, for text such as<br/>`"[INTELLIGENCE 7-] Hitting the reactor with a hammer should shut it off safely, right?"`

#### Player or NPC conditions
These conditions can be tested for the player using the `"u_"` form, and for the NPC using the `"npc_"` form.

example json:
```
"condition": { "u_has_strength": { "name" :"variable_name", "type":"test", "context":"documentation", "default":5 } }
```
Condition | Type | Description
--- | --- | ---
`"u_male"`<br/>`"npc_male"` | simple string | `true` if the player character or NPC is male.
`"u_female"`<br/>`"npc_female"` | simple string | `true` if the player character or NPC is female.
`"u_at_om_location"`<br/>`"npc_at_om_location"` | string or [variable object](#variable-object) | `true` if the player character or NPC is standing on an overmap tile with `u_at_om_location`'s id.  The special string `"FACTION_CAMP_ANY"` changes it to return true if the player or NPC is standing on a faction camp overmap tile.  The special string `"FACTION_CAMP_START"` changes it to return true if the overmap tile that the player or NPC is standing on can be turned into a faction camp overmap tile.
`"u_has_trait"`<br/>`"npc_has_trait"` | string or [variable object](#variable-object) | `true` if the player character or NPC has a specific trait.  Simpler versions of `u_has_any_trait` and `npc_has_any_trait` that only checks for one trait.
`"u_has_martial_art"`<br/>`"npc_has_martial_art"` | string or [variable object](#variable-object) | `true` if the player character or NPC knows a specific martial arts style.
`"u_has_flag"`<br/>`"npc_has_flag"` | string or [variable object](#variable-object) | `true` if the player character or NPC has the specified character flag.  The special trait flag `"MUTATION_THRESHOLD"` checks to see if the player or NPC has crossed a mutation threshold.
`"u_has_any_trait"`<br/>`"npc_has_any_trait"` | array of strings and/or [variable objects](#variable-object) | `true` if the player character or NPC has any trait or mutation in the array. Used to check multiple specific traits.
`"u_has_var"`, `"npc_has_var"` | string | `"type": type_str`, `"context": context_str`, and `"value": value_str` are required fields in the same dictionary as `"u_has_var"` or `"npc_has_var"`.<br/>`true` is the player character or NPC has a variable set by `"u_add_var"` or `"npc_add_var"` with the string, `type_str`, `context_str`, and `value_str`.
`"expects_vars"` | array of strings and/or [variable object](#variable-object) | `true` if each value provided is a variable that exists in the context.  Gives a debug error if this check fails.
`"u_compare_var"`, `"npc_compare_var"` | dictionary | `"type": type_str`, `"context": context_str`, `"op": op_str`, `"value"`: int or [variable object](#variable-object)  are required fields, referencing a var as in `"u_add_var"` or `"npc_add_var"`.<br/>`true` if the player character or NPC has a stored variable that is true for the provided operator `op_str` (one of `==`, `!=`, `<`, `>`, `<=`, `>=`) and value. <br/><br/>`compare_var` is deprecated, see [Math](#math) for the replacement.
`"u_compare_time_since_var"`, `"npc_compare_time_since_var_"` | dictionary | `"type": type_str`, `"context": context_str`, `"op": op_str`, `"time": time_string` are required fields, referencing a var as in `"u_add_var"` or `"npc_add_var"`.<br/>`true` if the player character or NPC has a stored variable and the current turn and that value (converted to a time point) plus the time_string is true for the provided operator `op_str` (one of `==`, `!=`, `<`, `>`, `<=`, `>=`).<br/><br/>Example: returns true if the player character has a "test", "test", "var_time_test" variable and the current turn is greater than that value plus 3 days' worth of turns.<pre>{<br/>  "u_compare_time_since_var": "test", "type": "test",<br/>  "context": "var_time_test", "op": ">", "time": "3 days"<br/>}</pre>
`"compare_string"` | array | The array must contain exactly two entries.  They can be either strings and/or [variable objects](#variable-object).  Returns true if the strings are the same.
`"u_has_strength"`<br/>`"npc_has_strength"` | int or [variable object](#variable-object) | `true` if the player character's or NPC's strength is at least the value of `u_has_strength` or `npc_has_strength`.
`"u_has_dexterity"`<br/>`"npc_has_dexterity"` | int or [variable object](#variable-object) | `true` if the player character's or NPC's dexterity is at least the value of `u_has_dexterity` or `npc_has_dexterity`.
`"u_has_intelligence"`<br/>`"npc_has_intelligence"` | int or [variable object](#variable-object) | `true` if the player character's or NPC's intelligence is at least the value of `u_has_intelligence` or `npc_has_intelligence`.
`"u_has_perception"`<br/>`"npc_has_perception"` | int or [variable object](#variable-object) | `true` if the player character's or NPC's perception is at least the value of `u_has_perception` or `npc_has_perception`.
`"u_has_hp"`<br/>`"npc_has_hp"` | int or [variable object](#variable-object) | `true` if the player character's or NPC's hp is at least the value of `u_has_hp` or `npc_has_hp`.  If optional parameter `bodypart` is supplied the hp of that part will be tested, otherwise the hp of all body parts will be summed and used.  Works on monsters.
`"u_has_part_temp"`<br/>`"npc_has_part_temp"` | int or [variable object](#variable-object) | `true` if the player character's or NPC's bodypart has at least the value of `u_has_part_temp` or `npc_has_part_temp` warmth.  Parameter `bodypart` tells what part to use.
`"u_has_item"`<br/>`"npc_has_item"` | string or [variable object](#variable-object) | `true` if the player character or NPC has something with `u_has_item`'s or `npc_has_item`'s `item_id` in their inventory.
`"u_has_items"`<br/>`"npc_has_item"` | dictionary | `u_has_items` or `npc_has_items` must be a dictionary with an `item` string or [variable object](#variable-object) and a `count` int or [variable object](#variable-object) or `charges` int or [variable object](#variable-object).<br/>`true` if the player character or NPC has at least `charges` charges or counts of `item` in their inventory.
`"u_has_item_category"`<br/>`"npc_has_item_category"` | string or [variable object](#variable-object) | `"count": `int or [variable object](#variable-object) is an optional field that must be in the same dictionary and defaults to 1 if not specified.  `true` if the player or NPC has `count` items with the same category as `u_has_item_category` or `npc_has_item_category`.
`"u_has_bionics"`<br/>`"npc_has_bionics"` | string or [variable object](#variable-object) | `true` if the player or NPC has an installed bionic with an `bionic_id` matching `"u_has_bionics"` or `"npc_has_bionics"`.  The special string "ANY" returns true if the player or NPC has any installed bionics.
`"u_has_effect"`<br/>`"npc_has_effect"`, (*optional* `intensity : int`),(*optional* `bodypart : string`) | string or [variable object](#variable-object) | `true` if the player character or NPC is under the effect with `u_has_effect` or `npc_has_effect`'s `effect_id`. If `intensity` is specified it will need to be at least that strong.  If `bodypart` is specified it will check only that bodypart for the effect.
`"u_can_stow_weapon"`<br/>`"npc_can_stow_weapon"` | simple string | `true` if the player character or NPC is wielding a weapon and has enough space to put it away.
`"u_can_drop_weapon"`<br/>`"npc_can_drop_weapon"` | simple string | `true` if the player character or NPC is wielding a weapon and can drop it on the ground, i.e. weapon isn't unwieldable like retracted bionic claws or monomolecular blade bionics.
`"u_has_weapon"`<br/>`"npc_has_weapon"` | simple string | `true` if the player character or NPC is wielding a weapon.
`"u_driving"`<br/>`"npc_driving"` | simple string | `true` if the player character or NPC is operating a vehicle.  <b>Note</b> NPCs cannot currently operate vehicles.
`"u_know_recipe"` | string or [variable object](#variable-object) | `true` if the player character knows the recipe specified in `u_know_recipe`.  It only counts as known if it is actually memorized--holding a book with the recipe in it will not count.
`"u_has_worn_with_flag"`<br/>`"npc_has_worn_with_flag"` | string or [variable object](#variable-object) | `true` if the player character or NPC is wearing something with the `u_has_worn_with_flag` or `npc_has_worn_with_flag` flag.
`"u_has_wielded_with_flag"`<br/>`"npc_has_wielded_with_flag"` | string or [variable object](#variable-object) | `true` if the player character or NPC is wielding something with the `u_has_wielded_with_flag` or `npc_has_wielded_with_flag` flag.
`"u_can_see"`<br/>`"npc_can_see"` | simple string | `true` if the player character or NPC is not blind and is either not sleeping or has the see_sleep trait.
`"u_is_deaf"`<br/>`"npc_is_deaf"` | simple string | `true` if the player character or NPC can't hear.
`"u_is_on_terrain"`<br/>`"npc_is_on_terrain"` | string or [variable object](#variable-object) | `true` if the player character or NPC is on terrain named `"u_is_on_terrain"` or `"npc_is_on_terrain"`.
`"u_is_in_field"`<br/>`"npc_is_in_field"` | string or [variable object](#variable-object) | `true` if the player character or NPC is in a field of type `"u_is_in_field"` or `"npc_is_in_field"`..
`"u_query"`<br/>`"npc_query", default : bool` | string or [variable object](#variable-object) | if the player character or NPC is the avatar will popup a yes/no query with the provided message and users response is used as the return value.  If called for a non avatar will return `default`.<br/><br/>Example:<pre>"condition": { "u_query": "Should we test?", "default": true },</pre>

#### Player Only conditions

Condition | Type | Description
--- | --- | ---
`"u_has_mission"` | string or [variable object](#variable-object) | `true` if the mission is assigned to the player character.
`"u_has_cash"` | int  or [variable object](#variable-object) | `true` if the player character has at least `u_has_cash` cash available.  *Deprecated*  Previously used to check if the player could buy something, but NPCs shouldn't use e-cash for trades anymore.
`"u_are_owed"` | int or [variable object](#variable-object) | `true` if the NPC's op_of_u.owed is at least `u_are_owed`.  Can be used to check if the player can buy something from the NPC without needing to barter anything.
`"u_has_camp"` | simple string | `true` is the player has one or more active base camps.
`"u_has_faction_trust"` | int or [variable object](#variable-object) | `true` if the player character has at least the given amount of trust with the speaker's faction.

#### Player and NPC interaction conditions

Condition | Type | Description
--- | --- | ---
`"at_safe_space" or "u_at_safe_space" or "npc_at_safe_space"` | simple string | `true` if u or the NPC's current overmap location passes the `is_safe()` test.
`"has_assigned_mission"` | simple string | `true` if the player character has exactly one mission from the NPC. Can be used for texts like "About that job...".
`"has_many_assigned_missions"` | simple string | `true` if the player character has several mission from the NPC (more than one). Can be used for texts like "About one of those jobs..." and to switch to the `"TALK_MISSION_LIST_ASSIGNED"` topic.
`"has_no_available_mission" or "npc_has_no_available_mission" or "u_has_no_available_mission"` | simple string | `true` if u or the NPC has no jobs available for the player character.
`"has_available_mission" or "u_has_available_mission" or "npc_has_available_mission"` | simple string | `true` if u or the NPC has one job available for the player character.
`"has_many_available_missions"` | simple string | `true` if the NPC has several jobs available for the player character.
`"mission_goal" or "npc_mission_goal" or "u_mission_goal"` | string or [variable object](#variable-object) | `true` if u or the NPC's current mission has the same goal as `mission_goal`.
`"mission_complete" or "npc_mission_complete" or "u_mission_complete"` | simple string | `true` if u or the NPC has completed the other's current mission.
`"mission_incomplete" or "npc_mission_incomplete" or "u_mission_incomplete"` | simple string | `true` if u or the NPC hasn't completed the other's current mission.
`"mission_failed" or "npc_mission_failed" or "u_mission_failed"` | simple string | `true` if u or the NPC has failed the other's current mission.
`"mission_has_generic_rewards"` | simple string | `true` if the NPC's current mission is flagged as having generic rewards.
`"npc_service"` | int | `true` if the NPC does not have the `"currently_busy"` effect and the player character has at least `npc_service` cash available.  Useful to check if the player character can hire an NPC to perform a task that would take time to complete.  Functionally, this is identical to `"and": [ { "not": { "npc_has_effect": "currently_busy" } }, { "u_has_cash": service_cost } ]`
`"npc_allies"` | int or [variable object](#variable-object) | `true` if the player character has at least `npc_allies` number of NPC allies _within the reality bubble_.
`"npc_allies_global"` | int or [variable object](#variable-object) | `true` if the player character has at least `npc_allies_global` number of NPC allies _anywhere_.
`"is_by_radio"` | simple string | `true` if the player is talking to the NPC over a radio.
`"u_available" or "npc_available"` | simple string | `true` if u or the NPC does not have effect `"currently_busy"`.
`"u_following" or "npc_following"` | simple string | `true` if u or the NPC is following the player character.
`"u_friend" or "npc_friend"` | simple string | `true` if u or the NPC is friendly to the player character.
`"u_hostile" or "npc_hostile"` | simple string | `true` if u or the NPC is an enemy of the player character.
`"u_train_skills" or "npc_train_skills"` | simple string | `true` if u or the NPC has one or more skills with more levels than the player.
`"u_train_styles" or "npc_train_styles"` | simple string | `true` if u or the NPC knows one or more martial arts styles that the player does not know.
`"u_has_class" or "npc_has_class"` | string or [variable object](#variable-object) | `true` if u or the NPC is a member of an NPC class.
`"u_near_om_location" or "npc_near_om_location"`, (*optional* `range : `int or [variable object](#variable-object) ) | string or [variable object](#variable-object) | same as at_om_location except it checks in a square stretching from the character range OMT's. NOTE: can only check OMT's in the reality bubble.
`"u_aim_rule" or "npc_aim_rule"` | string or [variable object](#variable-object) | `true` if u or the NPC follower AI rule for aiming matches the string.
`"u_engagement_rule" or "npc_engagement_rule"` | string or [variable object](#variable-object) | `true` if u or the NPC follower AI rule for engagement matches the string.
`"u_cbm_reserve_rule" or "npc_cbm_reserve_rule"` | string or [variable object](#variable-object) | `true` if u or the NPC follower AI rule for cbm, reserve matches the string.
`"u_cbm_recharge_rule" or "npc_cbm_recharge_rule"` | string or [variable object](#variable-object) | `true` if u or the NPC follower AI rule for cbm recharge matches the string.
`"u_rule" or "npc_rule"` | string or [variable object](#variable-object) | `true` if u or the NPC follower AI rule for that matches string is set.
`"u_override" or "npc_override"` | string or [variable object](#variable-object)| `true` if u or the NPC has an override for the string.
`"has_pickup_list" or "u_has_pickup_list" or "npc_has_pickup_list"` | simple string | `true` if u or the NPC has a pickup list.
`"roll_contested""`, `difficulty`: int or [variable object](#variable-object), (*optional* `die_size : `int or [variable object](#variable-object) ) | [int expression](#compare-integers-and-arithmetics) | Compares a roll against a difficulty. Returns true if a random number between 1 and `die_size` (defaults to 10) plus the integer expression is greater than `difficulty`. For example { "u_roll_contested": { "u_val": "strength" }, "difficulty": 6 } will return whether a random number between 1 and 10 plus strength is greater than 6.

#### NPC only conditions

Condition | Type | Description
--- | --- | ---
`"npc_role_nearby"` | string or [variable object](#variable-object) | `true` if there is an NPC with the same companion mission role as `npc_role_nearby` within 100 tiles.
`"has_reason"` | simple string | `true` if a previous effect set a reason for why an effect could not be completed.

#### Environment

Condition | Type | Description
--- | --- | ---
`"days_since_cataclysm"` | int or [variable object](#variable-object) | `true` if at least `days_since_cataclysm` days have passed since the Cataclysm.
`"is_season"` | string or [variable object](#variable-object) | `true` if the current season matches `is_season`, which must be one of "`spring"`, `"summer"`, `"autumn"`, or `"winter"`.
`"is_day"` | simple string | `true` if it is currently daytime.
`"u_is_outside"`</br>`"npc_is_outside"`  | simple string | `true` if you or the NPC is on a tile without a roof.
`"u_is_underwater"`</br>`"npc_is_underwater"`  | simple string | `true` if you or the NPC is underwater.
`"one_in_chance"` | int or [variable object](#variable-object) | `true` if a one in `one_in_chance` random chance occurs.
`"x_in_y_chance"` | object | `true` if a `x` in `y` random chance occurs. `x` and `y` are either ints  or [variable object](#variable-object).
`"is_weather"` | string or [variable object](#variable-object)  | `true` if current weather is `"is_weather"`.

#### Meta

Condition | Type | Description
--- | --- | ---
`"mod_is_loaded"` | string or [variable object](#variable-object) | `true` if the mod with the given ID is loaded.
`"get_condition"` | string or [variable object](#variable-object) | Runs the condition stored in the variable `get_condition` for the current dialogue.
`"get_game_option"` | string or [variable object](#variable-object) | gets the true or false game option for the provided string.

---

## Sample responses with conditions and effects
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
    "npc_add_var": "has_met_PC", "type": "general", "context": "examples", "value": "yes"
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

---

## Utility Structures

### Variable Object
`variable_object`: This is either an object, an `arithmetic`/`math` [expression](#compare-numbers-and-arithmetics) or array describing a variable name. It can either describe a double, a time duration or a string. If it is an array it must have 2 values the first of which will be a minimum and the second will be a maximum, the value will be randomly between the two. If it is a double `default` is a double which will be the value returned if the variable is not defined. If is it a duration then `default` can be either an int or a string describing a time span. `u_val`, `npc_val`, `context_val`, or `global_val` can be the used for the variable name element.  If `u_val` is used it describes a variable on player u, if `npc_val` is used it describes a variable on player npc, if `context_val` is used it describes a variable on the current dialogue context, if `global_val` is used it describes a global variable.  If this is a duration `infinite` will be accepted to be a virtually infinite value(it is actually more than a year, if longer is needed a code change to make this a flag or something will be needed).

Example:
```json
"effect": [ { "u_mod_focus": { "u_val":"test", "default": 1 } },
  { "u_mod_focus": [ 0, { "u_val":"test", "default": 1 } ] }
  { "u_add_morale": "morale_honey","bonus": -20,"max_bonus": -60, "decay_start": 1 },
  "duration": { "global_val": "test2", "default": "2 minutes" },
  {
    "u_spawn_monster": "mon_absence",
    "real_count": { "arithmetic": [ { "arithmetic": [ { "const":1 }, "+", { "const": 1 } ] }, "+", { "const": 1 } ] }
  }
]
```

### Mutators
`mutators`: take in an ammount of data and provide you with a relevant string. This can be used to get information about items, monsters, etc. from the id, or other data. Mutators can be used anywhere that a string [variable object](#variable-object) can be used. Mutators take the form:
```json
{ "mutator": "MUTATOR_NAME", "REQUIRED_KEY1": "REQUIRED_VALUE1", ..., "REQUIRED_KEYn": "REQUIRED_VALUEn" }
```

#### List Of Mutators
Mutator Name | Required Keys | Description
--- | --- | ---
`"mon_faction"` | `mtype_id`: String or [variable object](#variable-object). | Returns the faction of the monster with mtype_id.
`"game_option"` | `option`: String or [variable object](#variable-object). | Returns the value of the option as a string, for numerical options you should instead use the math function.


### Compare Numbers and Arithmetics
*`arithmetic` and `compare_num` are deprecated in the long term. See [Math](#math) for the replacement.*

`"compare_num"` can be used to compare two values to each other, while `"arithmetic"` can be used to take up to two values, perform arithmetic on them, and then save them in a third value. The syntax is as follows.
```json
{
  "text": "If player strength is more than or equal to 5, sets time since cataclysm to the player's focus times the player's maximum mana with at maximum a value of 15.",
  "topic": "TALK_DONE",
  "condition": { "compare_num": [ { "u_val": "strength" }, ">=", { "const": 5 } ] },
  "effect": { "arithmetic": [ { "time_since_cataclysm": "turns" }, "=", { "u_val": "focus" }, "*", { "u_val": "mana_max" } ], "max":15 }
},
```
`min` and `max` are optional double or variable_object values.  If supplied they will limit the result, it will be no lower than `min` and no higher than `max`. `min_time` and `max_time` work the same way but will parse times written as a string i.e. "10 hours".
`"compare_num"` supports the following operators: `"=="`, `"="` (Both are treated the same, as a compare), `"!="`, `"<="`, `">="`, `"<"`, and `">"`.

`"arithmetic"` supports the following operators: `"*"`(multiplication), `"/"`(divison), `"+"`(addition), `"-"`(subtraction), `"%"`(modulus), `"^"`(power) and the following results `"="`, `"*="`, `"/="`, `"+="`, `"-="`, `"%="`, `"++"`, and `"--"`

To get player character properties, use `"u_val"`. To get NPC properties, use same syntax but `"npc_val"` instead. For vars only `global_val` is also allowed.

#### List of values that can be read and/or written to

Example | Description
--- | ---
`"const": 5` | A constant value, in this case 5. Can be read but not written to.
`"time": "5 days"` | A constant time value. Will be converted to turns. Can be read but not written to.
`"time_since_cataclysm": "turns"` | Time since the start of the cataclysm in turns. Can instead take other time units such as minutes, hours, days, weeks, seasons, and years.
`"time_until_eoc": "eoc_id"` | Time from now until the next scheduled run of `eoc_id`. Optional member `unit` can sepcify a member to use instead of turns such as minutes, hours, days, weeks, seasons, and years.
`"rand": 20` | A random value between 0 and a given value, in this case 20. Can be read but not written to.
`"faction_trust": "free_merchants"` | The trust the faction has for the player (see [FACTIONS.md](FACTIONS.md)) for details.
`"faction_like": "free_merchants"` | How much the faction likes the player (see [FACTIONS.md](FACTIONS.md)) for details.
`"faction_respect": "free_merchants"` | How much the faction respects the player(see [FACTIONS.md](FACTIONS.md)) for details.
`"u_val": "strength"` | Player character's strength. Can be read but not written to. Replace `"strength"` with `"dexterity"`, `"intelligence"`, or `"perception"` to get such values.
`"u_val": "strength_base"` | Player character's strength. Replace `"strength_base"` with `"dexterity_base"`, `"intelligence_base"`, or `"perception_base"` to get such values.
`"u_val": "strength_bonus"` | Player character's current strength bonus. Replace `"strength_bonus"` with `"dexterity_bonus"`, `"intelligence_bonus"`, or `"perception_bonus"` to get such values.
`"u_val": "var"` | Custom variable. `"var_name"`, `"type"`, and `"context"` must also be specified. If `global_val` is used then a global variable will be used. If `default` is given as either an int or a variable_object then that value will be used if the variable is empty. If `default_time` is the same thing will happen, but it will be parsed as a time string aka "10 hours". Otherwise 0 will be used if the variable is empty.
`"u_val": "time_since_var"` | Time since a custom variable was set.  Unit used is turns. `"var_name"`, `"type"`, and `"context"` must also be specified.
`"u_val": "allies"` | Number of allies the character has. Only supported for the player character. Can be read but not written to.
`"u_val": "cash"` | Amount of money the character has. Only supported for the player character. Can be read but not written to.
`"u_val": "owed"` | Owed money to the NPC you're talking to.
`"u_val": "sold"` | Amount sold to the NPC you're talking to.
`"u_val": "hp"` | Amount of hp.  If `bodypart` is provided it will be for that part otherwise it will be the sum of all parts.
`"u_val": "warmth"` | Amount of warmth in a given bodypart.  `bodypart` is the id of the part to use.
`"u_val": "effect_intensity"` | Intensity of an effect.  `effect` is the id of the effect to test and `bodypart` is optionally the body part to look at.  If the effect is not present a -1 is returned.
`"u_val": "pos_x"` | Player character x coordinate. "pos_y" and "pos_z" also works as expected.
`"u_val": "power"` | Bionic power in millijoule.
`"u_val": "power_max"` | Max bionic power in millijoule. Can be read but not written to.
`"u_val": "power_percentage"` | Percentage of max bionic power. Should be a number between 0 to 100.
`"u_val": "morale"` | The current morale. Can be read but not written to for players and for monsters can be read and written to.
`"u_val": "mana"` | Current mana.
`"u_val": "mana_max"` | Max mana. Can be read but not written to.
`"u_val": "hunger"` | Current perceived hunger. Can be read but not written to.
`"u_val": "thirst"` | Current thirst.
`"u_val": "stored_kcal"` | Stored kcal in the character's body. 55'000 is considered healthy.
`"u_val": "stored_kcal_percentage"` | a value of 100 represents 55'000 kcal, which is considered healthy.
`"u_val": "item_count"` | Number of a given item in the character's inventory. `"item"` must also be specified. Can be read but not written to.
`"u_val": "exp"` | Total experience earned.
`"u_val": "addiction_intensity", "addiction": "caffeine"` | Current intensity of the given addiction. Allows for an optional field `"mod"` which accepts an integer to multiply againt the current intensity.
`"u_val": "addiction_turns", "addiction": "caffeine"` | Current duration left (in turns) for the given addiction.
`"u_val": "stim"` | Current stim level.
`"u_val": "pkill"` | Current painkiller level.
`"u_val": "rad"` | Current radiation level.
`"u_val": "focus"` | Current focus level.
`"u_val": "activity_level"` | Current activity level index, from 0-5
`"u_val": "fatigue"` | Current fatigue level.
`"u_val": "stamina"` | Current stamina level.
`"u_val": "health"` | Current health level.
`"u_val": "sleep_deprivation"` | Current sleep deprivation level.
`"u_val": "anger"` | Current anger level, only works for monsters.
`"u_val": "friendly"` | Current friendly level, only works for monsters.
`"u_val": "vitamin"` | Current vitamin level. `name` must also be specified which is the vitamins id.
`"u_val": "age"` | Current age in years.
`"u_val": "bmi_permil"` | Current BMI per mille (Body Mass Index x 1000)
`"u_val": "height"` | Current height in cm. When setting there is a range for your character size category. Setting it too high or low will use the limit instead. For tiny its 58, and 87. For small its 88 and 144. For medium its 145 and 200. For large its 201 and 250. For huge its 251 and 320.
`"u_val": "monsters_nearby"` | Number of monsters nearby. Optional params: `target_var` is a variable_object of a location variable to center the effect on, `id` is a variable_object, if its provided only monsters with this id will be counted, `radius` a variable_object of how far around the center to count from.
`"u_val": "spell_level"` | Level of a given spell. -1 means the spell is not known when read and that the spell should be forgotten if written. Optional params: `school` gives the highest level of spells known of that school (read only), `spell` reads or writes the level of the spell with matching spell id. If no parameter is provided, you will get the highest spell level of the spells you know (read only).
`"u_val": "spell_exp"` | Experience for a given spell. -1 means the spell is not known when read and that the spell should be forgotten if written. Required param: `spell` is the id of the spell in question.
`"u_val": "proficiency"` | Deals with a proficiency. Required params: `proficiency_id` is the id of the proficiency dealt with. `format` determines how the proficiency will be interacted with. `"format": <int>` will read or write how much you have trained a proficiency out of <int>. So for exaple, if you write a 5 to a proficiency using `"format": 10`, you will set the proficiency to be trained to 50%. `"format": "percent"` reads or writes how many percen done the learning is. `"format": "permille"` does likewise for permille. `"format": "total_time_required"` gives you total time required to train a given proficiency (read only). `"format": "time_spent"` deals with total time spent. `"format": "time_left"` sets the remaining time instead. For most formats possible, If the resulting time is set to equal or more than the time required to learn the proficiency, you learn it. If you read it and it gives back the total time required, it means it is learnt. Setting the total time practiced to a negative value completely removes the proficiency from your known and practiced proficiencies. If you try to read time spent on a proficiency that is not in your proficiency list, you will get back 0 seconds.
`"distance": []` | Distance between two targets. Valid targets are: "u","npc" and an object with a variable name.<br/><br/>Example:<pre>"condition": { "compare_num": [<br/>  { "distance": [ "u",{ "u_val": "stuck", "type": "ps", "context": "teleport" }  ] },<br/>  ">", { "const": 5 }<br/>] }</pre>
`"hour"` | Hours since midnight.
`"moon"` | Phase of the moon.<pre>MOON_NEW =0,<br/>WAXING_CRESCENT =1,<br/>HALF_MOON_WAXING =2,<br/>WAXING_GIBBOUS =3,<br/>FULL =4,<br/>WANING_GIBBOUS =5,<br/>HALF_MOON_WANING =6,<br/>WANING_CRESCENT =7</pre>
`"arithmetic"` | An arithmetic expression with no result.<br/><br/>Example:<pre>"real_count": { "arithmetic": [<br/>  { "arithmetic": [ { "const":1 }, "+", { "const": 1 } ] },<br/>  "+", { "const": 1 }<br/>] },</pre>

### Math
A `math` object lets you evaluate math expressions and assign them to dialogue variables or compare them for conditions. It takes the form
```JSON
{ "math": [ "2 + 2 - 3", "==", "1" ] }
```
or idiomatically
```JSON
{ "math": [ "lhs", "operator", "rhs" ] }
```
It takes an array as a parameter with 1, 2, or 3 strings:

#### One string = return value
```JSON
{ "math": [ "lhs" ] }
```
Example:
```JSON
{ "value": "LUMINATION", "add": { "math": [ "u_val('morale') * 3 - rng(0, 100)" ] } }
```
The expression in `lhs` is evaluated and passed on to the parent object.

#### Two strings = unary operation
```JSON
{ "math": [ "lhs", "operator" ] }
```
Example:
```JSON
"effect": [ { "math": [ "math_test", "++" ] } ]
```
`lhs` must be an [assignment target](#assignment-target). `operator` can be `++` or `--` to increment or decrement, respectively.

#### Three strings = assignment or comparison
```JSON
{ "math": [ "lhs", "operator", "rhs" ] }
```
If `operator` is `=`, `+=`, `-=`, `*=`, `/=`, or `%=` the operation is an assignment:
```JSON
"effect": { "math": [ "u_blorg", "=", "rng( 0, 2 ) + u_val('spell_level', 'spell: test_spell_pew') / 2" ] }
```
`lhs` must be an [assignment target](#assignment-target). `rhs` is evaluated and stored in the assignment target from `lhs`.


If `operator` is `==`, `>=`, `<=`, `>`, or `<`, the operation is a comparison:
```JSON
"condition": { "math": [ "u_val('stamina') * 2", ">=", "5000 + rand( 300 )" ] },
```
`lhs` and `rhs` are evaluated independently and the result of `operator` is passed on to the parent object.

#### Variables
Tokens that aren't numbers, [constants](#constants), [functions](#math-functions), or mathematical symbols are treated as dialogue variables. They are scoped by their name so `myvar` is a variable in the global scope, `u_myvar` is scoped on the alpha talker, `n_myvar` is scoped on the beta talker, and `_var` is a context variable.

Examples:
```JSON
    "//0": "return value of global var blorgy_counter",
    { "math": [ "blorgy_counter" ] },
    "//1": "result, x, and y are global variables",
    { "math": [ "result", "=", "x + y" ] },
    "//2": "u_z is the variable z on the alpha talker (avatar)",
    { "math": [ "result", "=", "( x + y ) * u_z" ] },
    "//3": "n_crazyness is the variable crazyness on the beta talker (npc)",
    { "math": [ "n_crazyness * 2", ">=", "( x + y ) * u_z" ] },
```

#### Constants
The tokens `π` (and `pi` alias ) and `e` are recognized as mathematical constants and get replaced by their nominal values.

#### Math functions
Common math functions are supported: 

`abs()`, `sqrt()`, `log()`, `floor()`, `trunc()`, `ceil()`, `round()`, `sin()`, `cos()`, and `tan()` take one argument, for example `sin( 2 * pi + 1 )`.

`floor( x )` returns the smallest integer not greater than x, for example `floor( 1.5 )` is 1, `floor( -1.5 )` is -2

`ceil( x )` returns the smallest integer not less than x, for example `ceil( 1.5 )` is 2, `ceil( -1.5 )` is -1

`trunc( x )` returns the nearest integer closer to zero, for example `trunc( 1.5 )` is 1, `trunc( -1.5 )` is -1

`clamp( x, lo, hi )` clamps x between lo and hi. hi must be greater than lo

`max()` and `min()` take any number of arguments, for example `max( 1, 2, 3 )`.

`rand()` takes one argument `x` and returns a random integer between 0 and x, for example `rand(100)`.

`rng()` takes two arguments `a` and `b` and returns a random floating point value between a and b, for example `rng(1.5, 2)`.

Function composition is also supported, for example `sin( rng(0, max( 0.5, u_sin_var ) ) )`

#### Dialogue functions
Dialogue functions take single-quoted strings as arguments and can return or manipulate game values. They are scoped just like [variables](#variables).

This section is a work in progress as functions are ported from `arithmetic` to `math`.

| Function | Eval | Assign |Scopes | Description |
|----------|------|--------|-------|-------------|
| game_option   |  ✅  |   ❌   | N/A<br/>(global)  | Return the numerical value of a game option<br/> Example:<br/>`"condition": { "math": [ "game_option('NPC_SPAWNTIME')", ">=", "5"] }`|
| pain     |  ✅  |   ✅   | u, n  | Return or set pain<br/> Example:<br/>`{ "math": [ "n_pain()", "=", "u_pain() + 9000" ] }`|
| skill    |  ✅  |   ✅   | u, n  | Return or set skill level<br/> Example:<br/>`"condition": { "math": [ "u_skill('driving')", ">=", "5"] }`|
| weather  |  ✅  |   ✅   | N/A<br/>(global)  | Return or set a weather aspect<br/><br/>Aspect must be one of:<br/>`temperature` (in Kelvin),<br/>`humidity` (as percentage),<br/>`pressure` (in millibar),<br/>`windpower` (in mph).<br/><br/>Temperature conversion functions are available: `celsius()`, `fahrenheit()`, `from_celsius()`, and `from_fahrenheit()`.<br/><br/>Examples:<br/>`{ "math": [ "weather('temperature')", "<", "from_fahrenheit( 33 )" ] }`<br/>`{ "math": [ "fahrenheit( weather('temperature') )", "==", "21" ] }`|

##### u_val shim
There is a `val()` shim available that can cover the missing arithmetic functions from `u_val` and `npc_val`:

```JSON
{ "math": [ "u_val('stamina')" ] }
```
where
```JSON
"u_val('stamina')"
```
is equivalent to
```JSON
{ "u_val": "stamina" }
```
Keyword-value pairs are also supported:
```JSON
"n_val('spell_level', 'spell: test_spell_pew')"
```
is equivalent to
```JSON
{ "npc_val": "spell_level", "spell": "test_spell_pew" }

```
Most of the values from [arithmetic](#list-of-values-that-can-be-read-andor-written-to) can be used with the shim, both for reading and writing.

More examples:
```JSON
    { "math": [ "u_val('age')" ] },
    { "math": [ "u_val('time: 1 d')" ] },
    { "math": [ "u_val('proficiency', 'proficiency_id: prof_test', 'format: percent')" ] }
```
#### Math functions defined in JSON
Math functions can be defined in JSON like this
```JSON
  {
    "type": "jmath_function",
    "id": "my_math_function",
    "num_args": 2,
    "return": "_0 * 2 + rand(_1)"
  },
```
where `_0`, `_1`, etc are positional parameters.

These functions can then be used like regular math functions, for example:
```JSON
  {
    "type": "effect_on_condition",
    "id": "EOC_do_something",
    "effect": [ { "math": [ "secret_value", "=", "my_math_function( u_pain(), 500)" ] } ]
  },
```
so `_0` takes the value of `u_pain()` and `_1` takes the value 500 inside `my_math_function()`

Function composition is also supported and the functions can be defined and used in any order.

#### Assignment target
An assignment target can be either a scoped [variable name](#variables) or a scoped [dialogue function](#dialogue-functions).
