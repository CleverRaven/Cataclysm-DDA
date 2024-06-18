# Martial arts and Techniques

### Martial arts

```C++
{
  "type": "martial_art",
  "id": "style_debug",        // Unique ID. Must be one continuous word,
                              // use underscores if necessary.
  "name": "Debug Mastery",    // In-game name displayed
  "description": "A secret martial art used only by developers and cheaters.",    // In-game description
  "initiate": [ "You stand ready.", "%s stands ready." ],     // Message shown when player or NPC chooses this art
  "autolearn": [ [ "unarmed", "2" ] ],     // A list of skill requirements that if met, automatically teach the player the martial art
  "teachable": true,          // Whether it's possible to teach this style between characters
  "allow_all_weapons": true,             // This martial art works always, no matter what weapon you equip (including no weapon)
  "force_unarmed": true,                 // You never use equipped item to make attacks with this martial art, and will use only your fist, legs or another limbs you have
  "prevent_weapon_blocking": true,       // You never block using weapon with this martial art
  "strictly_melee": true,                // This martial art can be used only with some weapon, no way to use it with bare hands
  "arm_block_with_bio_armor_arms": true, // Using this martial art, you can block damage using Arms Alloy Plating CBM
  "leg_block_with_bio_armor_legs": true, // Using this martial art, you can block damage using Legs Alloy Plating CBM
  "autolearn": [ [ "melee", 1 ] ], // This martial art is autolearned once you reach this level in specific skill or skills
  "primary_skill": "bashing", // The difficulty and effectiveness of this martial art scales from this skill; unarmed by default
  "learn_difficulty": 5,      // Difficulty to learn a style from book based on "primary skill"
                              // Total chance to learn a style from a single read of the book is equal to one in (10 + learn_difficulty - primary_skill)
  "arm_block": 99,            // Unarmed skill level at which arm blocking is unlocked
  "leg_block": 99,            // Unarmed skill level at which arm blocking is unlocked
  "nonstandard_block": 99,    // Unarmed skill level at which blocking with "nonstandard" mutated limbs is unlocked
  "static_buffs": [           // List of buffs that are automatically applied every turn; this syntax is applied for every field with `_buffs` in it's name
    {
      "id": "debug_elem_resist" // for detals of syntax see Buffs
    }
  ],
  "onmove_buffs": [],         // List of buffs that are automatically applied on movement
  "onpause_buffs": [],        // List of buffs that are automatically applied when passing a turn
  "onhit_buffs": [],          // List of buffs that are automatically applied on successful hit
  "onattack_buffs": [],       // List of buffs that are automatically applied after any attack, hit or miss
  "ondodge_buffs": [],        // List of buffs that are automatically applied on successful dodge
  "onblock_buffs": [],        // List of buffs that are automatically applied on successful block
  "ongethit_buffs": [],       // List of buffs that are automatically applied on being hit
  "onmiss_buffs": [],         // List of buffs that are automatically applied on a miss
  "oncrit_buffs": [],         // List of buffs that are automatically applied on a crit
  "onkill_buffs": [],         // List of buffs that are automatically applied upon killing an enemy
  "static_eocs": [            // List of eocs that are automatically triggered every turn
    "EOC_ID",
    { "id": "INLINE_EOC_ID" }
  ],
  "onmove_eocs": [],          // List of eocs that are automatically triggered on movement
  "onpause_eocs": [],         // List of eocs that are automatically triggered when passing a turn
  "onhit_eocs": [],           // List of eocs that are automatically triggered on successful hit
  "onattack_eocs": [],        // List of eocs that are automatically triggered after any attack, hit or miss
  "ondodge_eocs": [],         // List of eocs that are automatically triggered on successful dodge
  "onblock_eocs": [],         // List of eocs that are automatically triggered on successful block
  "ongethit_eocs": [],        // List of eocs that are automatically triggered on being hit
  "onmiss_eocs": [],          // List of eocs that are automatically triggered on a miss
  "oncrit_eocs": [],          // List of eocs that are automatically triggered on a crit
  "onkill_eocs": [],          // List of eocs that are automatically triggered upon killing an enemy
  "techniques": [             // List of techniques available when this martial art is used
    "tec_debug_slow",
    "tec_debug_arpen"
  ],
  "weapons": [ "tonfa" ],     // List of weapons usable with this art
  "weapon_category": [ "WEAPON_CAT1" ] // Weapons that have one of the categories in here are usable with this art.
}
```

### Techniques

```JSON
{
  "id": "tec_debug_arpen",    // Unique ID. Must be one continuous word
  "name": "phasing strike",   // In-game name displayed
  "attack_vectors": [ "vector_1", "vector_2" ], // What attack vector would be used for this technique; For more info see Attack vectors below
  "unarmed_allowed": true,    // Can an unarmed character use this technique
  "weapon_categories_allowed": [ "BLADES", "KNIVES" ], // Restrict technique to only these categories of weapons. If omitted, all weapon categories are allowed.
  "melee_allowed": true,      // Means that ANY melee weapon can be used, NOT just the martial art's weapons
  "powerful_knockback": true, //
  "skill_requirements": [ { "name": "melee", "level": 3 } ],     // Skills and their minimum levels required to use this technique. Can be any skill.
  "weapon_damage_requirements": [ { "type": "bash", "min": 5 } ],     // Minimum weapon damage required to use this technique. Can be any damage type.
  "required_buffs_any": [ "eskrima_hit_buff" ],    // This technique requires any of the named buffs to be active
  "required_buffs_all": [ "eskrima_hit_buff" ],    // This technique requires all of the named buffs to be active
  "forbidden_buffs_any": [ "eskrima_hit_buff" ],    // This technique is forbidden if any of the named buffs are active
  "forbidden_buffs_all": [ "eskrima_hit_buff" ],    // This technique is forbidden if all of the named buffs are active
  "req_flags": [ "" ],        // List of item flags the used weapon needs to be eligible for the technique
  "required_char_flags": [ "" ],    // List of "character" (bionic, trait, effect or bodypart) flags the character needs to be able to use this technique
  "required_char_flags_all": [ "" ], // This technique requires all of the listed character flags to trigger
  "forbidden_char_flags": [ "" ],   // List of character flags disabling this technique
  "needs_ammo": true,         // Technique works only if weapon is loaded; Consume 1 charge per attack 
  "crit_tec": true,           // This technique only works on a critical hit
  "crit_ok": true,            // This technique works on both normal and critical hits
  "reach_tec": true,          // This technique only works on a reach attack hit
  "reach_ok": true,           // This technique works on both normal and reach attack hits
  "condition": "u_is_outside",// Optional (array of) dialog conditions the attack requires to trigger.  Failing these will disqualify the tech from being selected
  "condition_desc": "Needs X",// Description string describing the conditions of this attack (since dialog conditions can't be automatically evaluated)       
  "repeat_min": 1,            // Technique's damage and any added effects are repeated rng(repeat_min, repeat_max) times. The target's armor and the effect's chances are applied for each repeat.
  "repeat_max": 1,
  "knockback_dist": 1,        // Distance target is knocked back
  "knockback_spread": 1,      // The knockback may not send the target straight back
  "knockback_follow": true,   // Attacker will follow target if they are knocked back
  "stun_dur": 2,              // Duration that target is stunned for
  "down_dur": 2,              // Duration that target is downed for
  "side_switch": true,        // Technique moves the target behind user
  "disarms": true,            // This technique can disarm the opponent, triggers only against armed NPCs or monsters with the "WIELDED_WEAPON" flag
  "take_weapon": true,        // Technique will disarm and equip target's weapon if hands are free
  "grab_break": true,         // This technique may break a grab against the user
  "aoe": "spin",              // This technique has an area-of-effect; doesn't work against solo targets
  "block_counter": true,      // This technique may automatically counterattack on a successful block
  "dodge_counter": true,      // This technique may automatically counterattack on a successful dodge
  "weighting": 2,             // Affects likelihood this technique will be selected when many are available. Negative weighting means the technique is only included in the list of possible techs once out of every `weighting` times ( 1/3 for a weighting of -3)
  "defensive": true,          // Game won't try to select this technique when attacking
  "miss_recovery": true,      // Misses while attacking will use half as many moves
  "messages": [               // What is printed when this technique is used by the player and by an npc
    "You phase-strike %s",
    "<npcname> phase-strikes %s"
  ],
  "eocs": [ "EOC_ID", { "id": "INLINE_EOC_ID" } ],    // EOCs that trigger each time this technique does, with the attacker as the speaker and the target as the listener
  "mult_bonuses": <array>,     // Any bonuses, as described below
  "flat_bonuses": <array>,
  "tech_effects": <array>      // List of effects applied by this technique, see below
}
```

### Attack vectors

Attack vectors define which (sub)bodypart is used for the attack in question, allow filtering of eligable bodyparts and apply the relevant worn armor's unarmed damage to the attack. Note for the (sub)part to apply its unarmed damage it needs unrestricted natural attacks.

```JSON
[ 
  {
    "type": "attack_vector",          // Always attack_vector
    "id": "vector_hand",              // ID
    "limbs": [ "hand_l", "hand_r" ],  // List of bodyparts used in this attack (relevant for HP/encumbrance/flag filtering)
    "contact_area": [ "hand_fingers_l", "hand_fingers_r" ],  // List of subbodyparts that can be used as a strike surface in the attack using the sbp's armor or intrinsic unarmed damage
    "strict_limb_definition": false,  // Bool, default false. When true *only* the bodyparts defined above are used for the vector, otherwise similar bodyparts can be used as long as both the contact area and the defined limb are similar, see JSON_INFO.md/Bodyparts for bodypart similarity
    "armor_bonus": true,              // Bool, default true, defines if the vector takes the unarmed damage bonus of the armor worn on the contact area into account
    "required_limb_flags": [ "foo", "bar" ],  // List of character flags required for the bodypart to be eligable for this vector
    "forbidden_limb_flags": [ "foo", "bar" ], // List of character flags that disqualify a limb from being usable by this vector
    "encumbrance_limit": 15,          // Int, default 100, encumbrance of the limb above this will disqualify it from this vector
    "bp_hp_limit": 75 ,                // Int, default 10, percent of bodypart limb HP necessary for the limbs to qualify for this vector. For minor (non-main) bodyparts the corresponding main part HP is taken into account.
    "limb_req": [ [ "arm", 2] ]        // Array of pairs. Limb type requirements for this vector. The character must have this many limbs of the given type above the limb's health limit (See JSON_INFO.md:Bodyparts). Requirements must all be met.

  }
]
```

### Tech effects
```C++
"tech_effects": [
  {
    "id": "eff_expl",    // Unique ID. Must be one continuous word
    "chance": 100,       // Percent chance to apply the effect on this attack
    "permanent": false,  // If true the effect won't decay (default false)
    "duration": 15,      // Duration of the effect in turns
    "on_damage": true,   // If true the effect will only be applied if the attack succeeded in doing damage (default true)
    "req_flag": "ANY",   // A single arbitrary character flag (from traits, bionics, effects, or bodyparts) required to apply this effect
    "message": "Example" // The message to print if you successfully apply the effect, %s can be substituted for the target's name
  }
]
```

### Buffs

```C++
{
  "id": "debug_elem_resist",         // Unique ID. Must be one continuous word
  "name": "Elemental Resistance",    // In-game name displayed, would be shown in character menu
  "description": "You are a walking tank!\n\nBash armor increased by 100% of Strength, Cut armor increased by 100% of Dexterity, Electricic armor increased by 100% of Intelligence, and Fire armor increased by 100% of Perception", // In-game description, would be shown in character menu
  "buff_duration": 2,                // Duration in turns that this buff lasts
  "persists": false,                 // Allow buff to remain when changing to a new style
  "unarmed_allowed": true,           // Effect is applied when you have no weapon equipped
  "melee_allowed": true,             // Effect is applied when you have some melee weapon equipped
  "strictly_unarmed": true,          // Effect is applied only when you have no weapon whatsoever, even unarmed weapon
  "wall_adjacent": true,            // Effect is applied when you stand near the wall
  "weapon_categories_allowed": [ "BLADES", "KNIVES" ], // Restrict buff to only these categories of weapons. If omitted, all weapon categories are allowed.
  "max_stacks": 8,                   // Maximum number of stacks on the buff. Buff bonuses are multiplied by current buff intensity
  "bonus_blocks": 1,                 // Extra blocks per turn
  "bonus_dodges": 1,                 // Extra dodges per turn
  "flat_bonuses": [],                // Flat bonuses, see below
  "mult_bonuses": [],                // Multiplicative bonuses, see below
}
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
"scaling-stat": scaling stat, any of: "str", "dex", "int", "per", "bashing", "cutting", "dodge", "melee", "stabbing", "swimming", "unarmed", "marksmanship", "pistols", "rifles", "shotguns", "SMGs", "archery", "throwing", "launchers", "driving", "health care", "spellcraft". Optional. If the scaling stat is specified, the value of the bonus is multiplied by the corresponding user stat/skill.

Bonuses must be written in the correct order.

Tokens of `useless` type will not cause an error, but will not have any effect.
For example, `speed` in a technique will have no effect (`movecost` should be used for techniques). Multiplicative bonuses are only for `damage` and `movecost`.

Flat bonuses are applied after multiplicative bonuses.

Examples:
Incoming bashing damage is decreased by 30% of strength value. Only useful on buffs:
* `flat_bonuses: [ { "stat": "armor", "type": "bash", "scaling-stat": "str", "scale": 0.3 } ]`

All cutting damage dealt is multiplied by `(10% of dexterity)*(damage)`:
* `mult_bonuses: [ { "stat": "damage", "type": "cut", "scaling-stat": "dex", "scale": 0.1 } ]`

Move cost is decreased by 100% of strength value
* `flat_bonuses: [ { "stat": "movecost", "scaling-stat": "str", "scale": -1.0 } ]`

### Place relevant items in the world and chargen

Starting trait selection of your martial art goes in mutations.json. Place your art in the right category (self-defense, Shaolin animal form, melee style, etc)

Use json/itemgroups/ to place your martial art book and any martial weapons you've made for the art into spawns in various locations in the world. If you don't place your weapons in there, only recipes to craft them will be an option.
