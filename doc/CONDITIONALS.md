
## Dialogue conditions
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

`variable_object`: This is an object describing a variable name. `default` is a required int which will be the value returned if the variable is not defined.

example json:
```
"condition": { "u_has_strength": { "name" :"variable_name", "type":"test", "context":"documentation", "default":5 } }
```
Condition | Type | Description
--- | --- | ---
`"u_male"`<br/>`"npc_male"` | simple string | `true` if the player character or NPC is male.
`"u_female"`<br/>`"npc_female"` | simple string | `true` if the player character or NPC is female.
`"u_at_om_location"`<br/>`"npc_at_om_location"` | string | `true` if the player character or NPC is standing on an overmap tile with `u_at_om_location`'s id.  The special string `"FACTION_CAMP_ANY"` changes it to return true if the player or NPC is standing on a faction camp overmap tile.  The special string `"FACTION_CAMP_START"` changes it to return true if the overmap tile that the player or NPC is standing on can be turned into a faction camp overmap tile.
`"u_has_trait"`<br/>`"npc_has_trait"` | string | `true` if the player character or NPC has a specific trait.  Simpler versions of `u_has_any_trait` and `npc_has_any_trait` that only checks for one trait.
`"u_has_flag"`<br/>`"npc_has_flag"` | string | `true` if the player character or NPC has the specified character flag.  The special trait flag `"MUTATION_THRESHOLD"` checks to see if the player or NPC has crossed a mutation threshold.
`"u_has_any_trait"`<br/>`"npc_has_any_trait"` | array | `true` if the player character or NPC has any trait or mutation in the array. Used to check multiple specific traits.
`"u_has_var"`, `"npc_has_var"` | string | `"type": type_str`, `"context": context_str`, and `"value": value_str` are required fields in the same dictionary as `"u_has_var"` or `"npc_has_var"`.<br/>`true` is the player character or NPC has a variable set by `"u_add_var"` or `"npc_add_var"` with the string, `type_str`, `context_str`, and `value_str`.
`"u_compare_var"`, `"npc_compare_var"` | dictionary | `"type": type_str`, `"context": context_str`, `"op": op_str`, `"value": value_num or variable_object` are required fields, referencing a var as in `"u_add_var"` or `"npc_add_var"`.<br/>`true` if the player character or NPC has a stored variable that is true for the provided operator `op_str` (one of `==`, `!=`, `<`, `>`, `<=`, `>=`) and value (or the value of the variable described by `value` see `variable_object` above).
`"u_compare_time_since_var"`, `"npc_compare_time_since_var_"` | dictionary | `"type": type_str`, `"context": context_str`, `"op": op_str`, `"time": time_string` are required fields, referencing a var as in `"u_add_var"` or `"npc_add_var"`.<br/>`true` if the player character or NPC has a stored variable and the current turn and that value (converted to a time point) plus the time_string is true for the provided operator `op_str` (one of `==`, `!=`, `<`, `>`, `<=`, `>=`).  *example*: `{ "u_compare_time_since_var": "test", "type": "test", "context": "var_time_test", "op": ">", "time": "3 days" }` returns true if the player character has a "test", "test", "var_time_test" variable and the current turn is greater than that value plus 3 days' worth of turns.
`"compare_string"` | array | The array must contain exactly two entries.  They can be either strings or objects containing a variable object.  Returns true if the strings are the same.
`"u_has_strength"`<br/>`"npc_has_strength"` | int or variable_object | `true` if the player character's or NPC's strength is at least the value of `u_has_strength` or `npc_has_strength` (or the value of the variable described see `variable_object` above).
`"u_has_dexterity"`<br/>`"npc_has_dexterity"` | int or variable_object | `true` if the player character's or NPC's dexterity is at least the value of `u_has_dexterity` or `npc_has_dexterity` ( or the value of the variable described see `variable_object` above).
`"u_has_intelligence"`<br/>`"npc_has_intelligence"` | int or variable_object| `true` if the player character's or NPC's intelligence is at least the value of `u_has_intelligence` or `npc_has_intelligence` ( or the value of the variable described see `variable_object` above).
`"u_has_perception"`<br/>`"npc_has_perception"` | int or variable_object| `true` if the player character's or NPC's perception is at least the value of `u_has_perception` or `npc_has_perception` ( or the value of the variable described see `variable_object` above).
`"u_has_item"`<br/>`"npc_has_item"` | string | `true` if the player character or NPC has something with `u_has_item`'s or `npc_has_item`'s `item_id` in their inventory.
`"u_has_items"`<br/>`"npc_has_item"` | dictionary | `u_has_items` or `npc_has_items` must be a dictionary with an `item` string and a `count` int or `charges` int.<br/>`true` if the player character or NPC has at least `charges` charges or counts of `item` in their inventory.
`"u_has_item_category"`<br/>`"npc_has_item_category"` | string | `"count": item_count` is an optional field that must be in the same dictionary and defaults to 1 if not specified.  `true` if the player or NPC has `item_count` items with the same category as `u_has_item_category` or `npc_has_item_category`.
`"u_has_bionics"`<br/>`"npc_has_bionics"` | string | `true` if the player or NPC has an installed bionic with an `bionic_id` matching `"u_has_bionics"` or `"npc_has_bionics"`.  The special string "ANY" returns true if the player or NPC has any installed bionics.
`"u_has_effect"`<br/>`"npc_has_effect"`, (*optional* `intensity : int`),(*optional* `bodypart : string`) | string | `true` if the player character or NPC is under the effect with `u_has_effect` or `npc_has_effect`'s `effect_id`. If `intensity` is specified it will need to be at least that strong.  If `bodypart` is specified it will check only that bodypart for the effect.
`"u_can_stow_weapon"`<br/>`"npc_can_stow_weapon"` | simple string | `true` if the player character or NPC is wielding a weapon and has enough space to put it away.
`"u_has_weapon"`<br/>`"npc_has_weapon"` | simple string | `true` if the player character or NPC is wielding a weapon.
`"u_driving"`<br/>`"npc_driving"` | simple string | `true` if the player character or NPC is operating a vehicle.  <b>Note</b> NPCs cannot currently operate vehicles.
`"u_has_skill"`<br/>`"npc_has_skill"` | dictionary | `u_has_skill` or `npc_has_skill` must be a dictionary with a `skill` string and a `level` int.<br/>`true` if the player character or NPC has at least the value of `level` in `skill`.
`"u_know_recipe"` | string | `true` if the player character knows the recipe specified in `u_know_recipe`.  It only counts as known if it is actually memorized--holding a book with the recipe in it will not count.
`"u_has_worn_with_flag"`<br/>`"npc_has_worn_with_flag"` | string | `true` if the player character or NPC is wearing something with the `u_has_worn_with_flag` or `npc_has_worn_with_flag` flag.
`"u_has_wielded_with_flag"`<br/>`"npc_has_wielded_with_flag"` | string | `true` if the player character or NPC is wielding something with the `u_has_wielded_with_flag` or `npc_has_wielded_with_flag` flag.
`"u_can_see"`<br/>`"npc_can_see"` | simple string | `true` if the player character or NPC is not blind and is either not sleeping or has the see_sleep trait.
`"u_is_deaf"`<br/>`"npc_is_deaf"` | int | `true` if the player character or NPC can't hear.
the value of `u_has_focus` or `npc_has_focus` ( or the value of the variable described see `variable_object` above).
`"u_is_on_terrain"`<br/>`"npc_is_on_terrain"` | string | `true` if the player character or NPC is on terrain named `"u_is_on_terrain"` or `"npc_is_on_terrain"`.
`"u_is_in_field"`<br/>`"npc_is_in_field"` | string | `true` if the player character or NPC is in a field of type `"u_is_in_field"` or `"npc_is_in_field"`..
`"u_query"`<br/>`"npc_query", default : bool` | string | if the player character or NPC is the avatar will popup a yes/no query with the provided message and users response is used as the return value.  If called for a non avatar will return `default`.
example
```
"condition": { "u_query": "Should we test?", "default": true },
```
#### Player Only conditions

Condition | Type | Description
--- | --- | ---
`"u_has_mission"` | string | `true` if the mission is assigned to the player character.
`"u_has_cash"` | int or variable_object | `true` if the player character has at least `u_has_cash`( or the value of the variable described see `variable_object` above) cash available.  *Deprecated*  Previously used to check if the player could buy something, but NPCs shouldn't use e-cash for trades anymore.
`"u_are_owed"` | int or variable_object | `true` if the NPC's op_of_u.owed is at least `u_are_owed`( or the value of the variable described see `variable_object` above).  Can be used to check if the player can buy something from the NPC without needing to barter anything.
`"u_has_camp"` | simple string | `true` is the player has one or more active base camps.
`"u_has_faction_trust"` | int | `true` if the player character has at least the given amount of trust with the speaker's faction.

#### Player and NPC interaction conditions

Condition | Type | Description
--- | --- | ---
`"at_safe_space" or "u_at_safe_space" or "npc_at_safe_space"` | simple string | `true` if u or the NPC's current overmap location passes the `is_safe()` test.
`"has_assigned_mission"` | simple string | `true` if the player character has exactly one mission from the NPC. Can be used for texts like "About that job...".
`"has_many_assigned_missions"` | simple string | `true` if the player character has several mission from the NPC (more than one). Can be used for texts like "About one of those jobs..." and to switch to the `"TALK_MISSION_LIST_ASSIGNED"` topic.
`"has_no_available_mission" or "npc_has_no_available_mission" or "u_has_no_available_mission"` | simple string | `true` if u or the NPC has no jobs available for the player character.
`"has_available_mission" or "u_has_available_mission" or "npc_has_available_mission"` | simple string | `true` if u or the NPC has one job available for the player character.
`"has_many_available_missions"` | simple string | `true` if the NPC has several jobs available for the player character.
`"mission_goal" or "npc_mission_goal" or "u_mission_goal"` | string | `true` if u or the NPC's current mission has the same goal as `mission_goal`.
`"mission_complete" or "npc_mission_complete" or "u_mission_complete"` | simple string | `true` if u or the NPC has completed the other's current mission.
`"mission_incomplete" or "npc_mission_incomplete" or "u_mission_incomplete"` | simple string | `true` if u or the NPC hasn't completed the other's current mission.
`"mission_has_generic_rewards"` | simple string | `true` if the NPC's current mission is flagged as having generic rewards.
`"npc_service"` | int | `true` if the NPC does not have the `"currently_busy"` effect and the player character has at least `npc_service` cash available.  Useful to check if the player character can hire an NPC to perform a task that would take time to complete.  Functionally, this is identical to `"and": [ { "not": { "npc_has_effect": "currently_busy" } }, { "u_has_cash": service_cost } ]`
`"npc_allies"` | int or variable_object | `true` if the player character has at least `npc_allies` ( or the value of the variable described see `variable_object` above) number of NPC allies.
`"is_by_radio"` | simple string | `true` if the player is talking to the NPC over a radio.
`"u_available" or "npc_available"` | simple string | `true` if u or the NPC does not have effect `"currently_busy"`.
`"u_following" or "npc_following"` | simple string | `true` if u or the NPC is following the player character.
`"u_friend" or "npc_friend"` | simple string | `true` if u or the NPC is friendly to the player character.
`"u_hostile" or "npc_hostile"` | simple string | `true` if u or the NPC is an enemy of the player character.
`"u_train_skills" or "npc_train_skills"` | simple string | `true` if u or the NPC has one or more skills with more levels than the player.
`"u_train_styles" or "npc_train_styles"` | simple string | `true` if u or the NPC knows one or more martial arts styles that the player does not know.
`"u_has_class" or "npc_has_class"` | array | `true` if u or the NPC is a member of an NPC class.
`"u_near_om_location" or "npc_near_om_location"`, (*optional* `range : int or variable_object`) | string | same as at_om_location except it checks in a square stretching from the character range OMT's. NOTE: can only check OMT's in the reality bubble.
`"u_aim_rule" or "npc_aim_rule"` | string | `true` if u or the NPC follower AI rule for aiming matches the string.
`"u_engagement_rule" or "npc_engagement_rule"` | string | `true` if u or the NPC follower AI rule for engagement matches the string.
`"u_cbm_reserve_rule" or "npc_cbm_reserve_rule"` | string | `true` if u or the NPC follower AI rule for cbm, reserve matches the string.
`"u_cbm_recharge_rule" or "npc_cbm_recharge_rule"` | string | `true` if u or the NPC follower AI rule for cbm recharge matches the string.
`"u_rule" or "npc_rule"` | string | `true` if u or the NPC follower AI rule for that matches string is set.
`"u_override" or "npc_override"` | string | `true` if u or the NPC has an override for the string.
`"has_pickup_list" or "u_has_pickup_list" or "npc_has_pickup_list"` | string | `true` if u or the NPC has a pickup list.

#### NPC only conditions

Condition | Type | Description
--- | --- | ---
`"npc_role_nearby"` | string | `true` if there is an NPC with the same companion mission role as `npc_role_nearby` within 100 tiles.
`"has_reason"` | simple string | `true` if a previous effect set a reason for why an effect could not be completed.

#### Environment

Condition | Type | Description
--- | --- | ---
`"days_since_cataclysm"` | int or variable_object | `true` if at least `days_since_cataclysm`( or the value of the variable described see `variable_object` above) days have passed since the Cataclysm.
`"is_season"` | string | `true` if the current season matches `is_season`, which must be one of "`spring"`, `"summer"`, `"autumn"`, or `"winter"`.
`"is_day"` | simple string | `true` if it is currently daytime.
`"u_is_outside"`</br>`"npc_is_outside"`  | simple string | `true` if you or the NPC is on a tile without a roof.
`"u_is_underwater"`</br>`"npc_is_underwater"`  | simple string | `true` if you or the NPC is underwater.
`"one_in_chance"` | int or variable_object | `true` if a one in `one_in_chance`( or the value of the variable described see `variable_object` above) random chance occurs.
`"x_in_y_chance"` | object | `true` if a `x` in `y` random chance occurs. `x` and `y` are either ints or `variable_object`s ( see `variable_object` above).
`"is_weather"` | int or variable_object | `true` if current weather is `"is_weather"`.


#### Compare itergers & arithmetics
`"compare_int"` can be used to compare two values to each other, while `"arithmetic"` can be used to take up to two values, perform arithmetic on them, and then save them in a third value. The syntax is as follows.
```
{
  "text": "If player strength is more than or equal to 5, sets time since cataclysm to the player's focus times the player's maximum mana with at maximum a value of 15.",
  "topic": "TALK_DONE",
  "condition": { "compare_int": [ { "u_val": "strength" }, ">=", { "const": 5 } ] }
  "effect": { "arithmetic": [ { "time_since_cataclysm": "turns" }, "=", { "u_val": "focus" }, "*", { "u_val": "mana_max" } ], "max":15 }
},
```
`min` and `max` are optional int or variable_object values.  If supplied they will limit the result, it will be no lower than `min` and no higher than `max`. `min_time` and `max_time` work the same way but will parse times written as a string i.e. "10 hours".
`"compare_int"` supports the following operators: `"=="`, `"="` (Both are treated the same, as a compare), `"!="`, `"<="`, `">="`, `"<"`, and `">"`.

`"arithmetic"` supports the following operators: `"*"`, `"/"`, `"+"`, `"-"`, `"%"`, `"&"`, `"|"`, `"<<"`, `">>"`, `"~"`, `"^"` and the following results `"="`, `"*="`, `"/="`, `"+="`, `"-="`, `"%="`, `"++"`, and `"--"`

To get player character properties, use `"u_val"`. To get NPC properties, use same syntax but `"npc_val"` instead. For vars only `global_val` is also allowed. A list of values that can be read and/or written to follows.

Example | Description
--- | ---
`"const": 5` | A constant value, in this case 5. Can be read but not written to.
`"time": "5 days"` | A constant time value. Will be converted to turns. Can be read but not written to.
`"time_since_cataclysm": "turns"` | Time since the start of the cataclysm in turns. Can instead take other time units such as minutes, hours, days, weeks, seasons, and years.
`"rand": 20` | A random value between 0 and a given value, in this case 20. Can be read but not written to.
`"weather": "temperature"` | Current temperature.
`"weather": "windpower"` | Current windpower.
`"weather": "humidity"` | Current humidity.
`"weather": "pressure"` | Current pressure.
`"u_val": "strength"` | Player character's strength. Can be read but not written to. Replace `"strength"` with `"dexterity"`, `"intelligence"`, or `"perception"` to get such values.
`"u_val": "strength_base"` | Player character's strength. Replace `"strength_base"` with `"dexterity_base"`, `"intelligence_base"`, or `"perception_base"` to get such values.
`"u_val": "strength_bonus"` | Player character's current strength bonus. Replace `"strength_bonus"` with `"dexterity_bonus"`, `"intelligence_bonus"`, or `"perception_bonus"` to get such values.
`"u_val": "var"` | Custom variable. `"var_name"`, `"type"`, and `"context"` must also be specified. If `global_val` is used then a global variable will be used. If `default` is given as either an int or a variable_object then that value will be used if the variable is empty. If `default_time` is the same thing will happen, but it will be parsed as a time string aka "10 hours". Otherwise 0 will be used if the variable is empty.
`"u_val": "time_since_var"` | Time since a custom variable was set.  Unit used is turns. `"var_name"`, `"type"`, and `"context"` must also be specified.
`"u_val": "allies"` | Number of allies the character has. Only supported for the player character. Can be read but not written to.
`"u_val": "cash"` | Amount of money the character has. Only supported for the player character. Can be read but not written to.
`"u_val": "owed"` | Owed money to the NPC you're talking to.
`"u_val": "sold"` | Amount sold to the NPC you're talking to.
`"u_val": "skill_level"` | Level in given skill. `"skill"` must also be specified.
`"u_val": "pos_x"` | Player character x coordinate. "pos_y" and "pos_z" also works as expected.
`"u_val": "pain"` | Pain level.
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
`"u_val": "sleep_deprivation"` | Current sleep deprivation level.
`"u_val": "anger"` | Current anger level, only works for monsters.
`"u_val": "friendly"` | Current friendly level, only works for monsters.
`"u_val": "vitamin"` | Current vitamin level. `name` must also be specified which is the vitamins id.
`"u_val": "age"` | Current age in years.
`"u_val": "bmi_permil"` | Current BMI per mille (Body Mass Index x 1000)
`"u_val": "height"` | Current height in cm. When setting there is a range for your character size category. Setting it too high or low will use the limit instead. For tiny its 58, and 87. For small its 88 and 144. For medium its 145 and 200. For large its 201 and 250. For huge its 251 and 320.
`"distance": []` | Distance between two targets. Valid targets are: "u","npc" and an object with a variable name.
`"hour"` | Hours since midnight.
`"moon"` | Phase of the moon. MOON_NEW =0, WAXING_CRESCENT =1, HALF_MOON_WAXING =2, WAXING_GIBBOUS =3, FULL =4, WANING_GIBBOUS =5, HALF_MOON_WANING =6, WANING_CRESCENT =7
```
"condition": { "compare_int": [ { "distance": [ "u",{ "u_val": "stuck", "type": "ps", "context": "teleport" }  ] }, ">", { "const": 5 } ] }
```
