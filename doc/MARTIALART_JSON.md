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
"skill_requirements": [ { "name": "melee", "level": 3 } ],     // Skills and their minimum levels required to use this technique. Can be any skill.
"weapon_damage_requirements": [ { "type": "bash", "min": 5 } ],     // Minimum weapon damage required to use this technique. Can be any damage type.
"req_buffs": [ "eskrima_hit_buff" ],    // This technique requires a named buff to be active
"crit_tec" : true,          // This technique only works on a critical hit
"crit_ok" : true,           // This technique works on both normal and critical hits
"downed_target": true,      // Technique only works on a downed target
"stunned_target": true,     // Technique only works on a stunned target
"human_target": true,       // Technique only works on a human-like target
"knockback_dist": 1,        // Distance target is knocked back
"knockback_spread": 1,      // The knockback may not send the target straight back
"knockback_follow": 1,      // Attacker will follow target if they are knocked back
"stun_dur": 2,              // Duration that target is stunned for
"down_dur": 2,              // Duration that target is downed for
"side_switch": true,        // Technique moves the target behind user
"disarms": true,            // This technique can disarm the opponent
"take_weapon": true,        // Technique will disarm and equip target's weapon if hands are free
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
"flat_bonuses" : [                  // Flat bonuses, see below
],
"mult_bonuses" : [                  // Multiplicative bonuses, see below
]
```

### Bonuses

The bonuses arrays contain any number of bonus entries like this:

```C++
{
  "stat": "damage",
  "type": "bash",
  "scaling-stat": "per",
  "scale": 0.15
}
```

"stat": affected statistic, any of: "hit", "dodge", "block", "speed", "movecost", "damage", "armor", "arpen",
"type": damage type for the affected statistic ("bash", "cut", "heat", etc.), only needed if the affected statistic is "damage", "armor", or "arpen".
"scale": the value of the bonus itself.
"scaling-stat": scaling stat, any of: "str", "dex", "int", "per". Optional. If the scaling stat is specified, the value of the bonus is multiplied by the corresponding user stat.

Bonuses must be written in the correct order.

Tokens of `useless` type will not cause an error, but will not have any effect.
For example, `speed` in a technique will have no effect (`movecost` should be used for techniques).

Currently extra elemental damage is not applied, but extra elemental armor is (after regular armor).

Examples:
Incoming bashing damage is decreased by 30% of strength value. Only useful on buffs:
* `flat_bonuses : [ { "stat": "armor", "type": "bash", "scaling-stat": "str", "scale": 0.3 } ]`

All cutting damage dealt is multiplied by `(10% of dexterity)*(damage)`:
* `mult_bonuses : [ { "stat": "damage", "type": "cut", "scaling-stat": "dex", "scale": 0.1 } ]`

Move cost is decreased by 100% of strength value
* `flat_bonuses : [ { "stat": "movecost", "scaling-stat": "str", "scale": -1.0 } ]`

### Place relevant items in the world and chargen

Starting trait selection of your martial art goes in mutations.json. Place your art in the right category (self-defense, Shaolin animal form, melee style, etc)

Use json/itemgroups/ to place your martial art book and any martial weapons you've made for the art into spawns in various locations in the world. If you don't place your weapons in there, only recipes to craft them will be an option.
