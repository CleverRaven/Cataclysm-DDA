# Mutations

## System info

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

The odds of a mutation being good or bad is directly determined by Instability, which is a hidden value derived on a per-tree basis (it is possible for the chances of a bad mutation to be different across multiple mutation trees simultaneously).  This value is determined by counting all mutations you have that aren't negative, and counting double any mutations that don't belong as double.  For example, for the Feline mutation tree, "Feline Ears" is worth a single point of instability, while "Hooves" would count as two points. "Sleepy" counts as no points of instability, since it is a negative mutation.  Only traits you have gained as mutations count; ones you started with don't matter. Traits which cannot be gained as mutations, such as the Exodii's "Standard Neurobionic Matrix" mutation, also do not count.  Currently, the Robust Genetics trait negates the double penalty for out-of-tree mutations.  All mutations are counted as "in tree" that way.  Lastly, mutations count their prior forms.  "Very Strong" is worth two instability raw, since it counts "Strong" too which you had to mutate in order to obtain it even though it only shows one mutation in your character's list.  So if neither "Very Strong" nor "Strong" are in the tree, having "Very Strong" gives four instability; if Strong is in the tree but Very Strong isn't, 3 instability.

This value is added up and then compared to the number of mutations that are non negative in the given tree. For example, Alpha has 21 non negative mutations, while Troglobite has 28, and Chimera has even more.  Half of your instability for that tree divided by the prior tally is your chance of rolling a bad mutation. For example if your Alpha instability was 7 you'd have a 1/3 chance of getting a bad mutation instead of a good one, and if for Trog your instability was also 7 you would have a 1/4 chance of getting a bad mutation.

Instability cannot decrease over time.  It is a static value that can only be lowered by lowering your number of mutations by using Purifier.  If you purify back to baseline after becoming heavily mutated, it like getting a reset on your instability value, but it cannot otherwise be lowered without giving up your mutations.

Finally, there are some sources of true-random mutations that can be inflicted by certain nether monsters for example.  These are always completely random regardless of instability, but any beneficial mutations obtained will cause instability as usual.

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
  "butchering_quality": 4,                    // Butchering quality of this mutations (default: 0).
  "bodytemp_modifiers": [ 100, 150 ],           // Range of additional bodytemp units (these units are described in 'weather.h'.  First value is used if the person is already overheated, second one if it's not.
  "initial_ma_styles": [ "style_crane" ],     // (optional) A list of IDs of martial art styles of which the player can choose one when starting a game.
  "mixed_effect": false,                      // Whether the trait has both positive and negative effects.  This is purely declarative and is only used for the user interface (default: false).
  "description": "Nothing gets you down!",    // In-game description.
  "starting_trait": true,                     // Can be selected at character creation (default: false).
  "valid": false,                             // Can be mutated ingame (default: true).  Note that prerequisites can even mutate invalid mutations.
  "purifiable": false,                        // Sets if the mutation be purified (default: true).
  "profession": true,                         // Trait is a starting profession special trait (default: false).
  "debug": false,                             // Trait is for debug purposes (default: false).
  "dummy": false,                             // Dummy mutations are special; they're not gained through normal mutating, and will instead be targeted for the purposes of removing conflicting mutations
  "threshold": false,                         //True if it's a threshold itself, and shouldn't be obtained *easily*.  Disallows mutating this trait directly
  "threshold_substitutes": [ "FOO", "BAR" ],   // The listed traits are accepted in place of this threshold trait for the purposes of gaining post-threshold mutations
  "strict_thresreq": false,                   // This trait needs an *exact* threshold match (ie. ignores threshold substitutions)
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
  "personality_score": [                               // Optional. Defines cosmetic flavor traits for NPCs which can be viewed during dialogue with the NPC.
    {                                                  // Traits with a defined `personality_score` will be automatically applied to NPCs with personalities that qualify, based
      "min_aggression": -10,                           // on the min and max values defined. NPC personalities' values are always integers within the range [-10,10].
      "max_aggression": 10,                            // All mins and maxes are optional, this document contains their default values. (A personality_score trait with no
      "min_bravery": -10,                              // mins or maxes defined *at all* will be applied to all NPCs.)
      "max_bravery": 10,                               //
      "min_collector": -10,                            // As an example, a trait with a `personality_score` which defines only `"min_bravery": -5` will be applied to NPCs with
      "max_collector": 10,                             // bravery of -5, -1, 0, 2, 7, or 10 (all of them being higher than the minimum), but not to a NPC with -6 (being lower than the
      "min_altruism": -10,                             // minimum). This can be used to define a range of values for which a trait should be applied (e.g. min 2 max 4 excludes all
      "max_altruism": 10,                              // numbers except 2, 3, and 4.)
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
  "active": true,                             // When set the mutation is an active mutation that the player needs to activate (default: false).
  "starts_active": true,                      // When true, this 'active' mutation starts active (default: false, requires 'active').
  "cost": 8,                                  // Cost to activate this mutation.  Needs one of the hunger, thirst, or sleepiness values set to true (default: 0).
  "time": 100,                                // Sets the amount of (turns * current player speed ) time units that need to pass before the cost is to be paid again.  Needs to be higher than one to have any effect (default: 0).
  "kcal": true,                               // If true, activated mutation consumes `cost` kcal. (default: false).
  "thirst": true,                             // If true, activated mutation increases thirst by cost (default: false).
  "sleepiness": true,                            // If true, activated mutation increases sleepiness by cost (default: false).
  "active_flags": [ "BLIND" ],                // activation of the mutation apply this flag on your character
  "allowed_items": [ "ALLOWS_TAIL" ],         // you can wear items with this flag with this mutation, bypassing restricts_gear restriction
  "integrated_armor": [ "integrated_fur" ],   // this item is worn on your character forever, until you get rid of this mutation
  "ranged_mutation": {                        // activation of the mutation allow you to shoot a fake gun
    "type": "pseudo_shotgun",                 // fake gun that is used to shoot
    "message": "SUDDEN SHOTGUN!."             // message that would be printed when you use it
  },
  "spawn_item": {                             // activation of this mutation spawns an item
    "type": "water_clean",                    // item to spawn
    "message": "You spawn a bottle of water." // message, that would be shown upon activation
  },
  "scent_intensity": 800,                     // int affecting the target scent toward which you current smell gravitates (default: 500).
  "scent_type": "sc_flower",                  // The scent_types you emit, as defined in scent_types.json (default: empty).
  "ignored_by": [ "ZOMBIE" ],                 // List of species ignoring you (default: empty).
  "anger_relations": [ [ "MARSHMALLOW", 20 ], [ "GUMMY", 5 ], [ "CHEWGUM", 20 ] ], // List of species angered by you and how much, negative values calm instead  (default: empty).
  "can_only_eat": [ "junk" ],                 // List of comestible materials (default: empty).
  "can_only_heal_with": [ "bandage" ],        // List of `MED` you are restricted to, includes mutagen, serum, aspirin, bandages etc. (default: empty).
  "can_heal_with": [ "caramel_ointement" ],   // List of `MED` that will work for you but not for anyone else.  See `CANT_HEAL_EVERYONE` flag for items (default: empty).
  "allowed_category": [ "ALPHA" ],            // List of categories you can mutate into (default: empty).
  "no_cbm_on_bp": [ "torso", "head", "eyes", "mouth", "arm_l" ],  // List of body parts that can't receive CBMs (default: empty).
  "lumination": [ [ "head", 20 ], [ "arm_l", 10 ] ],              // List of glowing bodypart and the intensity of the glow as a float (default: empty).
  "social_modifiers": { "persuade": -10 },    // Social modifiers.  Can be: intimidate, lie, persuade.
  "spells_learned": [ [ "spell_slime_spray", 1 ] ], // Spells learned and the level they're at after gaining the trait/mutation.
  "transform": {
    "target": "BIOLUM1",                      // Trait_id of the mutation this one will transform into.
    "msg_transform": "You turn your photophore OFF.", // Message displayed upon transformation.
    "active": false,                          // If true, mutation will start powered when activated (turn ON).
    "moves": 100,                              // Moves cost per activation (default: 0).
    "safe": false                              // If true the transformation will use the normal mutation progression rules - removing conflicting traits, requiring thresholds (but not using any vitamins or causing instability)
  },
  "triggers": [                               // List of sublist of triggers, all sublists must be True for the mutation to activate.
    [                                         // Sublist of trigger: at least one trigger must be true for the sublist to be true.
      { 
        "condition": { "math": [ "u_val('morale')", "<", "-50" ] },                                     // Dialogue condition (see NPCs.md).
        "msg_on": { "text": "Everything is terrible and this makes you so ANGRY!", "rating": "mixed" }  // Message displayed when the trigger activates.
      }
    ],
    [
      {
        "condition": {                        // Dialogue condition (see NPCs.md).
          "or": [ 
            { "math": [ "u_val('strength')", "<", "5" ] },
            { "math": [ "u_val('dexterity')", ">", "20" ] }
          ]
        },
        "msg_on": { "text": "Everything is terrible and this makes you so ANGRY!", "rating": "mixed" },// Message displayed when the trigger activates.
        "msg_off": { "text": "Your glow fades." }                                                      // Message displayed when the trigger deactivates the trait.
      }
    ]
  ],
  "activated_is_setup": true,                 // If this is true the bellow activated EOC runs then the mutation turns on for processing every turn. If this is false the below "activated_eocs" will run and then the mod will turn itself off.
  "activated_eocs": [ "eoc_id_1" ],           // List of effect_on_conditions that attempt to activate when this mutation is successfully activated.
  "processed_eocs": [ "eoc_id_1" ],           // List of effect_on_conditions that attempt to activate every time (defined above) units of time. Time of 0 means every turn it processes. Processed when the mutation is active for activatable mutations and always for non-activatable ones.
  "deactivated_eocs": [ "eoc_id_1" ],         // List of effect_on_conditions that attempt to activate when this mutation is successfully deactivated.
  "enchantments": [ "ench_id_1" ],            // List of enchantments granted by this mutation.  Can be either IDs or an inline definition of the enchantment (see MAGIC.md)
  "flags": [ "UNARMED_BONUS" ],               // List of flag_IDs and json_flag_IDs granted by the mutation.  Note: trait_IDs can be set and generate no errors, but they're not actually "active".
  "moncams": [ [ "mon_player_blob", 16 ] ],    // Monster cameras, ability to use friendly monster's from the list as additional source of vision. Max view distance is equal to monster's daytime vision. The number specifies the range at which it can "transmit" vision to the avatar.
  "override_look": { "id": "mon_cat_black", "tile_category": "monster" } // Change the character's appearance to another specified thing with a specified ID and tile category. Please ensure that the ID corresponds to the tile category. The valid tile category are "none", "vehicle_part", "terrain", "item", "furniture", "trap", "field", "lighting", "monster", "bullet", "hit_entity", "weather", "overmap_terrain", "map_extra", "overmap_note".
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
| `types`           | Nothing | A list of types that this mutation can be classified under. Each mutation with a certain type is mutually exclusive with other mutations that also have that type; if a trait has the `EXAMPLE` type defined, then no other trait with that type can exist on a character, and mutating towards such a trait would remove the existing one if it could.  An exception is made for same-typed prerequisite traits, these will not get removed.|
| `prereqs`         | Nothing | An array of mutation IDs that are possible requirements for this trait to be obtained. Only a single option from this list needs to be present.             |
| `prereqs2`        | Nothing | Identical to `prereqs`, and will throw errors if `prereqs` isn't defined. This is used to have multiple traits required to obtain another trait; one option must be present on the character from both `prereqs` and `prereqs2` for a trait to be obtainable. |
| `threshreq`       | Nothing | This is a dedicated prerequisite slot for threshold mutations, and functions identically to `prereq` and `prereq2`.                                         |
| `cancels`         | Nothing | Trait IDs defined in this array will be forcibly removed from the character when trait is mutated.                                                          |
| `changes_to`      | Nothing | Used for defining mutation lines with defined steps. This trait can further mutate into any other trait defined in this list.                               |
| `leads_to`        | Nothing | Mutations that add onto this one without removing it. Effectively a reverse of the `prereqs` tag.  Also prevents type conflicts with this trait!                                                    |
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
| `sleepiness`         | false   | If true, this active mutation will consume `cost` sleepiness during activation or upkeep.                                                                      |
| `enchantments`    | Nothing | A list of enchantments granted by this mutation. Can either be string IDs of a defined enchantment, or an inline definition.                                |

### Optional Fields

There are many, many optional fields present for mutations to let them do all sorts of things. You can see them documented above.

### EOC details

Mutations support EOC on activate, deactivate and for processing. As well for each of those the EOC has access to the context variable `this` which is the ID of the mutation itself.

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
