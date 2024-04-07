# Effects

## How to give effects in-game?
### Comestibles
The first way to give a player an effect in-game is through the drug system. To do this your item must have a use_action of type "consume_drug".
```C++
    "use_action" : {
        "type" : "consume_drug",
        "activation_message" : "You take some oxycodone.",
        "effects" : [
            {
                "id": "pkill3",
                "duration": 20
            },
            {
                "id": "pkill2",
                "duration": 200
            }
        ]
    },
```
Notice the "effects" field. Each effect has four potential fields:
```C++
"id" - Required
"duration" - Required
"bp" - This will cause the effect to target this body part specifically
"permanent" - This means the placed effect will be permanent, and will never decrease in duration
```
Valid "bp" entries are (no entry means the effect is untargeted):
```C++
"TORSO"
"HEAD"
"EYES"
"MOUTH"
"ARM_L"
"ARM_R"
"HAND_L"
"HAND_R"
"LEG_L"
"LEG_R"
"FOOT_L"
"FOOT_R"
```

### Creature attacks
Creatures have an effect field similar to the "consume_drug" entry for items. You can make a creature's attacks apply effects by adding an "attack_effs" entry for the creature.
```C++
    "attack_effs": [
        {
            "//": "applying this multiple times makes intensity go up by 3 instead of 1",
            "id": "paralyzepoison",
            "duration": 33
        },
        {
            "id": "paralyzepoison",
            "duration": 33
        },
        {
            "id": "paralyzepoison",
            "duration": 33
        }
    ],
```
The fields for "attack_effs" function identically to the ones for "consume_drug". However, creatures have an additional field:
```C++
"chance" - The percentage chance of the effect being applied on a good hit, defaults to 100%
```
If a creature successfully damages the player and their chance roll succeeds they will apply
all of the listed effects to the player. The effects are added one after another.

## Required fields
```C++
    "type": "effect_type",      - Required
    "id": "xxxx"                - Must be unique
```

## Optional fields

### Max intensity
```C++
    "max_intensity": 6,             - Used for many later fields, defaults to 1
    "max_effective_intensity": 3    - Maximum intensity level that will accumulate effects.
                                      Other intensity levels will only increase duration.
```

Each effect has an intensity that describes how strong or severe the effect currently is.  Intensity
levels above 1 can be assigned different names, and multiply any "scaling_mods" (see below).

The "max_intensity" field tells the absolute maximum value intensity can reach.  The related
"max_effective_intensity" field limits the multiplier effect of "scaling_mods".  By default, the
multipliers will be applied all the way up to "max_intensity".


### Name
```C++
    "name": ["XYZ"]
    or
    "name": [
        "ABC",
        "XYZ",
        "123"
    ]
```
If "max_intensity" > 1 and the number of entries in "name" >= "max_intensity" then
it will attempt to use the proper intensity name. In this case that means an intensity
of 1 would give the name "ABC", 2 would give "XYZ", and 3 would give "123". If "max_intensity" == 1
or the number of entries in "name" is less than "max_intensity", it will use the first entry followed by
the intensity in brackets if the current intensity > 1, i.e. "ABC", "ABC [2]", "ABC [3]". If the desired
entry of "name" is the empty string ("") or "name" is missing then the effect will not display to the player
in the status screen.

Each entry in "name" can also have an optional context:
```JSON
    "name": [ { "ctxt": "ECIG", "str": "Smoke" } ]
```
In this case, the game will translate the name with the given context "ECIG",
which makes it possible to distinguish the verb "Smoke" from the noun "Smoke"
in other languages.

```C++
    "speed_name" : "XYZ"        - Defaults to the first name value
```
This is the value used in the list of modifiers on a player's speed. It will default to the first entry in "name"
if it doesn't exist, and if neither one exists or if "speed_name" is the empty string (""), then it will not
appear in the list of modifiers on the players speed (though the effect might still have an effect).

### Descriptions
```C++
    "desc": ["XYZ"]
    or
    "desc": [
        "ABC",
        "XYZ",
        "123"
    ]
```
Descriptions operate identically to the name field when picking which one to use. In general, descriptions
should be only 1 line. Stats and effects do not need to be included, and will be automatically generated
from the other effect data. Should a description line be the empty string ("") it will only display the
stat changes in the effect description.

Descriptions also have a second field that can act as a modifier:
```C++
    "part_descs": true      - Defaults to false if not present
```
If "part_descs" == true then descriptions are preceded by "Your X", where X is the body part name, meaning
the prior descriptions would appear as "Your left arm ABC".

Descriptions can also have a reduced form:
```C++
    "reduced_desc": ["XYZ"]
    or
    "reduced_desc": [
        "ABC",
        "XYZ",
        "123"
    ]
```
This is the description that will be used if an effect is reduced. By default this will use the normal description
if it doesn't exist.

### Rating
```C++
    "rating": "good"        - Defaults to "neutral" if missing
```
This is used for how the messages when the effect is applied and removed are displayed. Also this affects "blood_analysis_description" (see below) field: effects with "good" rating will be colored green, effects with any other rating will be colored red when character conducts a blood analysis through some means.

If [apply_message](#advanced-apply_message) is an array you can't include this entry (it is handled with apply message).

Valid entries are:
```C++
"good"
"neutral"
"bad"
"mixed"
```

### Messages
```C++
    "apply_message": "message",
    "remove_message": "message"
```
If the "apply_message" or "remove_message" fields exist, the respective message will be
displayed upon the addition or removal of the effect. Note: "apply_message" will only display
if the effect is being added, not if it is simply incrementing a current effect (so only new bites, etc.).

### Advanced apply_message
```C++
    "apply_message": [
        ["Your effect is applied", "good"],
        ["You took way too much effect", "bad"],
    ] 
```
You can instead of having a string for apply_message and including a [rating](#rating) can do advanced apply_message. This is an array of arrays with each inner array matching up with an intensity level and including the message and rating. This is useful for effects that too much of is a bad thing.

When using an advanced apply_message you can not include a [rating: ""](#rating) entry.

### Hiding the effect

```C++
    "show_in_info": true
```

Default false; If true, the effect is shown when you inspect another NPC or monster. It doesn't hide the effect from the player's @ menu, for this effect should have empty string as a name and description:

```C++
  {
    "type": "effect_type",
    "id": "secret_effect",
    "name": [ "" ],
    "desc": [ "" ],
    "//": "You can't see this effect"
  },
```

### Memorial Log
```C++
    "apply_memorial_log": "log",
    "remove_memorial_log": "log"
```
If the "apply_memorial_log" or "remove_memorial_log" fields exist, the game will add the
respective message to the memorial log on addition or removal of the effect. Similar to
the message fields the "apply_memorial_log" will only be added to the log for new effect additions.

### Resistances
```C++
    "resist_traits": "NOPAIN",
    "resist_effects": [ "flumed" ]
```
These fields are used to determine if an effect is being resisted or not. If the player has the
matching trait or effect then they are "resisting" the effect, which changes its effects and description.

### Immunity Flags
```JSON
"immune_flags": [ "INFECTION_IMMUNE", "YOUR_FLAG" ]
```
Having any of the defined character flags (See JSON_FLAGS.md#Character flags) will make you immune to the effect. Note that these are completely JSON-driven, so you can add a custom flag for your effect without C++ changes.

### Bodypart Immunity Flags

```JSON
"immune_bp_flags": [ "LIMB_UPPER" ]
```

When applying the effect to a bodypart directly the part in question having this JSON character flag will prevent the effect from applying.

### Removes effects
```C++
    "removes_effects": ["bite", "flu"]
```
This field will cause an effect to automatically remove any other copies of the listed effects if they are present.
In the example above the placed effect would automatically cure any bite wounds or flu the player had. Any values here
automatically count for "blocks_effects" as well, no need to duplicate them there.

### Blocks effects
```C++
    "blocks_effects": ["cold", "flu"]
```
This field will cause an effect to prevent the placement of the listed effects. In the example above the effect would
prevent the player from catching the cold or the flu (BUT WOULD NOT CURE ANY ONGOING COLDS OR FLUS). Any effects present
in "removes_effects" are automatically added to "blocks_effects", no need for manual duplication.

### Modifies effect-on-hit durations
```C++
    "effect_dur_scaling": [ { "effect_id": "bleed", "modifier": 1.05, "same_bp": false } ]
```
This field will cause an effect to modify the effect-on-hit durations of "effect_id", as defined in body_parts.json.
In the example above, this effect causes incoming bleeding duration to be increased by 5%.
If an effect with effect_dur_scaling is applied to a bodypart and same_bp is "true", then it will affect effect-on-hit durations on only that bodypart. If false, it will affect all bodyparts.

### Effect limiters
```C++
    "max_duration": 100,    - Time duration string, defaults to 365 days
    "dur_add_perc": 150     - Defaults to 100(%)
```
These are utilized when adding to currently existing effects. "max_duration" limits the overall duration of the effect.
"dur_add_perc" is the percentage value of the normal duration for adding to an existing. An example:
1) I add effect A to the player for 100 ticks.
2) I add effect A to the player again for 100 ticks.
Because the "dur_add_perc" = 150 in the example above, the second addition adds a total of 100 * 150% = 150 ticks, for
a total value of 250 ticks from the two. This can also be below 100%, and should be able to even be negative, leading to
future applications decreasing the overall time left.

### Intensities
Intensities are used to control effect effects, names, and descriptions. They are defined with:
```JSON
    "int_add_val": 2         - Int, defaults to 0 meaning future applications will not increase intensity

    "int_decay_step": -2,    - Int, default -1, intensity levels removed every decay tick
    "int_decay_tick": 10     - Int, seconds between intensity decay (no decay at the default of 0)
    "int_decay_remove": true - Bool, default false, removes the intensity if decay would decrease it to zero
    or
    "int_dur_factor": 700
```
The first value is the amount an intensity will be incremented if adding to an already existing effect. As an example:
1) I add effect A to the player
2) I add effect A to the player again
Because "int_add_val" = 2, the second addition will change the effect intensity from 1 to 1 + 2 = 3.
NOTE: You must have at least one of the 3 intensity data sets for intensity to do anything!

"int_decay_step" and "int_decay_tick" require one another to do anything. If both exist then the game will automatically
increment the current effect intensities by "int_decay_step" every "int_decay_tick" ticks, capping the result at [0, "max_intensity"]
and removing effects if the intensity reaches zero and `int_decay_remove` is true.
This can be used to make effects automatically increase or decrease in intensity over time.

"int_dur_factor" overrides the other three intensities fields, and forces the intensity to be a number defined as
intensity = duration / "int_dur_factor" rounded up (so from 0 to "int_dur_factor" is intensity 1).

```C++
    "show_intensity": false     - Defaults to true
```
This permits or forbids showing intensity value next to name of a given effect in EFFECTS tab. E.g. show "Weakness [142]" or simply "Weakness" text.

### Miss messages
```C++
    "miss_messages": [["Your blisters distract you", 1]]
    or
    "miss_messages": [
        ["Your blisters distract you", 1],
        ["Your blisters don't like you", 10],
    ]
```
This will add the following miss messages at the given chances while the effect is in effect.

### Decay messages
```C++
    "decay_messages": [["The jet injector's chemicals wear off.  You feel AWFUL!", "bad"]]
    or
    "decay_messages": [
        ["The jet injector's chemicals wear off.  You feel AWFUL!", "bad"],
        ["OOGA-BOOGA.  You feel AWFUL!", "bad"],
    ]
```
The messages are matched to intensities, so the first message is for intensity 1, the second for intensity 2, and
so on. The messages will print whenever the intensity decreases to their matching intensity from a higher intensity,
whether through decay ticks or through "int_dur_factor". So if it decayed to intensity 2 from 3+ it would display
"OOGA-BOOGA.  You feel AWFUL!" as a bad message to the player.

### Targeting modifiers
```C++
    "main_parts_only": true     - Defaults to false
```
This automatically retargets any effect on a non-main part (hands, eyes, feet, etc.) to the matching
main part (arms, head, legs, etc.).

### Effect modifiers
```C++
    "pkill_addict_reduces": true,   - Defaults to false
    "pain_sizing": true,            - Defaults to false
    "hurt_sizing": true,            - Defaults to false
    "harmful_cough": true           - Defaults to false
```
"pkill_addict_reduces" makes a player's addiction to painkillers reduce the chance of the effect giving
them more pkill. "pain_sizing" and "hurt_sizing" cause large/huge mutations to affect the chance of pain
and hurt effects triggering. "harmful_cough" means that the coughs caused by this effect can hurt the player.

### Vitamin Mods

```json
    "vitamins": [
      {
        "vitamin": "foo",
        "rate": [ [ 1, 2 ] ],
        "resist_rate": [ [ 0, 1 ] ],
        "absorb_mult": [ 0.5 ],
        "resist_absorb_mult": [ 0.0 ],
        "tick": [ "2 m" ],
        "resist_tick": [ "1 s" ],
      }
    ],
```
- `vitamin` corresponds to the vitamin id that the following effects will be applied to
- `rate` A randomly generated number between the bounds specified (here, 1 and 2) will added to the vitamin counter of a character with this effect every `tick`.
- `absorb_mult` metabolically absorbed vitamins will be multiplied by this quantity before being added to the character.

The `resist_` variants of the above keys are the values chosen when the character is resistant to this effect.

All of these members are arrays, with each successive entry corresponding to the intensity level of an effect. If there are more intensity levels to the effect than entries in the array, the last entry in the array will be used.

As defined, this will cause non-resistant characters to gain between 1 and 2 of the vitamin foo every 2 minutes, and half their absorption rate of it, and resistant character to gain between 0 and 1 of this vitamin every second, and not absorb any of it from their food.

### Death

```json
    "chance_kill": [ [ 1, 25 ] ],
    "chance_kill_resist": [ [ 1, 250 ] ],
    "death_msg": "You died.",
    "death_event": "throws_up"
```

- `chance_kill` A first value in second value chance to kill the creature with this effect each turn.
- `chance_kill_resist` A first value in second value chance to kill the creature with this effect each turn, if the creature resists this effect.
- `death_msg` A message added to the log when the player dies from this effect.
- `death_event` An event that is sent when the player dies from this effect.

For `chance_kill` and `chance_kill_resist`, it accepts an array of arrays in the format described. Each entry in the array will be applied for a successive intensity level of the field. If the intensity level of the field is greater than the number of entries in the array, the last entry will be used.

### Limb score modifiers

```JSON
    "limb_score_mods": [
      {
        "limb_score": "lift",
        "modifier": 0.5,
        "resist_modifier": 0.75,
        "scaling": -0.1,
        "resist_scaling": -0.05
      }
    ]
```

- "limb_score"        Mandatory, string id of the limb score in question
- "modifier"          Optional float (default 1.0), the multiplier on the limb score at effect intensity 1
- "resist_modifier"   Optional float (default 1.0), the multiplier on the limb score at effect intensity 1 when the character resists the effect
- "scaling"           Optional float (default 0.0), amount added to the multiplier on every intensity level above 1
- "resist_scaling"    Optional float (default 0.0), amount added to the multiplier on every intensity level above 1 when the character resists the effect


Limb score modifiers work as multipliers in the limb score calculations.  The example modifier will halve the non-resistant player's lift score at intensity 1 and decrease it by 10% of the start value for every successive intensity level (*0.4 at int 2, *0.3 at int 3 etc.), while decreasing a resistant character's lift score by 25% and intensifying in 5% steps (*0.70 at int 2, *0.65 at int 3).  Limb score modifiers are applied after health/encumbrance penalties multiplicatively (ex. 1.0 starting lift score reduced by two *0.5 modifiers equals 0.25, not 0), and separate instances of the same effect applied to separate bodyparts will be counted separately (ex. two instances of "staggered" on a character's arms will each apply their limb score modifiers).


### Effect effects
```C++
    "base_mods" : {
        arguments
    },
    "scaling_mods": {
        arguments
    }
```
This is where the real meat of the effect JSON definition lies. Each one can take a variety of arguments.
Decimals are valid but must be formatted as "0.X" or "-0.X". The game will round towards zero at the end
when calculating actually applied values

Basic definitions:
```C++
"X_amount"      - Amount applied of X when effect is placed. Like apply messages it will only trigger on new effects
"X_min"         - Minimum amount of X applied when roll triggers
"X_max"         - Maximum amount of X applied when roll triggers (no entry means it will give exactly X_min each time instead of rng(min, max)
"X_min_val"     - Minimum value the effect will push you to, 0 means uncapped! Doesn't exist for some X's!
"X_max_val"     - Maximum value the effect will push you to, 0 means uncapped! Doesn't exist for some X's!
"X_chance"      - Basic chance of X triggering each time, depends on "X_chance_bot" for exact formula
"X_chance_bot"  - If this doesn't exist then the trigger chance is (1 in "X_chance"). If this does exist then the chance is ("X_chance" in "X_chance_bot")
"X_tick"        - Effect rolls for X triggering every Y ticks
```

Valid arguments:
```C++
"str_mod"           - Positive values raises stat, negative values lowers stat
"dex_mod"           - Positive values raises stat, negative values lowers stat
"per_mod"           - Positive values raises stat, negative values lowers stat
"int_mod"           - Positive values raises stat, negative values lowers stat
"speed_mod"         - Positive values raises stat, negative values lowers stat

"pain_amount"       - Positives raise pain, negatives don't make anything. Don't make it too high.
"pain_min"          - Minimal amount of pain, certain effect will give/take
"pain_max"          - if 0 or missing value will be exactly "pain_min"
"pain_max_val"      - Defaults to 0, which means uncapped
"pain_chance"       - Chance to get more pain
"pain_chance_bot"
"pain_tick"         - Defaults to every tick.

"hurt_amount"       - Positives will give damage, negatives will heal instead. Don't make it too high.
"hurt_min"          - Minimal amount of damage, certain effect will give/take
"hurt_max"          - if 0 or missing value will be exactly "hurt_min"
"hurt_chance"       - Chance to cause damage
"hurt_chance_bot"
"hurt_tick"         - Defaults to every tick

"sleep_amount"      - Amount of turns spent sleeping.
"sleep_min"         - Minimal amount of sleep in turns, certain effect can give
"sleep_max"         - if 0 or missing value will be exactly "sleep_min"
"sleep_chance"      - Chance to fall asleep
"sleep_chance_bot"
"sleep_tick"        - Defaults to every tick

"pkill_amount"      - Amount of painkiller effect. Don't go too high with it.
"pkill_min"         - Minimal amount of painkiller, certain effect will give
"pkill_max"         - if 0 or missing value will be exactly "pkill_min"
"pkill_max_val"     - Defaults to 0, which means uncapped
"pkill_chance"      - Chance to cause painkiller effect(lowers pain)
"pkill_chance_bot"
"pkill_tick"        - Defaults to every tick

"stim_amount"       - Negatives cause depressants effect and positives cause stimulants effect.
"stim_min"          - Minimal amount of stimulant, certain effect will give.
"stim_max"          - if 0 or missing value will be exactly "stim_min"
"stim_min_val"      - Defaults to 0, which means uncapped
"stim_max_val"      - Defaults to 0, which means uncapped
"stim_chance"       - Chance to cause one of two stimulant effects
"stim_chance_bot"
"stim_tick"         - Defaults to every tick

"health_amount"     - Negatives decrease health and positives increase it. It's semi-hidden stat, which affects healing.
"health_min"        - Minimal amount of health, certain effect will give/take.
"health_max"        - if 0 or missing value will be exactly "health_min"
"health_min_val"    - Defaults to 0, which means uncapped
"health_max_val"    - Defaults to 0, which means uncapped
"health_chance"     - Chance to change health
"health_chance_bot"
"health_tick"       - Defaults to every tick

"h_mod_amount"      - Affects health stat growth, positives increase it and negatives decrease it
"h_mod_min"         - Minimal amount of health_modifier, certain effect will give/take
"h_mod_max"         - if 0 or missing value will be exactly "h_mod_min"
"h_mod_min_val"     - Defaults to 0, which means uncapped
"h_mod_max_val"     - Defaults to 0, which means uncapped
"h_mod_chance"      - Chance to change health_modifier
"h_mod_chance_bot"
"h_mod_tick"        - Defaults to every tick

"rad_amount"        - Amount of radiation it can give/take. Just be aware that anything above [50] is fatal.
"rad_min"           - Minimal amount of radiation, certain effect will give/take
"rad_max"           - if 0 or missing value will be exactly "rad_min"
"rad_max_val"       - Defaults to 0, which means uncapped
"rad_chance"        - Chance to get more radiation
"rad_chance_bot"
"rad_tick"          - Defaults to every tick

"hunger_amount"     - Amount of hunger it can give/take.
"hunger_min"        - Minimal amount of hunger, certain effect will give/take
"hunger_max"        - if 0 or missing value will be exactly "hunger_min"
"hunger_min_val"    - Defaults to 0, which means uncapped
"hunger_max_val"    - Defaults to 0, which means uncapped
"hunger_chance"     - Chance to become more hungry
"hunger_chance_bot"
"hunger_tick"       - Defaults to every tick

"thirst_amount"     - Amount of thirst it can give/take.
"thirst_min"        - Minimal amount of thirst, certain effect will give/take
"thirst_max"        - if 0 or missing value will be exactly "thirst_min"
"thirst_min_val"    - Defaults to 0, which means uncapped
"thirst_max_val"    - Defaults to 0, which means uncapped
"thirst_chance"     - Chance to become more thirsty
"thirst_chance_bot"
"thirst_tick"       - Defaults to every tick

"perspiration_amount"     - Amount of perspiration it can give/take.
"perspiration_min"        - Minimal amount of perspiration, certain effect will give/take
"perspiration_max"        - if 0 or missing value will be exactly "perspiration_min"
"perspiration_min_val"    - Defaults to 0, which means uncapped
"perspiration_max_val"    - Defaults to 0, which means uncapped
"perspiration_chance"     - Chance to perspire
"perspiration_chance_bot"
"perspiration_tick"       - Defaults to every tick

"sleepiness_amount"    - Amount of sleepiness it can give/take. After certain amount character will need to sleep.
"sleepiness_min"       - Minimal amount of sleepiness, certain effect will give/take
"sleepiness_max"       - if 0 or missing value will be exactly "sleepiness_min"
"sleepiness_min_val"   - Defaults to 0, which means uncapped
"sleepiness_max_val"   - Defaults to 0, which means uncapped
"sleepiness_chance"    - Chance to get more tired
"sleepiness_chance_bot"
"sleepiness_tick"      - Defaults to every tick

"stamina_amount"    - Amount of stamina it can give/take.
"stamina_min"       - Minimal amount of stamina, certain effect will give/take
"stamina_max"       - if 0 or missing value will be exactly "stamina_min"
"stamina_min_val"   - Defaults to 0, which means uncapped
"stamina_max_val"   - Defaults to 0, which means uncapped
"stamina_chance"    - Chance to get stamina changes
"stamina_chance_bot"
"stamina_tick"      - Defaults to every tick

"heart_rate_amount"        - Amount of heart rate changes it can give/take.
"heart_rate_min"           - Minimal amount of heart rate, certain effect will give/take
"heart_rate_max"           - if 0 or missing value will be exactly "heart_rate_min"
"heart_rate_min_val"       - Defaults to 0, which means uncapped
"heart_rate_max_val"       - Defaults to 0, which means uncapped
"heart_rate_chance"        - Chance to change heart rate
"heart_rate_chance_bot"
"heart_rate_tick"          - Defaults to every tick

"blood_pressure_amount"    - Amount of blood pressure changes it can give/take.
"blood_pressure_min"       - Minimal amount of blood pressure, certain effect will give/take
"blood_pressure_max"       - if 0 or missing value will be exactly "blood_pressure_min"
"blood_pressure_min_val"   - Defaults to 0, which means uncapped
"blood_pressure_max_val"   - Defaults to 0, which means uncapped
"blood_pressure_chance"    - Chance to change blood pressure
"blood_pressure_chance_bot"
"blood_pressure_tick"      - Defaults to every tick

"respiratory_rate_amount"  - Amount of respiratory rate changes it can give/take.
"respiratory_rate_min"     - Minimal amount of respiratory rate, certain effect will give/take
"respiratory_rate_max"     - if 0 or missing value will be exactly "respiratory_rate_min"
"respiratory_rate_min_val" - Defaults to 0, which means uncapped
"respiratory_rate_max_val" - Defaults to 0, which means uncapped
"respiratory_rate_chance"  - Chance to change respiratory rate
"respiratory_rate_chance_bot"
"respiratory_rate_tick"    - Defaults to every tick

"cough_chance"      - Chance to cause cough
"cough_chance_bot"
"cough_tick"        - Defaults to every tick

// It is important to not vomit_chance interacts with vomit_multiplier in mutations, and as such is hardcoded. Base vomit chance is intensity/(base vomit chance + scaling vomit chance).
"vomit_chance"      - Chance to cause vomiting
"vomit_chance_bot"
"vomit_tick"        - Defaults to every tick

"healing_rate"      - Healed rate per day
"healing_head"      - Percentage of healing value for head
"healing_torso"     - Percentage of healing value for torso
"enchantments" - (_optional_) List of enchantments applied by this effect (see MAGIC.md for instructions on enchantment. NB: enchantments are not necessarily magic.) Values can either be the enchantments id or an inline definition of the enchantment.

"dodge_mod"         - Effective dodge chance
"hit_mod"           - Effective melee skill
"bash_mod"          - Additional bash bonus/penalty

```
Each argument can also take either one or two values.
```C++
    "thirst_min": [1]
    or
    "thirst_min": [1, 2]
```
If an effect is "resisted" (either through "resist_effect" or "resist_trait") then it will use the second
value. If there is only one value given it will always use that amount.

Base mods and Scaling mods:
While on intensity = 1 an effect will only have the basic effects of its "base_mods". However for each
additional intensity it gains it adds the value of each of its "scaling_mods" to the calculations. So:
```C++
Intensity 1 values = base_mods values
Intensity 2 values = base_mods values + scaling_mods values
Intensity 3 values = base_mods values + 2 * scaling_mods values
Intensity 4 values = base_mods values + 3 * scaling_mods values
```
and so on.

Special case:
The only special case is if base_mods' "X_chance_bot" + intensity * scaling_mods' "X_chance_bot" = 0 then it treats it
as if it were equal to 1 (i.e. trigger every time)

## Example Effect
```C++
    "type": "effect_type",
    "id": "drunk",
    "name": [
        "Tipsy",
        "Drunk",
        "Trashed",
        "Wasted"
    ],
    "max_intensity": 4,
    "apply_message": "You feel lightheaded.",
    "int_dur_factor": 1000,
    "miss_messages": [["You feel woozy.", 1]],
    "base_mods": {
        "str_mod": [1],
        "vomit_chance": [-43],
        "sleep_chance": [-1003],
        "sleep_min": [2500],
        "sleep_max": [3500]
    },
    "scaling_mods": {
        "str_mod": [-0.67],
        "per_mod": [-1],
        "dex_mod": [-1],
        "int_mod": [-1.42],
        "vomit_chance": [21],
        "sleep_chance": [501]
    }
```
First when "drunk" is applied to the player if they aren't already drunk it prints the message,
"You feel lightheaded". It also adds the "You feel woozy" miss message for as long as it is applied.
It has "int_dur_factor": 1000, meaning that its intensity will always be equal to its duration / 1000 rounded up, and
it has "max_intensity": 4 meaning the highest its intensity will go is 4 at a duration of 3000 or higher.
As it moves up through the different intensities, its name will change. Its description will simply display the stat
changes, with no additional description added.

As it moves up through the intensity levels its effects will be:
```C++
Intensity 1
    +1 STR
    No other effects (since both "X_chance"s are negative)
Intensity 2
    1 - .67 = .33 =         0 STR (Round towards zero)
    0 - 1 =                 -1 PER
    0 - 1 =                 -1 DEX
    0 -1.42 =               -1 INT
    -43 + 21 =              still negative, so no vomiting
    -1003 + 501 =           still negative, so no passing out
Intensity 3
    1 - 2 * .67 = -.34 =    0 STR (round towards zero)
    0 - 2 * 1 =             -2 PER
    0 - 2 * 1 =             -2 DEX
    0 - 2 * 1.43 =          -2 INT
    -43 + 2 * 21 = -1       still negative, no vomiting
    -1003 + 2 * 501 = -1    still negative, no passing out
Intensity 4
    1 - 3 * .67 = - 1.01 =  -1 STR
    0 - 3 * 1 =             -3 PER
    0 - 3 * 1 =             -3 DEX
    0 - 3 * 1.43 =          -4 INT
    -43 + 3 * 21 = 20       "vomit_chance_bot" doesn't exist, so a 1 in 20 chance of vomiting. "vomit_tick" doesn't exist, so it rolls every turn.
    -1003 + 3 * 501 = 500   "sleep_chance_bot" doesn't exist, so a 1 in 500 chance of passing out for rng(2500, 3500) turns. "sleep_tick" doesn't exist, so it rolls every turn.
```

### Blood analysis description
```C++
    "blood_analysis_description": "Minor Painkiller"
```
This description will be displayed for every effect which has this field when character conducts a blood analysis (for example, through Blood Analysis CBM).
