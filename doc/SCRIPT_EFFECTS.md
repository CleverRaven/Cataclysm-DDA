# Script Effects

## Introduction and confusing terminology
Called "effects" wherever they are used in JSON, these are termed "script effects" in documentation to distinguish them from [effects](./EFFECTS_JSON.md).  Designers take heed, at some point we should make it possible to call effects inside of scripts, and script effects in attacks, so that these become the same thing.  Previously, script effects were called "dialogue effects" because they could only be called from within NPC dialogue, but over time this system has expanded to include the [Effect on Condition](./EFFECT_ON_CONDITION.md) scripting language, hence the expanding confusion.

Script Effects are a very powerful way to alter the game state dynamically. They are, as the name implies, an internal CDDA scripting language in JSON that lets us write events and occurences within game in a parseable data-driven format.  In other words, this is how content writers can have the game change and respond to what's going on.

### How to use Script Effects
Regardless of how you activate a script effect, multiple effects should be arranged in a list and are processed in the order listed, not simultaneously.

#### Conditional statements
Most of the time if you are going to use script effects for all but a simple interaction, they will be coupled with [conditional statements](./CONDITIONALS.md).  For this reason the syntax of the two is quite similar, and learning one will often require learning the other.

#### Effect on Condition
[Effect on Condition](./EFFECT_ON_CONDITION.md) is essentially a scripting event language in CDDA, which uses [conditional statements](./conditionals.md) to specify script effects to run.  

#### Dialogue Effects
The `effect` field of `speaker_effect` or a `response` can be any of the available effects.  Note that the game is paused during dialogue, so while instantaneous effects (such as changing a variable) will apply immediately, any effect that processes at the end of the turn will not be applied until the dialogue closes.

### Parts of a Script Effect


#### Variables: General information
When storing a simple variable, think carefully on if it should be stored on the player avatar (`u_add_var`) or the active NPC (`npc_add_var`).  Variables stored on the NPC are local only, but will persist if the player avatar changes: if you store the record of an NPC mission being completed as an NPC var, then it cannot be easily referenced in a conversation with a different NPC, or in an EoC disconnected from that NPC.  Variables stored on the player avatar can be interacted with from outside a specific NPC conversation, but will not persist after that.  True global variables must currently be handled as a `variable_object`.  Often it may be appropriate to use multiples of these: if you finish a mission for a random NPC, we may wish to use `npc_add_var` so that this specific NPC remembers that you did this mission for them, `u_add_var` so that your avatar knows they did it themselves, and add a `global_var` so that we can track in general that this mission *has been done by someone* even if the player avatar changes. Then, for example, an EoC can run based on the success of that mission, but if you switch to a different character your avatar will not respond as though they had done the mission themselves.

#### Simple Variables
These are specified with the `u_add_var` or `npc_add_var` effect for string variables, or the `u_adjust_var` or `npc_adjust_var` for numeric variables.  This format of variable was the original way to store NPC variables, and is widely used.  It is distinct from a `variable_object`, which is a newer way of storing number variables that can be modified with math and used in place of integers in other effects.  In general, if you want a number value, it may be worth it to look into using a `variable_object` for that.



##### String Variables
TK

##### Numeric Variables
This format of variable should be slowly deprecated in favour of arithmetic variable objects.
TK: Explain why and show examples.

##### Timer Variables
This format of variable should be slowly deprecated in favour of arithmetic variable objects.
TK: Explain why and show examples.

#### Variable Objects
A `variable_object` is either an object or array describing a variable name.  As a standard JSON object it will look something like this:<br />
  `{ "u_val":"test", "default": 1 }`
  
- It can either describe an int (or a time duration. 
- If it is an array (eg. `[5, 10]`) it must have 2 values the first of which will be a **minimum** and the second will be a **maximum**, and the actual value will be a random number between the two. 
- If it is an int, `default` is a required additional field specifying an int which will be the value returned if the variable is not defined. 
- If is it a duration then `default` can be either an int or a string describing a time span. 

`u_val`, `npc_val`, or `global_val` can be the used for the variable name element.  If `u_val` is used it describes a variable on player u, if `npc_val` is used it describes a variable on player npc, if `global_val` is used it describes a global variable.

A `variable_object` can be defined independent of another effect using the `arithmetic` effect, eg;
`{ "arithmetic": [ { "global_val": "var", "var_name": "example" }, "=", { "const": 15 } ] },` - this sets the global_val "example" to be equal to 15.

##### Example 1
```
"queue_eocs": "EOC_Do_a_thing_eventually",
"time_in_future": [
  { "global_val": "min_wait", "default": "2 hours" },
  { "global_val": "max_wait", "default": "4 hours" }
]
```
Here we have an effect that will call an EoC at some random time in the future.
This effect uses an array in `variable_object` and each of the array entries is a separate `variable_object` specifying a time unit. If neither min_ nor max_wait is specified, this will choose a random time between 2 and 4 hours. If either of those durations have been set as a different time, it will use those variable durations instead.

##### Example 2

```json
"effect": [
  { "u_mod_focus": { "u_val":"test", "default": 1 } },
  { "u_mod_focus": [ 0, { "u_val":"test", "default": 1 } ] }
  { 
    "u_add_morale": "morale_honey","bonus": -20,"max_bonus": -60, "decay_start": 1,
    "duration": { "global_val": "test2", "type": "debug", "context": "testing", "default": "2 minutes" }
  }
]

```
In this example, we do two changes to the avatar's focus. 

- First, we adjust it by whatever is stored in the `u_val` variable with `var_name` "test". If "test" is not defined, we mod it by 1.
- Second, we mod focus again, by a random number from 0 to either the `test` variable or, again, the default 1.

Then, we have a more complex effect that, for duration, uses a global time variable `test2`. This variable has multiple optional name fields (`type` and `context`) which exist to help prevent a common test variable name from getting reused. In this case rather than an int, we use a standardized time unit.


#### Location Variables
These are a subtype of `variable_object` that specifies an x,y,z coordinate on the map.  They can be used for all kinds of things.
TK: Format and how to place them.

## Index of Effects

TK: This section could use a table of contents.

When editing this document please attempt to list effects alphabetically, but keep similar effects together even if it violates this. For example, `u_add_bionic` and `u_lose_bionic` should be together.

#### Missions

Effect | Required | Optional | Description | Example
---|---|---|---|---
`assign_mission` | `mission_type_id` | none | Assigns a mission to your avatar. | `"assign_mission": "mission_id"`
`clear_mission` | `mission_type_id` | none | Clears the mission from the your avatar's assigned missions. | `"clear_mission": "mission_id"`
`mission_failure` | `mission_type_id` | none | Resolves the mission as a failure. | `"mission_failure": "mission_id"`
`mission_reward` | `mission_type_id` | none | Gives the avatar the mission's reward. | `"mission_reward": "mission_id"`
`mission_success` | `mission_type_id` | none | Resolves the current mission successfully. | `"mission_success": "mission_id"`
`finish_mission` | `mission_type_id` | `success: bool`<br />`step: int` | Will complete mission `mission_type_id` to the player as a success if `success` is true, as a failure otherwise. If a `step` is provided that step of the mission will be completed. | `"finish_mission": "mission_id", "success": true`
`offer_mission` | `mission_type_id` string or array | none | Adds mission_type_id(s) to the npc's missions that they offer in their sequential mission list. | `"offer_mission": [ "mission_id1", "mission_id2" ]`

#### Morale and Cosmetic

Effect | Description
---|---
`barber_hair` | Opens a menu allowing the player to choose a new hair style.
`barber_beard` | Opens a menu allowing the player to choose a new beard style.
`buy_haircut` | Gives your character a haircut morale boost for 12 hours.
`buy_shave` | Gives your character a shave morale boost for 6 hours.
`morale_chat` | Gives your character a pleasant conversation morale boost for 6 hours.
`u_lose_morale: morale_string`<br/>`npc_lose_morale: morale_string` | Your character or the NPC will lose any morale of type `morale_string`.
`u_add_faction_trust: amount_int`<br/>`u_lose_faction_trust: amount_int` | Your character gains or loses trust with the speaking NPC's faction, which affects which items become available for trading from shopkeepers of that faction.


Effect | Required | Optional | Description | Example
---|---|---|---|---
`u_add_morale`<br />`npc_add_morale` | `morale_string` | `bonus`: `int` or `variable_object`<br />`max_bonus`: `int` or `variable_object`<br />`duration`: `time` or `variable_object`<br />`decay_start`: `time` or `variable_object`<br />`capped`: `bool` | Target gains a morale bonus of type `morale_string`. Morale is changed by the amount in `bonus` (default 1), with a maximum of up to `max_bonus` (default 1). It will last for `duration` (default 1 hour). It will begin to `decay` after specified time (default 30 minutes). `capped` states whether this morale is capped or not, (defaults false).

#### Wounds and Healing


Effect | Description
---|---
`give_aid` | Removes all bites, infection, and bleeding from your character's body and heals 10-25 HP of injury on each of your character's body parts.
`give_aid_all` | Performs `give_aid` on each of your character's NPC allies in range.
`lesser_give_aid` | Removes bleeding from your character's body and heals 5-15 HP of injury on each of your character's body parts.
`lesser_give_aid_all` | Performs `lesser_give_aid` on each of your character's NPC allies in range.

#### Mutations/Traits/Bionics

Note: Mutations are traits. The difference between "adding a trait" and "mutating" is that mutation takes time and is accompanied by special messages, while a trait should be added instantly without announcement.

Effect | Required | Optional | Description | Example
---|---|---|---|---
`bionic_install` | none | none | Open trade dialogue; NPC installs a bionic from avatar's inventory onto avatar, using very high skill, and charging according to difficulty.
`bionic_remove` | none | none | Open trade dialogue; NPC removes a bionic from avatar, using very high skill, and charging according to difficulty.
`u_add_bionic`, `npc_add_bionic` | `bionic_string` | none | Your avatar or the NPC will gain the bionic.| `"u_add_bionic": "cbm1"`
`u_lose_bionic`, `npc_lose_bionic` | `bionic_string` | none | Your avatar or the NPC will lose the bionic.| `"u_lose_bionic": "cbm1"`
`u_add_trait`, `npc_add_trait` | `trait_string` | none | Your avatar or the NPC will gain the trait.| `"u_add_trait": "egtrait"`
`u_lose_trait`, `npc_lose_trait | `trait_string` | none | Your avatar or the NPC will lose the trait.| `"u_lose_trait": "egtrait"`
`u_mutate`, `npc_mutate: | `chance_int` | `use_vitamins: vitamin_bool` (default true) | Your avatar or the NPC will attempt to mutate, with a one in `chance_int` chance of using the highest category, with 0 never using the highest category, using vitamins if indicated.| `"u_mutate": "egtrait", "use_vitamins": false`
`u_mutate_category`, `npc_mutate_category` | `category_str` | `use_vitamins: vitamin_bool` (default true)| Your avatar or the NPC will attempt to mutate in the category `category_str`, selecting a random mutation from the category, using vitamins if indicated.| `"u_mutate_category": "example_traits", "use_vitamins": false`

#### Variable operations

See **variables** above for details on the types of variables and their general use.  This quick reference is meant to complement that information, not replace it.

Effect | Required | Optional | Description | Example
---|---|---|---|---
`u_add_var`, `npc_add_var` | `var_name`, `value: string` | `type`, `context` | Store `value` as a variable that can be later retrieved by `u_has_var` or `npc_has_var`.  
`u_add_var`, `npc_add_var` | `var_name`, `time: true` | `type`, `context` | Store the current turn of the game as an integer.
`u_add_var`, `npc_add_var` | `var_name`, `possible_values: string_array` | `type`, `context` | Store one of the values given at random in the specified `string_array`. 
`u_lose_var`, `npc_lose_var` | `var_name` | `type`, `context` | Clear any stored variable that has the same string values for `var_name`, `type`, and `context`.
`u_adjust_var, npc_adjust_var` | `var_name`, `adjustment`: `int` or `variable_object`  | `type`, `context` |  Your character or the NPC will adjust the stored variable by a specified int, or the value in the variable_object.

Effect | Description
---|---
`set_string_var`: `type: string or variable object or array of either`, `target_var: variable_object`| Store string (or the variable described) from `set_string_var` in the variable object `target_var`. If an array is provided a random element will be used.
`u_location_variable, npc_location_variable`: `target_var`,*optional* `min_radius: min_radius_int or min_radius_variable_object`,*optional* `max_radius: max_radius_int or max_radius_variable_object`, *optional* `outdoor_only: outdoor_only_bool`, *optional* `target_params: assign_mission_target` parameters, *optional* `z_adjust: int or variable_object`, *optional* `z_override: bool` | If `target_params` is defined it will be used to find a tile. See [the missions docs](MISSIONS_JSON.md) for `assign_mission_target` parameters. Otherwise targets a point between `min_radius_int`( or `min_radius_variable_object`)(defaults to 0) and `max_radius_int`( or `max_radius_variable_object`)(defaults to 0) spaces of the target and if `outdoor_only_bool` is true(defaults to false) will only choose outdoor spaces. The chosen point will be saved to `target_var` which is a `variable_object`.  `z_adjust` will be used as the Z value if `z_override`(defaults false) is true or added to the current z value otherwise.

#### Character effects

Effect | Description
---|---
`u_add_effect: effect_string`, (*one of* `duration: duration_string`, `duration: duration_int`, `duration_variable_object`),(*optional* `target_part: target_part_string`, `intensity: intensity_int`)<br/>`npc_add_effect: effect_string`, (*one of* `duration: duration_string`, `duration: duration_int`, `duration_variable_object`), (*optional* `target_part: target_part_string`, `force: force_bool`, `intensity: intensity_int or intensity_variable_object`) | Your character or the NPC will gain the effect for `duration_string` or the value of the variable described by `duration_object` see `variable_object` above, turns at intensity `intensity_int` (or the value of the variable described by `intensity_variable_object` see `variable_object` above) or 0 if it was not supplied. If `force_bool` is true(defaults false) immunity will be ignored. If `target_part` is supplied that part will get the effect otherwise its a whole body effect. If `target_part` is `RANDOM` a random body part will be used. If `duration_string` is `"PERMANENT"`, the effect will be added permanently.
`u_lose_effect: effect_string`<br/>`npc_lose_effect: effect_string` | Your character or the NPC will lose the effect if they have it.
`u_learn_recipe: recipe_string`  | Your character will learn and memorize the recipe `recipe_string`.
`npc_first_topic: talk_topic_string` | Changes the initial talk_topic of the NPC in all future dialogues.
`u_add_wet: wet_int`<br/>`npc_add_wet: wet_int or wet_variable_object` | Your character or the NPC will be wet `wet_int` (or the value of the variable described by `wet_variable_object` see `variable_object` above) as if they were in the rain.
`u_make_sound, npc_make_sound: message_string`, `volume: volume_int`, `type: type_string`,*optional* `target_var: target_var_object`, *optional* `snippet: snippet_bool`, *optional* `same_snippet: same_snippet_bool`  | A sound of description `message_string` will be made at your character or the NPC's location of volume `volume_int` and type `type_string`. Possible types are: background, weather, music, movement, speech, electronic_speech, activity, destructive_activity, alarm, combat, alert, or order. If `target_var` is set this effect will be centered on a location saved to a variable with its name.  `target_var` is an object with `value`,`type` and `context` as string values and a bool `global` which determines if the variable is global or not. If `snippet_bool` is true(defaults to false) it will instead display a random snippet from `message_string` category, if `same_snippet_bool` is true(defaults to false) it will always use the same snippet and will set a variable that can be used for custom item names(this requires the snippets to have id's set)
`u_mod_healthy, npc_mod_healthy : amount_int or amount_variable_object, cap: cap_int or cap_variable_object` | Your character or the NPC will have `amount_int` ( or the value of the variable described by `amount_variable_object` see `variable_object` above) added or subtracted from its health value, but not beyond `cap_int` or `cap_variable_object`.
`u_message, npc_message: message_string`, (*optional* `sound: sound_bool`),(*optional* `outdoor_only: outdoor_only_bool`),(*optional* `snippet: snippet_bool`),(*optional* `same_snippet: snippet_bool`,(*optional* `type: type_string`),(*optional* `popup: popup_bool`) | Displays a message to either the player or the npc of `message_string`.  Will not display unless the player or npc is the actual player.  If `snippet_bool` is true(defaults to false) it will instead display a random snippet from `message_string` category, if `same_snippet_bool` is true(defaults to false) it will always use the same snippet and will set a variable that can be used for custom item names(this requires the snippets to have id's set).  If `sound` is true (defaults to false) it will only display the message if the player is not deaf.  `outdoor_only`(defaults to false) only matters when `sound` is true and will make the message less likely to be heard if the player is underground. Message will display as type of `type_string`. Type affects the color of message and can be any of the following values: good, neutral, bad, mixed, warning, info, debug, headshot, critical, grazing.  enums.h has more info on each types use. If `popup_bool` is true the message will be in a modal popup the user has to dismiss to continue.  You can use any of the  Special Custom Entries(defined above).
`u_cast_spell, npc_cast_spell : fake_spell_data`, *optional* `true_eocs: eocs_array`, *optional* `false_eocs: eocs_array` | The spell described by fake_spell_data will be cast with u or the npc as the caster and u or the npc's location as the target.  Fake spell data can have the following attributes: `id:string`: the id of the spell to cast, (*optional* `hit_self`: bool ( defaults to false ) if true can hit the caster, `trigger_message`: string to display on trigger, `npc_message`: string for message if npc uses, `max_level` int max level of the spell, `min_level` int min level of the spell ).  If the spell is cast, then all of the effect_on_conditions in `true_eocs` are run, otherwise all the effect_on_conditions in `false_eocs` are run.
`u_assign_activity, npc_assign_activity: activity_id_string`, `duration: duration_string or duration_variable_object`) | Your character or the NPC will start activity `activity_id_string`. It will last for `duration: duration_string` time or `duration_variable_object`.
`u_teleport, npc_teleport: target_var_object`, (*optional* `success_message: success_message_string`), (*optional* `fail_message: fail_message_string`) | u or npc are teleported to the destination stored in the variable named by `target_var`.  `target_var` is an object with `value`,`type` and `context` as string values and a bool `global` which determines if the variable is global or not. If the teleport succeeds and `success_message` is defined it will be displayed, if it fails and `fail_message` is defined it will be displayed.

#### Trade / Items

Effect | Description
---|---
`start_trade` | Opens the trade screen and allows trading with the NPC.
`give_equipment` | Allows your character to select items from the NPC's inventory and transfer them to your inventory.
`npc_gets_item` | Allows your character to select an item from your character's inventory and transfer it to the NPC's inventory.  The NPC will not accept it if they do not have space or weight to carry it, and will set a reason that can be referenced in a future dynamic line with `"use_reason"`.
`u_spawn_item: item_string`, (*optional* `count: count_num`, *optional* `container: container_string`) | Your character gains the item or `count_num` copies of the item, contained in container if specified. If used in an NPC conversation the items are said to be given by the NPC.
`u_buy_item: item_string`, (`cost: cost_num`, *optional* `count: count_num`, *optional* `container: container_string`, *optional* `true_eocs: eocs_array`, *optional* `false_eocs: eocs_array`) | The NPC will sell your character the item or `count_num` copies of the item, contained in `container`, and will subtract `cost_num` from `op_of_u.owed`.  If the `op_o_u.owed` is less than `cost_num`, the trade window will open and the player will have to trade to make up the difference; the NPC will not give the player the item unless `cost_num` is satisfied.
`u_sell_item: item_string`, (*optional* `cost: cost_num`, *optional* `count: count_num`, *optional* `true_eocs: eocs_array`, *optional* `false_eocs: eocs_array`) | Your character will give the NPC the item or `count_num` copies of the item, and will add `cost_num` to the NPC's `op_of_u.owed` if specified.<br/>If cost isn't present, the your character gives the NPC the item at no charge.<br/>This effect will fail if you do not have at least `count_num` copies of the item, so it should be checked with.  If the item is sold, then all of the effect_on_conditions in `true_eocs` are run, otherwise all the effect_on_conditions in `false_eocs` are run.
`u_bulk_trade_accept`<br/>`npc_bulk_trade_accept` *or*  `u_bulk_trade_accept: quantity_int` <br/>`npc_bulk_trade_accept: quantity_int`  | Only valid after a `repeat_response`.  The player trades all instances of the item from the `repeat_response` with the NPC.  For `u_bulk_trade_accept`, the player loses the items from their inventory and gains the same value of the NPC's faction currency; for `npc_bulk_trade_accept`, the player gains the items from the NPC's inventory and loses the same value of the NPC's faction currency.  If there is remaining value, or the NPC doesn't have a faction currency, the remainder goes into the NPC's `op_of_u.owed`. If `quantity_int` is specified only that many items/charges will be moved.
`u_bulk_donate`<br/>`npc_bulk_donate` *or*  `u_bulk_donate: quantity_int` <br/>`npc_bulk_donate: quantity_int`  | Only valid after a `repeat_response`.  The player or NPC transfers all instances of the item from the `repeat_response`.  For `u_bulk_donate`, the player loses the items from their inventory and the NPC gains them; for `npc_bulk_donate`, the player gains the items from the NPC's inventory and the NPC loses them. If `quantity_int` is specified only that many items/charges will be moved.
`u_spend_cash: cost_num`, *optional* `true_eocs: eocs_array`, *optional* `false_eocs: eocs_array` | Remove `cost_num` from your character's cash.  Negative values means your character gains cash.  *deprecated* NPCs should not deal in e-cash anymore, only personal debts and items. If the cash is spent, then all of the effect_on_conditions in `true_eocs` are run, otherwise all the effect_on_conditions in `false_eocs` are run.
`add_debt: mod_list` | Increases the NPC's debt to the player by the values in the `mod_list`.<br/>The following would increase the NPC's debt to the player by 1500x the NPC's altruism and 1000x the NPC's opinion of the player's value: `{ "effect": { "add_debt": [ [ "ALTRUISM", 3 ], [ "VALUE", 2 ], [ "TOTAL", 500 ] ] } }`
`u_consume_item`, `npc_consume_item: item_string`, (*optional* `count: count_num`), (*optional* `charges: charges_num`), (*optional* `popup: popup_bool`) | You or the NPC will delete the item or `count_num` copies of the item or `charges_num` charges of the item from their inventory.<br/>This effect will fail if the you or NPC does not have at least `count_num` copies of the item or `charges_num` charges of the item, so it should be checked with `u_has_items` or `npc_has_items`.<br/>If `popup_bool` is `true`, `u_consume_item` will show a message displaying the character giving the items to the NPC. It defaults to `false` if not defined, and has no effect when used in `npc_consume_item`.
`u_remove_item_with`, `npc_remove_item_with: item_string` | You or the NPC will delete any instances of item in inventory.<br/>This is an unconditional remove and will not fail if you or the NPC does not have the item.
`u_buy_monster: monster_type_string`, (*optional* `cost: cost_num`, *optional* `count: count_num`, *optional* `name: name_string`, *optional* `pacified: pacified_bool`, *optional* `true_eocs: eocs_array`, *optional* `false_eocs: eocs_array`) | The NPC will give your character `count_num` (default 1) instances of the monster as pets and will subtract `cost_num` from `op_of_u.owed` if specified.  If the `op_o_u.owed` is less than `cost_num`, the trade window will open and the player will have to trade to make up the difference; the NPC will not give the player the item unless `cost_num` is satisfied.<br/>If cost isn't present, the NPC gives your character the item at no charge.<br/>If `name_string` is specified the monster(s) will have the specified name. If `pacified_bool` is set to true, the monster will have the pacified effect applied.  If the monster is sold, then all of the effect_on_conditions in `true_eocs` are run, otherwise all the effect_on_conditions in `false_eocs` are run.

#### Wielding/Wearing

Effect | Description
---|---
`drop_weapon` | Makes the NPC drop their weapon.
`npc_gets_item_to_use` | Allow your character to select an item from your character's inventory and transfer it to the NPC's inventory.  The NPC will attempt to wield it and will not accept it if it is too heavy or is an inferior weapon to what they are currently using, and will set a reason that can be referenced in a future dynamic line with `"use_reason"`.
`player_weapon_away` | Makes your character put away (unwield) their weapon.
`player_weapon_drop` | Makes your character drop their weapon.

#### Behavior / AI Commands

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
`hostile` | Makes the NPC hostile and ends the conversation.
`flee` | Makes the NPC flee from your character.
`follow` | Makes the NPC follow your character, joining the "Your Followers" faction.
`leave` | Makes the NPC leave the "Your Followers" faction and stop following your character.
`follow_only` | Makes the NPC follow your character without changing factions.
`stop_following` | Makes the NPC stop following your character without changing factions.
`npc_thankful` | Makes the NPC positively inclined toward your character.
`stranger_neutral` | Changes the NPC's attitude to neutral.
`start_mugging` | The NPC will approach your character and steal from your character, attacking if your character resists.
`lead_to_safety` | The NPC will gain the LEAD attitude and give your character the mission of reaching safety.
`start_training` | The NPC will train your character in a skill or martial art.  NOTE: the code currently requires that you initiate training by directing the player through `"topic": "TALK_TRAIN"` where the thing to be trained is selected.  Initiating training outside of "TALK_TRAIN" will give an error.
`start_training_npc` | The NPC will accept training from the player in a skill or martial art.
`start_training_seminar` | Opens a dialog to select which characters will participate in the training seminar hosted by this NPC.
`companion_mission: role_string` | The NPC will offer you a list of missions for your allied NPCs, depending on the NPC's role.
`basecamp_mission` | The NPC will offer you a list of missions for your allied NPCs, depending on the local basecamp.
`npc_class_change: class_string` | Change the NPC's faction to `class_string`.
`npc_faction_change: faction_string` | Change the NPC's faction membership to `faction_string`.
`u_faction_rep: rep_num` | Increases your reputation with the NPC's current faction, or decreases it if `rep_num` is negative.
`toggle_npc_rule: rule_string` | Toggles the value of a boolean NPC follower AI rule such as `"use_silent"` or `"allow_bash"`
`set_npc_rule: rule_string` | Sets the value of a boolean NPC follower AI rule such as `"use_silent"` or `"allow_bash"`
`clear_npc_rule: rule_string` | Clears the value of a boolean NPC follower AI rule such as `"use_silent"` or `"allow_bash"`
`set_npc_engagement_rule: rule_string` | Sets the NPC follower AI rule for engagement distance to the value of `rule_string`.
`set_npc_aim_rule: rule_string` | Sets the NPC follower AI rule for aiming speed to the value of `rule_string`.
`npc_die` | The NPC will die at the end of the conversation.
`npc_set_goal:assign_mission_target_object`, *optional* `true_eocs: eocs_array`, *optional* `false_eocs: eocs_array` | The NPC will walk to `assign_mission_target_object`. See [the missions docs](MISSIONS_JSON.md) for `assign_mission_target` parameters.  If the goal is assigned, then all of the effect_on_conditions in `true_eocs` are run, otherwise all the effect_on_conditions in `false_eocs` are run.

#### Map Updates
Effect | Description
---|---
`mapgen_update: mapgen_update_id_string`<br/>`mapgen_update:` *list of `mapgen_update_id_string`s*, (optional `assign_mission_target` parameters), (optional `target_var: variable_object`), (*optional* `time_in_future: duration_string or duration_variable_object`) | With no other parameters, updates the overmap tile at the player's current location with the changes described in `mapgen_update_id` (or for each `mapgen_update_id` in the list). If `time_in_future` is set the update will happen that far in the future, in this case however the target location will be determined now and not changed even if its variables update.  The `assign_mission_target` parameters can be used to change the location of the overmap tile that gets updated.  See [the missions docs](MISSIONS_JSON.md) for `assign_mission_target` parameters and [the mapgen docs](MAPGEN.md) for `mapgen_update`. If `target_var` ( see `variable_object` above) is set this effect will be centered on a location saved to a variable with its name instead.
`revert_location: variable_object`, `time_in_future: duration_string or duration_variable_object` | `revert_location` is a variable object of the location.  The map tile at that location will be saved(terrain,furniture and traps) and restored at `time_in_future`.
`lightning` | Allows supercharging monster in electrical fields, legacy command for lightning weather.
`next_weather` | Forces a check for what weather it should be.
`custom_light_level: custom_light_level_int or custom_light_level_variable_object, length: duration_string or duration_variable_object` | Sets the ambient light from the sun/moon to be `custom_light_level_int` ( or the value of the variable described by `custom_light_level_variable_object` see `variable_object` above).  This can vary naturally between 0 and 125 depending on the sun to give a scale. This lasts `length`.
`u_transform_radius, npc_transform_radius: transform_radius_int or transform_radius_variable_object, ter_furn_transform: ter_furn_transform_string`, (*optional* `target_var: target_var_object`), (*optional* `time_in_future: duration_string or duration_variable_object`) | Applies the ter_furn_transform of id `ter_furn_transform` (See [the transform docs](TER_FURN_TRANSFORM.md)) in radius `translate_radius`. If `target_var` is set this effect will be centered on a location saved to a variable with its name.  `target_var` is an object with `value`,`type` and `context` as string values and a bool `global` which determines if the variable is global or not. If `time_in_future` is set the transform will that far in the future, in this case however the target location and radius will be determined now and not changed even if their variables update.
`u_spawn_monster: mtype_id_string, npc_spawn_monster: mtype_id_string`,(*optional* `group: group_bool`, *optional* `hallucination_count: hallucination_count_int or hallucination_count_variable_object`, *optional* `real_count: real_count_int or real_count_variable_object`,*optional* `min_radius: min_radius_int or min_radius_variable_object`,*optional* `max_radius: max_radius_int or max_radius_variable_object`,*optional* `outdoor_only: outdoor_only_bool`,*optional* `target_range : target_range_int or target_range_variable_object`), *optional* `lifespan: timespan_min_string or variable_object`, *optional* `target_var: target_var_object`,*optional* `spawn_message: spawn_message_string`,*optional* `spawn_message_plural: spawn_message_plural_spawn`, *optional* `true_eocs: eocs_array`, *optional* `false_eocs: eocs_array` | Spawns `real_count_int`( or the value of the variable described by `real_count_variable_object` see `variable_object` above)(defaults to 0) monsters and `hallucination_count_int`( or `hallucination_count_variable_object`) (defaults to 0) hallucinations near you or the npc. The spawn will be of type `mtype_id_string`, if `group_bool` is false(defaults to false, if it is true a random monster from monster_group `mtype_id_string` will be used), if this is an empty string it will instead be a random monster within `target_range_int`( or `target_range_variable_object`) spaces of you. The spawns will happen between `min_radius_int`( or `min_radius_variable_object`)(defaults to 1) and `max_radius_int`( or `max_radius_variable_object`)(defaults to 10) spaces of the target and if `outdoor_only_bool` is true(defaults to false) will only choose outdoor spaces. If `lifespan` (or the `variable_object`) is provided the monster or hallucination will only that long. If `target_var` is set this effect will be centered on a location saved to a variable with its name.  `target_var` is a `variable_object`. If at least one spawned creature is visible `spawn_message` will be displayed.  If `spawn_message_plural` is defined and more than one spawned creature is visible it will be used instead.  If at least one monster is spawned, then all of the effect_on_conditions in `true_eocs` are run, otherwise all the effect_on_conditions in `false_eocs` are run.
`u_set_field: field_id_string or npc_set_field: field_id_string`,(*optional* `intensity: intensity_int or intensity_variable_onject`, *optional* `age: age_int or variable_object`,*optional* `radius: radius_int or radius_variable_onject`,*optional* `outdoor_only: outdoor_only_bool`,*optional* `hit_player : hit_player_bool`,*optional* `target_var: target_var_object` ) | Add a field centered on you or the npc of type `field_type_id_string`, of intensity `intensity_int`( or the value of the variable described by `real_count_variable_object` see `variable_object` above)( defaults to 1,) of radius `radius_int`( or `radius_variable_object`)(defaults to 10000000) and age `age_int` (defaults 1) or `age_variable_object`. It will only happen outdoors if `outdoor_only` is true, it defaults to false. It will hit the player as if they entered it if `hit_player` it true, it defaults to true. If `target_var` is set this effect will be centered on a location saved to a variable with its name.  `target_var` is an object with `value`,`type` and `context` as string values and a bool `global` which determines if the variable is global or not.

#### General
Effect | Description
---|---
`sound_effect: sound_effect_id_string`, *optional* `sound_effect_variant: variant_string`, *optional* `outdoor_event: outdoor_event`,*optional* `volume: volume_int`  | Will play a sound effect of id `sound_effect_id_string` and variant `variant_string`. If `volume_int` is defined it will be used otherwise 80 is the default. If `outdoor_event`(defaults to false) is true this will be less likely to play if the player is underground.
`open_dialogue`, *optional* `true_eocs: eocs_array`, *optional* `false_eocs: eocs_array`. Opens up a dialog between the participants. This should only be used in effect_on_conditions.  If the dialog is opened, then all of the effect_on_conditions in `true_eocs` are run, otherwise all the effect_on_conditions in `false_eocs` are run.
`take_control`, *optional* `true_eocs: eocs_array`, *optional* `false_eocs: eocs_array`. If the npc is a character then take control of them and then all of the effect_on_conditions in `true_eocs` are run, otherwise all the effect_on_conditions in `false_eocs` are run.
`take_control_menu`. Opens up a menu to choose a follower to take control of.
`run_eocs : effect_on_condition_array or single effect_condition_object` | Will run up all members of the `effect_on_condition_array`. Members should either be the id of an effect_on_condition or an inline effect_on_condition.
`queue_eocs : effect_on_condition_array or single effect_condition_object`, `time_in_future: time_in_future_int or string or variable_object` | Will queue up all members of the `effect_on_condition_array`. Members should either be the id of an effect_on_condition or an inline effect_on_condition. Members will be run `time_in_future` seconds or if it is a string the future values of them or if they are variable objects the variable they name.  If the eoc is global the avatar will be u and npc will be invalid. Otherwise it will be queued for the current alpha if they are a character and not be queued otherwise.
`u_run_npc_eocs or npc_run_npc_eocs : effect_on_condition_array`, (*optional* `npcs_to_affect: npcs_to_affect_string_array`, (*optional* `npcs_must_see: npcs_must_see_bool`), (*optional* `npc_range: npc_range_int`) | Will run all members of the `effect_on_condition_array` on nearby npcs. Members should either be the id of an effect_on_condition or an inline effect_on_condition.  If any names are listed in `npcs_to_affect` then only they will be affected. If a value is given for `npc_range` the npc must be that close to the source and if `npcs_must_see`(defaults to false) is true the npc must be able to see the source. For `u_run_npc_eocs` u is the source for `npc_run_npc_eocs` it is the npc.
`weighted_list_eocs: array_array` | Will choose one of a list of eocs to activate based on weight. Members should be an array of first the id of an effect_on_condition or an inline effect_on_condition and second an object that resolves to an integer weight.
Example: This will cause "EOC_SLEEP" 1/10 as often as it makes a test message appear.
``` json
    "effect": [
      {
        "weighted_list_eocs": [
          [ "EOC_SLEEP", { "const": 1 } ],
          [ {
              "id": "eoc_test2",
              "effect": [ { "u_message": "A test message appears!", "type": "bad" } ]
            },
            { "const": 10 }
          ]
        ]
      }
    ]
```

#### Deprecated
Effect | Description
---|---
`deny_follow`<br/>`deny_lead`<br/>`deny_train`<br/>`deny_personal_info` | Sets the appropriate effect on the NPC for a few hours.<br/>These are *deprecated* in favor of the more flexible `npc_add_effect` described above.

#### Sample effects
```json
{ "topic": "TALK_EVAC_GUARD3_HOSTILE", "effect": [ { "u_faction_rep": -15 }, { "npc_change_faction": "hells_raiders" } ] }
{ "text": "Let's trade then.", "effect": "start_trade", "topic": "TALK_EVAC_MERCHANT" },
{ "text": "What needs to be done?", "topic": "TALK_CAMP_OVERSEER", "effect": { "companion_mission": "FACTION_CAMP" } }
{ "text": "Do you like it?", "topic": "TALK_CAMP_OVERSEER", "effect": [ { "u_add_effect": "concerned", "duration": 600 }, { "npc_add_effect": "touched", "duration": 3600 }, { "u_add_effect": "empathetic", "duration": "PERMANENT" } ] }
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
