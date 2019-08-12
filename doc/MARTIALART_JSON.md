# Martial arts and techniques JSON file contents

### Martial arts

```C++
"type" : "martial_art",
"id" : "style_debug",       // Unique ID. Must be one continuous word,
                            // use underscores if necessary.
"name" : "Debug Mastery",   // In-game name displayed
"description": "A secret martial art used only by developers and cheaters.",    // In-game description
"initiate": [ "You stand ready.", "%s stands ready." ],     // Message shown when player or NPC chooses this art
"autolearn": [ [ "unarmed", "2" ] ],     // A list of skill requirements that if met, automatically teach the player the martial art
"learn_difficulty": 5,      // Difficulty to learn a style from book based on "primary skill" 
                            // Total chance to learn a style from a single read of the book is equal to one in (10 + learn_difficulty - primary_skill)
"arm_block" : 99,           // Unarmed skill level at which arm blocking is unlocked
"leg_block" : 99,           // Unarmed skill level at which arm blocking is unlocked
"static_buffs" : [          // List of buffs that are automatically applied every turn
    "id" : "debug_elem_resist",
    "heat_arm_per" : 1.0
],
"ondodge_buffs" : []        // List of buffs that are automatically applied on successful dodge
"onattack_buffs" : []       // List of buffs that are automatically applied after any attack, hit or miss
"onhit_buffs" : []          // List of buffs that are automatically applied on successful hit
"onmove_buffs" : []         // List of buffs that are automatically applied on movement
"onmiss_buffs" : []         // List of buffs that are automatically applied on a miss
"oncrit_buffs" : []         // List of buffs that are automatically applied on a crit
"onkill_buffs" : []         // List of buffs that are automatically applied upon killing an enemy
"techniques" : [            // List of techniques available when this martial art is used
    "tec_debug_slow",
    "tec_debug_arpen"
]
"weapons": [ "tonfa" ]      // List of weapons usable with this art

```

### Techniques

```C++
"id" : "tec_debug_arpen",   // Unique ID. Must be one continuous word
"name" : "phasing strike",  // In-game name displayed
"unarmed_allowed" : true,   // Can an unarmed character use this technique
"unarmed_weapons_allowed" : true,    // Does this technique require the character to be actually unarmed or does it allow unarmed weapons
"melee_allowed" : true,     // Means that ANY melee weapon can be used, NOT just the martial art's weapons 
"min_melee" : 3,            // Minimum skill and its level required to use this technique. Can be any skill.
"req_buffs": [ "eskrima_hit_buff" ],    // This technique requires a named buff to be active
"crit_tec" : true,          // This technique only works on a critical hit
"crit_ok" : true,           // This technique works on both normal and critical hits
"downed_target": true,      // Technique only works on a downed target
"stunned_target": true,     // Technique only works on a stunned target
"knockback_dist": 1,        // Distance target is knocked back
"knockback_spread": 1,      // The knockback may not send the target straight back
"knockback_follow": 1,      // Attacker will follow target if they are knocked back
"stun_dur": 2,              // Duration that target is stunned for
"down_dur": 2,              // Duration that target is downed for
"disarms": true,            // This technique can disarm the opponent
"grab_break": true,         // This technique may break a grab against the user
"aoe": "spin",              // This technique has an area-of-effect; doesn't work against solo targets
"block_counter": true,      // This technique may automatically counterattack on a successful block
"dodge_counter": true,      // This technique may automatically counterattack on a successful dodge
"weighting": 2,             // Affects likelihood this technique will be seleted when many are available
"defensive": true,          // Game won't try to select this technique when attacking
"miss_recovery": true,      // Misses while attacking will use fewer moves
"messages" : [              // What is printed when this technique is used by the player and by an npc
    "You phase-strike %s",
    "<npcname> phase-strikes %s"
]
"movecost_mult" : 0.3       // Any bonuses, as described below
```

### Buffs

```C++
"id" : "debug_elem_resist",         // Unique ID. Must be one continuous word
"name" : "Elemental resistance",    // In-game name displayed
"description" : "+Strength bash armor, +Dexterity acid armor, +Intelligence electricity armor, +Perception fire armor.",    // In-game description
"buff_duration": 2,                 // Duration in turns that this buff lasts
"unarmed_allowed" : true,           // Can this buff be applied to an unarmed character
"unarmed_allowed" : false,          // Can this buff be applied to an armed character
"unarmed_weapons_allowed" : true,          // Does this buff require the character to be actually unarmed. If true, allows unarmed weapons (brass knuckles, punch daggers)
"max_stacks" : 8,                   // Maximum number of stacks on the buff. Buff bonuses are multiplied by current buff intensity
"bonus_blocks": 1       // Extra blocks per turn
"bonus_dodges": 1       // Extra dodges per turn
"flat_bonuses" : [                  // Flat bonuses
    ["armor", "bash", "str", 1.0],
    ["armor", "cut", "dex", 1.0],
    ["armor", "electric", "int", 1.0],
    ["armor", "heat", "per", 1.0]
],
"mult_bonuses" : [                  // Multiplicative bonuses
    ["damage", "bash", 2.0],
    ["damage", "heat", "int", 1.1]
]
```

### Bonuses

Bonuses contain 2 to 4 of the following tokens, in order:

* Affected statistic. Any of: "hit", "dodge", "block", "speed", "movecost", "damage", "armor", "arpen"
* Damage type ("bash", "cut", "heat", etc.) if the affected statistic is damage, armor, or arpen
* Scaling stat. Any of: "str", "dex", "int", "per"
* The value of the bonus itself

Bonuses must be written in the correct order.

If the affected statistic requires a damage type, a damage type must be provided. Otherwise, damage type must not be specified.

If the scaling stat is specified, the value of the bonus is multiplied by the corresponding user stat.

Tokens of `useless` type will not cause an error, but will not have any effect.
For example, `speed` in a technique will have no effect (`movecost` should be used for techniques).

Currently extra elemental damage is not applied, but extra elemental armor is (after regular armor).

Examples:
* `flat_bonuses : [["armor", "bash", "str", 0.3]], // Incoming bashing damage is decreased by 30% of strength value. Only useful on buffs`
* ``mult_bonuses : [["damage", "cut", "dex", 0.1]], // All cutting damage dealt is multiplied by `(10% of dexterity)*(damage)` ``
* `flat_bonuses : [["movecost", "str", -1.0]],     // Move cost is decreased by 100% of strength value`

### Place relevant items in the world and chargen

Starting trait selection of your martial art goes in mutations.json. Place your art in the right category (self-defense, Shaolin animal form, melee style, etc)

Use json/itemgroups/ to place your martial art book and any martial weapons you've made for the art into spawns in various locations in the world. If you don't place your weapons in there, only recipes to craft them will be an option.
