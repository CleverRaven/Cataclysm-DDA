# System info

Mutations in Dark Days Ahead happen through a series of EOCs and hardcoded checks. Different items - usually dedicated mutagens or primers - give the character certain vitamins, and recurring EOCs will use these vitamins to mutate them. Other than uncontrolled mutations from sources like radiation, which are completely random, the traits that a character will get through mutation are determined entirely by the vitamins they have in their body.

Traits and mutations are the same thing in DDA's code. The terms are used interchangeably in this page to avoid repetition.

### Mutagens and primers

There are two substances required to mutate: mutagen and primer. All mutagen and primers are handled as vitamins in the code.

Mutagen is the core nutrient, and it's what is required to initate and maintain mutation. Thematically, this is the stuff that stimulates the character's infection and gets them mutating. It comes in a ingestable liquid form (which is toxic and generally a bad idea to drink without further refinement) and an injectable catalyst form (which is much safer and much more powerful).

Primers do not cause mutations to happen on their own, but instead influence what mutations the player gains. Think of mutagen as a car that can only drive on roads, and each type of primer as a possible road for the car to drive on. Every type of mutation category has an associated primer vitamin, and when mutating, the game will mutate down the category that has the most respective primer present. The car can't drive without any roads, but at the same time, the roads will do nothing without the car. Both nutrients are needed to cause mutation.

As a minor but important note: "liquid mutagen" items exist for each category, but these are actually just a mixture of regular mutagen and the category's primer. The same behavior would happen if you used an injectable mutagen catalyst and an injectable primer separately, albeit at a much smaller effect.

#### Purifier

Many different animals have an associated type of primer. Humans are no exception; human primer, however, is called purifier. Code-wise, it functions the same as every other primer, but the mutations actually defined for the human category are special. Instead of being traditional traits you gain and lose, the traits targeted by purifier are abstract and only exist to cancel out other traits.

For instance, if you have the Ursine Vision mutation (which has the `EYES` and `VISION` categories), and the game picks the dummy trait Human Eyes to mutate towards, you will lose the Ursine Vision trait and nothing else will happen. You never actually gain the Human Eyes trait; its only purpose is to be *targeted* by the game for removing other traits. The engine will do everything to simulate mutating towards that trait, but it will back out before it actually gives it to the character. Mutation logic will specifically try to find a human trait that has conflicting entries; this is so that purifier always attempts to do something, instead of, say, mutating towards Human Skin when you already have a normal body, which might leave your hypothetically mutated eyes untouched.

Like any other primer, purifier requires normal mutagen in the body to function. It will not work on its own.

### Thresholds

Every mutation category has an associated "threshold" mutation, and having that threshold mutation is required to access the most powerful and unique traits in a category. Gaining one of these threshold mutations is called "crossing the threshold", and once a character has crossed the threshold for a category, it is permanent - they can never cross any other thresholds. Doing this requires extreme amounts of primer in the body, so it's very unlikely that a character will accidentally breach a threshold without intending to do so.

Traits that can only be gained after crossing a threshold are generally called "post-threshold" mutations, for obvious reasons.

### How mutation works

The mutation system works in several steps. All time references are in game time.

1. Every 30 to 45 minutes, the game checks if the player has 450 or more `mutagen` vitamin in their body.
  * If the character doesn't have the required mutagen, nothing happens.
  * Otherwise, two outcomes can happen: either the character starts mutating, or nothing happens again. The chance to start mutating is equal to the amount of mutagen in the character, while the chance to do nothing is a constant 2500. One of these options is then picked based on weight. Thus, for a player with 1250 mutagen in their body, the compared weights would be 1250 to begin mutating versus 2500 to do nothing, or a 1 in 3 chance to start mutating.
2. Once mutation begins, the character mutates once immediately and the player receives a unique message. The character then gains the invisible `Changing` trait, which signifies to the game that they are actively undergoing periodic mutation.
3. Every 1 to 6 hours, the game will attempt to mutate the character. As long as the character has enough mutagen when the effect fires, the attempt will proceed and the timer will restart. Between 60 and 140 mutagen is removed from their body *when the attempt is made* - it does not have to succeed for the mutagen to be deducted.
4. The engine attempts to mutate the character based on the primers they have in their body. 
  * If the character has no primer in their body, the player receives a distinct message and nothing happens. The `Changing` trait will immediately be removed when this happens, stopping the process.
  * Otherwise, the game chooses a random mutation in that category. This is the target mutation.
  * If the character has an existing trait that conflicts with the target mutation, the conflicting trait will be removed or downgraded, and nothing else will happen. Otherwise, the player will gain that mutation.
  * Each mutation attempt can take only one "step". For instance, if the game attempts to mutate towards the Beautiful trait while the character has no beauty-related traits, it would simply give them the Pretty trait, because Pretty is a prerequisite of Beautiful. However, if the character had the Ugly trait, then that trait would be removed and nothing else would happen.
  * On a successful mutation, primer of that mutation's category is removed from the character's body. This defaults to 100, but can be overriden on a by-trait basis.
  * Finally, Instability equal to the primer cost is added to the player. Instability is explained in the next section.
5. When the player mutates towards a trait, if they have at least 2200 of the primer tied to that trait's category, attempt to cross the threshold. The game rolls a chance to pass the threshold, which is heavily influenced by how mutated into that category the player is. If the check succeeds, the player gains the threshold mutation and receives a unique message - they have "crossed the threshold".
6. Repeat from step 3 until the character no longer has enough mutagen in their body to continue mutating. The `Changing` trait will be then removed, and the game will begin repeating step 1 once more until the character takes enough mutagen to begin mutating again.

### Instability and the odds of a good mutation

The odds of a mutation being good or bad is directly determined by Instability, which is a stat tracked using a vitamin. It represents long-term genetic damage; a character will begin the game with 0 Instability and obtain only positive or neutral mutations, but with enough Instability, they will become almost exclusively negative ones.

These chances are determined on a curve, ranging from 0 Instability (default) to 8000 Instability (the maximum):
* Neutral mutations (those which are neither negative nor positive) are always eligible.
* From roughly 0 to 800 Instability, there is a 100% chance for a positive or neutral mutation and a 0% chance for a negative one.
* Positive and negative chances then quickly slope to meet each other at roughly 2800 Instability. At this point, there are equal chances for positive and negative mutations.
* Chances then gradually continue their current trends until reaching the limit. At the maximum of 8000 Instability, there is roughly a 70% chance for a negative mutation to be selected and a 30% chance for a positive one. As before, regardless of whether a positive or negative mutation is selected, a neutral mutation is also possible.

Instability very slowly decreases on its own, at a rate of 1 per day. Traits can influence this; for instance, the Robust Genetics trait vastly speeds this up by removing a further 1 Instability per hour, for a total of 25 per day. The Genetic Downward Spiral trait does the opposite, *increasing* Instability at the extremely fast rate of 1 per minute.

Instability can't currently be viewed numerically in normal play, but the player will receive a visible effect on their character sheet whenever they have any Instability. This serves to give a general ballpark of how unstable the character is without telling them the exact amount.

### tl;dr

* Characters need mutagen and primer to mutate. Mutagen causes the actual mutation, while primer determines the category that mutations will be in. One will not function without the other; they need both.
* Once a character has enough mutagen in their body, they will begin mutating, causing them to gain a new mutation every few hours until they no longer have enough mutagen in their body to keep doing so.
* If a character has a huge amount of one type of primer in their body when they mutate, they might cross the threshold for that primer's category if the conditions are right.
* The odds of a mutation being good or bad is determined by Instability, which increases with every new mutation. Mutations start out as exclusively beneficial or neutral, but quickly become biased towards negatives after becoming too unstable.
* Human primer is called purifier, and is used to "remove" a character's gained traits by mutating towards dummy "human" mutations that cancel out other ones.

### Further reading and relevant files

`data/json/mutations/changing_eocs.json` - contains the EOCs that power the mutation system
`data/json/mutations/mutations.json` - type definitions for every mutation in the game

# Syntax documentation

## Mutations

Specific mutations are extremely versatile. A mutation only needs to have a few mandatory fields filled out, but a very high number of optional fields exist, in addition to supporting EOCs.

Note that **all new traits that can be obtained through mutation must be purifiable** - otherwise, unit tests will fail. To make a mutation purifiable, just add it to the `cancels` field for an existing appropriate dummy mutation, or define its `types` if you want it to be mutually exclusive with certain other mutations.

### Example

```json
{
  "id": "LIGHTEATER",                         // Unique ID.
  "name": "Optimist",                         // In-game name displayed.
  "points": 2,                                // Point cost of the trait.  Positive values cost points and negative values give points.
  "vitamin_cost"                              // Category vitamin cost of gaining this trait (default: 100)
  "visibility": 0,                            // Visibility of the trait for purposes of NPC interaction (default: 0).
  "ugliness": 0,                              // Ugliness of the trait for purposes of NPC interaction (default: 0).
  "cut_dmg_bonus": 3,                         // Bonus to unarmed cut damage (default: 0).
  "pierce_dmg_bonus": 3,                      // Bonus to unarmed pierce damage (default: 0.0).
  "bash_dmg_bonus": 3,                        // Bonus to unarmed bash damage (default: 0).
  "butchering_quality": 4,                    // Butchering quality of this mutations (default: 0).
  "rand_cut_bonus": { "min": 2, "max": 3 },   // Random bonus to unarmed cut damage between min and max.
  "rand_bash_bonus": { "min": 2, "max": 3 },  // Random bonus to unarmed bash damage between min and max.
  "bodytemp_modifiers": [ 100, 150 ],           // Range of additional bodytemp units (these units are described in 'weather.h'.  First value is used if the person is already overheated, second one if it's not.
  "bodytemp_sleep": 50,                       // Additional units of bodytemp which are applied when sleeping.
  "initial_ma_styles": [ "style_crane" ],     // (optional) A list of IDs of martial art styles of which the player can choose one when starting a game.
  "mixed_effect": false,                      // Whether the trait has both positive and negative effects.  This is purely declarative and is only used for the user interface (default: false).
  "description": "Nothing gets you down!",    // In-game description.
  "starting_trait": true,                     // Can be selected at character creation (default: false).
  "valid": false,                             // Can be mutated ingame (default: true).
  "purifiable": false,                        // Sets if the mutation be purified (default: true).
  "profession": true,                         // Trait is a starting profession special trait (default: false).
  "debug": false,                             // Trait is for debug purposes (default: false).
  "player_display": true,                     // Trait is displayed in the `@` player display menu and mutations screen.
  "vanity": false,                            // Trait can be changed any time with no cost, like hair, eye color and skin color.
  "variants": [                               // Cosmetic variants of this mutation.
    {
      "id": "red",                                        // string (mandatory): ID of the variant.
      "name": { "str": "Glass-Half-Full Optimist" },      // Name displayed in place of the mutation name.
      "description": "You think the glass is half-full.", // Description displayed in place of mutation description, unless append_desc is true.
      "apped_desc": false,                                // If true, append the description, instead of replacing.
      "weight": 1                                         // Used to randomly select variant when this is mutated.  Chance of being selected is weight/sum-of-all-weights.  If no weight is specified or weight is 0, variant will not be selected.
    }
  ],
  "category": [ "MUTCAT_BIRD", "MUTCAT_INSECT" ],         // Categories containing this mutation.
      // prereqs and prereqs2 specify prerequisites of the current mutation.
      // Both are optional, but if prereqs2 is specified prereqs must also be specified.
      // The example below means: ( "SLOWREADER" OR "ILLITERATE") AND ("CEPH_EYES" OR "LIZ_EYE").
  "prereqs": [ "SLOWREADER", "ILLITERATE" ],
  "prereqs2": [ "CEPH_EYES", "LIZ_EYE" ],
  "threshreq": [ "THRESH_SPIDER" ],           // Required threshold for this mutation to be possible.
  "cancels": [ "ROT1", "ROT2", "ROT3" ],      // Cancels these mutations when mutating.
  "changes_to": [ "FASTHEALER2" ],            // Can change into these mutations when mutating further.
  "leads_to": [ ],                            // Mutations that add to this one.
  "passive_mods": {                           // Increases stats with the listed value.  Negative means a stat reduction.
    "per_mod": 1,                             // Possible values: per_mod, str_mod, dex_mod, int_mod
    "str_mod": 2
  },
  "wet_protection": [ { "part": "head", "good": 1 } ],    // Wet Protection on specific bodyparts.  Possible values: "neutral/good/ignored".  Good increases pos and cancels neg, neut cancels neg, ignored cancels both.
  "vitamin_rates": [ [ "vitC", -1200 ] ],     // How much extra vitamins do you consume, one point per this many seconds.  Negative values mean production.
  "vitamins_absorb_multi": [                  // Multiplier of vitamin absorption based on material.  "all" includes every material.  Supports multiple materials.
    [ "flesh", [ [ "vitA", 0 ], [ "vitB", 0 ], [ "vitC", 0 ], [ "calcium", 0 ], [ "iron", 0 ] ] ],
    [ "all", [ [ "vitA", 2 ], [ "vitB", 2 ], [ "vitC", 2 ], [ "calcium", 2 ], [ "iron", 2 ] ] ]
  ],
  "craft_skill_bonus": [ [ "electronics", -2 ], [ "tailor", -2 ], [ "mechanics", -2 ] ], // Skill affected by the mutation and their bonuses.  Bonuses can be negative, a bonus of 4 is worth 1 full skill level.
  "restricts_gear": [ "torso" ],              // List of bodyparts that get restricted by this mutation.
  "remove_rigid": [ "torso" ],                // List of bodyparts that any rigid armor will be removed from on mutation. Any integrated armor items are considered directly
  "allow_soft_gear": true,                    // If there is a list of 'restricts_gear', this sets if the location still allows items made out of soft materials (only one of the types need to be soft for it to be considered soft) (default: false).
  "destroys_gear": true,                      // If true, destroys the gear in the 'restricts_gear' location when mutated into (default: false).
  "encumbrance_always": [                     // Adds this much encumbrance to selected body parts.
    [ "arm_l", 20 ],
    [ "arm_r", 20 ]
  ],
  "encumbrance_covered": [                    // Adds this much encumbrance to selected body parts, but only if the part is covered by not-OVERSIZE worn equipment.
    [ "hand_l", 50 ],
    [ "hand_r", 50 ]
  ],
  "encumbrance_multiplier_always": {          // If the bodypart has encumbrance caused by a mutation, multiplies that encumbrance penalty by this multiplier.
    "arm_l": 0.75,                            // Note: Does not affect clothing encumbrance.
    "arm_r": 0.75
  },
  "armor": [                                  // Protects selected body parts this much.  Resistances use syntax like `PART RESISTANCE` below.
    {
      "part_types": [ "arm", "leg" ],         // The resistance provided to the body part type(s) selected above.
      "bash": 2,
      "electric": 5
    },
    {
      "parts": [ "arm_l", "arm_r" ],          // Overrides the above settings for those specific body parts with the following values.
      "bash": 1,
      "cut": 2
    }
  ],
  "stealth_modifier": 0,                      // Percentage to be subtracted from player's visibility range, capped to 60.  Negative values work, but are not very effective due to the way vision ranges are capped.
  "active": true,                             // When set the mutation is an active mutation that the player needs to activate (default: false).
  "starts_active": true,                      // When true, this 'active' mutation starts active (default: false, requires 'active').
  "cost": 8,                                  // Cost to activate this mutation.  Needs one of the hunger, thirst, or fatigue values set to true (default: 0).
  "time": 100,                                // Sets the amount of (turns * current player speed ) time units that need to pass before the cost is to be paid again.  Needs to be higher than one to have any effect (default: 0).
  "kcal": true,                               // If true, activated mutation consumes `cost` kcal. (default: false).
  "thirst": true,                             // If true, activated mutation increases thirst by cost (default: false).
  "fatigue": true,                            // If true, activated mutation increases fatigue by cost (default: false).
  "scent_modifier": 0.0,                      // float affecting the intensity of your smell (default: 1.0).
  "scent_intensity": 800,                     // int affecting the target scent toward which you current smell gravitates (default: 500).
  "scent_mask": -200,                         // int added to your target scent value (default: 0).
  "scent_type": "sc_flower",                  // The scent_types you emit, as defined in scent_types.json (default: empty).
  "consume_time_modifier": 1.0,               // time to eat or drink is multiplied by this.
  "fat_to_max_hp": 1.0,                       // Amount of hp_max gained for each unit of bmi above character_weight_category::normal (default: 0.0).
  "healthy_rate": 0.0,                        // How fast your health can change.  If set to 0 it never changes (default: 1.0).
  "weakness_to_water": 5,                     // How much damage water does to you, negative values heal instead (default: 0).
  "ignored_by": [ "ZOMBIE" ],                 // List of species ignoring you (default: empty).
  "anger_relations": [ [ "MARSHMALLOW", 20 ], [ "GUMMY", 5 ], [ "CHEWGUM", 20 ] ], // List of species angered by you and how much, negative values calm instead  (default: empty).
  "can_only_eat": [ "junk" ],                 // List of comestible materials (default: empty).
  "can_only_heal_with": [ "bandage" ],        // List of `MED` you are restricted to, includes mutagen, serum, aspirin, bandages etc. (default: empty).
  "can_heal_with": [ "caramel_ointement" ],   // List of `MED` that will work for you but not for anyone else.  See `CANT_HEAL_EVERYONE` flag for items (default: empty).
  "allowed_category": [ "ALPHA" ],            // List of categories you can mutate into (default: empty).
  "no_cbm_on_bp": [ "torso", "head", "eyes", "mouth", "arm_l" ],  // List of body parts that can't receive CBMs (default: empty).
  "lumination": [ [ "head", 20 ], [ "arm_l", 10 ] ],              // List of glowing bodypart and the intensity of the glow as a float (default: empty).
  "metabolism_modifier": 0.333,               // Extra metabolism rate multiplier (1.0 doubles, -0.5 halves).
  "thirst_modifier": 0.1,                     // Extra thirst modifier (1.0 doubles, -0.5 halves).
  "fatigue_modifier": 0.5,                    // Extra fatigue rate multiplier (1.0 doubles usage, -0.5 halves).
  "fatigue_regen_modifier": 0.333,            // Modifier for the rate at which fatigue and sleep deprivation drops when resting.
  "stamina_regen_modifier": 0.1,              // Increase stamina regen by this proportion (1.0 being 100% of normal regen).
  "cardio_multiplier": 1.5,                   // Multiplies total cardio fitness by this amount.
  "healing_multiplier": 0.5,                  // Multiplier to PLAYER/NPC_HEALING_RATE.
  "healing_awake": 1.0,                       // Percentage of healing rate used while awake.
  "mending_modifier": 1.2,                    // Multiplier on how fast your limbs mend (1.2 is 20% faster).
  "attackcost_modifier": 0.9,                 // Attack cost modifier (0.9 is 10% faster, 1.1 is 10% slower).
  "movecost_modifier": 0.9,                   // Overall movement speed cost modifier (0.9 is 10% faster, 1.1 is 10% slower).
  "movecost_flatground_modifier": 0.9,        // Movement speed cost modifier on flat terrain, free from obstacles (0.9 is 10% faster, 1.1 is 10% slower).
  "movecost_obstacle_modifier": 0.9,          // Movement speed cost modifier on rough, uneven terrain (0.9 is 10% faster, 1.1 is 10% slower).
  "movecost_swim_modifier": 0.9,              // Swimming speed cost modifier (0.9 is 10% faster, 1.1 is 10% slower).
  "weight_capacity_modifier": 0.9,            // Carrying capacity modifier (0.9 is 10% less, 1.1 is 10% more).
  "social_modifiers": { "persuade": -10 },    // Social modifiers.  Can be: intimidate, lie, persuade.
  "spells_learned": [ [ "spell_slime_spray", 1 ] ], // Spells learned and the level they're at after gaining the trait/mutation.
  "transform": { 
    "target": "BIOLUM1",                      // Trait_id of the mutation this one will transform into.
    "msg_transform": "You turn your photophore OFF.", // Message displayed upon transformation.
    "active": false,                          // If true, mutation will start powered when activated (turn ON).
    "moves": 100                              // Moves cost per activation (default: 0).
  },
  "triggers": [                               // List of sublist of triggers, all sublists must be True for the mutation to activate.
    [                                         // Sublist of trigger: at least one trigger must be true for the sublist to be true.
      {
        "condition": { "compare_num": [ { "u_val": "morale" }, "<", { "const": -50 } ] },               // Dialog condition (see NPCs.md).
        "msg_on": { "text": "Everything is terrible and this makes you so ANGRY!", "rating": "mixed" }  // Message displayed when the trigger activates.
      }
    ],
    [
      {
        "condition": {                        // Dialog condition (see NPCs.md).
          "or": [
            { "compare_num": [ "hour", "<", { "const": 2 } ] },
            { "compare_num": [ "hour", ">", { "const": 20 } ] }
          ]
        },
        "msg_on": { "text": "Everything is terrible and this makes you so ANGRY!", "rating": "mixed" },// Message displayed when the trigger activates.
        "msg_off": { "text": "Your glow fades." }                                                      // Message displayed when the trigger deactivates the trait.
      }
    ]
  ],
  "activated_eocs": [ "eoc_id_1" ],           // List of effect_on_conditions that attempt to activate when this mutation is successfully activated.
  "deactivated_eocs": [ "eoc_id_1" ],         // List of effect_on_conditions that attempt to activate when this mutation is successfully deactivated.
  "enchantments": [ "ench_id_1" ],            // List of enchantments granted by this mutation.  Can be either IDs or an inline definition of the enchantment (see MAGIC.md)
  "temperature_speed_modifier": 0.5,          // If nonzero, become slower when cold, and faster when hot (1.0 gives +/-1% speed for each degree above or below 65 F).
  "pain_modifier": 5,                         // Flat increase (for positive numbers)\ reduction (for negative) to the amount of pain recived. Reduction can go all the way to 0. Applies after pain enchantment. (so if you have Pain Resistant trait along with 5 flat pain reduction and recive 20 pain, you would gain 20*(1-0.25)-5=10 pain)
  "mana_modifier": 100,                       // Positive or negative change to total mana pool.
  "flags": [ "UNARMED_BONUS" ],               // List of flag_IDs and json_flag_IDs granted by the mutation.  Note: trait_IDs can be set and generate no errors, but they're not actually "active".
  "moncams": [ [ "mon_player_blob", 16 ] ]    // Monster cameras, ability to use friendly monster's from the list as additional source of vision. Max view distance is equal to monster's daytime vision. The number specifies the range at which it can "transmit" vision to the avatar.
}

```

### Mandatory Fields

These fields are mandatory for every mutation object, and the game will fail to load if any of them are missing for a given trait.

|    Identifier     |                                                   Description                                                     |
| ----------------- | ----------------------------------------------------------------------------------------------------------------- |
| `type`            | Must be `"mutation"`.                                                                                             |
| `id`              | ID string. Mutations can't have the same ID: every one must be unique.                                            |
| `name`            | The name for the mutation as it appears in the character sheet.                                                   |
| `description`     | A human-readable description for the mutation.                                                                    |
| `points`          | The point cost of this trait.  Positive values cost points, while negative values add them.  0 is a valid option. |

### Supplementary Fields

These fields are optional, but are very frequently used in mutations and their code, so they're highlighted here first.

|    Identifier     | Default |                                                                          Description                                                                        |
| ----------------- | ------- | ----------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `category`        | Nothing | An array of string IDs representing mutation categories. This defines which categories the trait is considered part of (such as `ALPHA`, `BEAST`, `CEPHALOPOD`, and so on) and so it determines which primers must be used for the player to mutate them. |
| `types`           | Nothing | A list of types that this mutation can be classified under. Each mutation with a certain type is mutually exclusive with other mutations that also have that type; if a trait has the `EXAMPLE` type defined, then no other trait with that type can exist on a character, and mutating towards such a trait would remove the existing one if it could.  |
| `prereqs`         | Nothing | An array of mutation IDs that are possible requirements for this trait to be obtained. Only a single option from this list needs to be present.             |
| `prereqs2`        | Nothing | Identical to `prereqs`, and will throw errors if `prereqs` isn't defined. This is used to have multiple traits required to obtain another trait; one option must be present on the character from both `prereqs` and `prereqs2` for a trait to be obtainable. |
| `threshreq`       | Nothing | This is a dedicated prerequisite slot for threshold mutations, and functions identically to `prereq` and `prereq2`.                                         |
| `cancels`         | Nothing | Trait IDs defined in this array will be forcibly removed from the character when trait is mutated.                                                          |
| `changes_to`      | Nothing | Used for defining mutation lines with defined steps. This trait can further mutate into any other trait defined in this list.                               |
| `leads_to`        | Nothing | Mutations that add onto this one without removing it. Effectively a reverse of the `prereqs` tag.                                                           |
| `starting_trait`  | false   | If true, this trait can be selected during character creation.                                                                                              |
| `valid`           | true    | Whether or not this trait can be obtained through mutation. Invalid traits are still obtainable while creating a character.                                 |
| `purifiable`      | true    | Whether or not this trait can be removed. If false, the trait cannot be removed by any means.                                                               |
| `player_display`  | true    | If false, this trait is invisible, and will not appear in messages or the character sheet.                                                                  |
| `mixed_effect`    | false   | Whether this trait has both positive and negative effects. Used only for UI; mixed mutations will appear with purple text instead of green, yellow, or red (which is usually automatically determined by point cost). |
| `vanity`          | false   | This trait is purely cosmetic, and can be changed at any time. This is for things like skin color, eye color, hair color, etc.                              |
| `debug`           | false   | Identifies this trait as a debug trait that should never be obtainable in normal play. Debug traits will have a distinct teal color on the character sheet. |
| `visibility`      | 0       | How visible this mutation is on NPCs when inspecting them, ranging between 0 and 10. Higher numbers are more visible, and can be seen from farther with a lower Perception score. |
| `ugliness`        | 0       | How unpleasant this mutation is to look at. Ugly mutations increase NPC fear and decrease their trust. Negative values are allowed, and will do the opposite by making NPCs more trusting and less afraid. |
| `active`          | false   | Active traits have special effects that only occur when the trait is manually or automatically activated; they don't happen on their own.                   |
| `starts_active`   | false   | If true, a trait will start activated, instead of deactivated. Will throw an error if `active` is not defined.                                              |
| `time`            | 0       | The time interval (measured in turns) between an active mutation requiring its cost again. This can be used to create active mutations that continually drain resources while active. If 0 or below, the cost is a one-time requirement instead. |
| `cost`            | 0       | For active mutations, this value is the cost to activate them. At least one of the following three values will need to be `true` for this to function.      |
| `kcal`            | false   | If true, this active mutation will consume `cost` kcal during activation or upkeep.                                                                         |
| `thirst`          | false   | If true, this active mutation will consume `cost` thirst during activation or upkeep.                                                                       |
| `fatigue`         | false   | If true, this active mutation will consume `cost` fatigue during activation or upkeep.                                                                      |
| `enchantments`    | Nothing | A list of enchantments granted by this mutation. Can either be string IDs of a defined enchantment, or an inline definition.                                |

### Optional Fields

There are many, many optional fields present for mutations to let them do all sorts of things. You can see them documented above.

### Sample trait: Example Sleep

```json
  {
    "type": "mutation",
    "id": "EXAMPLE_SLEEP",
    "name": { "str": "Example Sleep" },
    "points": 1,
    "description": "This is an example trait that makes it easier for the character to fall asleep at all times.  It can't be obtained through mutations; it only appears in character creation.",
    "changes_to": [ "EASYSLEEPER2" ],
    "valid": false,
    "starting_trait": true,
    "enchantments": [ { "condition": "ALWAYS", "values": [ { "value": "SLEEPY", "add": 24 } ] } ]
  }
```

## Mutation Categories

A Mutation Category identifies a set of interrelated mutations that as a whole establish an entirely new identity for a mutant character. Categories can and usually do have their own "flavor" of mutagen, the properties of which are defined in the category definition itself. A second kind of mutagen, called a "mutagen serum" or "IV mutagen" is necessary to trigger "threshold mutations" which cause irrevocable changes to the character.

| Identifier              | Description
|---                      |---
| `id`                    | Unique ID. Must be one continuous word, use underscores when necessary.
| `name`                  | Human readable name for the category of mutations.
| `threshold_mut`         | A special mutation that marks the point at which the identity of the character is changed by the extent of mutation they have experienced.
| `threshold_min`         | Amount of primer the character needs to have in their body to attempt breaking the threshold.  Default 2200.
| `mutagen_message`       | A message displayed to the player when they mutate in this category.
| `memorial_message`      | The memorial message to display when a character crosses the associated mutation threshold.
| `vitamin`               | The vitamin id of the primer of this category. The character's vitamin level will act as the weight of this category when selecting which category to try and mutate towards, and gets decreased on successful mutation by the trait's mutagen cost.
| `base_removal_chance`   | Int, percent chance for a mutation of this category removing a conflicting base (starting) trait, rolled per `Character::mutate_towards` attempts.  Default 100%.  Removed base traits will **NOT** be considered base traits from here on, even if you regain them later. 
| `base_removal_cost_mul` | Float, multiplier on the primer cost of the trait that removed a canceled starting trait, down to 0.0 for free mutations as long as a starting trait was given up.  Default 3.0, used for human-like categories and lower as categories become more inhuman.
| `wip`                   | A flag indicating that a mutation category is unfinished and shouldn't have consistency tests run on it. See tests/mutation_test.cpp.
| `skip_test`             | If true, this mutation category will be skipped in consistency tests; this should only be used if you know what you're doing. See tests/mutation_test.cpp.

## Trait Migrations

A mutation migration can be used to migrate a mutation that formerly existed gracefully into a proficiency, another mutation (potentially a specific variant), or to simply remove it without any fuss.

```json
[
  {
    "type": "TRAIT_MIGRATION",
    "id": "dead_trait1",
    "trait": "new_trait",
    "variant": "correct_variant"
  },
  {
    "type": "TRAIT_MIGRATION",
    "id": "trait_now_prof",
    "proficiency": "prof_old_trait"
  },
  {
    "type": "TRAIT_MIGRATION",
    "id": "deleted_trait",
    "remove": true
  }
]
```

| Identifier    | Description
|---            |---
| `type`        | Mandatory. String. Must be `"TRAIT_MIGRATION"`
| `id`          | Mandatory. String. Id of the trait that has been removed.
| `trait`       | Optional\*. String. Id of the trait this trait is being migrated to.
| `variant`     | Optional. String. Can only be specified if `trait` is specified. Id of a variant of `trait` that this mutation will be set to.
| `proficiency` | Optional\*. String. Id of proficiency that will replace this trait.
| `remove`      | Optional\*. Boolean. If neither `trait` or `variant` are specified, this must be true.

\*One of these three must be specified.

