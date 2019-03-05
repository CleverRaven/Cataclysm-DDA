# Martial arts and techniques JSON file contents

### Martial arts

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

### Techniques

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

### Buffs

```C++
"id" : "debug_elem_resist",         // Unique ID. Must be one continuous word
"name" : "Elemental resistance",    // In-game name displayed
"description" : "+Strength bash armor, +Dexterity acid armor, +Intelligence electricity armor, +Perception fire armor.",    // In-game description
"unarmed_allowed" : true,           // Can this buff be applied to an unarmed character
"unarmed_allowed" : false,          // Can this buff be applied to an armed character
"strictly_unarmed" : true,          // Does this buff require the character to be actually unarmed. If false, allows unarmed weapons (brass knuckles, punch daggers)
"max_stacks" : 8,                   // Maximum number of stacks on the buff. Buff bonuses are multiplied by current buff intensity
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
