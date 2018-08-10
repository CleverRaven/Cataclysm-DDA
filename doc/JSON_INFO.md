# JSON file contents

## File descriptions
Here's a quick summary of what each of the JSON files contain, broken down by folder.

## `raw/`

| Filename               | Description
|---                     |---
| bionics.json           | bionics, does NOT include bionic effects
| dreams.json            | dream text and linked mutation categories
| item_groups.json       | item spawn groups
| materials.json         | material types
| monstergroups.json     | monster spawn groups
| monster_factions.json  | monster factions
| names.json             | names used for NPC/player name generation
| professions.json       | profession definitions
| recipes.json           | crafting/disassembly recipes
| road_vehicles.json     | vehicle spawn information for roads
| skills.json            | skill descriptions and ID's
| snippets.json          | flier/poster descriptions
| mutations.json         | traits/mutations
| mutation_ordering.json | draw order for mutation overlays in tiles mode
| vehicle_groups.json    | vehicle spawn groups
| vehicle_parts.json     | vehicle parts, does NOT affect flag effects

## `raw/vehicles/`

Groups of vehicle definitions with self-explanatory names of files:

| Filename
|---
| bikes.json
| cars.json
| carts.json
| emergency.json
| farm.json
| military.json
| trucks.json
| utility.json
| vans_busses.json
| vehicles.json

## `raw/items/`

| Filename           | Description
|---                 |---
| archery.json       | bows and arrows
| ranged.json        | guns
| tools.json         | tools and items that can be (a)ctivated
| ammo.json          | ammo
| books.json         | books
| comestibles.json   | food/drinks
| containers.json    | containers
| melee.json         | anything that doesn't go in the other item jsons, melee weapons
| mods.json          | gunmods

## `json/`

| Filename           | Description
|---                 |---
| furniture.json     | furniture, and features treated like furniture
| terrain.json       | terrain types and definitions

# Raw JS

### All Files

```json
"//" : "comment", // Preferred method of leaving comments inside json files.
```

### Bionics

| Identifier         | Description
|---                 |---
| id                 | Unique ID. Must be one continuous word, use underscores if necessary.
| name               | In-game name displayed.
| active             | Whether the bionic is active or passive. (default: `passive`)
| power_source       | Whether the bionic provides power. (default: `false`)
| faulty             | Whether it is a faulty type. (default: `false`)
| cost               | How many PUs it costs to use the bionic. (default: `0`)
| time               | How long, when activated, between drawing cost. If 0, it draws power once. (default: `0`)
| description        | In-game description.
| canceled_mutations | (_optional_) A list of mutations/traits that are removed when this bionic is installed (e.g. because it replaces the fault biological part).
| included_bionics   | (_optional_) Additional bionics that are installed automatically when this bionic is installed. This can be used to install several bionics from one CBM item, which is useful as each of those can be activated independently.

```json
{
    "id"           : "bio_batteries",
    "name"         : "Battery System",
    "active"       : false,
    "power_source" : false,
    "faulty"       : false,
    "cost"         : 0,
    "time"         : 0,
    "description"  : "You have a battery draining attachment, and thus can make use of the energy contained in normal, everyday batteries. Use 'E' to consume batteries.",
    "canceled_mutations": ["HYPEROPIC"],
    "included_bionics": ["bio_blindfold"]
}
```

### Dreams

| Identifier | Description
|---         |---
| messages   | List of potential dreams.
| category   | Mutation category needed to dream.
| strength   | Mutation category strength required (1 = 20-34, 2 = 35-49, 3 = 50+).

```json
{
    "messages" : [
        "You have a strange dream about birds.",
        "Your dreams give you a strange feathered feeling."
    ],
    "category" : "MUTCAT_BIRD",
    "strength" : 1
}
```

### Item Groups

Item groups have been expanded, look at doc/ITEM_SPAWN.md to their new description.
The syntax listed here is still valid.

| Identifier | Description
|---         |---
| id         | Unique ID. Must be one continuous word, use underscores if necessary
| items      | List of potential item ID's. Chance of an item spawning is x/T, where X is the value linked to the specific item and T is the total of all item values in a group.
| groups     | ??

```json
{
    "id":"forest",
    "items":[
        ["rock", 40],
        ["stick", 95],
        ["mushroom", 4],
        ["mushroom_poison", 3],
        ["mushroom_magic", 1],
        ["blueberries", 3]
    ],
    "groups":[]
}
```

### Materials

| Identifier       | Description
|---               |---
| `ident`          | Unique ID. Must be one continuous word, use underscores if necessary.
| `name`           | In-game name displayed.
| `bash_resist`    | How well a material resists bashing damage.
| `cut_resist`     | How well a material resists cutting damage.
| `bash_dmg_verb`  | Verb used when material takes bashing damage.
| `cut_dmg_verb`   | Verb used when material takes cutting damage.
| `dmg_adj`        | Description added to damaged item in ascending severity.
| `acid_resist`    | Ability of a material to resist acid.
| `elec_resist`    | Ability of a material to resist electricity.
| `fire_resist`    | Ability of a material to resist fire.
| `density`        | Density of a material.

```json
{
    "ident"         : "hflesh",
    "name"          : "Human Flesh",
    "bash_resist"   : 1,
    "cut_resist"    : 1,
    "bash_dmg_verb" : "bruised",
    "cut_dmg_verb"  : "sliced",
    "dmg_adj"       : ["bruised", "mutilated", "badly mutilated", "thoroughly mutilated"],
    "acid_resist"   : 1,
    "elec_resist"   : 1,
    "fire_resist"   : 0,
    "density"       : 5
}
```

### Monster Groups

#### Group definition

| Identifier | Description
|---         |---
| `name`     | Unique ID. Must be one continuous word, use underscores if necessary.
| `default`  | Default monster, automatically fills in any remaining spawn chances.
| `monsters` | To choose a monster for spawning, the game creates 1000 entries and picks one. Each monster will have a number of entries equal to it's "freq" and the default monster will fill in the remaining. See the table below for how to build the single monster definitions.

#### Monster definition

| Identifier   | Description
|---           |---
| `monster`    | The monster's id.
| `freq`       | Chance of occurrence, out of a thousand.
| `multiplier` | How many monsters each monster in this definition should count as, if spawning a limited number of monsters.
| `pack_size`  | (_optional_) The minimum and maximum number of monsters in this group that should spawn together.  (default: `[1,1]`)
| `conditions` | Conditions limit when monsters spawn. Valid options: `SUMMER`, `WINTER`, `AUTUMN`, `SPRING`, `DAY`, `NIGHT`, `DUSK`, `DAWN`. Multiple Time-of-day conditions (`DAY`, `NIGHT`, `DUSK`, `DAWN`) will be combined together so that any of those conditions makes the spawn valid. Multiple Season conditions (`SUMMER`, `WINTER`, `AUTUMN`, `SPRING`) will be combined together so that any of those conditions makes the spawn valid.

```json
{
    "name" : "GROUP_ANT",
    "default" : "mon_ant",
    "monsters" : [
        { "monster" : "mon_ant_larva", "freq" : 40, "multiplier" : 0 },
        { "monster" : "mon_ant_soldier", "freq" : 90, "multiplier" : 5 },
        { "monster" : "mon_ant_queen", "freq" : 0, "multiplier" : 0 },
        { "monster" : "mon_thing", "freq" : 100, "multiplier" : 0, "pack_size" : [3,5], "conditions" : ["DUSK","DAWN","SUMMER"] }
    ]
}
```

### Monster Factions

| Identifier      | Description
|---              |---
| `name`          | Unique ID. Must be one continuous word, use underscores when necessary.
| `base_faction`  | Optional base faction. Relations to other factions are inherited from it and relations of other factions to this one check this.
| `by_mood`       | Be hostile towards this faction when angry, neutral otherwise. Default attitude to all other factions.
| `neutral`       | Always be neutral towards this faction.
| `friendly`      | Always be friendly towards this faction. By default a faction is friendly towards itself.

```json
{
    "name"         : "cult",
    "base_faction" : "zombie",
    "by_mood"      : ["blob"],
    "neutral"      : ["nether"],
    "friendly"     : ["blob"]
}
```

### Monsters

See MONSTERS.md

### Names

```json
{ "name" : "Aaliyah", "gender" : "female", "usage" : "given" }, // Name, gender, "given"/"family"/"city" (first/last/city name).
// NOTE: Please refrain from adding name PR's in order to maintain kickstarter exclusivity
```

### Professions

Professions are specified as JSON object with "type" member set to "profession":

```JSON
{
    "type": "profession",
    "ident": "hunter",
    ...
}
```

The ident member should be the unique id of the profession.

The following properties (mandatory, except if noted otherwise) are supported:

#### `description`
(string)

The in-game description.

#### `name`
(string or object with members "male" and "female")

The in-game name, either one gender-neutral string, or an object with gender specific names. Example:
```JSON
"name": {
    "male": "Groom",
    "female": "Bride"
}
```

#### `points`
(integer)

Point cost of profession. Positive values cost points and negative values grant points.

#### `addictions`
(optional, array of addictions)

List of starting addictions. Each entry in the list should be an object with the following members:
- "type": the string id of the addiction (see JSON_FLAGS.md),
- "intensity": intensity (integer) of the addiction.

Example:
```JSON
"addictions": [
    { "type": "nicotine", "intensity": 10 }
]
```

Mods can modify this list (requires `"edit-mode": "modify"`, see example) via "add:addictions" and "remove:addictions", removing requires only the addiction type. Example:
```JSON
{
    "type": "profession",
    "ident": "hunter",
    "edit-mode": "modify",
    "remove:addictions": [
        "nicotine"
    ],
    "add:addictions": [
        { "type": "alcohol", "intensity": 10 }
    ]
}
```

#### `skills`

(optional, array of skill levels)

List of starting skills. Each entry in the list should be an object with the following members:
- "name": the string id of the skill (see skills.json),
- "level": level (integer) of the skill. This is added to the skill level that can be chosen in the character creation.

Example:
```JSON
"skills": [
    { "name": "archery", "level": 2 }
]
```

Mods can modify this list (requires `"edit-mode": "modify"`, see example) via "add:skills" and "remove:skills", removing requires only the skill id. Example:
```JSON
{
    "type": "profession",
    "ident": "hunter",
    "edit-mode": "modify",
    "remove:skills": [
        "archery"
    ],
    "add:skills": [
        { "name": "computer", "level": 2 }
    ]
}
```

#### `items`

(optional, object with optional members "both", "male" and "female")

Items the player starts with when selecting this profession. One can specify different items based on the gender of the character. Each lists of items should be an array of items ids, or pairs of item ids and snippet ids. Item ids may appear multiple times, in which case the item is created multiple times. The syntax for each of the three lists is identical.

Example:
```JSON
"items": {
    "both": [
        "pants",
        "rock",
        "rock",
        ["tshirt_text", "allyourbase"],
        "socks"
    ],
    "male": [
        "briefs"
    ],
    "female": [
        "panties"
    ]
}
```

This gives the player pants, two rocks, a t-shirt with the snippet id "allyourbase" (giving it a special description), socks and (depending on the gender) briefs or panties.

Mods can modify the lists of existing professions. This requires the "edit-mode" member with value "modify" (see example). Adding items to the lists can be done with via "add:both" / "add:male" / "add:female". It allows the same content (it allows adding items with snippet ids). Removing items is done via "remove:both" / "remove:male" / "remove:female", which may only contain items ids.

Example for mods:

```JSON
{
    "type": "profession",
    "ident": "hunter",
    "edit-mode": "modify",
    "items": {
        "remove:both": [
            "rock",
            "tshirt_text"
        ],
        "add:both": [ "2x4" ],
        "add:female": [
            ["tshirt_text", "allyourbase"]
        ]
    }
}
```

This mod removes one of the rocks (the other rock is still created), the t-shirt, adds a 2x4 item and gives female characters a t-shirt with the special snippet id.

#### `flags`

(optional, array of strings)

A list of flags. TODO: document those flags here.

Mods can modify this via `add:flags` and `remove:flags`.

#### `cbms`

(optional, array of strings)

A list of CBM ids that are implanted in the character.

Mods can modify this via `add:CBMs` and `remove:CBMs`.

#### `traits`

(optional, array of strings)

A list of trait/mutation ids that are applied to the character.

Mods can modify this via `add:traits` and `remove:traits`.

### Recipes

```json
"result": "javelin",         // ID of resulting item
"category": "CC_WEAPON",     // Category of crafting recipe. CC_NONCRAFT used for disassembly recipes
"id_suffix": "",             // Optional (default: empty string). Some suffix to make the ident of the recipe unique. The ident of the recipe is "<id-of-result><id_suffix>".
"override": false,           // Optional (default: false). If false and the ident of the recipe is already used by another recipe, loading of recipes fails. If true and a recipe with the ident is already defined, the existing recipe is replaced by the new recipe.
"skill_used": "fabrication", // Skill trained and used for success checks
"requires_skills": [["survival", 1], ["throw", 2]], // Skills required to unlock recipe
"book_learn": [              // (optional) Array of books that this recipe can be learned from. Each entry contains the id of the book and the skill level at which it can be learned.
    [ "textbook_anarch", 7, "something" ], // The optional third entry defines a name for the recipe as it should appear in the books description (default is the name of resulting item of the recipe)
    [ "textbook_gaswarfare", 8, "" ] // If the name is empty, the recipe is hidden, it will not be shown in the description of the book.
],
"difficulty": 3,             // Difficulty of success check
"time": 5000,                // Time to perform recipe (where 1000 ~= 10 turns ~= 1 minute game time)
"reversible": false,         // Can be disassembled.
"autolearn": true,           // Automatically learned upon gaining required skills
"autolearn" : [              // Automatically learned upon gaining listed skills
    [ "survival", 2 ],
    [ "fabrication", 3 ]
],
"decomp_learn" : 4,          // Can be learned by disassembling an item of same type as result at this level of the skill_used
"decomp_learn" : [           // Can be learned by disassembling an item of same type as result at specified levels of skills
    [ "survival", 1 ],
    [ "fabrication", 2 ]
],
"batch_time_factors": [25, 15], // Optional factors for batch crafting time reduction. First number specifies maximum crafting time reduction as percentage, and the second number the minimal batch size to reach that number. In this example given batch size of 20 the last 6 crafts will take only 3750 time units.
"flags": [                   // A set of strings describing boolean features of the recipe
  "BLIND_EASY",
  "ANOTHERFLAG"
],
"qualities": [               // Generic qualities of tools needed to craft
  {"id":"CUT","level":1,"amount":1}
],
"tools": [                   // Specific tools needed to craft
[
    [ "fire", -1 ]           // Charges consumed when tool is used, -1 means no charges are consumed
]],
"components": [              // Equivalent tools or components are surrounded by a single set of brackets
[
  [ "spear_wood", 1 ],       // Number of charges/items required
  [ "pointy_stick", 1 ]
],
[
  [ "rag", 1 ],
  [ "leather", 1 ],
  [ "fur", 1 ]
],
[
  [ "plant_fibre", 20, false ], // Optional flag for recoverability, default is true.
  [ "sinew", 20, false ],
  [ "thread", 20, false ],
  [ "duct_tape", 20 ]  // Certain items are flagged as unrecoverable at the item definition level.
]
]
```

## Skills

```json
"ident" : "smg",  // Unique ID. Must be one continuous word, use underscores if necessary
"name" : "submachine guns",  // In-game name displayed
"description" : "Your skill with submachine guns and machine pistols. Halfway between a pistol and an assault rifle, these weapons fire and reload quickly, and may fire in bursts, but they are not very accurate.", // In-game description
"tags" : ["gun_type"]  // Special flags (default: none)
```

### Traits/Mutations

```json
"id": "LIGHTEATER",  // Unique ID
"name": "Optimist",  // In-game name displayed
"points": 2,         // Point cost of the trait. Positive values cost points and negative values give points
"visibility": 0,     // Visibility of the trait for purposes of NPC interaction (default: 0)
"ugliness": 0,       // Ugliness of the trait for purposes of NPC interaction (default: 0)
"bodytemp_modifiers" : [100, 150], // Range of additional bodytemp units (these units are described in 'weather.h'. First value is used if the person is already overheated, second one if it's not.
"bodytemp_sleep" : 50, // Additional units of bodytemp which are applied when sleeping
"initial_ma_styles": [ "style_crane" ], // (optional) A list of ids of martial art styles of which the player can choose one when starting a game.
"mixed_effect": false, // Wheather the trait has both positive and negative effects. This is purely declarative and is only used for the user interface. (default: false)
"description": "Nothing gets you down!" // In-game description
"starting_trait": true, // Can be selected at character creation (default: false)
"valid": false,      // Can be mutated ingame (default: true)
"purifiable": false, //Sets if the mutation be purified (default: true)
"profession": true, //Trait is a starting profession special trait. (default: false)
"initial_ma_styles" : [ "style_centipede", "style_venom_snake" ], //List of starting martial arts types. One of the list is selectable at start. Only works at character creation.
"category": ["MUTCAT_BIRD", "MUTCAT_INSECT"], // Categories containing this mutation
"prereqs": ["SKIN_ROUGH"], // Needs these mutations before you can mutate toward this mutation
"prereqs2": ["LEAVES"], //Also need these mutations before you can mutate towards this mutation. When both set creates 2 different mutation paths, random from one is picked. Only use together with "prereqs"
"threshreq": ["THRESH_SPIDER"], //Required threshold for this mutation to be possible
"cancels": ["ROT1", "ROT2", "ROT3"], // Cancels these mutations when mutating
"changes_to": ["FASTHEALER2"], // Can change into these mutations when mutating further
"leads_to": [], // Mutations that add to this one
"passive_mods" : { //increases stats with the listed value. Negative means a stat reduction
            "per_mod" : 1, //Possible values per_mod, str_mod, dex_mod, int_mod
            "str_mod" : 2
},
"wet_protection":[{ "part": "HEAD", // Wet Protection on specific bodyparts
                    "good": 1 } ] // "neutral/good/ignored" // Good increases pos and cancels neg, neut cancels neg, ignored cancels both
"vitamin_rates": [ [ "vitC", -1200 ] ], // How much extra vitamins do you consume per minute. Negative values mean production
"restricts_gear" : [ "TORSO" ], //list of bodyparts that get restricted by this mutation
"allow_soft_gear" : true, //If there is a list of 'restricts_gear' this sets if the location still allows items made out of soft materials (Only one of the types need to be soft for it to be considered soft). (default: false)
"destroys_gear" : true, //If true, destroys the gear in the 'restricts_gear' location when mutated into. (default: false)
"encumbrance_always" : [ // Adds this much encumbrance to selected body parts
    [ "ARM_L", 20 ],
    [ "ARM_R", 20 ]
],
"encumbrance_covered" : [ // Adds this much encumbrance to selected body parts, but only if the part is covered by not-OVERSIZE worn equipment
    [ "HAND_L", 50 ],
    [ "HAND_R", 50 ]
],
"armor" : [ // Protects selected body parts this much. Resistances use syntax like `PART RESISTANCE` below.
    [
        [ "ALL" ], // Shorthand that applies the selected resistance to the entire body
        { "bash" : 2 } // The resistance provided to the body part(s) selected above
    ],
    [   // NOTE: Resistances are applies in order and ZEROED between applications!
        [ "ARM_L", "ARM_R" ], // Overrides the above settings for those body parts
        { "bash" : 1 }        // ...and gives them those resistances instead
    ]
],
"active" : true, //When set the mutation is an active mutation that the player needs to activate (default: false)
"starts_active" : true, //When true, this 'active' mutation starts active (default: false, requires 'active')
"cost" : 8, // Cost to activate this mutation. Needs one of the hunger, thirst, or fatigue values set to true. (default: 0)
"time" : 100, //Sets the amount of (turns * current player speed ) time units that need to pass before the cost is to be paid again. Needs to be higher than one to have any effect. (default: 0)
"hunger" : true, //If true, activated mutation increases hunger by cost. (default: false)
"thirst" : true, //If true, activated mutation increases thirst by cost. (default: false)
"fatigue" : true, //If true, activated mutation increases fatigue by cost. (default: false)
```

### Vehicle Groups


```json
"id":"city_parked",            // Unique ID. Must be one continuous word, use underscores if necessary
"vehicles":[                 // List of potential vehicle ID's. Chance of a vehicle spawning is X/T, where
  ["suv", 600],           //    X is the value linked to the specific vehicle and T is the total of all
  ["pickup", 400],          //    vehicle values in a group
  ["car", 4700],
  ["road_roller", 300]
]
```

### Vehicle Parts

```json
"id": "wheel",                // Unique identifier
"name": "wheel",              // Displayed name
"symbol": "0",                // ASCII character displayed when part is working
"looks_like": "small_wheel",  // hint to tilesets if this part has no tile, use the looks_like tile
"color": "dark_gray",         // Color used when part is working
"broken_symbol": "x",         // ASCII character displayed when part is broken
"broken_color": "light_gray", // Color used when part is broken
"damage_modifier": 50,        // (Optional, default = 100) Dealt damage multiplier when this part hits something, as a percentage. Higher = more damage to creature struck
"durability": 200,            // How much damage the part can take before breaking
"wheel_width": 9,             /* (Optional, default = 0)
                               * SPECIAL: A part may have at most ONE of the following fields:
                               *    wheel_width = base wheel width in inches
                               *    size        = trunk/box storage volume capacity
                               *    power       = base engine power (in liters)
                               *    bonus       = bonus granted; muffler = noise reduction%, seatbelt = bonus to not being thrown from vehicle
                               *    par1        = generic value used for unique bonuses, like the headlight's light intensity */
"fuel_type": "NULL",          // (Optional, default = "NULL") Type of fuel/ammo the part consumes, as an item id
"item": "wheel",              // The item used to install this part, and the item obtained when removing this part
"difficulty": 4,              // Your mechanics skill must be at least this level to install this part
"breaks_into" : [             // When the vehicle part is destroyed, items from this item group (see ITEM_SPAWN.md) will be spawned around the part on the ground.
  {"item": "scrap", "count": [0,5]} // instead of an array, this can be an inline item group,
],
"breaks_into" : "some_item_group", // or just the id of an item group.
"flags": [                    // Flags associated with the part
     "EXTERNAL", "MOUNT_OVER", "WHEEL", "MOUNT_POINT", "VARIABLE_SIZE"
],
"damage_reduction" : {        // Flat reduction of damage, as described below. If not specified, set to zero
    "all" : 10,
    "physical" : 5
}
```

### Part Resistance

```json
"all" : 0.0f,        // Initial value of all resistances, overriden by more specific types
"physical" : 10,     // Initial value for bash, cut and stab
"non_physical" : 10, // Initial value for acid, heat, cold, electricity and biological
"biological" : 0.2f, // Resistances to specific types. Those override the general ones.
"bash" : 3,
"cut" : 3,
"acid" : 3,
"stab" : 3,
"heat" : 3,
"cold" : 3,
"electric" : 3
```

### Vehicle Placement
```json
"id":"road_straight_wrecks",  // Unique ID. Must be one continuous word, use underscores if necessary
"locations":[ {               // List of potential vehicle locations. When this placement is used, one of those locations will be chosen at random.
  "x" : [0,19],               // The x placement. Can be a single value or a range of possibilities.
  "y" : 8,                    // The y placement. Can be a single value or a range of possibilities.
  "facing" : [90,270]         // The facing of the vehicle. Can be a single value or an array of possible values.
} ]
```

### Vehicle Spawn

```json
"id":"default_city",            // Unique ID. Must be one continuous word, use underscores if necessary
"spawn_types":[ {       // List of spawntypes. When this vehicle_spawn is applied, it will choose from one of the spawntypes randomly, based on the weight.
  "description" : "Clear section of road",           //    A description of this spawntype
  "weight" : 33,          //    The chance of this spawn type being used.
  "vehicle_function" : ""jack-knifed_semi" // This is only needed if the spawntype uses a built-in json function.
  "vehicle_json" : {      // This is only needed for a json-specified spawntype.
  "vehicle" : "car",      // The vehicle or vehicle_group to spawn.
  "placement" : "%t_parked",  // The vehicle_placement to use when spawning the vehicle. This is not needed if the x, y, and facing are specified.
  "x" : [0,19],     // The x placement. Can be a single value or a range of possibilities. Not needed if placement is specified.
  "y" : 8,   // The y placement. Can be a single value or a range of possibilities. Not needed if placement is specified.
  "facing" : [90,270], // The facing of the vehicle. Can be a single value or an array of possible values. Not needed if placement is specified.
  "number" : 1, // The number of vehicles to spawn.
  "fuel" : -1, // The fuel of the new vehicles.
  "status" : 1,  // The status of the new vehicles.
} } ]
```

### Vehicles

```json
"id": "shopping_cart",                     // Internally-used name.
"name": "Shopping Cart",                   // Display name, subject to i18n.
"blueprint": "#",                          // Preview of vehicle - ignored by the code, so use only as documentation
"parts": [                                 // Parts list
    {"x": 0, "y": 0, "part": "box"},       // Part definition, positive x direction is to the left, positive y is to the right
    {"x": 0, "y": 0, "part": "casters"}    // See vehicle_parts.json for part ids
]
                                           /* Important! Vehicle parts must be defined in the same order you would install
                                            * them in the game (ie, frames and mount points first).
                                            * You also cannot break the normal rules of installation
                                            * (you can't stack non-stackable part flags). */
```

# `raw/items/` JSONs

### Generic Items

```json
"type" : "GENERIC",               // Defines this as some generic item
"id" : "socks",                   // Unique ID. Must be one continuous word, use underscores if necessary
"name" : "socks",                 // The name appearing in the examine box.  Can be more than one word separated by spaces
"name_plural" : "pairs of socks", // (Optional)
"container" : "null",             // What container (if any) this item should spawn within
"color" : "blue",                 // Color of the item symbol.
"symbol" : "[",                   // The item symbol as it appears on the map. Must be a Unicode string exactly 1 console cell width.
"looks_like": "rag",              // hint to tilesets if this item has no tile, use the looks_like tile
"description" : "Socks. Put 'em on your feet.", // Description of the item
"phase" : "solid",                // (Optional, default = "solid") What phase it is
"weight" : 350,                   // Weight of the item in grams
"volume" : 1,                     // Volume, measured in 1/4 liters
"integral_volume" : 0,            // Volume added to base item when item is integrated into another (eg. a gunmod integrated to a gun)
"rigid": false,                   // For non-rigid items volume (and for worn items encumbrance) increases proportional to contents
"price" : 100,                    // Used when bartering with NPCs
"material" : ["COTTON"],          // Material types, can be as many as you want.  See materials.json for possible options
"cutting" : 0,                    // (Optional, default = 0) Cutting damage caused by using it as a melee weapon
"bashing" : -5,                   // (Optional, default = 0) Bashing damage caused by using it as a melee weapon
"to_hit" : 0,                     // (Optional, default = 0) To-hit bonus if using it as a melee weapon (whatever for?)
"flags" : ["VARSIZE"],            // Indicates special effects, see JSON_FLAGS.md
"magazine_well" : 0,              // Volume above which the magazine starts to protrude from the item and add extra volume
"magazines" : [                   // Magazines types for each ammo type (if any) which can be used to reload this item
    [ "9mm", [ "glockmag" ] ]     // The first magazine specified for each ammo type is the default
    [ "45", [ "m1911mag", "m1911bigmag" ] ],
],
"explode_in_fire" : true,         // Should the item explode if set on fire
"explosion": {                    // Physical explosion data
    "power" : 10,                 // Measure of explosion power in grams of TNT equivalent explosive, affects damage and range.
    "distance_factor" : 0.9,      // How much power is retained per traveled tile of explosion. Must be lower than 1 and higher than 0.
    "fire" : true,                // Should the explosion leave fire
    "shrapnel": 200,              // Total mass of casing, rest of fragmentation variables set to reasonable defaults.
    "shrapnel" : {
        "casing_mass" : 200,      // Total mass of casing, casing/power ratio determines fragment velocity.
        "fragment_mass" : 0.05,   // Mass of each fragment in grams. Large fragments hit harder, small fragments hit more often.
        "recovery" : 10,          // Percentage chance to drop an item at landing point.
        "drop" : "nail"           // Which item to drop at landing point.
    }
},
```

### Ammo

```json
"type" : "AMMO",      // Defines this as ammo
...                   // same entries as above for the generic item.
                      // additional some ammo specific entries:
"ammo_type" : "shot", // Determines what it can be loaded in
"damage" : 18,        // Ranged damage when fired
"pierce" : 0,         // Armor piercing ability when fired
"range" : 5,          // Range when fired
"dispersion" : 0,     // Inaccuracy of ammo, measured in quarter-degrees
"recoil" : 18,        // Recoil caused when firing
"count" : 25,         // Number of rounds that spawn together
"stack_size" : 50,    // (Optional) How many rounds are in the above-defined volume. If omitted, is the same as 'count'
"effects" : ["COOKOFF", "SHOT"] // Special effects
```

### Magazine

```json
"type": "MAGAZINE",   // Defines this as a MAGAZINE
...                   // same entries as above for the generic item.
                      // additional some magazine specific entries:
"ammo_type": "223",   // What type of ammo this magazine can be loaded with
"capacity" : 15,      // Capacity of magazine (in equivalent units to ammo charges)
"count" : 0,          // Default amount of ammo contained by a magazine (set this for ammo belts)
"default_ammo": "556",// If specified override the default ammo (optionally set this for ammo belts)
"reliability" : 8,    // How reliable this this magazine on a range of 0 to 10? (see GAME_BALANCE.md)
"reload_time" : 100,  // How long it takes to load each unit of ammo into the magazine
"linkage" : "ammolink"// If set one linkage (of given type) is dropped for each unit of ammo consumed (set for disintegrating ammo belts)
```

### Armor

Armor can be defined like this:

```json
"type" : "ARMOR",     // Defines this as armor
...                   // same entries as above for the generic item.
                      // additional some armor specific entries:
"covers" : ["FEET"],  // Where it covers.  Possible options are TORSO, HEAD, EYES, MOUTH, ARMS, HANDS, LEGS, FEET
"storage" : 0,        //  (Optional, default = 0) How many volume storage slots it adds
"warmth" : 10,        //  (Optional, default = 0) How much warmth clothing provides
"environmental_protection" : 0,  //  (Optional, default = 0) How much environmental protection it affords
"encumbrance" : 0,    // Base encumbrance (unfitted value)
"coverage" : 80,      // What percentage of body part
"material_thickness" : 1  // Thickness of material, in millimetre units (approximately).  Generally ranges between 1 - 5, more unusual armor types go up to 10 or more
"power_armor" : false, // If this is a power armor item (those are special).
```
Alternately, every item (book, tool, gun, even food) can be used as armor if it has armor_data:
```json
"type" : "TOOL",      // Or any other item type
...                   // same entries as for the type (e.g. same entries as for any tool),
"armor_data" : {      // additionally the same armor data like above
    "covers" : ["FEET"],
    "storage" : 0,
    "warmth" : 10,
    "environmental_protection" : 0,
    "encumbrance" : 0,
    "coverage" : 80,
    "material_thickness" : 1
    "power_armor" : false
}
```

### Books

Books can be defined like this:

```json
"type" : "BOOK",      // Defines this as a BOOK
...                   // same entries as above for the generic item.
                      // additional some book specific entries:
"max_level" : 5,      // Maximum skill level this book will train to
"intelligence" : 11,  // Intelligence required to read this book without penalty
"time" : 35,          // Time (in minutes) a single read session takes
"fun" : -2,           // Morale bonus/penalty for reading
"skill" : "computer", // Skill raised
"chapters" : 4,       // Number of chapters (for fun only books), each reading "consumes" a chapter. Books with no chapters left are less fun (because the content is already known to the character).
"required_level" : 2  // Minimum skill level required to learn
```
Alternately, every item (tool, gun, even food) can be used as book if it has book_data:
```json
"type" : "TOOL",      // Or any other item type
...                   // same entries as for the type (e.g. same entries as for any tool),
"book_data" : {       // additionally the same book data like above
    "max_level" : 5,
    "intelligence" : 11,
    "time" : 35,
    "fun" : -2,
    "skill" : "computer",
    "chapters" : 4,
    "use_action" : "MA_MANUAL", // The book_data can have use functions (see USE ACTIONS) that are triggered when the books has been read. These functions are not triggered by simply activating the item (like tools would).
    "required_level" : 2
}
```

#### Color Key

When adding a new book, please use this color key:

* Magazines: `pink`
* “Paperbacks” Short enjoyment books (including novels): `light_cyan`
* “Hardbacks” Long enjoyment books (including novels): `light_blue`
* “Small textbook” Beginner level textbooks, guides and martial arts books: `dark_green`
* “Large textbook” Advanced level textbooks and advanced guides: `dark_blue`
* Religious books: `dark_gray`
* “Printouts” (including spiral-bound and similar) Technical documents, (technical?) protocols, (lab) journals: `light_green`
* Other reading material/non-books (use only if every other category does not apply): `light_gray`

A few exceptions to this color key may apply, for example for books that don’t are what they seem to be.
Never use `yellow` and `red`, those colors are reserved for sounds and infrared vision.

### Comestibles

```json
"type" : "COMESTIBLE",      // Defines this as a COMESTIBLE
...                         // same entries as above for the generic item.
                            // additional some comestible specific entries:
"addiction_type" : "crack", // Addiction type
"spoils_in" : 0,            // How long a comestible is good for. 0 = no spoilage
"use_action" : "CRACK",     // What effects a comestible has when used, see special definitions below
"stim" : 40,                // Stimulant effect
"comestible_type" : "MED",  // Comestible type, used for inventory sorting
"quench" : 0,               // Thirst quenched
"heal" : -2,                // Health effects (used for sickness chances)
"addiction_potential" : 80, // Ability to cause addictions
"nutrition" : 0,            // Hunger satisfied
"tool" : "apparatus",       // Tool required to be eaten/drank
"charges" : 4,              // Number of uses when spawned
"stack_size" : 8,           // (Optional) How many uses are in the above-defined volume. If omitted, is the same as 'charges'
"fun" : 50                  // Morale effects when used
```

### Containers

```json
"type": "CONTAINER",  // Defines this as a container
...                   // same data as for the generic item (see above).
"contains": 200,      // How much volume this container can hold
"seals": false,       // Can be resealed, this is a required for it to be used for liquids. (optional, default: false)
"watertight": false,  // Can hold liquids, this is a required for it to be used for liquids. (optional, default: false)
"preserves": false,   // Contents do not spoil. (optional, default: false)
```
Alternately, every item can be used as container:
```json
"type": "ARMOR",      // Any type is allowed here
...                   // same data as for the type
"container_data" : {  // The container specific data goes here.
    "contains": 200,
}
```
This defines a armor (you need to add all the armor specific entries), but makes it usable as container.
It could also be written as a generic item ("type": "GENERIC") with "armor_data" and "container_data" entries.

### Melee

```json
"id": "hatchet",       // Unique ID. Must be one continuous word, use underscores if necessary
"symbol": ";",         // ASCII character used in-game
"color": "light_gray", // ASCII character color
"name": "hatchet",     // In-game name displayed
"description": "A one-handed hatchet. Makes a great melee weapon, and is useful both for cutting wood, and for use as a hammer.", // In-game description
"price": 95,           // Used when bartering with NPCs
"material": ["iron", "wood"], // Material types.  See materials.json for possible options
"weight": 907,         // Weight, measured in grams
"volume": 6,           // Volume, measured in 1/4 liters
"bashing": 12,         // Bashing damage caused by using it as a melee weapon
"cutting": 12,         // Cutting damage caused by using it as a melee weapon
"flags" : ["CHOP"],    // Indicates special effects
"to_hit": 1            // To-hit bonus if using it as a melee weapon
```

### Gun

Guns can be defined like this:

```json
"type": "GUN",        // Defines this as a GUN
...                   // same entries as above for the generic item.
                      // additional some gun specific entries:
"skill": "pistol",    // Skill used for firing
"ammo": "nail",       // Ammo type accepted for reloading
"ranged_damage": 0,   // Ranged damage when fired
"range": 0,           // Range when fired
"dispersion": 32,     // Inaccuracy of gun, measured in quarter-degrees
// When sight_dispersion and aim_speed are present in a gun mod, the aiming system picks the "best"
// sight to use for each aim action, which is the fastest sight with a dispersion under the current
// aim threshold.
"sight_dispersion": 10, // Inaccuracy of gun derived from the sight mechanism, also in quarter-degrees
"aim_speed": 3,       // A measure of how quickly the player can aim, in moves per point of dispersion.
"recoil": 0,          // Recoil caused when firing, in quarter-degrees of dispersion.
"durability": 8,      // Resistance to damage/rusting, also determines misfire chance
"burst": 5,           // Number of shots fired in burst mode
"clip_size": 100,     // Maximum amount of ammo that can be loaded
"ups_charges": 0,     // Additionally to the normal ammo (if any), a gun can require some charges from an UPS. This also works on mods. Attaching a mod with ups_charges will add/increase ups drain on the weapon.
"reload": 450,         // Amount of time to reload, 100 = 6 seconds = 1 "turn"
"built_in_mods": ["m203"], //An array of mods that will be integrated in the weapon using the IRREMOVABLE tag.
"default_mods": ["m203"] //An array of mods that will be added to a weapon on spawn.
```
Alternately, every item (book, tool, armor, even food) can be used as gun if it has gun_data:
```json
"type" : "TOOL",      // Or any other item type
...                   // same entries as for the type (e.g. same entries as for any tool),
"gun_data" : {        // additionally the same gun data like above
    "skill": ...,
    "recoil": ...,
    ...
}
```

### Gunmod

Gun mods can be defined like this:

```json
"type": "GUNMOD",              // Defines this as a GUNMOD
...                            // Same entries as above for the generic item.
                               // Additionally some gunmod specific entries:
"location": "stock",           // Mandatory. Where is this gunmod is installed?
"mod_targets": [ "crossbow" ], // Mandatory. What kind of weapons can this gunmod be used with?
"acceptable_ammo": [ "9mm" ],  // Optional filter restricting mod to guns with those base (before modifiers) ammo types
"install_time": 100,           // Optional number of moves installation takes. Installation is instantaneous if unspecified
"ammo_modifier": "57",         // Optional field which if specified modifies parent gun to use this ammo type
"burst_modifier": 3,           // Optional field increasing or decreasing base gun burst size
"damage_modifier": -1,         // Optional field increasing or decreasing base gun damage
"dispersion_modifier": 15,     // Optional field increasing or decreasing base gun dispersion
"loudness_modifier": 4,        // Optional field increasing or decreasing base guns loudness
"range_modifier": 2,           // Optional field increasing or decreasing base gun range
"recoil_modifier": -100,       // Optional field increasing or decreasing base gun recoil
"ups_charges": 200,            // Optional field increasing or decreasing base gun UPS consumption (per shot)
```

### Tools

```json
"id": "torch_lit",    // Unique ID. Must be one continuous word, use underscores if necessary
"type": "TOOL",       // Defines this as a TOOL
"symbol": "/",        // ASCII character used in-game
"color": "brown",     // ASCII character color
"name": "torch (lit)", // In-game name displayed
"description": "A large stick, wrapped in gasoline soaked rags. This is burning, producing plenty of light", // In-game description
"price": 0,           // Used when bartering with NPCs
"material": "wood",   // Material types.  See materials.json for possible options
"techniques": "FLAMING", // Combat techniques used by this tool
"flags": "FIRE",      // Indicates special effects
"weight": 831,        // Weight, measured in grams
"volume": 6,          // Volume, measured in 1/4 liters
"bashing": 12,        // Bashing damage caused by using it as a melee weapon
"cutting": 0,         // Cutting damage caused by using it as a melee weapon
"to_hit": 3,          // To-hit bonus if using it as a melee weapon
"max_charges": 75,    // Maximum charges tool can hold
"initial_charges": 75, // Charges when spawned
"rand_charges: [10, 15, 25], // Randomize the charges when spawned. This example has a 50% chance of rng(10, 15) charges and a 50% chance of rng(15, 25) (The endpoints are included)
"charges_per_use": 1, // Charges consumed per tool use
"turns_per_charge": 20, // Charges consumed over time
"ammo": "NULL",       // Ammo type used for reloading
"revert_to": "torch_done", // Transforms into item when charges are expended
"use_action": "TORCH_LIT" // Action performed when tool is used, see special definition below
```

### Seed Data

Every item type can have optional seed data, if the item has seed data, it's considered a seed and can be planted:

```
"seed_data" : {
    "fruits": "weed", // The item id of the fruits that this seed will produce.
    "seeds": false, // (optional, default is true). If true, harvesting the plant will spawn seeds (the same type as the item used to plant). If false only the fruits are spawned, no seeds.
    "fruit_div": 2, // (optional, default is 1). Final amount of fruit charges produced is divided by this number. Works only if fruit item is counted by charges.
    "byproducts": ["withered", "straw_pile"], // A list of further items that should spawn upon harvest.
    "plant_name": "sunflower", // The name of the plant that grows from this seed. This is only used as information displayed to the user.
    "grow" : 91 // Time it takes for a plant to fully mature. Based around a 91 day season length (roughly a real world season) to give better accuracy for longer season lengths
                // Note that growing time is later converted based upon the season_length option, basing it around 91 is just for accuracy purposes
                // A value 91 means 3 full seasons, a value of 30 would mean 1 season.
}
```

### Artifact Data

Every item type can have optional artifact properties (which makes it an artifact):

```JSON
"artifact_data" : {
    "charge_type": "ARTC_PAIN",
    "effects_carried": ["AEP_INT_DOWN"],
    "effects_wielded": ["AEP_DEX_UP"],
    "effects_activated": ["AEA_BLOOD", "AEA_NOISE"],
    "effects_worn": ["AEP_STR_UP"]
}
```

### Brewing Data

Every item type can have optional brewing data, if the item has brewing data, it can be placed in a vat and will ferment into a different item type.

Currently only vats can only accept and produce liquid items.

```JSON
"brewable" : {
    "time": 3600, // Time (in turns) the fermentation will take.
    "result": "beer" // The id of the result of the fermentation.
}
```

#### `Charge_type`

(optional, default: `ARTC_NULL`)

How the item is recharged.

For this to work, the item needs to be a tool that consumes charges upon invocation and has non-zero max_charges. Possible values (see src/artifact.h for an up-to-date list):

- `ARTC_NULL` Never recharges!
- `ARTC_TIME` Very slowly recharges with time
- `ARTC_SOLAR` Recharges in sunlight
- `ARTC_PAIN` Creates pain to recharge
- `ARTC_HP` Drains HP to recharge

#### `Effects_carried`

(optional, default: empty list)

Effects of the artifact when it's in the inventory (main inventory, wielded, or worn) of the player.

Possible values (see src/enums.h for an up-to-date list):

- `AEP_STR_UP` Strength + 4
- `AEP_DEX_UP` Dexterity + 4
- `AEP_PER_UP` Perception + 4
- `AEP_INT_UP` Intelligence + 4
- `AEP_ALL_UP` All stats + 2
- `AEP_SPEED_UP` +20 speed
- `AEP_IODINE` Reduces radiation
- `AEP_SNAKES` Summons friendly snakes when you're hit
- `AEP_INVISIBLE` Makes you invisible
- `AEP_CLAIRVOYANCE` See through walls
- `AEP_SUPER_CLAIRVOYANCE` See through walls to a great distance
- `AEP_STEALTH` Your steps are quieted
- `AEP_EXTINGUISH` May extinguish nearby flames
- `AEP_GLOW` Four-tile light source
- `AEP_PSYSHIELD` Protection from stare attacks
- `AEP_RESIST_ELECTRICITY` Protection from electricity
- `AEP_CARRY_MORE` Increases carrying capacity by 200
- `AEP_SAP_LIFE` Killing non-zombie monsters may heal you
- `AEP_HUNGER` Increases hunger
- `AEP_THIRST` Increases thirst
- `AEP_SMOKE` Emits smoke occasionally
- `AEP_EVIL` Addiction to the power
- `AEP_SCHIZO` Mimicks schizophrenia
- `AEP_RADIOACTIVE` Increases your radiation
- `AEP_MUTAGENIC` Mutates you slowly
- `AEP_ATTENTION` Draws netherworld attention slowly
- `AEP_STR_DOWN` Strength - 3
- `AEP_DEX_DOWN` Dex - 3
- `AEP_PER_DOWN` Per - 3
- `AEP_INT_DOWN` Int - 3
- `AEP_ALL_DOWN` All stats - 2
- `AEP_SPEED_DOWN` -20 speed
- `AEP_FORCE_TELEPORT` Occasionally force a teleport
- `AEP_MOVEMENT_NOISE` Makes noise when you move
- `AEP_BAD_WEATHER` More likely to experience bad weather
- `AEP_SICK` Decreases health over time

#### `effects_worn`
(optional, default: empty list)

Effects of the artifact when it's worn (it must be an armor item to be worn).

Possible values are the same as for effects_carried.

#### `effects_wielded`

(optional, default: empty list)

Effects of the artifact when it's wielded.

Possible values are the same as for effects_carried.

#### `effects_activated`

(optional, default: empty list)

Effects of the artifact when it's activated (which require it to have a `"use_action": "ARTIFACT"` and it must have a non-zero max_charges value).

Possible values (see src/artifact.h for an up-to-date list):

- `AEA_STORM` Emits shock fields
- `AEA_FIREBALL` Targeted
- `AEA_ADRENALINE` Adrenaline rush
- `AEA_MAP` Maps the area around you
- `AEA_BLOOD` Shoots blood all over
- `AEA_FATIGUE` Creates interdimensional fatigue
- `AEA_ACIDBALL` Targeted acid
- `AEA_PULSE` Destroys adjacent terrain
- `AEA_HEAL` Heals minor damage
- `AEA_CONFUSED` Confuses all monsters in view
- `AEA_ENTRANCE` Chance to make nearby monsters friendly
- `AEA_BUGS` Chance to summon friendly insects
- `AEA_TELEPORT` Teleports you
- `AEA_LIGHT` Temporary light source
- `AEA_GROWTH` Grow plants, a la triffid queen
- `AEA_HURTALL` Hurts all monsters!
- `AEA_RADIATION` Spew radioactive gas
- `AEA_PAIN` Increases player pain
- `AEA_MUTATE` Chance of mutation
- `AEA_PARALYZE` You lose several turns
- `AEA_FIRESTORM` Spreads minor fire all around you
- `AEA_ATTENTION` Attention from sub-prime denizens
- `AEA_TELEGLOW` Teleglow disease
- `AEA_NOISE` Loud noise
- `AEA_SCREAM` Noise & morale penalty
- `AEA_DIM` Darkens the sky slowly
- `AEA_FLASH` Flashbang
- `AEA_VOMIT` User vomits
- `AEA_SHADOWS` Summon shadow creatures

### Software Data

Every item type can have software data, it does not have any behavior:

```
"software_data" : {
    "type": "USELESS", // unused
    "power" : 91 // unused
}
```

### Use Actions

The contents of use_action fields can either be a string indicating a built-in function to call when the item is activated (defined in iuse.cpp), or one of several special definitions that invoke a more structured function.

```json
"use_action": {
    "type": "transform",  // The type of method, in this case one that transforms the item.
    "target": "gasoline_lantern_on", // The item to transform to.
    "active": true,       // Whether the item is active once transformed.
    "msg": "You turn the lamp on.", // Message to display when activated.
    "need_fire": 1,                 // Whether fire is needed to activate.
    "need_fire_msg": "You need a lighter!", // Message to display if there is no fire.
    "need_charges": 1,                      // Number of charges the item needs to transform.
    "need_charges_msg": "The lamp is empty." // Message to display if there aren't enough charges.
    "target_charges" : 3, // Number of charges the transformed item has.
    "container" : "jar",  // Container holding the target item.
    "moves" : 500         // Moves required to transform the item in excess of a normal action.
},
"use_action": {
    "type": "explosion", // An item that explodes when it runs out of charges.
    "sound_volume": 0, // Volume of a sound the item makes every turn.
    "sound_msg": "Tick.", // Message describing sound the item makes every turn.
    "no_deactivate_msg": "You've already pulled the %s's pin, try throwing it instead.", // Message to display if the player tries to activate the item, prevents activation from succeeding if defined.
    "explosion": { // Optional: physical explosion data
        // Specified like `"explosion"` field in generic items
    }
    "draw_explosion_radius" : 5, // How large to draw the radius of the explosion.
    "draw_explosion_color" : "ltblue", // The color to use when drawing the explosion.
    "do_flashbang" : true, // Whether to do the flashbang effect.
    "flashbang_player_immune" : true, // Whether the player is immune to the flashbang effect.
    "fields_radius": 3, // The radius of spread for fields produced.
    "fields_type": "fd_tear_gas", // The type of fields produced.
    "fields_min_density": 3,
    "fields_max_density": 3,
    "emp_blast_radius": 4,
    "scrambler_blast_radius": 4
},
"use_action": {
    "type": "unfold_vehicle", // Transforms the item into a vehicle.
    "vehicle_name": "bicycle", // Vehicle name to create.
    "unfold_msg": "You painstakingly unfold the bicycle and make it ready to ride.", // Message to display when transforming.
    "moves": 500 // Number of moves required in the process.
},
"use_action" : {
    "type" : "consume_drug", // A drug the player can consume.
    "activation_message" : "You smoke your crack rocks.  Mother would be proud.", // Message, ayup.
    "effects" : { "high": 15 }, // Effects and their duration.
    "stat_adjustments": {"hunger" : -10}, // Adjustment to make to player stats.
    "fields_produced" : {"cracksmoke" : 2}, // Fields to produce, mostly used for smoke.
    "charges_needed" : { "fire" : 1 }, // Charges to use in the process of consuming the drug.
    "tools_needed" : { "apparatus" : -1 }, // Tool needed to use the drug.
    "moves": 50 // Number of moves required in the process, default value is 100.
},
"use_action": {
    "type": "place_monster", // place a turret / manhack / whatever monster on the map
    "monster_id": "mon_manhack", // monster id, see monsters.json
    "difficulty": 4, // difficulty for programming it (manhacks have 4, turrets 6, ...)
    "hostile_msg": "It's hostile!", // (optional) message when programming the monster failed and it's hostile.
    "friendly_msg": "Good!", // (optional) message when the monster is programmed properly and it's friendly.
    "place_randomly": true, // if true: places the monster randomly around the player, if false: let the player decide where to put it (default: false)
    "skill1": "throw", // Id of a skill, higher skill level means more likely to place a friendly monster.
    "skill2": "unarmed", // Another id, just like the skill1. Both entries are optional.
    "moves": 60 // how many move points the action takes.
},
"use_action": {
    "type": "ups_based_armor", // Armor that can be activated and uses power from an UPS, needs additional json code to work
    "activate_msg": "You activate your foo.", // Message when the player activates the item.
    "deactive_msg": "You deactivate your foo.", // Message when the player deactivates the item.
    "out_of_power_msg": "Your foo runs out of power and deactivates itself." // Message when the UPS runs out of power and the item is deactivated automatically.
}
"use_action" : {
    "type" : "delayed_transform", // Like transform, but it will only transform when the item has a certain age
    "transform_age" : 600, // The minimal age of the item. Items that are younger wont transform. In turns (10 turn = 1 minute)
    "not_ready_msg" : "The yeast has not been done The yeast isn't done culturing yet." // A message, shown when the item is not old enough
},
"use_action": {
    "type": "picklock", // picking a lock on a door
    "pick_quality": 3 // "quality" of the tool, higher values mean higher success chance, and using it takes less moves.
},
"use_action": {
    "type": "firestarter", // Start a fire, like with a lighter.
    "moves_cost": 15 // Number of moves it takes to start the fire.
},
"use_action": {
    "type": "extended_firestarter", // Start a fire (like with magnifying glasses or a fire drill). This action can take many turns, not just some moves like firestarter, it can also be canceled (firestarter can't).
    "need_sunlight": true // Whether the character needs to be in direct sunlight, e.g. to use magnifying glasses.
},
"use_action": {
    "type": "salvage", // Try to salvage base materials from an item, e.g. cutting up cloth to get rags or leather.
    "moves_per_part": 25, // Number of moves it takes (optional).
    "material_whitelist": [ // List of material ids (not item ids!) that can be salvage from.
        "cotton",           // The list here is the default list, used when there is no explicit martial list given.
        "leather",          // If the item (that is to be cut up) has any material not in the list, it can not be cut up.
        "fur",
        "nomex",
        "kevlar",
        "plastic",
        "wood",
        "wool"
    ]
},
"use_action": {
    "type": "inscribe", // Inscribe a message on an item or on the ground.
    "on_items": true, // Whether the item can inscribe on an item.
    "on_terrain": false, // Whether the item can inscribe on the ground.
    "material_restricted": true, // Whether the item can only inscribe on certain item materials. Not used when inscribing on the ground.
    "material_whitelist": [ // List of material ids (not item ids!) that can be inscribed on.
        "wood",             // Only used when inscribing on an item, and only when material_restricted is true.
        "plastic",          // The list here is the default that is used when no explicit list is given.
        "glass",
        "chitin",
        "iron",
        "steel",
        "silver"
    ]
},
"use_action": {
    "type": "cauterize", // Cauterize the character.
    "flame": true // If true, the character needs 4 charges of fire (e.g. from a lighter) to do this action, if false, the charges of the item itself are used.
},
"use_action": {
    "type": "enzlave" // Make a zlave.
},
"use_action": {
    "type": "fireweapon_off", // Activate a fire based weapon.
    "target_id": "firemachete_on", // The item type to transform this item into.
    "success_message": "Your No. 9 glows!", // A message that is shows if the action succeeds.
    "failure_message": "", // A message that is shown if the action fails, for whatever reason. (Optional, if not given, no message will be printed.)
    "lacks_fuel_message": "Out of fuel", // Message that is shown if the item has no charges.
    "noise": 0, // The noise it makes to active the item, Optional, 0 means no sound at all.
    "moves": 0, // The number of moves it takes the character to even try this action (independent of the result).
    "success_chance": 0 // How likely it is to succeed the action. Default is to always succeed. Try numbers in the range of 0-10.
},
"use_action": {
    "type": "fireweapon_on", // Function for active (burning) fire based weapons.
    "noise_chance": 1, // The chance (one in X) that the item will make a noise, rolled on each turn.
    "noise": 0, // The sound volume it makes, if it makes a noise at all. If 0, no sound is made, but the noise message is still printed.
    "noise_message": "Your No. 9 hisses.", // The message / sound description (if noise is > 0), that appears when the item makes a sound.
    "voluntary_extinguish_message": "Your No. 9 goes dark.", // Message that appears when the item is turned of by player.
    "charges_extinguish_message": "Out of ammo!", // Message that appears when the item runs out of charges.
    "water_extinguish_message": "Your No. 9 hisses in the water and goes out.", // Message that appears if the character walks into water and the fire of the item is extinguished.
    "auto_extinguish_chance": 0, // If > 0, this is the (one in X) chance that the item goes out on its own.
    "auto_extinguish_message": "Your No. 9 cuts out!" // Message that appears if the item goes out on its own (only required if auto_extinguish_chance is > 0).
},
"use_action": {
    "type": "musical_instrument", // The character plays an instrument (this item) while walking around.
    "speed_penalty": 10, // This is subtracted from the characters speed.
    "volume": 12, // Volume of the sound of the instrument.
    "fun": -5, // Together with fun_bonus, this defines how much morale the character gets from playing the instrument. They get `fun + fun_bonus * <character-perception>` morale points out of it. Both values and the result may be negative.
    "fun_bonus": 2,
    "description_frequency": 20, // Once every Nth turn, a randomly chosen description (from the that array) is displayed.
    "descriptions": [
        "You play a little tune on your flute.",
        "You play a beautiful piece on your flute.",
        "You play a piece on your flute that sounds harmonious with nature."
    ]
},
"use_action": {
    "type": "holster", // Holster or draw a weapon
    "holster_prompt": "Holster item", // Prompt to use when selecting an item
    "holster_msg": "You holster your %s", // Message to show when holstering an item
    "max_volume": 6, // Maximum volume of each item that can be holstered
    "min_volume": 3,  // Minimum volume of each item that can be holstered or 1/3 max_volume if unspecified
    "max_weight": 2000, // Maximum weight of each item. If unspecified no weight limit is imposed
    "multi": 1, // Total number of items that holster can contain
    "draw_cost": 10, // Base move cost per unit volume when wielding the contained item
    "skills": ["pistol", "shotgun"], // Guns using any of these skills can be holstered
    "flags": ["SHEATH_KNIFE", "SHEATH_SWORD"] // Items with any of these flags set can be holstered
},
"use_action": {
    "type": "bandolier", // Store ammo and later reload using it
    "capacity": 10, // Total number of rounds that can be stored
    "ammo": [ "shot", "9mm" ], // What types of ammo can be stored?
},
"use_action": {
    "type": "reveal_map", // reveal specific terrains on the overmap
    "radius": 180, // radius around the player where things are revealed. A single overmap is 180x180 tiles.
    "terrain": ["hiway", "road"], // ids of overmap terrain types that should be revealed (as many as you want).
    "message": "You add roads and tourist attractions to your map." // Displayed after the revelation.
},
"use_action": {
    "type" : "heal",        // Heal damage, possibly some statuses
    "limb_power" : 10,      // How much hp to restore when healing limbs? Mandatory value
    "head_power" : 7,       // How much hp to restore when healing head? If unset, defaults to 0.8 * limb_power.
    "torso_power" : 15,     // How much hp to restore when healing torso? If unset, defaults to 1.5 * limb_power.
    "bleed" : 0.4,          // Chance to remove bleed effect.
    "bite" : 0.95           // Chance to remove bite effect.
    "infect" : 0.1          // Chance to remove infected effect.
    "move_cost" : 250       // Cost in moves to use the item.
    "long_action" : true,   // Is using this item a long action. Setting this to true will divide move cost by (first aid skill + 1).
    "limb_scaling" : 1.2,   // How much extra limb hp should be healed per first aid level. Defaults to 0.25 * limb_power.
    "head_scaling" : 1.0,   // How much extra limb hp should be healed per first aid level. Defaults to (limb_scaling / limb_power) * head_power.
    "torso_scaling" : 2.0,  // How much extra limb hp should be healed per first aid level. Defaults to (limb_scaling / limb_power) * torso_power.
    "effects" : [           // Effects to apply to patient on finished healing. Same syntax as in consume_drug effects.
        { "id" : "pkill1", "duration" : 120 }
    ],
    "used_up_item" : "rag_bloody" // Item produced on successful healing. If the healing item is a tool, it is turned into the new type. Otherwise a new item is produced.
},
"use_action": {
    "type": "place_trap", // places a trap
    "allow_underwater": false, // (optional) allow placing this trap when the player character is underwater
    "allow_under_player": false, // (optional) allow placing the trap on the same square as the player character (e.g. for benign traps)
    "needs_solid_neighbor": false, // (optional) trap must be placed between two solid tiles (e.g. for tripwire).
    "needs_neighbor_terrain": "t_tree", // (optional, default is empty) if non-empty: a terrain id, the trap must be placed adjacent to that terrain.
    "outer_layer_trap": "tr_blade", // (optional, default is empty) if non-empty: a trap id, makes the game place a 3x3 field of traps. The center trap is the one defined by "trap", the 8 surrounding traps are defined by this (e.g. tr_blade for blade traps).
    "bury_question": "", // (optional) if non-empty: a question that will be asked if the player character has a shoveling tool and the target location is diggable. It allows to place a buried trap. If the player answers the question (e.g. "Bury the X-trap?") with yes, the data from the "bury" object will be used.
    "bury": { // If the bury_question was answered with yes, data from this entry will be used instead of outer data.
         // This json object should contain "trap", "done_message", "practice" and (optional) "moves", with the same meaning as below.
    },
    "trap": "tr_engine", // The trap to place.
    "done_message": "Place the beartrap on the %s.", // The message that appears after the trap has been placed. %s is replaced with the terrain name of the place where the trap has been put.
    "practice": 4, // How much practice to the "traps" skill placing the trap gives.
    "moves": 10 // (optional, default is 100): the move points that are used by placing the trap.
}
```

###random Descriptions

Any item with a "snippet_category" entry will have random descriptions, based on that snippet category:
```
"snippet_category": "newspaper",
```
The item descriptions are taken from snippets, which can be specified like this (the value of category must match the snippet_category in the item definition):
```
{
    "type" : "snippet",
    "category" : "newspaper",
    "id" : "snippet-id",          // id is optional, it's used when the snippet is referenced in the item list of professions
    "text": "your flavor text"
}
```
or several snippets at once:
```
{
    "type" : "snippet",
    "category" : "newspaper",
    "text": [
        "your flavor text",
        "more flavor",
        // entries can also bo of this form to have a id to reference that specific snippet.
        { "id" : "snippet-id", "text" : "another flavor text" }
    ]
    "text": [ "your flavor text", "another flavor text", "more flavor" ]
}
```
Multiple snippets for the same category are possible and actually recommended. The game will select a random one for each item of that type.

One can also put the snippets directly in the item definition:
```
"snippet_category": [ "text 1", "text 2", "text 3" ],
```
This will automatically create a snippet category specific to that item and populate that category with the given snippets.
The format also support snippet ids like above.

# `json/` JSONs

### Harvest

```JSON
{
  "id": "mon_broken_cyborg",
  "type": "harvest",
  "message": "You search for any salvagable bionic hardware in what's left of this failed experiment",
  "entries": [
    { "drop": "bio_power_storage", "base_num": [ 0, 0 ], "scale_num": [ 0.05, 0.2 ], "max": 1 },
    { "drop": "burnt_out_bionic", "scale_num": [ 0.1, 0.3 ], "max": 1 },
    { "drop": "sinew", "base_num": [ 5, 15 ], "scale_num": [ 0.6, 0.9 ], "max": 20 },
    { "drop": "cable", "base_num": [ 1, 3 ], "scale_num": [ 0.2, 0.6 ], "max": 8 },
    { "drop": "bone_human", "base_num": [ 1, 2 ], "scale_num": [ 0.4, 0.7 ], "max": 10 },
    { "drop": "scrap", "base_num": [ 1, 5 ], "scale_num": [ 0.3, 0.7 ], "max": 12 }
  ]
}
```

#### `id`

Unique id of the harvest definition.

#### `type`

Should always be `harvest` to mark the object as a harvest definition.

#### `message`

Optional message to be printed when a creature using the harvest definition is butchered. May be omitted from definition.

#### `entries`

Array of dictionaries defining possible items produced on butchering and their likelihood of being produced. `drop` value should be the `id` string of the item to be produced. `base_num` value should be an array with two elements in which the first defines the minimum number of the corresponding item produced and the second defines the maximum number. `scale_num` value should be an array with two elements, increasing the minimum and maximum drop numbers respectively by element value * survival skill.

### Furniture

```JSON
{
    "type": "furniture",
    "id": "f_toilet",
    "name": "toilet",
    "symbol": "&",
    "looks_like": "chair",
    "color": "white",
    "move_cost_mod": 2,
    "required_str": 18,
    "flags": ["TRANSPARENT", "BASHABLE", "FLAMMABLE_HARD"],
    "crafting_pseudo_item": "anvil",
    "examine_action": "toilet",
    "close": "f_foo_closed",
    "open": "f_foo_open",
    "bash": "TODO",
    "deconstruct": "TODO",
    "max_volume": 4000
}
```

#### `type`

Fixed string, must be `furniture` to identify the JSON object as such.

`"id", "name", "symbol", "looks_like", "color", "bgcolor", "max_volume", "open", "close", "bash", "deconstruct", "examine_action", "flgs`

Same as for terrain, see below in the chapter "Common to furniture and terrain".

#### `move_cost_mod`

Movement cost modifier (`-10` = impassable, `0` = no change). This is added to the movecost of the underlying terrain.

#### `required_str`

Strength required to move the furniture around. Negative values indicate an unmovable furniture.

#### `crafting_pseudo_item`

(Optional) Id of an item (tool) that will be available for crafting when this furniture is range (the furniture acts as an item of that type).

### Terrain

```JSON
{
    "type": "terrain",
    "id": "t_spiked_pit",
    "name": "spiked pit",
    "symbol": "0",
    "looks_like": "pit",
    "color": "ltred",
    "move_cost": 10,
    "trap": "spike_pit",
    "max_volume": 4000,
    "flags": ["TRANSPARENT", "DIGGABLE"],
    "connects_to" : "WALL",
    "close": "t_foo_closed",
    "open": "t_foo_open",
    "bash": "TODO",
    "deconstruct": "TODO",
    "harvestable": "blueberries",
    "transforms_into": "t_tree_harvested",
    "harvest_season": "WINTER",
    "roof": "t_roof",
    "examine_action": "pit"
}
```

#### `type`

Fixed string, must be "terrain" to identify the JSON object as such.

`"id", "name", "symbol", "looks_like", "color", "bgcolor", "max_volume", "open", "close", "bash", "deconstruct", "examine_action", "flgs`

Same as for furniture, see below in the chapter "Common to furniture and terrain".

#### `move_cost`

Move cost to move through. A value of 0 means it's impassable (e.g. wall). You should not use negative values. The positive value is multiple of 50 move points, e.g. value 2 means the player uses 2*50 = 100 move points when moving across the terrain.

#### `trap`

(Optional) Id of the build-in trap of that terrain.

For example the terrain `t_pit` has the built-in trap `tr_pit`. Every tile in the game that has the terrain `t_pit` also has, therefore, an implicit trap `tr_pit` on it. The two are inseparable (the player can not deactivate the built-in trap, and changing the terrain will also deactivate the built-in trap).

A built-in trap prevents adding any other trap explicitly (by the player and through mapgen).

#### `harvestable`

(Optional) If defined, the terrain is harvestable. This entry defines the item type of the harvested fruits (or similar). To make this work, you also have to set one of the `harvest_*` `examine_action` functions.

#### `transforms_into`

(Optional) Used for various transformation of the terrain. If defined, it must be a valid terrain id. Used for example:

- When harvesting fruits (to change into the harvested form of the terrain).
- In combination with the `HARVESTED` flag and `harvest_season` to change the harvested terrain back into a terrain with fruits.

#### `harvest_season`

(Optional) On of "SUMMER", "AUTUMN", "WINTER", "SPRING", used in combination with the "HARVESTED" flag to revert the terrain back into a terrain that can be harvested.

#### `roof`

(Optional) The terrain of the terrain on top of this (the roof).

### Common To Furniture And Terrain

Some values can/must be set for terrain and furniture. They have the same meaning in each case.

#### `id`

Id of the object, this should be unique among all object of that type (all terrain or all furniture types). By convention (but technically not needed), the id should have the "f_" prefix for furniture and the "t_" prefix for terrain. This is not translated. It must not be changed later as that would break save compatibility.

#### `name`

Displayed name of the object. This will be translated.

#### `flags`

(Optional) Various additional flags, see "doc/JSON_FLAGS.md".

#### `connects_to`

(Optional) The group of terrains to which this terrain connects. This affects tile rotation and connections, and the ASCII symbol drawn by terrain with the flag "AUTO_WALL_SYMBOL".

Current values:
- `CHAINFENCE`
- `RAILING`
- `WALL`
- `WATER`
- `WOODFENCE`

Example: `-` , `|` , `X` and `Y` are terrain which share the same `connects_to` value. `O` does not have it. `X` and `Y` also have the `AUTO_WALL_SYMBOL` flag. `X` will be drawn as a T-intersection (connected to west, south and east), `Y` will be drawn as a horizontal line (going from west to east, no connection to south).

```
-X-    -Y-
 |      O
```

#### `symbol`

ASCII symbol of the object as it appears in the game. The symbol string must be exactly one character long. This can also be an array of 4 strings, which define the symbol during the different seasons. The first entry defines the symbol during spring. If it's not an array, the same symbol is used all year round.

#### `looks_like`

id of a similar item that this item looks like. The tileset loader will try to load the tile for that item if this item doesn't have a tile. looks_like entries are implicitly chained, so if 'throne' has looks_like 'big_chair' and 'big_chair' has looks_like 'chair', a throne will be displayed using the chair tile if tiles for throne and big_chair do not exist. If a tileset can't find a tile for any item in the looks_like chain, it will default to the ascii symbol.

#### `color` or `bgcolor`

Color of the object as it appears in the game. "color" defines the foreground color (no background color), "bgcolor" defines a solid background color. As with the "symbol" value, this can be an array with 4 entries, each entry being the color during the different seasons.

> **NOTE**: You must use ONLY ONE of "color" or "bgcolor"

#### `max_volume`

(Optional) Maximal volume that can be used to store items here.

#### `examine_action`

(Optional) The json function that is called when the object is examined. See "src/iexamine.h".

#### `close" And "open`

(Optional) The value should be a terrain id (inside a terrain entry) or a furniture id (in a furniture entry). If either is defined, the player can open / close the object. Opening / closing will change the object at the affected tile to the given one. For example one could have object "safe_c", which "open"s to "safe_o" and "safe_o" in turn "close"s to "safe_c". Here "safe_c" and "safe_o" are two different terrain (or furniture) types that have different properties.

#### `bash`

(Optional) Defines whether the object can be bashed and if so, what happens. See "map_bash_info".

#### `deconstruct`

(Optional) Defines whether the object can be deconstructed and if so, what the results shall be. See "map_deconstruct_info".

#### `map_bash_info`

Defines the various things that happen when the player or something else bashes terrain or furniture.

```JSON
{
    "str_min": 80,
    "str_max": 180,
    "str_min_blocked": 15,
    "str_max_blocked": 100,
    "str_min_supported": 15,
    "str_max_supported": 100,
    "sound": "crunch!",
    "sound_vol": 2,
    "sound_fail": "whack!",
    "sound_fail_vol": 2,
    "ter_set": "t_dirt",
    "furn_set": "f_rubble",
    "explosive": 1,
    "collapse_radius": 2,
    "destroy_only": true,
    "bash_below": true,
    "tent_centers": ["f_groundsheet", "f_fema_groundsheet", "f_skin_groundsheet"],
    "items": "bashed_item_result_group"
}
```

#### `str_min`, `str_max`, `str_min_blocked`, `str_max_blocked`, `str_min_supported`, `str_max_supported`

TODO

#### `sound`, `sound_fail`, `sound_vol`, `sound_fail_vol`
(Optional) Sound and volume of the sound that appears upon destroying the bashed object or upon unsuccessfully bashing it (failing). The sound strings are translated (and displayed to the player).

#### `furn_set`, `ter_set`

The terrain / furniture that will be set when the original is destroyed. This is mandatory for bash entries in terrain, but optional for entries in furniture (it defaults to no furniture).

#### `explosive`
(Optional) If greater than 0, destroying the object causes an explosion with this strength (see `game::explosion`).

#### `destroy_only`
TODO

#### `bash_below`
TODO

#### `tent_centers`, `collapse_radius`
(Optional) For furniture that is part of tents, this defines the id of the center part, which will be destroyed as well when other parts of the tent get bashed. The center is searched for in the given "collapse_radius" radius, it should match the size of the tent.

#### `items`

(Optional) An item group (inline) or an id of an item group, see doc/ITEM_SPAWN.md. The default subtype is "collection". Upon successful bashing, items from that group will be spawned.

#### `map_deconstruct_info`

```JSON
{
    "furn_set": "f_safe",
    "ter_set": "t_dirt",
    "items": "deconstructed_item_result_group"
}
```

#### `furn_set`, `ter_set`

The terrain / furniture that will be set after the original has been deconstructed. "furn_set" is optional (it defaults to no furniture), "ter_set" is only used upon "deconstruct" entries in terrain and is mandatory there.

### `items`

(Optional) An item group (inline) or an id of an item group, see doc/ITEM_SPAWN.md. The default subtype is "collection". Upon deconstruction the object, items from that group will be spawned.

# Scenarios

Scenarios are specified as JSON object with `type` member set to `scenario`.

```JSON
{
    "type": "scenario",
    "ident": "schools_out",
    ...
}
```

The ident member should be the unique id of the scenario.

The following properties (mandatory, except if noted otherwise) are supported:


## `description`
(string)

The in-game description.

## `name`
(string or object with members "male" and "female")

The in-game name, either one gender-neutral string, or an object with gender specific names. Example:
```JSON
"name": {
    "male": "Runaway groom",
    "female": "Runaway bride"
}
```

## `points`
(integer)

Point cost of scenario. Positive values cost points and negative values grant points.

## `items`
(optional, object with optional members "both", "male" and "female")

Items the player starts with when selecting this scenario. One can specify different items based on the gender of the character. Each lists of items should be an array of items ids. Ids may appear multiple times, in which case the item is created multiple times.

Example:
```JSON
"items": {
    "both": [
        "pants",
        "rock",
        "rock"
    ],
    "male": [ "briefs" ],
    "female": [ "panties" ]
}
```
This gives the player pants, two rocks and (depending on the gender) briefs or panties.

Mods can modify the lists of an existing scenario via "add:both" / "add:male" / "add:female" and "remove:both" / "remove:male" / "remove:female".

Example for mods:
```JSON
{
    "type": "scenario",
    "ident": "schools_out",
    "edit-mode": "modify",
    "items": {
        "remove:both": [ "rock" ],
        "add:female": [ "2x4" ]
    }
}
```

## `flags`
(optional, array of strings)

A list of flags. TODO: document those flags here.

Mods can modify this via "add:flags" and "remove:flags".

## `cbms`
(optional, array of strings)

A list of CBM ids that are implanted in the character.

Mods can modify this via "add:CBMs" and "remove:CBMs".

## `traits", "forced_traits", "forbidden_traits`
(optional, array of strings)

Lists of trait/mutation ids. Traits in "forbidden_traits" are forbidden and can't be selected during the character creation. Traits in "forced_traits" are automatically added to character. Traits in "traits" enables them to be chosen, even if they are not starting traits.

Mods can modify this via "add:traits" / "add:forced_traits" / "add:forbidden_traits" and "remove:traits" / "remove:forced_traits" / "remove:forbidden_traits".

## `allowed_locs`
(optional, array of strings)

A list of starting location ids (see start_locations.json) that can be chosen when using this scenario.

## `start_name`
(string)

The name that is shown for the starting location. This is useful if the scenario allows several starting locations, but the game can not list them all at once in the scenario description. Example: if the scenario allows to start somewhere in the wilderness, the starting locations would contain forest and fields, but its "start_name" may simply be "wilderness".

## `professions`
(optional, array of strings)

A list of allowed professions that can be chosen when using this scenario. The first entry is the default profession. If this is empty, all professions are allowed.

## `map_special`
(optional, string)

Add a map special to the starting location, see JSON_FLAGS for the possible specials.

# Starting locations

Starting locations are specified as JSON object with "type" member set to "start_location":
```JSON
{
    "type": "start_location",
    "ident": "field",
    "name": "An empty field",
    "target": "field",
    ...
}
```

The ident member should be the unique id of the location.

The following properties (mandatory, except if noted otherwise) are supported:

## `name`
(string)

The in-game name of the location.

## `target`
(string)

The id of an overmap terrain type (see overmap_terrain.json) of the starting location. The game will chose a random place with that terrain.

## `flags`
(optional, array of strings)

Arbitrary flags. Mods can modify this via "add:flags" / "remove:flags". TODO: document them.

### `tile_config`
Each tileset has a tile_config.json describing how to map the contents of a sprite sheet to various tile identifiers, different orientations, etc. The ordering of the overlays used for displaying mutations can be controlled as well. The ordering can be used to override the default ordering provided in `mutation_ordering.json`. Example:

```JSON
  {                                             // whole file is a single object
    "tile_info": [                              // tile_info is mandatory
      {
        "height": 32,
        "width": 32,
        "iso" : true,                             //  Optional. Indicates an isometric tileset. Defaults to false.
        "pixelscale" : 2                          //  Optional. Sets a multiplier for resizing a tileset. Defaults to 1.
      }
    ],
    "tiles-new": [                              // tiles-new is an array of sprite sheets
      {                                           //   alternately, just one "tiles" array
        "file": "tiles.png",                      // file containing sprites in a grid
        "tiles": [                                // array with one entry per tile
          {
            "id": "10mm",                         // id is how the game maps things to sprites
            "fg": 1,                              //   lack of prefix mostly indicates items
            "bg": 632,                            // fg and bg can be sprite indexes in the image
            "rotates": false
          },
          {
            "id": "t_wall",                       // "t_" indicates terrain
            "fg": [2918, 2919, 2918, 2919],       // 2 or 4 sprite numbers indicates pre-rotated
            "bg": 633,
            "rotates": true,
            "multitile": true,
            "additional_tiles": [                 // connected/combined versions of sprite
              {                                   //   or variations, see below
                "id": "center",
                "fg": [2919, 2918, 2919, 2918]
              },
              {
                "id": "corner",
                "fg": [2924, 2922, 2922, 2923]
              },
              {
                "id": "end_piece",
                "fg": [2918, 2919, 2918, 2919]
              },
              {
                "id": "t_connection",
                "fg": [2919, 2918, 2919, 2918]
              },
              {
                "id": "unconnected",
                "fg": 2235
              }
            ]
          },
          {
            "id": "vp_atomic_lamp",               // "vp_" vehicle part
            "fg": 3019,
            "bg": 632,
            "rotates": false,
            "multitile": true,
            "additional_tiles": [
              {
                "id": "broken",                   // variant sprite
                "fg": 3021
              }
            ]
          },
          {
            "id": "t_dirt",
            "rotates": false,
            "fg": [
              { "weight":50, "sprite":640},       // weighted random variants
              { "weight":1, "sprite":3620},
              { "weight":1, "sprite":3621},
              { "weight":1, "sprite":3622}
            ]
          }
        ]
      },
      {                                           // second entry in tiles-new
        "file": "moretiles.png",                  // another sprite sheet
        "tiles": [
          {
            "id": ["xxx","yyy"],                  // define two ids at once
            "fg": 1,
            "bg": 234
          }
        ]
      }
    ],
    "overlay_ordering": [
      {
        "id" : "WINGS_BAT",                         // mutation name, in a string or array of strings
        "order" : 1000                              // range from 0 - 9999, 9999 being the topmost layer
      },
      {
        "id" : [ "PLANTSKIN", "BARK" ],             // mutation name, in a string or array of strings
        "order" : 3500                              // order is applied to all items in the array
      }
    ]
  }
```

# Mutation overlay ordering

The file `mutation_ordering.json` defines the order that visual mutation overlays are rendered on a character ingame. The layering value from 0 (bottom) - 9999 (top) sets the order.

Example:
```JSON
[
    {
        "type" : "overlay_order",
        "overlay_ordering" :
        [
        {
            "id" : [ "BEAUTIFUL", "BEAUTIFUL2", "BEAUTIFUL3", "LARGE", "PRETTY", "RADIOACTIVE1", "RADIOACTIVE2", "RADIOACTIVE3", "REGEN" ],
            "order" : 1000
        },{
            "id" : [ "HOOVES", "ROOTS1", "ROOTS2", "ROOTS3", "TALONS" ],
            "order" : 4500
        },{
            "id" : "FLOWERS",
            "order" : 5000
        },{
            "id" : [ "PROF_CYBERCOP", "PROF_FED", "PROF_PD_DET", "PROF_POLICE", "PROF_SWAT", "PHEROMONE_INSECT" ],
            "order" : 8500
        }
        ]
    }
]
```

## `id`
(string)

The internal ID of the mutation. Can be provided as a single string, or an array of strings. The order value provided will be applied to all items in the array.

## `order`
(integer)

The ordering value of the mutation overlay. Values range from 0 - 9999, 9999 being the topmost drawn layer. Mutations that are not in any list will default to 9999.
