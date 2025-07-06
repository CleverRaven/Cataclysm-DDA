# Martial arts and Techniques

A Martial art (MA for short) just like in real life, is a set of physical practices, skills and stances for combat and sport.  In CDDA, these represent different combat styles of the survivor.

These are defined in JSON as `martial_art`, which sets rules for usage and application, and importantly, the list of `techniques` that can be used.


## Martial arts

```jsonc
{
  "type": "martial_art",
  "id": "style_debug",                   // Unique ID. Must be one continuous word, use underscores if necessary
  "name": "Debug Mastery",               // In-game name displayed
  "description": "A secret martial art used only by developers and cheaters.",    // In-game description
  "initiate": [ "You stand ready.", "%s stands ready." ],    // Message shown when player or NPC selects this MA
  "autolearn": [ [ "unarmed", "2" ] ],   // A list of skill requirements that if met, automatically teach the player the MA
  "teachable": true,                     // Whether it's possible to teach this style between characters
  "allow_all_weapons": true,             // This MA always works, no matter what weapon is equipped (including no weapon)
  "force_unarmed": true,                 // You never use the equipped item to make attacks with this MA, and will use only your fist, legs or another limbs you have
  "prevent_weapon_blocking": true,       // You never block using weapon with this MA
  "strictly_melee": true,                // This style can only be used with weapons, it can't be used with bare hands
  "arm_block_with_bio_armor_arms": true, // Enables blocking damage using the Arms Alloy Plating CBM
  "leg_block_with_bio_armor_legs": true, // Enables blocking damage using the Legs Alloy Plating CBM
  "autolearn": [ [ "melee", 1 ] ],       // The style is autolearned once you reach this level in specific skill or skills
  "primary_skill": "bashing",            // The difficulty and effectiveness of this MA scales from this skill (Default is "unarmed")
  "learn_difficulty": 5,                 // Difficulty to learn the style from a book based on "primary_skill". Total chance to learn a style from a single read of the book is equal to one in (10 + learn_difficulty - primary_skill)
  "arm_block": 99,                       // Unarmed skill level at which arm blocking is unlocked
  "leg_block": 99,                       // Unarmed skill level at which arm blocking is unlocked
  "nonstandard_block": 99,               // Unarmed skill level at which blocking with "nonstandard" mutated limbs is unlocked
  "static_buffs": [ { "id": "debug_elem_resist" } ],    // List of buffs that are automatically applied every turn. Same syntax for the following fields with "_buffs" in their name. For further details, see Martial art buffs
  "onmove_buffs": [  ],                  // List of buffs that are automatically applied on movement
  "onpause_buffs": [  ],                 // List of buffs that are automatically applied when passing a turn
  "onattack_buffs": [  ],                // List of buffs that are automatically applied after any attack, hit or miss
  "onhit_buffs": [  ],                   // List of buffs that are automatically applied on successful hit
  "onmiss_buffs": [  ],                  // List of buffs that are automatically applied on a miss
  "oncrit_buffs": [  ],                  // List of buffs that are automatically applied on a crit
  "ongethit_buffs": [  ],                // List of buffs that are automatically applied on being hit
  "ondodge_buffs": [  ],                 // List of buffs that are automatically applied on successful dodge
  "onblock_buffs": [  ],                 // List of buffs that are automatically applied on successful block
  "onkill_buffs": [  ],                  // List of buffs that are automatically applied upon killing an enemy
  "static_eocs": [                       // List of eocs that are automatically triggered every turn
    "EOC_ID",                            // Syntax allows either eoc IDs or as inline blocks
    { "id": "INLINE_EOC_ID" }
  ],
  "onmove_eocs": [  ],                   // List of eocs that are automatically triggered on movement
  "onpause_eocs": [  ],                  // List of eocs that are automatically triggered when passing a turn
  "onattack_eocs": [  ],                 // List of eocs that are automatically triggered after any attack, hit or miss
  "onhit_eocs": [  ],                    // List of eocs that are automatically triggered on successful hit
  "onmiss_eocs": [  ],                   // List of eocs that are automatically triggered on a miss
  "oncrit_eocs": [  ],                   // List of eocs that are automatically triggered on a crit
  "ongethit_eocs": [  ],                 // List of eocs that are automatically triggered on being hit
  "ondodge_eocs": [  ],                  // List of eocs that are automatically triggered on successful dodge
  "onblock_eocs": [  ],                  // List of eocs that are automatically triggered on successful block
  "onkill_eocs": [  ],                   // List of eocs that are automatically triggered upon killing an enemy
  "techniques": [                        // List of techniques available when this MA is used
    "tec_debug_slow",
    "tec_debug_arpen"
  ],
  "weapons": [ "tonfa" ],                // List of weapons usable with this art
  "weapon_category": [ "MEDIUM_SWORDS" ] // List of weapons categories usable with this MA
}
```


### Martial art buffs

```jsonc
...
  {
    "id": "debug_elem_resist",           // Unique ID. Must be one continuous word
    "name": "Elemental Resistance",      // In-game name displayed, would be shown in character menu
    "description": "You are a walking tank!\n\nBash armor increased by 100% of Strength (...)",    // In-game description, would be shown in character menu
    "buff_duration": 2,                  // Duration in turns that this buff lasts
    "persists": false,                   // Allow buff to remain when changing to a new style
    "unarmed_allowed": true,             // Effect is applied when you have no weapon equipped
    "melee_allowed": true,               // Effect is applied when you have some melee weapon equipped
    "strictly_unarmed": true,            // Effect is applied only when you have no weapon whatsoever, even unarmed weapons
    "wall_adjacent": true,               // Effect is applied when you stand near the wall
    "weapon_categories_allowed": [ "BLADES", "KNIVES" ], // Restrict buff to only these categories of weapons. If omitted, all weapon categories are allowed
    "max_stacks": 8,                     // Maximum number of stacks on the buff. Buff bonuses are multiplied by current buff intensity
    "bonus_blocks": 1,                   // Extra blocks per turn
    "bonus_dodges": 1,                   // Extra dodges per turn
    "flat_bonuses": [  ],                // Flat bonuses, see Bonuses below
    "mult_bonuses": [  ],                // Multiplicative bonuses, see Bonuses below
  }
...
```

Martial art buffs (`static_buffs`, `onmove_buffs`, etc.) are inlined in their parent `martial_art` as seen in the [previous example](MARTIALART_JSON.md#martial-arts), and thus lack their own `type`.

Additionally, when read externally (such as, by EOCs), their syntax is `mabuff:ID`:

```jsonc
  "condition": { "u_has_effect": "mabuff:buff_aikido_static1" },
```


## Techniques

Techniques are special attacks performed while using a given MA style.  These are automatically used according to their specific requirements, as defined in their JSON blocks:

```jsonc
{
  "type": "technique",
  "id": "tec_debug_arpen",               // Unique ID. Must be one continuous word
  "name": "phasing strike",              // In-game name displayed
  "attack_vectors": [ "vector_1", "vector_2" ],    // What attack vector would be used for this technique. For more info see Attack vectors below
  "unarmed_allowed": true,               // Can an unarmed character use this technique
  "weapon_categories_allowed": [ "BLADES", "KNIVES" ],    // Restrict technique to only these categories of weapons. If omitted, all weapon categories are allowed
  "melee_allowed": true,                 // Means that ANY melee weapon can be used, NOT just the MAs weapons
  "powerful_knockback": true,            //
  "skill_requirements": [ { "name": "melee", "level": 3 } ],    // Skills and their minimum levels required to use this technique. Can be any skill
  "weapon_damage_requirements": [ { "type": "bash", "min": 5 } ],    // Minimum weapon damage required to use this technique. Can be any damage type
  "required_buffs_any": [ "eskrima_hit_buff" ],    // This technique requires any of the named buffs to be active
  "required_buffs_all": [ "eskrima_hit_buff" ],    // This technique requires all of the named buffs to be active
  "forbidden_buffs_any": [ "eskrima_hit_buff" ],   // This technique is forbidden if any of the named buffs are active
  "forbidden_buffs_all": [ "eskrima_hit_buff" ],   // This technique is forbidden if all of the named buffs are active
  "req_flags": [ "" ],                   // List of item flags the used weapon needs to be eligible for the technique
  "required_char_flags": [ "" ],         // List of "character" (bionic, trait, effect or bodypart) flags the character needs to be able to use this technique
  "required_char_flags_all": [ "" ],     // This technique requires all of the listed character flags to trigger
  "forbidden_char_flags": [ "" ],        // List of character flags disabling this technique
  "needs_ammo": true,                    // Technique works only if weapon is loaded. Consumes 1 charge per attack 
  "crit_tec": true,                      // This technique only works on a critical hit
  "crit_ok": true,                       // This technique works on both normal and critical hits
  "reach_tec": true,                     // This technique only works on a reach attack hit
  "reach_ok": true,                      // This technique works on both normal and reach attack hits
  "condition": "u_is_outside",           // Array of dialog conditions the attack requires to trigger. Failing these will disqualify the tech from being selected. Same syntax as the eoc "condition" field
  "condition_desc": "Needs X",           // Description string describing the conditions of this attack (since dialog conditions can't be automatically evaluated)       
  "repeat_min": 1,                       // Technique's damage and any added effects are repeated rng(repeat_min, repeat_max) times. The target's armor and the effect's chances are applied for each repeat
  "repeat_max": 1,
  "knockback_dist": 1,                   // Distance target is knocked back
  "knockback_spread": 1,                 // The knockback may not send the target straight back
  "knockback_follow": true,              // Attacker will follow target if they are knocked back
  "stun_dur": 2,                         // Duration that target is stunned for
  "down_dur": 2,                         // Duration that target is downed for
  "side_switch": true,                   // Technique moves the target behind user
  "disarms": true,                       // This technique can disarm the opponent, triggers only against armed NPCs or monsters with the "WIELDED_WEAPON" flag
  "take_weapon": true,                   // Technique will disarm and equip target's weapon if hands are free
  "grab_break": true,                    // This technique may break a grab against the user
  "aoe": "spin",                         // This technique has an area-of-effect. Doesn't proc against solo targets
  "block_counter": true,                 // This technique may automatically counterattack on a successful block
  "dodge_counter": true,                 // This technique may automatically counterattack on a successful dodge
  "weighting": 2,                        // Affects likelihood this technique will be selected when many are available. Negative weighting means the technique is only included in the list of possible techs once out of every `weighting` times ( 1/3 for a weighting of -3)
  "defensive": true,                     // Game won't try to select this technique when attacking
  "miss_recovery": true,                 // Misses while attacking will use half as many moves
  "messages": [                          // What is printed when this technique is used by the player and by an npc. Should NOT end with punctuation mark, for it is then extended with ` but deal no damage` or ` and deal X damage`
    "You phase-strike %s",
    "<npcname> phase-strikes %s"
  ],
  "eocs": [ "EOC_ID", { "id": "INLINE_EOC_ID" } ],    // EOCs that trigger each time this technique does, with the attacker as the speaker and the target as the listener. Syntax allows either eoc IDs or as inline blocks
  "flat_bonuses": <array>,               // Any bonuses, see Bonuses below
  "mult_bonuses": <array>,
  "tech_effects": <array>                // List of effects applied by this technique, see Bonuses below
}
```

All fields, except for the ID, type and name are optional.


### Technique effects

```jsonc
...
  "tech_effects": [
    {
      "id": "eff_expl",                  // Unique ID. Must be one continuous word
      "chance": 100,                     // Percent chance to apply the effect on this attack
      "permanent": false,                // If true the effect won't decay (default: false)
      "duration": 15,                    // Duration of the effect in turns
      "on_damage": true,                 // If true the effect will only be applied if the attack succeeded in doing damage (default: true)
      "req_flag": "ANY",                 // A single arbitrary character flag (from traits, bionics, effects, or bodyparts) required to apply this effect
      "message": "Example"               // The message to print if you successfully apply the effect, %s can be substituted for the target's name
    }
  ]
...
```

`tech_effects` are inlined in their parent `technique` as seen in the [previous example](MARTIALART_JSON.md#techniques), and thus lack their own `type`.


## Bonuses

The fields `flat_bonuses` and `mult_bonuses` can contain any number of bonus entries:

```jsonc
  "flat_bonuses": [
    { "stat": "dodge", "scale": 1.0 },
    { "stat": "damage", "type": "bash", "scaling-stat": "per", "scale": 0.15 }
  ]
```

Bonuses can be defined with the following values:

Field            | Description
----             |----
`stat`           | Affected statistic.  Can be any of: `hit`, `dodge`, `block`, `speed`, `movecost`, `damage`, `armor`, `arpen`.
`type`           | Damage type for the affected statistic (`bash`, `cut`, `heat`, etc.).   Only needed if the affected statistic is either `damage`, `armor`, or `arpen`.
`scale`          | Value of the bonus.
`scaling-stat`   | (Optional) Stat used for scaling.  Can be any of the four classic stats (`str`, `dex`, `int`, `per`) or any skill (`bashing`, `dodge`, `unarmed`, `rifles`, `spellcraft`, etc.).  If the scaling stat is specified, the value of the bonus is multiplied by the corresponding user stat/skill.

* Bonuses must be written in the correct order.
* Tokens of "useless" type will not cause an error, but will not have any effect.  For example, `speed` in a technique will have no effect (`movecost` should be used for techniques).
* Multiplicative bonuses only support `damage` and `movecost`.
* Flat bonuses are applied after multiplicative bonuses.


### Examples

Incoming bashing damage is decreased by 30% of strength value.  Only useful on buffs:

```jsonc
  "flat_bonuses": [ { "stat": "armor", "type": "bash", "scaling-stat": "str", "scale": 0.3 } ]
```


All cutting damage done is multiplied by 10% of dexterity:

```jsonc
  "mult_bonuses": [ { "stat": "damage", "type": "cut", "scaling-stat": "dex", "scale": 0.1 } ]
```


Move cost is decreased by 100% of strength value

```jsonc
  "flat_bonuses": [ { "stat": "movecost", "scaling-stat": "str", "scale": -1.0 } ]
```


## Attack vectors

Attack vectors define which (sub)bodypart are used for the attack in question.  It allows filtering eligible bodyparts and apply the relevant worn armor's unarmed damage to the attack.  The list is present at [attack_vectors.json](/data/json/attack_vectors.json)

Note: for the (sub)part to apply its unarmed damage it needs unrestricted natural attacks.

```jsonc
[ 
  {
    "type": "attack_vector",             // Always attack_vector
    "id": "vector_hand",                 // Unique ID. Must be one continuous word
    "limbs": [ "hand_l", "hand_r" ],     // List of bodyparts used in this attack (relevant for HP/encumbrance/flag filtering)
    "contact_area": [ "hand_fingers_l", "hand_fingers_r" ],    // List of subbodyparts that can be used as a strike surface in the attack using the subbodypart's armor or intrinsic unarmed damage
    "strict_limb_definition": false,     // Bool, default false. When true *only* the bodyparts defined above are used for the vector, otherwise similar bodyparts can be used as long as both the contact area and the defined limb are similar, see JSON_INFO.md/Bodyparts for bodypart similarity
    "armor_bonus": true,                 // Bool, defines if the vector takes the unarmed damage bonus of the armor worn on the contact area into account (default: true)
    "required_limb_flags": [ "foo", "bar" ],    // List of character flags required for the bodypart to be eligible for this vector
    "forbidden_limb_flags": [ "foo", "bar" ],   // List of character flags that disqualify a limb from being usable by this vector
    "encumbrance_limit": 15,             // Int, default 100, encumbrance of the limb above this will disqualify it from this vector
    "bp_hp_limit": 75 ,                  // Int, default 10, percent of bodypart limb HP necessary for the limbs to qualify for this vector. For minor (non-main) bodyparts the corresponding main part HP is taken into account
    "limb_req": [ [ "arm", 2] ]          // Array of pairs. Limb type requirements for this vector. The character must have this many limbs of the given type above the limb's health limit (See JSON_INFO.md:Bodyparts). Requirements must all be met

  }
]
```


## How to use

MA can be granted to characters in different ways.  They can be `autolearn`ed (as in the MA example), learned from [books](JSON_INFO.md#books) (as `martial_art`), be granted by traits (as `initial_ma_styles`), or dynamically by [eocs](EFFECT_ON_CONDITION.md#u_learn_martial_artnpc_learn_martial_art).

