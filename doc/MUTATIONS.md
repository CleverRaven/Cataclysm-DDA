# Mutations

DDA 

## Mutation Logic



## Mutations

```C++
"id": "LIGHTEATER",  // Unique ID
"name": "Optimist",  // In-game name displayed
"points": 2,         // Point cost of the trait. Positive values cost points and negative values give points
"visibility": 0,     // Visibility of the trait for purposes of NPC interaction (default: 0)
"ugliness": 0,       // Ugliness of the trait for purposes of NPC interaction (default: 0)
"cut_dmg_bonus": 3, // Bonus to unarmed cut damage (default: 0)
"pierce_dmg_bonus": 3, // Bonus to unarmed pierce damage (default: 0.0)
"bash_dmg_bonus": 3, // Bonus to unarmed bash damage (default: 0)
"butchering_quality": 4, // Butchering quality of this mutations (default: 0)
"rand_cut_bonus": { "min": 2, "max": 3 }, // Random bonus to unarmed cut damage between min and max.
"rand_bash_bonus": { "min": 2, "max": 3 }, // Random bonus to unarmed bash damage between min and max.
"bodytemp_modifiers" : [100, 150], // Range of additional bodytemp units (these units are described in 'weather.h'. First value is used if the person is already overheated, second one if it's not.
"bodytemp_sleep" : 50, // Additional units of bodytemp which are applied when sleeping
"initial_ma_styles": [ "style_crane" ], // (optional) A list of ids of martial art styles of which the player can choose one when starting a game.
"mixed_effect": false, // Whether the trait has both positive and negative effects. This is purely declarative and is only used for the user interface. (default: false)
"description": "Nothing gets you down!" // In-game description
"starting_trait": true, // Can be selected at character creation (default: false)
"valid": false,      // Can be mutated ingame (default: true)
"purifiable": false, //Sets if the mutation be purified (default: true)
"profession": true, //Trait is a starting profession special trait. (default: false)
"debug": false,     //Trait is for debug purposes (default: false)
"player_display": true, //Trait is displayed in the `@` player display menu
"vanity": false, //Trait can be changed any time with no cost, like hair, eye color and skin color
"category": ["MUTCAT_BIRD", "MUTCAT_INSECT"], // Categories containing this mutation
// prereqs and prereqs2 specify prerequisites of the current mutation
// Both are optional, but if prereqs2 is specified prereqs must also be specified
// The example below means: ( "SLOWREADER" OR "ILLITERATE") AND ("CEPH_EYES" OR "LIZ_EYE")
"prereqs": [ "SLOWREADER", "ILLITERATE"],
"prereqs2": [ "CEPH_EYES", "LIZ_EYE" ],
"threshreq": ["THRESH_SPIDER"], //Required threshold for this mutation to be possible
"cancels": ["ROT1", "ROT2", "ROT3"], // Cancels these mutations when mutating
"changes_to": ["FASTHEALER2"], // Can change into these mutations when mutating further
"leads_to": [], // Mutations that add to this one
"passive_mods" : { //increases stats with the listed value. Negative means a stat reduction
            "per_mod" : 1, //Possible values per_mod, str_mod, dex_mod, int_mod
            "str_mod" : 2
},
"wet_protection":[{ "part": "head", // Wet Protection on specific bodyparts
                    "good": 1 } ] // "neutral/good/ignored" // Good increases pos and cancels neg, neut cancels neg, ignored cancels both
"vitamin_rates": [ [ "vitC", -1200 ] ], // How much extra vitamins do you consume per minute. Negative values mean production
"vitamins_absorb_multi": [ [ "flesh", [ [ "vitA", 0 ], [ "vitB", 0 ], [ "vitC", 0 ], [ "calcium", 0 ], [ "iron", 0 ] ], [ "all", [ [ "vitA", 2 ], [ "vitB", 2 ], [ "vitC", 2 ], [ "calcium", 2 ], [ "iron", 2 ] ] ] ], // multiplier of vitamin absorption based on material. "all" is every material. supports multiple materials.
"craft_skill_bonus": [ [ "electronics", -2 ], [ "tailor", -2 ], [ "mechanics", -2 ] ], // Skill affected by the mutation and their bonuses. Bonuses can be negative, a bonus of 4 is worth 1 full skill level.
"restricts_gear" : [ "torso" ], //list of bodyparts that get restricted by this mutation
"allow_soft_gear" : true, //If there is a list of 'restricts_gear' this sets if the location still allows items made out of soft materials (Only one of the types need to be soft for it to be considered soft). (default: false)
"destroys_gear" : true, //If true, destroys the gear in the 'restricts_gear' location when mutated into. (default: false)
"encumbrance_always" : [ // Adds this much encumbrance to selected body parts
    [ "arm_l", 20 ],
    [ "arm_r", 20 ]
],
"encumbrance_covered" : [ // Adds this much encumbrance to selected body parts, but only if the part is covered by not-OVERSIZE worn equipment
    [ "hand_l", 50 ],
    [ "hand_r", 50 ]
],
"encumbrance_multiplier_always": { // if the bodypart has encumbrance caused by a mutation, multiplies that encumbrance penalty by this multiplier.
  "arm_l": 0.75,                   // DOES NOT AFFECT CLOTHING ENCUMBRANCE
  "arm_r": 0.75
},
"armor" : [ // Protects selected body parts this much. Resistances use syntax like `PART RESISTANCE` below.
  {
    "part_types": [ "arm", "leg" ],
    "bash": 2,
    "electric": 5                  // The resistance provided to the body part type(s) selected above
  },
  {
    "parts": [ "arm_l", "arm_r" ], // Overrides the above settings for those specific body parts
    "bash": 1,                     // ...and gives them those resistances instead
    "cut": 2
  }
],
"stealth_modifier" : 0, // Percentage to be subtracted from player's visibility range, capped to 60. Negative values work, but are not very effective due to the way vision ranges are capped
"active" : true, //When set the mutation is an active mutation that the player needs to activate (default: false)
"starts_active" : true, //When true, this 'active' mutation starts active (default: false, requires 'active')
"cost" : 8, // Cost to activate this mutation. Needs one of the hunger, thirst, or fatigue values set to true. (default: 0)
"time" : 100, //Sets the amount of (turns * current player speed ) time units that need to pass before the cost is to be paid again. Needs to be higher than one to have any effect. (default: 0)
"kcal" : true, //If true, activated mutation consumes `cost` kcal. (default: false)
"thirst" : true, //If true, activated mutation increases thirst by cost. (default: false)
"fatigue" : true, //If true, activated mutation increases fatigue by cost. (default: false)
"scent_modifier": 0.0,// float affecting the intensity of your smell. (default: 1.0)
"scent_intensity": 800,// int affecting the target scent toward which you current smell gravitates. (default: 500)
"scent_mask": -200,// int added to your target scent value. (default: 0)
"scent_type": "sc_flower",// scent_typeid, defined in scent_types.json, The type scent you emit. (default: empty)
"consume_time_modifier": 1.0f,//time to eat or drink is multiplied by this
"bleed_resist": 1000, // Int quantifying your resistance to bleed effect, if it's > to the intensity of the effect you don't get any bleeding. (default: 0)
"fat_to_max_hp": 1.0, // Amount of hp_max gained for each unit of bmi above character_weight_category::normal. (default: 0.0)
"healthy_rate": 0.0, // How fast your health can change. If set to 0 it never changes. (default: 1.0)
"weakness_to_water": 5, // How much damage water does to you, negative values heal you. (default: 0)
"ignored_by": [ "ZOMBIE" ], // List of species ignoring you. (default: empty)
"anger_relations": [ [ "MARSHMALLOW", 20 ], [ "GUMMY", 5 ], [ "CHEWGUM", 20 ] ], // List of species angered by you and by how much, can use negative value to calm.  (default: empty)
"can_only_eat": [ "junk" ], // List of materiel required for food to be comestible for you. (default: empty)
"can_only_heal_with": [ "bandage" ], // List of med you are restricted to, this includes mutagen,serum,aspirin,bandages etc... (default: empty)
"can_heal_with": [ "caramel_ointement" ], // List of med that will work for you but not for anyone. See `CANT_HEAL_EVERYONE` flag for items. (default: empty)
"allowed_category": [ "ALPHA" ], // List of category you can mutate into. (default: empty)
"no_cbm_on_bp": [ "torso", "head", "eyes", "mouth", "arm_l" ], // List of body parts that can't receive cbms. (default: empty)
"lumination": [ [ "head", 20 ], [ "arm_l", 10 ] ], // List of glowing bodypart and the intensity of the glow as a float. (default: empty)
"metabolism_modifier": 0.333, // Extra metabolism rate multiplier. 1.0 doubles usage, -0.5 halves.
"fatigue_modifier": 0.5, // Extra fatigue rate multiplier. 1.0 doubles usage, -0.5 halves.
"fatigue_regen_modifier": 0.333, // Modifier for the rate at which fatigue and sleep deprivation drops when resting.
"stamina_regen_modifier": 0.1, // Increase stamina regen by this proportion (1.0 being 100% of normal regen)
"cardio_multiplier": 1.5, // Multiply total cardio fitness by this amount
"healing_awake": 1.0, // Healing rate per turn while awake.
"healing_resting": 0.5, // Healing rate per turn while resting.
"mending_modifier": 1.2 // Multiplier on how fast your limbs mend - This value would make your limbs mend 20% faster
"transform": { "target": "BIOLUM1", // Trait_id of the mutation this one will transform into
               "msg_transform": "You turn your photophore OFF.", // message displayed upon transformation
               "active": false , // Will the target mutation start powered ( turn ON ).
               "moves": 100 // how many moves this costs. (default: 0)
},
"triggers": [ // List of sublist of triggers, all sublists must be True for the mutation to activate
  [ // Sublist of trigger: at least one trigger must be true for the sublist to be true
      {
        "condition": { "compare_int": [ { "u_val": "morale" }, "<", { "const": -50 } ] }, //dialog condition(see NPCs.md)
        "msg_on": { "text": "Everything is terrible and this makes you so ANGRY!", "rating": "mixed" } // message displayed when the trigger activates
      }
  ],
  [
    {
      "condition": { //dialog condition(see NPCs.md)
        "or": [
          { "compare_int": [ { "hour", "<", { "const": 2 } ] },
          { "compare_int": [ { "hour", ">", { "const": 20 } ] }
        ]
      },
      "msg_on": { "text": "Everything is terrible and this makes you so ANGRY!", "rating": "mixed" } // message displayed when the trigger activates
      "msg_off": { "text": "Your glow fades." } // message displayed when the trigger deactivates the trait
    }
  ]
],
"activated_eocs": [ "eoc_id_1" ],  // List of effect_on_conditions that attempt to activate when this mutation is successfully activated.
"deactivated_eocs": [ "eoc_id_1" ],  // List of effect_on_conditions that attempt to activate when this mutation is successfully deactivated.
"enchantments": [ "ench_id_1" ],   // List of enchantments granted by this mutation, can be either string ids of the enchantment or an inline definition of the enchantment
"temperature_speed_modifier": 0.5, // If nonzero, become slower when cold, and faster when hot
                                   // 1.0 gives +/-1% speed for each degree above or below 65F
"mana_modifier": 100               // Positive or negative change to total mana pool

```

## Mutation Categories

A Mutation Category identifies a set of interrelated mutations that as a whole establish an entirely new identity for a mutant character. Categories can and usually do have their own "flavor" of mutagen, the properties of which are defined in the category definition itself. A second kind of mutagen, called a "mutagen serum" or "IV mutagen" is necessary to trigger "threshold mutations" which cause irrevocable changes to the character.

| Identifier         | Description
|---                 |---
| `id`               | Unique ID. Must be one continuous word, use underscores when necessary.
| `name`             | Human readable name for the category of mutations.
| `threshold_mut`    | A special mutation that marks the point at which the identity of the character is changed by the extent of mutation they have experienced.
| `mutagen_message`  | A message displayed to the player when they mutate in this category.
| `memorial_message` | The memorial message to display when a character crosses the associated mutation threshold.
| `wip`              | A flag indicating that a mutation category is unfinished and shouldn't have consistency tests run on it. See tests/mutation_test.cpp.
