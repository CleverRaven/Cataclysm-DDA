# NPCs

## Contents

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
```jsonc
{
  "type": "npc_class",
  "id": "NC_EXAMPLE",                                                      // Mandatory, unique id that refers to this class.
  "name": { "str": "Example NPC" },                                        // Mandatory, display name for this class.
  "job_description": "I'm helping you learn the game.",                    // Mandatory
  "common": false,                                                         // Optional, defaults true. Whether or not this class can appear via random generation. Randomly generated NPCs will have skills, proficiencies, and bionics applied to them as a default new player character would.
  "common_spawn_weight": 1.5,                                              // Optional (float), default 1.0 . For classes with common, this is how often they spawn. Higher numbers spawn more often.
  "sells_belongings": false,                                               // Optional. See [Shopkeeper NPC configuration](#shopkeeper-npc-configuration)
  "bonus_str": { "rng": [ -4, 0 ] },                                       // Optional. Modifies stat by the given value. This example shows a random distribution between -4 and 0.
  "bonus_dex": 100,                                                        // Optional. This example always adds exactly 100 to the stat.
  "bonus_int": { "one_in": 3 },                                            // Optional. This example adds 1 to the stat, but only about ~33.33% of the time (1 in 3).
  "bonus_per": { "sum": [ { "constant": 100 }, { "dice": [ 10, 10 ] } ] }, // Optional. This example adds 100 + 10d10 (10 dice each with 10 sides) to the stat.
  "bonus_aggression": { "rng": [ -4, 0 ] },                                // Optional. Modifies NPC's personality score like the above examples. Resulting value will be
                                                                           // clamped to within a range of -10, 10. (e.g. if aggression would be -12 it's instead set to -10)
  "bonus_bravery": 100,                                                    // Optional.
  "bonus_collector": { "one_in": 3 },                                      // Optional.
  "bonus_altruism": -10,                                                   // Optional.
  "skills": [
    {
      "skill": "ALL",                                                      // Optional. Applies bonuses/penalties to skills like the examples above. `ALL` is a special string which
                                                                           // applies to all skills not otherwise modified. See data/json/skills.json for a list of skill IDs.
      "level": { "mul": [ { "one_in": 3 }, { "sum": [ { "dice": [ 2, 2 ] }, { "constant": -2 }, { "one_in": 4 } ] } ] }
    }
  ],
  "worn_override": "NC_EXAMPLE_worn",                                      // Optional. Defines an [item group](ITEM_SPAWN.md) that replaces all of their worn items.
  "carry_override": "NC_EXAMPLE_carried",                                  // Optional. Defines an item group that replaces their carried items (in pockets, etc). Items which cannot
                                                                           // be carried will overflow(fall to the ground) when the NPC is loaded.
  "weapon_override": "NC_EXAMPLE_weapon",                                  // Optional. Defines an item group that replaces their wielded weapon.
  "bye_message_override": "<fuck_you>",                                    // Optional. If used, overrides the default bye message (picked from <bye> snippet) to any another custom snippet
  "shopkeeper_item_group": [                                               // Optional. See [Shopkeeper NPC configuration](#shopkeeper-npc-configuration) below.
    { "group": "example_shopkeeper_itemgroup1" },
    { "group": "example_shopkeeper_itemgroup2", "trust": 10 },
    { "group": "example_shopkeeper_itemgroup3", "trust": 20, "rigid": true }
    { "group": "example_shopkeeper_itemgroup3", "trust": 40, "strict": true },
    {
      "group": "example_shopkeeper_itemgroup4",
      "condition": { "compare_string": [ "yes", { "u_val": "general_examples_VIP" } ] }
    }
  ],
  "shopkeeper_consumption_rates": "basic_shop_rates",
  "shopkeeper_price_rules": [
    "price": 10000 },
  ],
  "shopkeeper_blacklist": "test_blacklist",
  "restock_interval": "6 days",
  "proficiencies": [ "prof_gunsmithing_basic", "prof_spotting" ],         // Optional. Note that prereqs do not need to be defined. NPCs of this class will learn this proficiency *and all pre-requisite proficiencies*.
  "traits": [ { "group": "BG_survival_story_EVACUEE" }, { "group": "NPC_starting_traits" }, { "group": "Appearance_demographics" } ]     // Optional
}
```

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
```jsonc
  "type": "shopkeeper_consumption_rates",
  "id": "basic_shop_rates",
  "default_rate": 5, // defined as units/day since last restock
  "junk_threshold": "10 cent", // items below this price will be consumed completely regardless of matches below
  "rates": [ // lower entries override higher ones
    { "item": "hammer", "rate": 1 },
    {
      "item": "hammer",
      "rate": 10,
      "condition": { "compare_string": [ "yes", { "npc_val": "bool_dinner_hammer_eater" } ] }
    },
    { "category": "ammo", "rate": 10 },
    { "group": "EXODII_basic_trade", "rate": 100 }
    { "group": "EXODII_basic_trade", "category": "ammo", "rate": 200 }
  ]
```
`condition` is checked with avatar as alpha and npc as beta. See [Player or NPC conditions](#player-or-npc-conditions).

#### Shopkeeper blacklists
Specifies blacklist of items that shopkeeper will not accept for trade.  Format is similar to `shopkeeper_consumption_rates`.

```jsonc
  "type": "shopkeeper_blacklist",
  "id": "basic_blacklist",
  "entries": [
    {
      "item": "hammer",
      "condition": { "compare_string": [ "yes", { "npc_val": "bool_test_hammer_hater" } ] },
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
```jsonc
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
  "death_eocs": [ "EOC_DEATH_NPC_TEST" ],
  "age": 30,
  "height": 180,
  "str": 7,
  "dex": 8,
  "int": 9,
  "per": 10,
  "personality": {
    "aggression": -1,
    "bravery":  2,
    "collector":  -3,
    "altruism":  4
  }
}
```
This is the JSON that creates the NPC ID that is used to spawn an NPC in "mapgen" (map generation). If optional fields are omitted, values are defined randomly.

| field | Description
|---    | ---
| `name_unique` | Set name of NPC.
| `name_suffix` | Set name suffix of NPC.
| `attitude`    | _(mandatory)_ Based on the enum in `npc.h`. The important ones are `0=NPCATT_NULL`, `1=NPCATT_TALK`, `3=NPCATT_FOLLOW`, `10=NPCATT_KILL`, and `11=NPCATT_FLEE`.
| `mission`     | _(mandatory)_ Based on the enum in `npc.h`. The important ones are `0=NPC_MISSION_NULL`, `3=NPC_MISSION_SHOPKEEP`, `7=NPC_MISSION_GUARD`, and `8=NPC_MISSION_GUARD_PATROL`.
| `chat`        | _(mandatory)_ Covered in the dialogue examples below.
| `faction`     | Set faction NPC belongs to (see [FACTIONS.md](FACTIONS.md)).
| `death_eocs`  | String `effect_on_condition` ids and/or inline `effect_on_condition`s (see [EFFECT_ON_CONDITION.md](EFFECT_ON_CONDITION.md)). When the npc dies all of these `eoc`s are run with the victim as u and the killer as npc.
| `age`         | Set age of NPC.
| `height`      | Set height of NPC. (in cm)
| `str`         | Set strength of NPC.
| `dex`         | Set dexterity of NPC.
| `int`         | Set intelligence of NPC.
| `per`         | Set perception of NPC.
| `personality` | Set personality of NPC. For example, positive value of `aggression` means NPC is aggressive, negative value means NPC is defensive.

---

# Writing dialogues
Dialogues work like state machines. They start with a certain topic (the NPC says something), the player character can then respond (choosing one of several responses), and that response sets the new talk topic. This goes on until the dialogue is finished, or the NPC turns hostile.

Note that it is perfectly fine to have a response that switches the topic back to itself.

NPC missions are controlled by a separate but related JSON structure and are documented in
[the missions docs](MISSIONS_JSON.md).

Two topics are special:
- `TALK_DONE` ends the dialogue immediately.
- `TALK_NONE` goes to the previously talked about topic.

If `npc` has the following fields, the game will display a dialogue with the indicated topic instead of default topic.

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

```jsonc
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
`<acknowledged>` | `<acknowledged>` | see data/json/npcs/snippets/talk_tags_follower.json
`<camp_food_thanks>` | `<camp_food_thanks>` | see data/json/npcs/snippets/talk_tags_follower.json
`<camp_larder_empty>` | `<camp_larder_empty>` | see data/json/npcs/snippets/talk_tags_follower.json
`<camp_water_thanks>` | `<camp_water_thanks>` | see data/json/npcs/snippets/talk_tags_follower.json
`<cant_flee>` | `<cant_flee>` | see data/json/npcs/snippets/talk_tags_combat.json
`<close_distance>` | `<close_distance>` | see data/json/npcs/snippets/talk_tags.json
`<combat_noise_warning>` | `<combat_noise_warning>` | see data/json/npcs/snippets/talk_tags_combat.json
`<danger_close_distance>` | `<danger_close_distance>` | see data/json/npcs/snippets/talk_combat.json
`<done_mugging>` | `<done_mugging>` | see data/json/npcs/snippets/talk_tags_combat.json
`<far_distance>` | `<far_distance>` | see data/json/npcs/snippets/talk_tags.json
`<fire_bad>` | `<fire_bad>` | see data/json/npcs/snippets/talk_tags_line.json
`<fire_in_the_hole_h>` | `<fire_in_the_hole_h>` | see data/json/npcs/snippets/talk_tags_combat.json
`<fire_in_the_hole>` | `<fire_in_the_hole>` | see data/json/npcs/snippets/talk_tags_combat.json
`<general_danger_h>` | `<general_danger_h>` | see data/json/npcs/snippets/talk_tags_combat.json
`<general_danger>` | `<general_danger>` | see data/json/npcs/snippets/talk_tags_combat.json
`<heal_self>` | `<heal_self>` | see data/json/npcs/snippets/talk_tags_combat.json
`<hungry>` | `<hungry>` | see data/json/npcs/snippets/talk_tags_follower.json
`<im_leaving_you>` | `<im_leaving_you>` | see data/json/npcs/snippets/talk_tags_follower.json
`<its_safe_h>` | `<its_safe_h>` | see data/json/npcs/snippets/talk_tags_combat.json
`<its_safe>` | `<its_safe>` | see data/json/npcs/snippets/talk_tags_combat.json
`<keep_up>` | `<keep_up>` | see data/json/npcs/snippets/talk_tags_follower.json
`<kill_npc_h>` | `<kill_npc_h>` | see data/json/npcs/snippets/talk_tags_combat.json
`<kill_npc>` | `<kill_npc>` | see data/json/npcs/snippets/talk_tags_combat.json
`<kill_player_h>` | `<kill_player_h>` | see data/json/npcs/snippets/talk_tags_combat.json
`<let_me_pass>` | `<let_me_pass>` | see data/json/npcs/snippets/talk_tags_line.json
`<lets_talk>` | `<lets_talk>` | see data/json/npcs/snippets/talk_tags_combat.json
`<medium_distance>` | `<medium_distance>` | see data/json/npcs/snippets/talk_tags.json
`<monster_warning_h>` | `<monster_warning_h>` | see data/json/npcs/snippets/talk_tags_combat.json
`<monster_warning>` | `<monster_warning>` | see data/json/npcs/snippets/talk_tags_combat.json
`<movement_noise_warning>` | `<movement_noise_warning>` | see data/json/npcs/snippets/talk_tags_combat.json
`<need_batteries>` | `<need_batteries>` | see data/json/npcs/snippets/talk_tags_follower.json
`<need_booze>` | `<need_booze>` | see data/json/npcs/snippets/talk_tags_follower.json
`<need_fuel>` | `<need_fuel>` | see data/json/npcs/snippets/talk_tags_follower.json
`<no_to_thorazine>` | `<no_to_thorazine>` | see data/json/npcs/snippets/talk_tags_line.json
`<run_away>` | `<run_away>` | see data/json/npcs/snippets/talk_tags_combat.json
`<speech_warning>` | `<speech_warning>` | see data/json/npcs/snippets/talk_tags_combat.json
`<thirsty>` | `<thirsty>` | see data/json/npcs/snippets/talk_tags_follower.json
`<wait>` | `<wait>` | see data/json/npcs/snippets/talk_tags_follower.json
`<warn_sleep>` | `<warn_sleep>` | see data/json/npcs/snippets/talk_tags_follower.json
`<yawn>` | `<yawn>` | see data/json/npcs/snippets/talk_tags_follower.json
`<yes_to_lsd>` | `<yes_to_lsd>` | see data/json/npcs/snippets/talk_tags_line.json
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
`<total_kills>` | total kills of the Player
`<time_survived>` | time since start of the game
`<topic_item>` | referenced item
`<topic_item_price>` | referenced item unit price
`<topic_item_my_total_price>` | TODO Add
`<topic_item_your_total_price>` | TODO Add
`<interval>` | displays the time remaining until restock
`<u_val:VAR>` | The user variable VAR
`<npc_val:VAR>` | The npc variable VAR
`<context_val:VAR>` | The context variable VAR
`<global_val:VAR>` | The global variable VAR
`<item_name:ID>` | The name of the item from ID
`<item_description:ID>` | The description of the item from ID
`<trait_name:ID>` | The name of the trait from ID
`<trait_description:ID>` | The description of the trait from ID
`<spell_name:ID>` | The description of the name from ID
`<spell_description:ID>` | The description of the trait from ID

item_name and similar tags, that parse the text out of the id, are able to parse the tags of variables, so it is possible to use `<item_name:<global_val:VAR>>`

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
```jsonc
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

#### `"insert_before_standard_exits"`
For mod usage to insert dialogue above the `TALK_DONE` and `TALK_NONE` lines. Defaults to false.
```jsonc
  {
    "id": "TALK_REFUGEE_Draco_1a",
    "type": "talk_topic",
    "insert_before_standard_exits": true,
    "responses": [ { "text": "Have you seen anything that could help me?", "topic": "TALK_REFUGEE_Draco_changeling_breadcrumb" } ]
  }
```

#### `id`
The topic id can be one of the built-in topics or a new id. However, if several talk topics *in json* have the same id, the last topic definition will override the previous ones.

The topic id can also be an array of strings. This is loaded as if several topics with the exact same content have been given in json, each associated with an id from the `id`, array. Note that loading from json will append responses and, if defined in json, override the `dynamic_line` and the `replace_built_in_responses` setting. This allows adding responses to several topics at once.

This example adds the "I'm going now!" response to all the listed topics.
```jsonc
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

Note that if `dynamic_line` is an object and contains a `str` member, it is treated as a translation object in which you can specifiy the translation context, translator comments, and so on.

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
```jsonc
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
```jsonc
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
```jsonc
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

```jsonc
{
  "give_hint": true
}
```

#### A list of potential faction camp sites
The dynamic line will list all of the possible starting sites for faction camps.

```jsonc
{
  "list_faction_camp_sites": true
}
```

#### Based on a previously generated reason
The dynamic line will be chosen from a reason generated by an earlier effect.  The reason will be cleared.  Use of it should be gated on the `"has_reason"` condition.

```jsonc
{
  "has_reason": { "use_reason": true },
  "no": "What is it, boss?"
}
```

#### Based on any dialogue condition
The dynamic line will be chosen based on whether a single dialogue condition is true or false.  Dialogue conditions cannot be chained via `"and"`, `"or"`, or `"not"`.  If the condition is true, the `"yes"` response will be chosen and otherwise the `"no"` response will be chosen.  Both the `'"yes"` and `"no"` responses are optional.  Simple string conditions may be followed by `"true"` to make them fields in the dynamic line dictionary, or they can be followed by the response that will be chosen if the condition is true and the `"yes"` response can be omitted.

```jsonc
{
  "npc_need": "sleepiness",
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
  "math": [ "time_since('cataclysm', 'unit':'days') >= 30" ],
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
```jsonc
"speaker_effect": {
  "sentinel": "...",
  "condition": "...",
  "effect": "..."
}
```
or:
```jsonc
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
A response contains at least a text, which is display to the user and "spoken" by the player character (its content has no meaning for the game) and a topic to which the dialogue will switch to. The "condition" field dictates whether the response will be shown. It can also have a trial object which can be used to either lie, persuade, or intimidate the NPC, or even test another conditional [see below](#trials) for details; different "effect"s can be applied for trial success/failure.

Format:
```jsonc
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
```jsonc
{
  "text": "I, the player, say to you...",
  "effect": "...",
  "topic": "TALK_WHATEVER"
}
```
The short format is equivalent to (an unconditional switching of the topic, `effect` is optional):
```jsonc
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

When using a conditional you can specify the response to still appear but be marked as unavailable. This can be done by adding a `failure_explanation` or `failure_topic` in the bellow example if the condition fails `*Didn't have enough: I, the player, say to you...` will be what appears in the responses, and if selected it will instead go to `TALK_EXPLAIN_FAILURE` and wont trigger the other effects:
```jsonc
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

```jsonc
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

```jsonc
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
```jsonc
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
```jsonc
{ "effect": "follow", "opinion": { "trust": 1, "value": 1 }, "topic": "TALK_DONE" }
{ "topic": "TALK_DENY_FOLLOW", "effect": "deny_follow", "opinion": { "fear": -1, "value": -1, "anger": 1 } }
```

#### `mission_opinion`
Similar to `opinion`, but adjusts the NPC's opinion of your character according to the mission value. The NPC's opinion is modified by the value of the current mission divided by the value of the keyword.

### Response Availability

#### `condition`
This is an optional condition which can be used to prevent the response under certain circumstances. If not defined, it defaults to always `true`. If the condition is not met, the response is not included in the list of possible responses. For possible content, [see Dialogue Conditions below](#dialogue-conditions) for details.

#### `switch and default`
The optional boolean keys "switch" and "default" are false by default.  Only the first response with `"switch": true`, `"default": false`, and a valid condition will be displayed, and no other responses with `"switch": true` will be displayed.  If no responses with `"switch": true` and `"default":  false` are displayed, then any and all responses with `"switch": true` and `"default": true` will be displayed.  In either case, all responses that have `"switch": false` (whether or not they have `"default": true` is set) will be displayed as long their conditions are satisfied.

#### `show_condition`
An optional key that, if defined and evaluates to true, will allow the response to be displayed even if it has a condition evaluating false. The response still will not be selectable if its condition is false, unless debug mode is ON. Cannot be defined if `show_always: true`. Note: do not confuse `show_condition` with `condition`. Empty by default.

#### `show_always`
Shorthand for "show_condition": takes a boolean instead of a condition. False by default.

#### `show_reason`
An optional key that, if defined, will append its contents to the end of a response displayed because of `show_always` or `show_condition`. Empty by default.

Example:
```jsonc
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
```jsonc
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
| `finish_mission`  | Resolves a specific mission defined by ID                       |
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

[See EFFECT_ON_CONDITION.md, #Character effects](EFFECT_ON_CONDITION.md##Character-effects)

#### Trade / Items

Effect | Description
---|---
`start_trade` | Opens the trade screen and allows trading with the NPC.
`give_equipment` | Allows your character to select items from the NPC's inventory and transfer them to your inventory.
`npc_gets_item` | Allows your character to select an item from your character's inventory and transfer it to the NPC's inventory.  The NPC will not accept it if they do not have space or weight to carry it, and will set a reason that can be referenced in a future dynamic line with `"use_reason"`.
`npc_gets_item_to_use` | Allow your character to select an item from your character's inventory and transfer it to the NPC's inventory.  The NPC will attempt to wield it and will not accept it if it is too heavy or is an inferior weapon to what they are currently using, and will set a reason that can be referenced in a future dynamic line with `"use_reason"`.
`u_spawn_item: `string or [variable object](#variable-object), (*optional* `count: `int or [variable object](#variable-object)), (*optional* `container: `string or [variable object](#variable-object)), (*optional* `use_item_group: `bool), (*optional* `suppress_message: `bool), (*optional* `flags: `array of string or [variable object](#variable-object)) | Your character gains the item or `count` copies of the item, contained in `container` if specified. If used in an NPC conversation the items are said to be given by the NPC.  If a variable item is passed for the name an item of the type contained in it will be used.  If `use_item_group` is true (defaults to false) , it will instead create items from the item group given. ("count" and "container" will be ignored since they are defined in the item group.)  If `suppress_message` is true (defaults to false) no message will be shown. If `force_equip` is true (defaults to false) characters will equip the items if they can.  The item will have all the flags from the array `flags`.
`u_buy_item: `string or [variable object](#variable-object), `cost: `int or [variable object](#variable-object), (*optional* `count: `int or [variable object](#variable-object)), (*optional* `container: `string or [variable object](#variable-object)), (*optional* `true_eocs: eocs_array`), (*optional* `false_eocs: eocs_array`), (*optional* `use_item_group: `bool), (*optional* `suppress_message: `bool), (*optional* `flags: `array of string or [variable object](#variable-object)) | The NPC will sell your character the item or `count` copies of the item, contained in `container`, and will subtract `cost` from `op_of_u.owed`.  If the `op_o_u.owed` is less than `cost`, the trade window will open and the player will have to trade to make up the difference; the NPC will not give the player the item unless `cost` is satisfied.  If `use_item_group` is true (defaults to false) , it will instead create items from the item group given. ("count" and "containter" will be ignored since they are defined in the item group.)  If `suppress_message` is true (defaults to false) no message will be shown.  The item will have all the flags from the array `flags`.
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
`distribute_food_auto` | The NPC will immediately distribute all food on their tile and adjacent tiles into the local camp's larder. Requires that such a camp exists and that the NPC has access to that camp.
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

[Map Updates](EFFECT_ON_CONDITION.md##Map_Updates)

#### General

[Map Updates](EFFECT_ON_CONDITION.md##General)

#### Deprecated

Effect | Description
---|---
`deny_follow`<br/>`deny_lead`<br/>`deny_train`<br/>`deny_personal_info` | Sets the appropriate effect on the NPC for a few hours.<br/>These are *deprecated* in favor of the more flexible `npc_add_effect` described above.

#### Sample effects
```jsonc
{ "topic": "TALK_EVAC_GUARD3_HOSTILE", "effect": [ { "u_faction_rep": -15 }, { "npc_change_faction": "hells_raiders" } ] }
{ "text": "Let's trade then.", "effect": "start_trade", "topic": "TALK_EVAC_MERCHANT" }
{ "text": "Show me what needs to be done at the camp.", "topic": "TALK_DONE", "effect": "basecamp_mission", "condition": { "npc_at_om_location": "FACTION_CAMP_ANY" } }
{ "text": "Do you like it?", "topic": "TALK_EXAMPLE", "effect": [ { "u_add_effect": "concerned", "duration": 600 }, { "npc_add_effect": "touched", "duration": 3600 }, { "u_add_effect": "empathetic", "duration": "PERMANENT" } ] }
```

---

## Dialogue Conditions

[Conditions](EFFECT_ON_CONDITION.md#Condition)

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
`"at_safe_space" or "u_at_safe_space" or "npc_at_safe_space"` | simple string | `true` if the only monsters present in the talker's OMT are in a monster group with `is_safe`. False otherwise.
`"has_assigned_mission"` | simple string | `true` if the player character has exactly one mission from the NPC. Can be used for texts like "About that job...".
`"has_many_assigned_missions"` | simple string | `true` if the player character has several mission from the NPC (more than one). Can be used for texts like "About one of those jobs..." and to switch to the `"TALK_MISSION_LIST_ASSIGNED"` topic.
`"has_no_available_mission" or "npc_has_no_available_mission" or "u_has_no_available_mission"` | simple string | `true` if u or the NPC has no jobs available for the player character.
`"has_available_mission" or "u_has_available_mission" or "npc_has_available_mission"` | simple string | `true` if u or the NPC has one job available for the player character.
`"has_many_available_missions"` | simple string | `true` if the NPC has several jobs available for the player character.
`"mission_goal" or "npc_mission_goal" or "u_mission_goal"` | string or [variable object](#variable-object) | `true` if u or the NPC's current mission has the same goal as `mission_goal`.
`"u_has_activity" or "npc_has_activity" | simple string | `true` if the [selected talker](EFFECT_ON_CONDITION.md#alpha-and-beta-talkers) is currently performing an [activity](PLAYER_ACTIVITY.md).
`"u_is_travelling" or "npc_is_travelling" | simple string | `true` if the [selected talker](EFFECT_ON_CONDITION.md#alpha-and-beta-talkers) has a current destination. Note that this just checks the destination exists, not whether u or npc is actively moving to it.
`"mission_complete" or "npc_mission_complete" or "u_mission_complete"` | simple string | `true` if u or the NPC has completed the other's current mission.
`"mission_incomplete" or "npc_mission_incomplete" or "u_mission_incomplete"` | simple string | `true` if u or the NPC hasn't completed the other's current mission.
`"mission_failed" or "npc_mission_failed" or "u_mission_failed"` | simple string | `true` if u or the NPC has failed the other's current mission.
`"mission_has_generic_rewards"` | simple string | `true` if the NPC's current mission is flagged as having generic rewards.
`"npc_service"` | int or [variable object](#variable-object) | `true` if the NPC does not have the `"currently_busy"` effect and the player character has at least `npc_service` cash available.  Useful to check if the player character can hire an NPC to perform a task that would take time to complete.  Functionally, this is identical to `"and": [ { "not": { "npc_has_effect": "currently_busy" } }, { "u_has_cash": service_cost } ]`
`"npc_allies"` | int or [variable object](#variable-object) | `true` if the player character has at least `npc_allies` number of NPC allies _within the reality bubble_.
`"npc_allies_global"` | int or [variable object](#variable-object) | `true` if the player character has at least `npc_allies_global` number of NPC allies _anywhere_.
`"is_by_radio"` | simple string | `true` if the player is talking to the NPC over a radio.
`"u_available" or "npc_available"` | simple string | `true` if u or the NPC does not have effect `"currently_busy"`.
`"u_following" or "npc_following"` | simple string | `true` if u or the NPC is following the player character.
`"u_friend" or "npc_friend"` | simple string | `true` if u or the NPC is friendly to the player character.
`"u_hostile" or "npc_hostile"` | simple string | `true` if u or the NPC is an enemy of the player character.
`"u_train_skills" or "npc_train_skills"` | simple string | `true` if u or the NPC has one or more skills with more levels than the player.
`"u_train_styles" or "npc_train_styles"` | simple string | `true` if u or the NPC knows one or more martial arts styles that the player does not know.
`"u_has_class" or "npc_has_class"` | string or [variable object](#variable-object) | `true` if u or the NPC's class id matches the provided string (e.g. `NC_BANDIT_TRADER`).
`"u_near_om_location" or "npc_near_om_location"`, (*optional* `range : `int or [variable object](#variable-object) ) | string or [variable object](#variable-object) | same as at_om_location except it checks in a square stretching from the character range OMT's. NOTE: can only check OMT's in the reality bubble (maximum of ~2 OMTs distance from player's position)
`"u_aim_rule" or "npc_aim_rule"` | string or [variable object](#variable-object) | `true` if u or the NPC follower AI rule for aiming matches the string.
`"u_engagement_rule" or "npc_engagement_rule"` | string or [variable object](#variable-object) | `true` if u or the NPC follower AI rule for engagement matches the string.
`"u_cbm_reserve_rule" or "npc_cbm_reserve_rule"` | string or [variable object](#variable-object) | `true` if u or the NPC follower AI rule for cbm, reserve matches the string.
`"u_cbm_recharge_rule" or "npc_cbm_recharge_rule"` | string or [variable object](#variable-object) | `true` if u or the NPC follower AI rule for cbm recharge matches the string.
`"u_rule" or "npc_rule"` | string or [variable object](#variable-object) | `true` if u or the NPC follower AI rule for that matches string is set.
`"u_override" or "npc_override"` | string or [variable object](#variable-object)| `true` if u or the NPC has an override for the string.
`"has_pickup_list" or "u_has_pickup_list" or "npc_has_pickup_list"` | simple string | `true` if u or the NPC has a pickup list.
`"roll_contested""`, `difficulty`: int or [variable object](#variable-object), (*optional* `die_size : `int or [variable object](#variable-object) ) | [math expression](#math) | Compares a roll against a difficulty. Returns true if a random number between 1 and `die_size` (defaults to 10) plus the integer expression is greater than `difficulty`. For example { "u_roll_contested": { "math": [ "u_val('strength')" ] }, "difficulty": 6 } will return whether a random number between 1 and 10 plus strength is greater than 6.

#### NPC only conditions

Condition | Type | Description
--- | --- | ---
`"npc_role_nearby"` | string or [variable object](#variable-object) | `true` if there is an NPC with the same companion mission role as `npc_role_nearby` within 100 tiles.
`"has_reason"` | simple string | `true` if a previous effect set a reason for why an effect could not be completed.

#### Environment

Condition | Type | Description
--- | --- | ---
`"is_season"` | string or [variable object](#variable-object) | `true` if the current season matches `is_season`, which must be one of "`spring"`, `"summer"`, `"autumn"`, or `"winter"`.
`"is_day"` | simple string | `true` if it is currently daytime (sun is at or above the [civil dawn](https://en.wikipedia.org/wiki/Dawn#Civil_dawn) point)
`"u_is_outside"`<br/>`"npc_is_outside"`<br/>`"is_outside"`  | simple string or object | `true` if you or the NPC is on a tile without a roof. if `is_outside`, you can pass a variable with coordinates to check if this tile is outside, like `{ "is_outside": { "context_val": "loc" } }`
`"u_is_underwater"`<br/>`"npc_is_underwater"`  | simple string | `true` if you or the NPC is underwater.
`"one_in_chance"` | int or [variable object](#variable-object) | `true` if a one in `one_in_chance` random chance occurs.
`"x_in_y_chance"` | object | `true` if a `x` in `y` random chance occurs. `x` and `y` are either ints  or [variable object](#variable-object).
`"is_weather"` | string or [variable object](#variable-object)  | `true` if current weather is `"is_weather"`.

#### Meta

Condition | Type | Description
--- | --- | ---
`"mod_is_loaded"` | string or [variable object](#variable-object) | `true` if the mod with the given ID is loaded.
`"get_condition"` | string or [variable object](#variable-object) | Runs the condition stored in the variable `get_condition` for the current dialogue.

---

## Sample responses with conditions and effects
```jsonc
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
      "compare_string": [ "yes", { "npc_val": "general_examples_has_met_PC" } ]
    }
  },
  "effect": {
    "npc_add_var": "general_examples_has_met_PC", "value": "yes"
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
  "condition": { "not": { "compare_string": [ "yes", { "u_val": "sentinel_old_guard_rep_asked_about_vault" } ] } },
  "effect": [
    { "u_add_var": "sentinel_old_guard_asked_about_vault", "value": "yes" },
    { "mapgen_update": "hulk_hairstyling", "om_terrain": "necropolis_a_13", "om_special": "Necropolis", "om_terrain_replace": "field", "z": 0 }
  ]
},
{
  "text": "Why do zombies keep attacking every time I talk to you?",
  "topic": "TALK_RUN_AWAY_MORE_ZOMBIES",
  "condition": { "compare_string": [ "yes", { "u_val": "trigger_learning_experience_even_more_zombies" } ] },
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
`variable_object`: This is either an object, a `math` [expression](#math) or array describing a variable name. It can either describe a double, a time duration or a string. If it is an array it must have 2 values the first of which will be a minimum and the second will be a maximum, the value will be randomly between the two. If it is a double `default` is a double which will be the value returned if the variable is not defined. If is it a duration then `default` can be either an int or a string describing a time span. `u_val`, `npc_val`, `context_val`, `var_val` or `global_val` can be the used for the variable name element.  If `u_val` is used it describes a variable on player u, if `npc_val` is used it describes a variable on player npc, if `context_val` is used it describes a variable on the current dialogue context, `var_val` tries to resolve a variable stored in a `context_val` (more explanation bellow), if `global_val` is used it describes a global variable.  If this is a duration `infinite` will be accepted to be a virtually infinite value(it is actually more than a year, if longer is needed a code change to make this a flag or something will be needed).

Example:
```jsonc
"effect": [ { "u_mod_focus": { "u_val":"test", "default": 1 } },
  { "u_mod_focus": [ 0, { "u_val":"test", "default": 1 } ] }
  { "u_add_morale": "morale_honey","bonus": -20,"max_bonus": -60, "decay_start": 1 },
  "duration": { "global_val": "test2", "default": "2 minutes" },
  {
    "u_spawn_monster": "mon_absence", 
    "real_count": { "math": [ "1 + rand(2)" ] }
  }
]
```

#### var_val
var_val is a unique variable object in the fact that it attempts to resolve the variable stored inside a context variable. The values for var_val use the same syntax for scope that math [variables](#variables) do. 

| Prefix of the value in var_val| Resolved as      |
|------------------|------------------|
| No Prefix or `g_`| global_val       |
| `_`              | context_val      |
| `u_`             | u_val            |
| `n_`             | npc_val          |
| `v_`             | var_val          |

In practice, `{ "var_val": "name" }` can be understood as `{ "global_val/context_val/u_val/npc_val": { "context_val": "name" } }`.

So if you had

| Name | Type | Value |
| --- | --- | --- |
| ref | context_val | key1 |
| ref2 | context_val | u_key2 |
| key1 | global_val | SOME TEXT |
| key2 | u_val | SOME OTHER TEXT |

If you access "ref" as a context val it will have the value of "key1", if you access it as a var_val it will have a value of "SOME TEXT". 
If you access "ref2" as a context val it will have the value of "u_key2", if you access it as a var_val it will have a value of "SOME OTHER TEXT". 

### Mutators
`mutators`: take in an amount of data and provide you with a relevant string. This can be used to get information about items, monsters, etc. from the id, or other data. Mutators can be used anywhere that a string [variable object](#variable-object) can be used. Mutators take the form:
```jsonc
{ "mutator": "MUTATOR_NAME", "REQUIRED_KEY1": "REQUIRED_VALUE1", ..., "REQUIRED_KEYn": "REQUIRED_VALUEn" }
```

#### List Of Mutators

Mutator Name | Required Keys | Description
--- | --- | ---
`"mon_faction"` | `mtype_id`: String or [variable object](#variable-object). | Returns the faction of the monster with mtype_id.
`"game_option"` | `option`: String or [variable object](#variable-object). | Returns the value of the option as a string, for numerical options you should instead use the math function.
`"ma_technique_name"` | `matec_id`: String or [variable object](#variable-object). | Returns the name of the martial arts tech with ID `matec_id` 
`"ma_technique_description"` | `matec_id`: String or [variable object](#variable-object). | Returns the description of the martial arts tech with ID `matec_id` 
`"valid_technique`" | `blacklist`: array of String or [variable object](#variable-object). <br/> `crit`: bool <br/> `dodge_counter`: bool <br/> `block_counter`: bool | Returns a random valid technique for the alpha talker to use against the beta talker with the provided specifications.
`"u_/npc_loc_relative"` | `target`: String or [variable object](#variable-object). | target should be a string like "(x,y,z)" where x,y,z are coordinates relative to the player. Returns the abs_ms coordinates as a string (ready to store as a location variable), in the form "(x,y,z)" of the provided point relative to the alpha/beta talker respectively. So `"target":"(0,1,0)"` would return the point south of the talker.
`"topic_item"` | | Returns current topic_item as a string. See [Repeat Responses](#repeat-responses)

### Math
A `math` object lets you evaluate math expressions and assign them to dialogue variables or compare them for conditions. It takes the form
```jsonc
{ "math": [ "2 + 2 - 3 == 1" ] }
```
or idiomatically
```jsonc
{ "math": [ "lhs operator rhs" ] }
```
Example:
```jsonc
{ "value": "LUMINATION", "add": { "math": [ "u_val('morale') * 3 - rng(0, 100)" ] } }
```
The expression in `lhs` is evaluated and passed on to the parent object.


The expression can be split into any number of strings in order to get neat line breaks:
```jsonc
    "effect": [
      {
        "math": [
          "BILE_IRRITANT_DURATION",
          "=",
          "u_effect_duration('bile_irritant') > 0 ? u_effect_duration('bile_irritant') + 120 + rand(90) : 240 + rand(180)"
        ]
      }
    ]
```

The assignment `operators` `=`, `+=`, `-=`, `*=`, `/=`, `%=`, `++`, and `--` can only be used at the top level. In this case, `lhs` must be a single [assignment target](#assignment-target). The `rhs` expression is evaluated and stored in the assignment target from `lhs`.

#### Variables
Tokens that aren't numbers, [constants](#constants), [functions](#math-functions), or mathematical symbols are treated as dialogue variables. They are scoped by their name so `myvar` is a variable in the global scope, `u_myvar` is scoped on the alpha talker, `n_myvar` is scoped on the beta talker, and `_var` is a context variable, `v_var` is a [var_val](#var_val).

Examples:
```jsonc
    "//0": "return value of global var blorgy_counter",
    { "math": [ "blorgy_counter" ] },
    "//1": "result, x, and y are global variables",
    { "math": [ "result = x + y" ] },
    "//2": "u_z is the variable z on the alpha talker (avatar)",
    { "math": [ "result = ( x + y ) * u_z" ] },
    "//3": "n_crazyness is the variable craziness on the beta talker (npc)",
    { "math": [ "n_crazyness * 2 >= ( x + y ) * u_z" ] },
```

#### Tripoints
Members of tripoint variables can be accessed with the dot operator in the usual way
```jsonc
      { "u_location_variable": { "u_val": "mypos" } },
      { "math": [ "u_myx = u_mypos.x" ] },
      { "math": [ "u_myy = u_mypos.y" ] },
      { "math": [ "u_myz = u_mypos.z" ] },
      // assignment works too
      { "math": [ "u_mypos.z = 12" ] },
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

#### Ternary and inline boolean operators
Inline [comparison operators](#three-strings--assignment-or-comparison) evaluate as 1 for true and 0 for false.

Ternary operators take the form `condition ? true_value : false_value`. They are right-associative so a chained ternary like `a ? b : c ? d :e` is parsed as `a ? b : (c ? d : e)`.

Examples:
```jsonc
    "//0": "returns 5 if u_blorg is greater than 4, otherwise 0",
    { "math": [ "( u_blorg > 4 ) * 5" ] },
    "//1": "returns rng( 0.5, 5 ) if u_blorg is greater than 5, otherwise rand(100)"
    { "math": [ "u_blorg > 5 ? rng( 0.5, 5 ) : rand(100)" ] },
```

#### Dialogue functions
Dialogue functions return or manipulate game values. They are scoped just like [variables](#variables).

These functions support keyword-value pairs as optional arguments (kwargs) of the form `'keyword': argument`.

_function arguments are `d`oubles (or sub-expressions), `s`trings, or `v`[ariables](#variables)_<br/>
_some functions support array arguments or kwargs, denoted with square brackets [] (ex: [`d`/`v`])_

| Function | Eval | Assign |Scopes | Description |
|----------|------|--------|-------|-------------|
| armor(`s`/`v`,`s`/`v`)    |  ✅   |   ❌  | u, n  | Return the numerical value for a characters armor on a body part, for a damage type.<br/>Arguments are damagetype ID, bodypart ID.<br/><br/>Example:<br/>`"condition": { "math": [ "u_armor('bash', 'torso') >= 5"] }`|  
| attack_speed()    |  ✅   |   ❌  | u, n  | Return the characters current adjusted attack speed with their current weapon.<br/><br/>Example:<br/>`"condition": { "math": [ "u_attack_speed() >= 10"] }`| 
| speed()    |  ✅   |   ❌  | u, n  | Return the character or monster current movement speed, in moves (default character speed is 100).<br/><br/>Example:<br/>`"condition": { "math": [ "u_speed() >= 10"] }`| 
| addiction_intensity(`s`/`v`)    |  ✅   |   ❌  | u, n  | Return the characters current intensity of the given addiction.<br/>Argument is addiction type ID.<br/><br/>Example:<br/>`"condition": { "math": [ "u_addiction_intensity('caffeine') >= 1"] }`|
| addiction_turns(`s`/`v`)    |  ✅   |   ✅  | u, n  | Return the characters current duration left (in turns) for the given addiction.<br/>Argument is addiction type ID. <br/><br/>Example:<br/>`"condition": { "math": [ "u_addiction_turns('caffeine') >= 3600"] }`|
| characters_nearby()     |  ✅  |   ❌   | u, n, global  | Return the number of nearby characters (that is, the avatar and/or NPCs who are not hallucinations). <br/><br/>Optional kwargs:<br/>`radius`: `d`/`v` - limit to radius (rl_dist)<br/>`location`: `v` - center search on this location<br/>`attitude`: `s`/`v` - attitude filter. Must be one of `hostile`, `allies`, `not_allies`, `any`. Assumes `any` if not specified<br/><br/>The `location` kwarg is mandatory in the global scope.<br/>`allow_hallucinations`: `d`/`v` - False by default. If set to any non-zero number, all hallucinated NPCs in range will be counted as well.<br/><br/>Examples:<br/>`"condition": { "math": [ "u_characters_nearby('radius': u_search_radius * 3, 'attitude': 'not_allies' ) > 0" ] }`<br/><br/>`"condition": { "math": [ "characters_nearby( 'radius': u_search_radius * 3, 'location': u_search_loc) > 5" ] }`|
| charge_count(`s`/`v`)    |  ✅   |   ❌  | u, n  | Return the charges of a given item in the character's inventory.<br/>Argument is item ID.<br/><br/>Example:<br/>`"condition": { "math": [ "u_charge_count('light_battery_cell') >= 100"] }`|
| coverage(`s`/`v`)    |  ✅   |   ❌  | u, n  | Return the characters total coverage of a body part.<br/>Argument is bodypart ID. <br/>For items, returns typical coverage of the item. <br/><br/>Example:<br/>`"condition": { "math": [ "u_coverage('torso') > 0"] }`|
| distance(`s`/`v`,`s`/`v`)    |  ✅   |   ❌  | g  | Return distance between two targets.<br/>Arguments are location variables or special strings (`u`, `npc`). `u` means your location. `npc` means NPC's location.<br/><br/>Example:<br/>`"condition": { "math": [ "distance('u', loc) <= 50"] }`|
| effect_intensity(`s`/`v`)    |  ✅   |   ❌  | u, n  | Return the characters intensity of effect.<br/>Argument is effect ID.<br/><br/>Optional kwargs:<br/>`bodypart`: `s`/`v` - Specify the bodypart to get/set intensity of effect.<br/><br/> Example:<br/>`"condition": { "math": [ "u_effect_intensity('bite', 'bodypart': 'torso') > 1"] }`|
| effect_duration(`s`/`v`)    |  ✅   |   ✅  | u, n  | Return the characters duration of effect.<br/>Argument is effect ID.<br/><br/>Optional kwargs:<br/>`bodypart`: `s`/`v` - Specify the bodypart to get/set duration of effect.<br/>`unit`: `s`/`v` - Specify the unit of the duration. Omitting will use seconds.<br/><br/> Example:<br/>`"condition": { "math": [ "u_effect_duration('bite', 'bodypart': 'torso') > 1"] }`<br/>`{ "math": [ "_thing = u_effect_duration('yrax_overcharged', 'bodypart': 'torso', 'unit': 'hours')" ] }`|
| encumbrance(`s`/`v`)    |  ✅   |   ❌  | u, n  | Return the characters total encumbrance of a body part.<br/>Argument is bodypart ID. <br/> For items, returns typical encumbrance of the item. <br/><br/>Example:<br/>`"condition": { "math": [ "u_encumbrance('torso') > 0"] }`|
| health(`d`/`v`)    |  ✅   |   ✅  | u, n  | Return character current health .<br/><br/>Example:<br/>`{ "math": [ "u_health() -= 1" ] }`|
| energy(`s`/`v`)    |  ✅   |   ❌  | u, n  | Return a numeric value (in millijoules) for an energy string (see [Units](JSON_INFO.md#units)).<br/><br/>Example:<br/>`{ "math": [ "u_val('power') -= energy('25 kJ')" ] }`|
| faction_like(`s`/`v`)<br/>faction_respect(`s`/`v`)<br/>faction_trust(`s`/`v`)<br/>faction_food_supply(`s`/`v`)<br/>faction_wealth(`s`/`v`)<br/>faction_power(`s`/`v`)<br/>faction_size(`s`/`v`)    |   ✅   |   ✅  | N/A<br/>(global)  | Return the like/respect/trust/fac_food_supply/wealth/power/size value a faction has for the avatar.<br/>Argument is faction ID.<br/>`faction_food_supply` has an optional second argument(kwarg) for getting/setting vitamins.<br/><br/>Example:<br/>`"condition": { "math": [ "faction_like('hells_raiders') < -60" ] }`<br/><br/>`{ "math": [ "calcium_amount", "=", "faction_food_supply('your_followers', 'vitamin':'calcium')" ] },`<br/>`{ "u_message": "Calcium stored is <global_val:calcium_amount>", "type": "good" },`|
| field_strength(`s`/`v`)    |   ✅   |   ❌  | u, n, global  | Return the strength of a field on the tile.<br/>Argument is field ID.<br/><br/>Optional kwargs:<br/> `location`: `v` - center search on this location<br/><br/>The `location` kwarg is mandatory in the global scope.<br/><br/>Examples:<br/>`"condition": { "math": [ "u_field_strength('fd_blood') > 5" ] }`<br/><br/>`"condition": { "math": [ "field_strength('fd_blood_insect', 'location': u_search_loc) > 5" ] }`|
| has_flag(`s`/`v`) | ✅ | ❌ | u, n | Check whether the actor has a flag. Meant to be used as condition for ternaries. Argument is trait ID.<br/><br/> Example:<br/>`"condition": { "math": [ "u_blorg = u_has_flag('MUTATION_TRESHOLD') ? 100 : 15" ] }`|
| has_trait(`s`/`v`)    |  ✅   |   ❌  | u, n  | Check whether the actor has a trait. Meant to be used as condition for ternaries. Argument is trait ID.<br/><br/> Example:<br/>`"condition": { "math": [ "u_blorg = u_has_trait('FEEBLE') ? 100 : 15" ] }`|
| sum_traits_of_category(`s`/`v`)    |  ✅   |   ❌  | u, n  | Return sum of all positive, negative, or all mutation in a mutation tree (not one character has). Argument is category id.<br/><br/>Optional kwargs:<br/> `type`: `s`/`v` - should it be only positive (points >= 0), only negative(points <= 0), or all traits altogether; possible values are `POSITIVE`, `NEGATIVE` and `ALL`(default).<br/><br/> Example:<br/>`"condition": { "math": [ "u_sum_traits_of_category('RAT') < u_sum_traits_of_category('RAT', 'type': 'POSITIVE')" ] }`|
| u_sum_traits_of_category_char_has(`s`/`v`)    |  ✅   |   ❌  | u, n  | Return sum of all positive, negative, or all mutation in a mutation tree talker has at this moment. Argument is category id.<br/><br/>Optional kwargs:<br/> `type`: `s`/`v` - should it be only positive (points >= 0), only negative(points <= 0), or all traits altogether; possible values are `POSITIVE`, `NEGATIVE` and `ALL`(default).<br/><br/> Example:<br/>`"condition": { "math": [ "u_sum_traits_of_category_char_has('RAT') < u_sum_traits_of_category_char_has('RAT', 'type': 'POSITIVE')" ] }`|
| has_proficiency(`s`/`v`)    |  ✅   |   ❌  | u, n  | Check whether the actor has a proficiency. Meant to be used as condition for ternaries. Argument is proficiency ID.<br/><br/> Example:<br/>`"condition": { "math": [ "u_blorg = u_has_proficiency('prof_intro_biology') ? 100 : 15" ] }`|
| has_var(`v`)    |  ✅   |   ❌  | g  | Check whether the variable is defined. Meant to be used as condition for ternaries.<br/><br/> Example:<br/>`"condition": { "math": [ "u_blorg = has_var(fancy_var) ? fancy_var : 15" ] }`|
| hp(`s`/`v`)    |  ✅   |   ✅  | u, n  | Return or set the characters hp. Argument is bodypart ID. For special values `ALL`, `ALL_MAJOR`, `ALL_MINOR`, get hp sum of all/major/minor bodyparts or set hp of all/major/minor bodyparts.<br/><br/>For items, returns current amount of damage required to destroy item.<br/><br/>Example:<br/>`"condition": { "math": [ "hp('torso') > 100"] }`|
| hp_max(`s`/`v`)    |  ✅   |   ❌  | u, n  | Return the characters max amount of hp on a body part.<br/>Argument is bodypart ID.<br/> For items, returns max amount of damage required to destroy item.<br/><br/>Example:<br/>`"condition": { "math": [ "u_hp_max('torso') >= 100"] }`|
| game_option(`s`/`v`)   |  ✅  |   ❌   | N/A<br/>(global)  | Return the numerical value of a game option<br/><br/>Example:<br/>`"condition": { "math": [ "game_option('NPC_SPAWNTIME') >= 5"] }`|
| gun_damage(`s`/`v`)     |  ✅  |   ❌   | u, n  | Return the item's gun damage. Argument is damage type. For special value `ALL`, return sum for all damage types. <br/><br/>Actor must be an item.<br/><br/>Example:<br/>`{ "math": [ "mygun = n_gun_damage('ALL')" ] }`<br/>See `EOC_test_weapon_damage` for a complete example|
| item_count(`s`/`v`)    |  ✅   |   ❌  | u, n  | Return the number of a given item in the character's inventory.<br/>Argument is item ID.<br/><br/>Example:<br/>`"condition": { "math": [ "u_item_count('backpack') >= 1"] }`|
| item_rad(`s`/`v`)    |  ✅   |   ❌  | u, n  | Return irradiation of worn items with the specified flag.<br/>Argument is flag ID.<br/><br/>Optional kwargs:<br/>`aggregate`: `s`/`v` - Specify the aggregation function to run, in case there's more than one item. Valid values are `min`/`max`/`sum`/`average`/`first`/`last`. Defaults to `min` if not specified. <br/><br/>Example:<br/>`"condition": { "math": [ "u_item_rad('RAD_DETECT') >= 1"] }`|
| melee_damage(`s`/`v`)     |  ✅  |   ❌   | u, n  | Return the item's melee damage. Argument is damage type. For special value `ALL`, return sum for all damage types. <br/><br/>Actor must be an item.<br/><br/>Example:<br/>`{ "math": [ "mymelee = n_melee_damage('ALL')" ] }`<br/>See `EOC_test_weapon_damage` for a complete example|
| monsters_nearby(`s`/`v`...)     |  ✅  |   ❌   | u, n, global  | Return the number of nearby monsters. Takes any number of `s`tring or `v`ariable positional parameters as monster IDs. <br/><br/>Optional kwargs:<br/>`radius`: `d`/`v` - limit to radius (rl_dist)<br/>`location`: `v` - center search on this location<br/>`attitude`: `s`/`v` - attitude filter. Must be one of `hostile`, `friendly`, `both`. Assumes `hostile` if not specified<br/><br/>The `location` kwarg is mandatory in the global scope.<br/><br/>Examples:<br/>`"condition": { "math": [ "u_monsters_nearby('radius': u_search_radius * 3) > 5" ] }`<br/><br/>`"condition": { "math": [ "monsters_nearby('mon_void_maw', 'mon_void_limb', mon_fotm_var, 'radius': u_search_radius * 3, 'location': u_search_loc)", ">", "5" ] }`|
| mod_load_order(`s`/`v`)  |  ✅   |   ❌  | N/A<br/>(global)  | Returns the load order of specified mod. Argument is mod id. Returns -1 is the mod is not loaded. |
| mon_species_nearby(`s`/`v`...)     |  ✅  |   ❌   | u, n, global  | Same as `monsters_nearby()`, but arguments are monster species |
| mon_groups_nearby(`s`/`v`...)     |  ✅  |   ❌   | u, n, global  | Same as `monsters_nearby()`, but arguments are monster groups |
| moon_phase()     |  ✅  |   ❌   | N/A<br/>(global)  | Returns current phase of the Moon. <pre>MOON_NEW = 0,<br/>WAXING_CRESCENT = 1,<br/>HALF_MOON_WAXING = 2,<br/>WAXING_GIBBOUS = 3,<br/>FULL = 4,<br/>WANING_GIBBOUS = 5,<br/>HALF_MOON_WANING = 6,<br/>WANING_CRESCENT = 7 |
| num_input(`s`/`v`,`d`/`v`)   |  ✅  |   ❌   | N/A<br/>(global)  | Prompt the player for a number.<br/>Arguments are Prompt text, Default Value:<br/>`"math": [ "u_value_to_set = num_input('Playstyle Perks Cost?', 4)" ]`|
| pain()     |  ✅  |   ✅   | u, n  | Return or set pain.  Optional kwargs:<br/>`type`: `s/v` - return the value of specific format or assign in specific way.  Can be `perceived` ( for return, return pain value minus painkillers; for assigning, apply pain taking into account possible mutations, effects, cbms etc (pain sentitivity or pain deafness, for example)) or `raw` (for return, return flat amount of pain; for assigning, bypasses all checks).  If not used, `raw` is used by default. <br/> Example:<br/>`{ "math": [ "n_pain() = u_pain() + 9000" ] }` <br/>`{ "math": [ "u_pain('type': 'perceived' ) >= 40" ] }` <br/>`{ "math": [ "u_pain('type': 'perceived' ) += 22" ] }` |
| proficiency(`s`/`v`)    |  ✅   |   ✅  | u, n  | Return or set proficiency<br/>Argument is proficiency ID.<br/><br/> Optional kwargs:<br/>`format`: `s` - `percent` return or set how many percent done the learning is. `permille` does likewise for permille. `time_spent` return or set total time spent. `time_left` return or set the remaining time. `total_time_required` return total time required to train a given proficiency (read only).<br/>`direct`: `true`/`false`/`d` - false (default) perform the adjustment by practicing the proficiency for the given amount of time. This will likely result in different values than specified. `true` perform the adjustment directly, bypassing other factors that may affect it.<br/><br/>Example:<br/>`{ "math": [ "u_proficiency('prof_intro_chemistry', 'format': 'percent') = 50" ] }`|
| school_level(`s`/`v`)    |  ✅   |   ❌  | u, n  | Return the highest level of spells known of that school.<br/>Argument is school ID.<br/><br/>Example:<br/>`"condition": { "math": [ "u_school_level('MAGUS') >= 3"] }`|
| school_level_adjustment(`s`/`v`)    |   ✅  |  ✅   | u, n  | Return or set temporary caster level adjustment. Only useable by EoCs that trigger on the event `opens_spellbook`. Old values will be reset to 0 before the event triggers. To avoid overwriting values from other EoCs, it is recommended to adjust the values here with `+=` or `-=` instead of setting it to an absolute value.<br/>Argument is school ID.<br/><br/>Example:<br/>`{ "math": [ "u_school_level_adjustment('MAGUS')++"] }`|
| skill(`s`/`v`)    |  ✅  |   ✅   | u, n  | Return or set skill level<br/><br/>Example:<br/>`"condition": { "math": [ "u_skill('driving') >= 5"] }`<br/>`"condition": { "math": [ "u_skill(someskill) >= 5"] }`|
| spell_difficulty(`s`/`v`)    |  ✅  |   ✅   | u,n | Returns the difficulty of the given spell id, modified by temporary difficulty modifiers if the character knows the spell; otherwise just the base difficulty calculations.  <br/> Example:<br/>`"math": [ "_difficulty = u_spell_difficulty('test_spell_id')"] }`|
| skill_exp(`s`/`v`)    |  ✅  |   ✅   | u, n  | Return or set skill experience<br/> Argument is skill ID. <br/><br/> Optional kwargs:<br/>`format`: `s`/`v` - must be `percentage` or `raw`.<br/><br/>Example:<br/>`{ "math": [ "u_skill_exp('driving', 'format': crt_format)--"] }`|
| spell_exp(`s`/`v`)    |  ✅  |   ✅   | u, n  | Return or set spell xp<br/> Example:<br/>`"condition": { "math": [ "u_spell_exp('SPELL_ID') >= 5"] }`|
| spell_exp_for_level(`s`/`v`, `d`/`v`)    |  ✅  |   ❌   | g  | Return the amount of XP necessary for a spell level for the given spell. Arguments are spell id and desired level.  <br/> Example:<br/>`"math": [ "spell_exp_for_level('SPELL_ID', u_spell_level('SPELL_ID') ) * 5"] }`|
| spell_count()    |  ✅   |   ❌  | u, n  | Return number of spells the character knows.<br/><br/> Optional kwargs:<br/>`school`: `s/v` - return number of spells known of that school.<br/><br/> Example:<br/>`"condition": { "math": [ "u_spell_count('school': 'MAGUS') >= 10"] }`|
| spell_level_sum()    |  ✅   |   ❌  | u, n  | Return sum of all spell levels character has; having one spell of class A with level 5, and another with lvl 10 would return 15. <br/><br/> Optional kwargs:<br/>`school`: `s/v` - return number of spells known of that school. Omitting return sum of all spells character has, no matter of the class.<br/>`level`: `d/v` - count only spells that are higher or equal this field. Default 0.<br/><br/> Example:<br/>`{ "math": [ "test_var1 = u_spell_level_sum()" ] }`<br/>`{ "math": [ "test_var2 = u_spell_level_sum('school': 'MAGUS')" ] }`<br/>`{ "math": [ "test_var3 = u_spell_level_sum('school': 'MAGUS', 'level': '10')" ] }`|
| spell_level(`s`/`v`)    |  ✅   |   ✅  | u, n  | Return or set level of a given spell. -1 means the spell is not known when read and that the spell should be forgotten if written.<br/>Argument is spell ID. If `"null"` is given, return the highest level of spells the character knows (read only).<br/> Example:<br/>`"condition": { "math": [ "u_spell_level('SPELL_ID') == -1"] }`|
| spell_level_adjustment(`s`/`v`)    |   ✅  |  ✅   | u, n  | Return or set temporary caster level adjustment. Only useable by EoCs that trigger on the event `opens_spellbook`. Old values will be reset to 0 before the event triggers. To avoid overwriting values from other EoCs, it is recommended to adjust the values here with `+=` or `-=` instead of setting it to an absolute value.<br/>Argument is spell ID. If `"null"` is given, adjust all spell level.<br/><br/>Example:<br/>`{ "math": [ "u_spell_level_adjustment('SPELL_ID') += 3"] }`|
| spellcasting_adjustment(`s`/`v`)    |   ❌  |  ✅   | u  | Temporary alters a property of spellcasting. Only useable by EoCs that trigger on the event `opens_spellbook`. Old values will be reset to default values before the event triggers. Multipliers have a default value of 1, while adjustments have a default value of 0. Assignment functions as an adjustment to the default value by the value assigned. So a `=` functions as you would expect a `+=` to function. Reading these values are not possible, and therefore using `+=` is not possible. <br/><br/>Possible argument values: <br/>`caster_level` - Adjustment - alters the caster level of the given spell(s). Works much like spell_level_adjustment, but will not be readable by `math` functions.<br/>`casting_time` - Multiplier - alters the casting time of the given spell(s).<br/>`damage` - Multiplier - alters the damage of the given spell(s).  Negative damage result in healing.<br/>`cost` - Multiplier - alters the cost of the given spells. Note that this does not change what items may be consumed by the spell(s).<br/>`aoe` - Multiplier - alters the area of effect of the spell(s).<br/>`range` - Multiplier - alters the range of the spell(s).<br/>`duration` - Multiplier - alters the duration of the spell(s).<br/>`difficulty` - Adjustment - alters the difficulty of the spell(s), thus altering the probability of failing spellcasting.<br/>`somatic_difficulty` - Multiplier - alters how much encumbrance affects spellcasting time and difficulty. If set to 0, it will also remove the need to have your hands free while casting. Note that as a multiplier, it starts of at 1, and setting the value actually adjusts it. So setting the valute to -1 will result in a final value of 0. Alternatively, setting it to -0,5 twice would also do the trick.<br/>`sound` - Multiplier - alters the loudness and how much mouth encumbrance affects the spell(s).<br/>`concentration` - Multiplier - alters how much focus alters the difficulty of the spell(s).<br/><br/> Optional kwargs:<br/>`flag_blacklist`: `s/v` and<br/>`flag_whitelist`: `s/v` - Only applies the modifier to spells that matches the blacklist and/or whitelist<br/>`mod`: `s/v`,  `school`: `s/v`,  `spell`: `s/v` - Only one of these can be applied. Limits what spells will be affected. If none are specified, the modification will apply to all spells (whitelist and blacklist still applies separately).<br/><br/>Example:<br/>`{ "math": [ "u_spellcasting_adjustment('casting_time', 'mod': 'magiclysm', 'flag_blacklist': 'CONSUMES_RUNES' ) = -0.95" ] }`|
| value_or(`v`,`d`/`v`)    |  ✅   |   ❌  | g  | Return value of variable if defined, otherwise the provided value<br/><br/> Example:<br/>`"condition": { "math": [ "u_blorg = value_or( fancy_var, 15 )" ] }`|
| time(`s`/`v`)    |  ✅   |   ✅  | N/A<br/>(global)  | Return a numeric value (in turns) for a time period string (see [Units](JSON_INFO.md#units)).<br/><br/>Special Values:<br/>`now` - returns duration since turn zero<br/>`cataclysm` - returns duration between cataclysm and turn zero<br/><br/>`time('now')` can serve as an assignment target to change current turn.<br/><br/>Optional kwargs:<br/> `unit`: specify return unit. Assumes `turns` if unspecified or empty.<br/><br/>Example:<br/>`{ "math": [ "time('now') - u_timer_caravan_RandEnc > time('1 h')" ] }`|
| time_since(`v`)<br/>time_since('cataclysm')<br/>time_since('midnight')    |  ✅   |   ❌  | N/A<br/>(global)  | Convenience function that returns a numeric value (in turns) for the time period since time point stored in variable.<br/><br/>Special values:<br/>`cataclysm` - return time since start of cataclysm<br/>`midnight` - return time since midnight<br/>`noon` - when the ingame clock reads 12:00<br/><br/>Optional kwargs:<br/> `unit`: specify return unit. Assumes `turns` if unspecified or empty.<br/><br/>Returns -1 if the argument is an undefined variable<br/><br/>Example:<br/>`{ "math": [ "time_since(u_timer_caravan_RandEnc) > time('1 h')" ] }`<br/>`{ "math": [ "time_since('cataclysm', 'unit':'years') > 1" ] }`|
| time_until(`v`)<br/>time_until('daylight_time')<br/>time_until('night_time')<br/>time_until('sunset')<br/>time_until('sunrise')    |  ✅   |   ❌  | N/A<br/>(global)  | Convenience function that returns a numeric value (in turns) for the time period until time point stored in variable.<br/><br/>Special values:<br/>`daylight_time` - when sun rises above the [civil dawn](https://en.wikipedia.org/wiki/Dawn#Civil_dawn) point<br/>`night_time` - when sun sets below civil dawn<br/>`sunrise` - sun above horizon<br/>`sunset` - sun below horizon<br/>`noon` - when the ingame clock reads 12:00<br/><br/>Optional kwargs:<br/> `unit`: specify return unit. Assumes `turns` if unspecified or empty.<br/><br/>Returns -1 if the argument is an undefined variable<br/><br/>Example:<br/>`{ "math": [ "TIME_TILL_SUNRISE = time_until('sunrise', 'unit':'minutes')" ] },`<br/>`{ "u_message": "Minutes remaining until daylight is <global_val:TIME_TILL_SUNRISE>", "type": "good" },`|
| time_until_eoc(`s`/`v`)  |  ✅   |   ❌  | N/A<br/>(global)  | Returns time until next scheduled run of an EOC. Argument is EOC id.<br/><br/>Optional kwargs:<br/> `unit`: specify return unit<br/><br/>Returns -1 is the EOC is not scheduled. |
| val(`s`)    |  ✅   |   varies  | u, n  | Return or set a Character or item value. Argument is a [Character/item aspect](#list-of-character-and-item-aspects).<br/><br/>These are all in one function for legacy reasons and are generally poorly tested. If you need one of them, consider porting it to a native math function.<br/><br/>Example:<br/>`{ "math": [ "u_val('strength') = 2" ] }`|
| npc_anger()    |  ✅   |  ✅   | u, n  | Return NPC anger toward opposite talker. <br/><br/>Example:<br/> `{ "math": [ "n_npc_anger() > 2" ] }`|
| npc_fear()    |  ✅   |  ✅   | u, n  | Return NPC fear toward opposite talker. <br/><br/>Example:<br/> `{ "math": [ "n_npc_fear() < 2" ] }`|
| npc_trust()    |  ✅   |  ✅   | u, n  | Return NPC trust toward opposite talker. <br/><br/>Example:<br/> `{ "math": [ "n_npc_trust() = 2" ] }`|
| npc_value()    |  ✅   |  ✅   | u, n  | Return NPC value toward opposite talker. <br/><br/>Example:<br/> `{ "math": [ "n_npc_value() += 2" ] }`|
| ugliness()    |  ✅   |  ❌   | u, n  | Return ugliness of character. <br/><br/>Example:<br/> `{ "math": [ "n_ugliness() > 0" ] }`|
| weight()    |  ✅   |  ❌   | u, n  | Return creature or item weight, in miligrams. <br/><br/>Example:<br/> `{ "math": [ "u_weight() < 1000000" ] }`|
| volume()    |  ✅   |  ❌   | u, n  | Return creature or item volume, in mililiters. <br/><br/>Example:<br/> `{ "math": [ "u_volume() < 1000" ] }`|
| vitamin(`s`/`v`)    |  ✅   |   ✅  | u, n  | Return or set the characters vitamin level.<br/>Argument is vitamin ID.<br/><br/>Example:<br/>`{ "math": [ "u_vitamin('mutagen') = 0" ] }`|
| warmth(`s`/`v`)    |  ✅   |   ❌  | u, n  | Return the characters warmth on a body part.<br/>Argument is bodypart ID.<br/><br/>Example:<br/> The value displayed in-game is calculated as follows.<br/> `"{ "math": [ "u_warmth_in_game = (u_warmth('torso') / 100) * 2 - 100"] }`|
| vision_range()    |  ✅   |   ❌  | u, n  | Return the character's or monsters visual range, adjusted by their mutations, effects, and other issues.<br/><br/>Example:<br/> `"{ "math": [ "n_vision_range() < 30"] }`|
| weather(`s`)  |  ✅  |   ✅   | N/A<br/>(global)  | Return or set a weather aspect<br/><br/>Aspect must be one of:<br/>`temperature` (in Kelvin),<br/>`humidity` (as percentage),<br/>`pressure` (in millibar),<br/>`windpower` (in mph).<br/>`precipitation` (in mm / h) either 0.5 (very_light ), 1.5 (light), or 3 (heavy). Read only.<br/><br/>Temperature conversion functions are available: `celsius()`, `fahrenheit()`, `from_celsius()`, and `from_fahrenheit()`.<br/><br/>Examples:<br/>`{ "math": [ "weather('temperature') < from_fahrenheit( 33 )" ] }`<br/>`{ "math": [ "fahrenheit( weather('temperature') ) == 21" ] }`|
| damage_level()    |  ✅   |   ❌  | u, n  | Return the damage level of the talker, which must be an item.<br/><br/>Example:<br/>`"condition": { "math": [ "n_damage_level() < 1" ] }`|
| climate_control_str_heat()    |  ✅   |   ❌  | u, n  | return amount of heat climate control that character currently has (character feels better in warm places with it), in warmth points; default 0, affected by CLIMATE_CONTROL_HEAT enchantment.<br/><br/>Example:<br/>`"condition": { "math": [ "u_climate_control_str_heat() < 0" ] }`|
| climate_control_str_chill()    |  ✅   |   ❌  | u, n  | return amount of chill climate control that character currently has (character feels better in cold places with it), in warmth points; default 0, affected by CLIMATE_CONTROL_HEAT enchantment.<br/><br/>Example:<br/>`"condition": { "math": [ "n_climate_control_str_chill() < 0" ] }`|
| calories()    |  ✅   |   ✅  | u, n  | Return amount of calories character has. If used on item, return amount of calories this item gives when consumed (not affected by enchantments or mutations).  Optional kwargs:<br/>`format`: `s`/`v` - return the value in specific format.  Can be `percent` (return percent to the healthy amount of calories, `100` being the target, bmi 25, or 110000 kcal) or `raw`.  If now used, `raw` is used by default.<br/>`dont_affect_weariness`: `true`/`false` (default false) When assigning value, whether the gained/spent calories should be tracked by weariness.<br/><br/>Example:<br/>`"condition": { "math": [ "u_calories() < 0" ] }`<br/>`"condition": { "math": [ "u_calories('format': 'percent') > 0" ] }`<br/>`"condition": { "math": [ "u_calories() = 110000" ] }`|
| get_calories_daily()  |  ✅   |   ❌  | g  | Return amount of calories character consumed before, up to 30 days, in kcal. Calorie diary is something only character has, so it can't be used with NPCs. Optional kwargs:<br/>`day`: `d/v` - picks the date the value would be pulled from, from 0 to 30. Default 0, meaning amount of calories you consumed today.<br/>`type`: `s/v` - picks the data that would be pulled. Possible values are: `spent` - how much calories character spent in different activities throughout the day; `gained` - how much calories character ate that day; `ingested` - how much calories character processed that day; `total` - `gained` minus `spent`. Default is `total`;<br/><br/>Example:<br/>`"condition": { "math": [ "get_calories_daily() > 1000" ] }`<br/> `{ "math": [ "foo = get_calories_daily('type':'gained', 'day':'1')" ] }`|
| quality( `s` / `v` )  |  ✅   |   ❌  | u, n | Return the level of a specified item tool quality. Only usable on item talkers.<br/>Argument is the quality ID. Returns the lowest integer value if the item lacks the specified quality.<br/>Optional kwargs:<br/>`strict`: `true` / `false` (default false) When true the item must be empty to have the boiling quality.<br/><br/>Example<br/>`"condition: { "math": [ " u_quality('HACK') > 0 " ] }`<br/>`{ "math": [ "_cut_quality = u_quality('CUT') " ] }`<br/>`condition: { "math": [ " u_quality('BOIL', 'strict': true ) > 0" ] }`

#### List of Character and item aspects
These can be read or written to with `val()`.

| Value | Supports assignment | Description |
--- | --- | --- |
| `activity_level` | ❌ | Current activity level index as a floored integer, from 0-5. Roughly: <pre>0.45 = SLEEP_EXERCISE (floored and returns 0),<br/>0.5 = NO_EXERCISE(floored and returns 0),<br/>1 = LIGHT_EXERCISE,<br/>2 = MODERATE_EXERCISE,<br/>3 = BRISK_EXERCISE,<br/>4 = ACTIVE_EXERCISE,<br/>5 = EXTRA_EXERCISE. |
| `age` | ✅ | Current age in years. |
| `allies` | ❌ | The avatar's number of allies |
| `anger` | ✅ | Current anger level. Only works for monsters. |
| `bmi_permil` | ❌ | Current BMI per mille (Body Mass Index x 1000) |
| `body_temp` | ❌ | Current body temperature. |
| `body_temp_delta` | ❌ | Difference in temperature between the hottest/coldest part and what feels like the hottest/coldest part. |
| `cash` | ✅ | Amount of money |
| `dodge` | ❌ | Current effective dodge |
| `exp` | ✅ | Total experience earned. |
| `sleepiness` | ✅ | Current sleepiness level. |
| `fine_detail_vision_mod` | ❌ | Returned values range from 1.0 (unimpeded vision) to 11.0 (totally blind). |
| `focus` | ✅ | Current focus level. |
| `focus_effective` | ❌ | Effective focus level, modified by focus enchants. |
| `friendly` | ✅ | Current friendly level. Only works for monsters. |
| `grab_strength` | ❌ | Grab strength as defined in the monster definition. Only works for monsters |
| `height` | ✅ | Current height in cm. When setting there is a range for your character size category. Setting it too high or low will use the limit instead. For tiny its 58, and 87. For small its 88 and 144. For medium its 145 and 200. For large its 201 and 250. For huge its 251 and 320. |
| `hunger` | ❌ | Current perceived hunger. |
| `instant_thirst` | ❌ | Current thirst minus water in the stomach that hasn't been absorbed by the body yet. |
| `mana` | ✅ | Current mana. |
| `mana_max` | ❌ | Max mana. |
| `mana_percentage` | ❌ | Current mana as percent. |
| `morale` | ✅* | The current morale. Assigment only works for monsters. |
| `owed` | ✅ | Amount of money the Character owes the avatar. |
| `pkill` | ✅ | Current painkiller level. |
| `pos_x`<br/>`pos_y`<br/>`pos_z` | ✅ | Absolute coordinate of the character |
| `power` | ✅ | Bionic or item power. Bionic power is returned in millijoules. Item power is returned in battery charges. |
| `power_percentage` | ✅ | Percentage of max bionic or item power. |
| `power_max` | ❌ | Max bionic or item power. Bionic power is returned in millijoules. Item power is returned in battery charges. |
| `rad` | ✅ | Current radiation level. |
| `size` | ❌ | Size category from 1 (tiny) to 5 (huge). |
| `sleep_deprivation` | ✅ | Current sleep deprivation level. |
| `sold` | ✅ | Amount of money the avatar has sold the Character |
| `stamina` | ✅ | Current stamina level. |
| `stim` | ✅ | Current stim level. |
| `strength`<br/>`dexterity`<br/>`intelligence`<br/>`perception` | ✅ | Current attributes |
| `strength_base`<br/>`dexterity_base`<br/>`intelligence_base`<br/>`perception_base` | ✅ | Base attributes |
| `strength_bonus`<br/>`dexterity_bonus`<br/>`intelligence_bonus`<br/>`perception_bonus` | ✅ | Bonus attributes |
| `thirst` | ✅ | Current thirst. |
| `volume` | ❌ | Current volume in mL. Only works for monsters |
| `weight` | ❌ | Current weight in mg. |
| `count`  | ❌ | Count of an item. |
| `vehicle_facing`  | ❌ | Angle that the vehicle is facing. Only works for vehicles. 0 is North |
| `current_speed`  | ❌ | Vehicle's current speed. Only works for vehicles. |
| `unloaded_weight`  | ❌ | How much the vehicle would weigh without passengers or cargo. Only works for vehicles. |
| `friendly_passenger_count`  | ❌ | How many people plus monsters are onboard that are not hostile. Only works for vehicles. |
| `hostile_passenger_count`  | ❌ | How many people plus monsters are onboard that are not friendly. Only works for vehicles. |


#### Math functions defined in JSON
Math functions can be defined in JSON like this
```jsonc
  {
    "type": "jmath_function",
    "id": "my_math_function",
    "num_args": 2,
    "return": "_0 * 2 + rand(_1)"
  },
```
where `_0`, `_1`, etc. are positional parameters.

These functions can then be used like regular math functions, for example:
```jsonc
  {
    "type": "effect_on_condition",
    "id": "EOC_do_something",
    "effect": [ { "math": [ "secret_value = my_math_function( u_pain(), 500)" ] } ]
  },
```
so `_0` takes the value of `u_pain()` and `_1` takes the value 500 inside `my_math_function()`

Function composition is also supported and the functions can be defined and used in any order.

#### Assignment target
An assignment target can be either a scoped [variable name](#variables) or a scoped [dialogue function](#dialogue-functions).
