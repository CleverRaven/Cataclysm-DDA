# Martial arts and technique JSON file contents

###Martial arts
```C++
"type" : "martial_art", 
"id" : "style_debug",       // Unique ID. Must be one continuous word,
                            // use underscores if necessary.
"name" : "Debug Mastery",   // In-game name displayed
"description": "A secret martial art used only by developers and cheaters.",    // In-game description
"arm_block" : 99,           // Unarmed skill level at which arm blocking is unlocked
"leg_block" : 99,           // Unarmed skill level at which arm blocking is unlocked
"static_buffs" : [          // List of buffs that are automatically applied every turn
    "id" : "debug_elem_resist",
    "heat_arm_per" : 1.0
],
"ondodge_buffs" : []        // List of buffs that are automatically applied on successful dodge
"onhit_buffs" : []          // List of buffs that are automatically applied on successful hit
"techniques" : [            // List of techniques available when this martial art is used
    "tec_debug_slow",
    "tec_debug_arpen"
]
```

###Techniques
```C++
"id" : "tec_debug_arpen",   // Unique ID. Must be one continuous word
"name" : "phasing strike",  // In-game name displayed
"unarmed_allowed" : true,   // Can an unarmed character use this technique
"strict_unarmed" : true,    // Does this buff require the character to be actually unarmed or does it allow unarmed weapons
"melee_allowed" : true,     // Can an armed character use this technique
"min_melee" : 3,            // Minimum skill and its level required to use this technique. Can be any skill.
"crit_tec" : true,          // Can this technique be used on crit (and only on crit)?
"messages" : [              // What is printed when this technique is used by the player and by an npc
    "You phase-strike %s",
    "<npcname> phase-strikes %s"
]
"movecost_mult" : 0.3       // Any bonuses, as described below
```

###Buffs
```C++
"id" : "debug_elem_resist",         // Unique ID. Must be one continuous word
"name" : "Elemental resistance",    // In-game name displayed
"description" : "+Strength bash armor, +Dexterity acid armor, +Intelligence electricity armor, +Perception fire armor.",    // In-game description
"unarmed_allowed" : true,           // Can this buff be applied to an unarmed character
"unarmed_allowed" : false,          // Can this buff be applied to an armed character
"strictly_unarmed" : true,          // Does this buff require the character to be actually unarmed. If false, allows unarmed weapons (brass knuckles, punch daggers)
"max_stacks" : 8,                   // Maximum number of stacks on the buff. Buff bonuses are multiplied by current buff intensity
"bash_arm_str" : 1.0,               // Bonuses
"cut_arm_dex" : 1.0,
"electric_arm_int" : 1.0,
"heat_arm_per" : 1.0
```

###Bonuses
Bonuses contain 2 to 4 tokens, in any order:
* "mult" tag, which describes multiplicative bonuses
* Affected statistic. Any of: "hit", "dodge", "block", "speed", "movecost", "damage", "armor", "arpen"
* Damage type ("bash", "cut", "heat", etc.) when the statistic is damage, armor or arpen
* Scaling stat. Any of: "str", "dex", "int", "per"

If statistic isn't specified, but damage type is, bonus is set to damage.

Tokens of the same type must not be encountered more than once in the same bonus.
For example, "dex_str_bash" will cause an error.

Tokens of "useless" type will not cause an error, but will not have any effect.
For example, "speed_mult" in a technique will have no effect ("movecost_mult" should be used for techniques).

Currently extra elemental damage is not applied, but extra elemental armor is (after regular armor).
Currently all bonuses that affect cutting also affect stabbing, unless a stabbing bonus of the same type, stat and "mult" is specified.

Examples:
* "bash_arm_str" : 0.3, // Incoming bashing damage is decreased by 30% of strength value. Only useful on buffs
* "cut_dex_mult" : 0.1, // All cutting and stabbing damage is increased by `(10% of dexterity)*(damage)`
* "stab_cut_mult" : 0.0, // If specified with the above, will remove the stabbing bonus and only apply it to cutting.
* "movecost_str" : -1.0, // Move cost is decreased by 100% of strength value
