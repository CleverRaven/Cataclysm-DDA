# System info

Mutations in Dark Days Ahead happen through a series of EOCs and hardcoded checks. Different items - usually dedicated mutagens or primers - give the character certain vitamins, and recurring EOCs will use these vitamins to mutate them. Other than uncontrolled mutations from sources like radiation, which are completely random, the traits that a character will get through mutation are determined entirely by the vitamins they have in their body.

Traits and mutations are the same thing in DDA's code. The terms are used interchangeably in this page to avoid repetition.

### Mutagens and primers

There are two substances required to mutate: mutagen and primer. All mutagen and primers are handled as vitamins in the code.

Mutagen is the core nutrient, and it's what is required to initate and maintain mutation. Thematically, this is the stuff that stimulates the character's infection and gets them mutating. It comes in a ingestable liquid form (which is toxic and generally very messy to drink) and an injectable catalyst form (which is much safer and more concentrated).

Primers do not cause mutations to happen on their own, but instead influence what mutations the player gains. Think of mutagen as a car that can only drive on roads, and each type of primer as a possible road for the car to drive on. Every type of mutation category has an associated primer vitamin, and when mutating, the game will mutate down the category that has the most respective primer present.

As a minor but important note: "liquid mutagens" exist for each category, but these are actually just a mixture of regular mutagen and the category's primer. The same behavior would happen if you used an injectable mutagen catalyst and an injectable primer separately.

#### Purifier

Many different animals have an associated type of primer. Humans are no exception; human primer, however, is called purifier. Code-wise, it functions the same as every other primer, but the mutations actually defined for the human category are special. Instead of being traditional traits you gain and lose, the traits targeted by purifier are abstract and only exist to cancel out other traits.

For instance, if you have the Ursine Vision mutation (which has the `EYES` and `VISION` categories), and the game picks the dummy trait Human Eyes to mutate towards, you will lose the Ursine Vision trait and nothing else will happen. You never actually gain the Human Eyes trait; its only purpose is to be *targeted* by the game for removing other traits. The engine will do everything to simulate mutating towards that trait, but it will back out before it actually gives it to the character.

Like any other primer, purifier requires normal mutagen in the body to function. It will not work on its own.

### Thresholds

Every mutation category has an associated "threshold" mutation, and having that threshold mutation is required to access the most powerful and unique traits in a category. Gaining one of these threshold mutations is called "crossing the threshold", and once a character has crossed the threshold for a category, it is permanent - they can never cross any other thresholds. Doing this requires extreme amounts of primer in the body, so it's very unlikely that a character will accidentally breach a threshold without intending to do so.

Traits that can only be gained after crossing a threshold are generally called "post-threshold" mutations, for obvious reasons.

### How mutation works

The mutation system works in several steps. All time references are in game time.

1. Every 30 to 45 minutes, the game checks if the player has 450 or more `mutagen` vitamin in their body.
  * If the character doesn't have the required mutagen, nothing happens.
  * Otherwise, two outcomes can happen: either the character starts mutating, or nothing happens again. The chance to start mutating is equal to the amount of mutagen in the character, while the chance to do nothing is a constant 2500. One of these options is then picked based on weight. Thus, for a player with 1250 mutagen in their body, the compared weights would be 1250 to begin mutating versus 2500 to do nothing, or a 1 in 3 chance to start mutating.
2. Once mutation begins, the character mutates once immediately and the player receives a unique message. The character then gain sthe invisible `Changing` trait, which signifies to the engine that they are actively undergoing periodic mutation.
3. Every 1 to 6 hours, the game will attempt to mutate the character. As long as the character has enough mutagen when the effect fires, the attempt will proceed and the timer will restart. Between 60 and 140 mutagen is removed from their body *when the attempt is made* - it does not have to succeed for the mutagen to be deducted.
4. The engine attempts to mutate the character based on the primers they have in their body. 
  * If the character has no primer in their body, the player receives a distinct message and nothing happens.
  * Otherwise, the game chooses a random mutation in that category, and the character mutates towards it.
  * If the character has an existing trait that conflicts with the target mutation, the conflicting trait will be removed or downgraded.
  * Each mutation attempt can take only one "step". For instance, if the game attempts to mutate towards the Beautiful trait while the character has no beauty-related traits, it would simply give them the Pretty trait, because Pretty is a prerequisite of Beautiful. However, if the character had the Ugly trait, then that trait would be removed and nothing else would happen.
  * On a successful mutation, primer of that mutation's category is removed from the character's body. This defaults to 100, but can be overriden on a by-type basis.
  * Finally, Instability equal to the primer cost is added to the player. Instability is explained in the next section.
5. When the player mutates towards a trait, if they have at least 2200 of the primer tied to that trait's category, attempt to cross the threshold. If the RNG gods bless the player, or the character is heavily mutated in the relevant category, they will cross the threshold and gain the relevant threshold mutation.
6. Repeat from step 3 until the character no longer has enough mutagen in their body to continue mutating. The Changing trait will be then removed, and the game will begin repeating step 1 once more until the character takes enough mutagen to begin mutating again.

### Instability and odds of a good mutation

The odds of a mutation being good or bad is directly determined by Instability, which is a hidden stat tracked using a vitamin. It represents long-term genetic damage; a character will begin the game with 0 Instability and obtain only positive or neutral mutations, but with enough Instability, they will become almost exclusively negative ones.

These chances are determined on a curve, ranging from 0 Instability (default) to 8000 Instability (the maximum):
* There is always a flat 10% chance to obtain a neutral mutation that is neither positive or negative.
* From roughly 0 to 800 Instability, there is a 90% chance for a positive mutation and a 0% chance for a negative one.
* Positive and negative chances then quickly slope to meet each other at roughly 2800 Instability. At this point, there is a 45% chance for a positive mutation and for a negative mutation.
* Chances then gradually continue their current trends until reaching the limit. At the maximum of 8000 Instability, there is roughly a 65% chance for a negative mutation and a 25% chance for a positive one.

Instability very slowly decreases on its own, at a rate of 1 per day. Traits can influence this; for instance, the Robust Genetics trait vastly speeds this up by removing a further 1 Instability per hour, for a total of 25 per day. The Genetic Downward Spiral trait does the opposite, *increasing* Instability at the extremely fast rate of 1 per minute.

### tl;dr

* Characters need mutagen and primer to mutate. Mutagen causes the actual mutation, while primer determines the category that mutations will be in.
* Once a character has enough mutagen in their body, they will begin mutating, causing them to gain a new mutation every few hours until they no longer have enough mutagen in their body to keep doing so.
* If a character has a huge amount of one type of primer in their body when they mutate, they might cross the threshold for that primer's category if the conditions are right.
* The odds of a mutation being good or bad is determined by Instability, which increases with every new mutation. Mutations start being exclusively beneficial or neutral but quickly become biased towards negatives after mutating enough.
* Human primer is called purifier, and characters can "remove" their gained mutations by using it as their primer while mutating.

### Further reading and relevant files

`data/json/mutations/changing_eocs.json` - contains the EOCs that power the mutation system
`data/json/mutations/mutations.json` - type definitions for every mutation in the game

# Syntax documentation

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
