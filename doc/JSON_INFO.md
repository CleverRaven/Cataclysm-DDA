# JSON file contents

##raw
* bionics.json       - bionics, does NOT include bionic effects
* dreams.json        - dream text and linked mutation categories
* item_groups.json   - item spawn groups
* materials.json     - material types
* monstergroups.json - monster spawn groups
* monster_factions.json - monster factions
* names.json         - names used for NPC/player name generation
* professions.json   - profession definitions
* recipes.json       - crafting/disassembly recipes
* road_vehicles.json - vehicle spawn information for roads
* skills.json        - skill descriptions and ID's
* snippets.json      - flier/poster descriptions
* mutations.json     - traits/mutations
* vehicle_groups.json - vehicle spawn groups
* vehicle_parts.json - vehicle parts, does NOT affect flag effects

##raw/vehicles
groups of vehicle definitions with self-explanatory names of files:
* bikes.json
* cars.json
* carts.json
* emergency.json
* farm.json
* military.json
* trucks.json
* utility.json
* vans_busses.json
* vehicles.json

##raw/items
* archery.json       - bows and arrows
* ranged.json        - guns
* tools.json         - tools and items that can be (a)ctivated
* ammo.json          - ammo
* books.json         - books
* comestibles.json   - food/drinks
* containers.json    - containers
* instruments.json   - instruments
* melee.json         - anything that doesn't go in the other item jsons, melee weapons
* mods.json          - gunmods

##json
* furniture.json     - furniture, and features treated like furniture
* terrain.json       - terrain types and definitions

#raw jsons

###BIONICS
```C++
"id"           : "bio_batteries",  // Unique ID. Must be one continuous word,
                                   // use underscores if necessary.
"name"         : "Battery System", // In-game name displayed
"active"       : false,  // Whether the bionic is active or passive (default: passive)
"power_source" : false,  // Whether the bionic provides power (default: false)
"faulty"       : false,  // Whether it is a faulty type (default: false)
"cost"         : 0,  // How many PUs it costs to use the bionic. (default: 0)
"time"         : 0,  // How long, when activated, between drawing cost.
                     // If 0, it draws power once. (default: 0)
"description"  : "You have a battery draining attachment, and thus can make use of the energy contained in normal, everyday batteries.  Use 'E' to consume batteries." // In-game description
```
###DREAMS
```C++
"messages" : [                // List of potential dreams
    "You have a strange dream about birds.",
    "Your dreams give you a strange feathered feeling."
],
"category" : "MUTCAT_BIRD",   // Mutation category needed to dream
"strength" : 1                // Mutation category strength required
                                     1 = 20 - 34
                                     2 = 35 - 49
                                     3 = 50+
```
###ITEM GROUPS
Item groups have been expanded, look at doc/ITEM_SPAWN.md to their new description.
The syntax listed here is still valid.
```C++
"id":"forest",            // Unique ID. Must be one continuous word, use underscores if necessary
"items":[                 // List of potential item ID's. Chance of an item spawning is x/T, where
  ["rock", 40],           //    X is the value linked to the specific item and T is the total of all
  ["stick", 95],          //    item values in a group
  ["mushroom", 4],
  ["mushroom_poison", 3],
  ["mushroom_magic", 1],
  ["blueberries", 3]
],
"groups":[]               // ?
```
###MATERIALS
```C++
"ident"         : "hflesh",       // Unique ID. Must be one continuous word, use underscores if necessary
"name"          : "Human Flesh",  // In-game name displayed
"bash_resist"   :   1,            // How well a material resists bashing damage
"cut_resist"    :   1,            // How well a material resists cutting damage
"bash_dmg_verb" : "bruised",      // Verb used when material takes bashing damage
"cut_dmg_verb"  : "sliced",       // Verb used when material takes cutting damage
"dmg_adj"       : ["bruised", "mutilated", "badly mutilated", "thoroughly mutilated"], // Description added to damaged item in ascending severity
"acid_resist"   :   1,            // Ability of a material to resist acid
"elec_resist"   :   1,            // Ability of a material to resist electricity
"fire_resist"   :   0,            // Ability of a material to resist fire
"density"       :   5             // Density of a material
```
###MONSTER GROUPS
```C++
"name" : "GROUP_ANT",             // Unique ID. Must be one continuous word, use underscores if necessary
"default" : "mon_ant",            // Default monster, automatically fills in any remaining spawn chances
"monsters" : [                    // To choose monster spawned game creates 1000 entries and picks one.
  { "monster" : "mon_ant_larva", "freq" : 40, "multiplier" : 0 },   // Each monster will have a number of entries equal to it's "freq" and 
  { "monster" : "mon_ant_soldier", "freq" : 90, "multiplier" : 5 }, // the default monster will fill in the remaining. "multiplier" increases
  { "monster" : "mon_ant_queen", "freq" : 0, "multiplier" : 0 }     // how much each monster counts for in a spawn group (i.e. will spawn 5 larva or 1 soldier)
  { "monster" : "mon_thing",              // Monsters id
    "freq" : 100,                         // Chance of occurrence, out of a thousand
    "multiplier" : 0,                     // How many monsters each monster in this definition should count as, if spawning a limited number of monsters
    // The minimum and maximum number of monsters in this group that should spawn together. Optional, defaults [1,1]
    "pack_size" : [3,5],                    
    // Conditions limiting when monsters spawn. Valid options: SUMMER, WINTER, AUTUMN, SPRING, DAY, NIGHT, DUSK, DAWN
    // Multiple Time-of-day conditions (DAY, NIGHT, DUSK, DAWN) will be combined together so that any of those conditions makes the spawn valid
    // Multiple Season conditions (SUMMER, WINTER, AUTUMN, SPRING) will be combined together so that any of those conditions makes the spawn valid
    "conditions" : ["DUSK","DAWN","SUMMER"]
  }
```
###MONSTER FACTIONS
```C++
"name" : "cult",            // Unique ID. Must be one continuous word, use underscores when necessary
"base_faction" : "zombie",  // Optional base faction. Relations to other factions are inherited from it and relations of other factions to this one check this
"by_mood" : ["vermin"],     // Be hostile towards this faction when angry, neutral otherwise. Default attitude to all other factions    
"neutral" : ["nether"],     // Always be neutral towards this faction
"friendly" : ["blob"],      // Always be friendly towards this faction. By default a faction is friendly towards itself
###MONSTERS
```C++
"type" : "MONSTER",					// Should always be "MONSTER"
"id" : "mon_bat",					// Unique ID. Must be one continuous word, use underscores when necessary. Standard is to preface the ID with "mon"
"name" : "bat",						// Name displayed in-game
"species" : "MAMMAL",				// Monster species
"symbol" : "r",						// Symbol representing monster in-game
"color" : "brown",					// Color of symbol in-game
"size" : "TINY",					// Size flag, can be TINY, SMALL, MEDIUM, LARGE, or HUGE. See JSON_FLAGS.md for reference
"material" : "flesh",				// The material the monster is primarily composed of
"diff" : 4,							// Monster difficulty. Impacts the shade used to label the monster, and if it is above 30 a kill will be recorded in the memorial log. Some example values: (Zombie, 3) (Mi-go, 26) (Zombie Hulk, 50) 
"aggression" : -25,					// Defines how aggressive the monster is. Ranges from -99 (totally passive) to 100 (guaranteed hostility on detection)
"morale" : 5,						// Monster morale
"speed" : 230,						// Monster speed. 100 is the normal speed for a human being - higher values are faster and lower values are slower.
"melee_skill" : 4,					// Monster melee skill, ranges from 0 - 10, with 4 being an average mob. See GAME_BALANCE.txt for more examples
"melee_dice" : 1,					// Number of dice rolled on monster melee attack
"melee_dice_sides" : 1,				// Number of faces of dice rolled on monster melee attack
"melee_cut" : 1,					// Amount of cutting damage added to die roll on monster melee attack
"dodge" : 8,						// Monster dodge skill. See GAME_BALANCE.txt for an explanation of dodge mechanics
"armor_bash" : 0,					// Monster protection from bashing damage
"armor_cut" : 0,					// Monster protection from cutting damage
"vision_day" : 10,                  // Vision range in full daylight
"vision_night" : 1,                 // Vision range in total darkness
"luminance" : 0,					// Amount of light passively output by monster. Ranges from 0 to 10.
"hp" : 10,							// Monster hit points
"death_drops": "some-group-id",     // An item group that is used to spawn items when the monster dies. This can be an inlined item group, see ITEM_SPAWN.md. The default subtype is "distribution".
"special_freq" : 0,					// Number of turns required to "charge" a monster's special attack
"death_function" : "NORMAL",		// How the monster behaves on death. See JSON_FLAGS.md for a list of possible functions. Supports multiple death functions
"special_attack" : "BITE",			// Monster's special attack. See JSON_FLAGS.md for a list of possible special attacks. A monster can have only one special attack
"description": "One of the vesper bats, a family of winged insect-eating mammals. It roosts in caves and other hollows, and uses a form of echolocation to aerially navigate through tricky terrain at rapid speeds.",	
									// In-game description for the monster
"flags" : ["SEES", "HEARS", etc],	// Monster flags. See JSON_FLAGS.md for a full list
"fear_triggers" : ["SOUND", etc],	// What makes the monster afraid. See JSON_FLAGS.md for a full list
"anger_triggers" : ["PLAYER_CLOSE"],// What makes the monster angry. See JSON_FLAGS.md for a full list
"placate_triggers" : ["MEAT"],		// What calms the monster. See JSON_FLAGS.md for a full list
"revert_to_itype": "bot_turret",    // (optional) if not empty and a valid item id, the monster (usually a robot) can be converted into this item by the player (only when it's already friendly).
"categories" : ["WILDLIFE"]			// Monster categories. Can be NULL, CLASSIC (only mobs found in classic zombie movies) or WILDLIFE (natural animals). If they are not CLASSIC or WILDLIFE, they will not spawn in classic mode
```	
###NAMES
```C++
{ "name" : "Aaliyah", "gender" : "female", "usage" : "given" }, // Name, gender, "given"/"family"/"city" (first/last/city name).
// NOTE: Please refrain from adding name PR's in order to maintain kickstarter exclusivity
```
###PROFESSIONS
```C++
"description":"Ever since you were a child you loved hunting, and you loved the challenge of hunting with a bow even more. You start with a level in archery and survival.", // In-game description
"ident":"hunter",      // Unique ID. Must be one continuous word, use underscores if necessary
"items":[              // ID's of items player starts with when selecting this profession
 "army_top",
 "boots_steel",
 ["survnote", "snippet-id"],
                       // Entries can also be an array containing the item id and a snippet id.
                       // The id must match a snippet id from the snippet category that is
                       // used by that item type.
 "jeans"
],
"name":"Bow Hunter",   // In-game name displayed
"points":2,            // Point cost of profession. Positive values cost points and negative values grant points
"addictions" : [       // Optional list of starting addictions.
 {
	"type": "nicotine", // ID of addiction
	"intensity" : 10,  // Intensity of starting addiction
 }
"skills":[             // Skills that the player starts with when selecting this profession, stacks with purchased skills
 {
	"level":1,         // Skill level granted
	"name":"archery"   // ID of granted skill
 },
 {
	"level":1,
	"name":"survival"
 }
]
```
###RECIPES
```C++
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
###SKILLS
```C++
"ident" : "smg",  // Unique ID. Must be one continuous word, use underscores if necessary
"name" : "submachine guns",  // In-game name displayed
"description" : "Your skill with submachine guns and machine pistols. Halfway between a pistol and an assault rifle, these weapons fire and reload quickly, and may fire in bursts, but they are not very accurate.", // In-game description
"tags" : ["gun_type"]  // Special flags (default: none)
```
###TRAITS/MUTATIONS
```C++
"id": "LIGHTEATER",  // Unique ID
"name": "Optimist",  // In-game name displayed
"points": 2,         // Point cost of the trait. Positive values cost points and negative values give points
"visibility": 0,     // Visibility of the trait for purposes of NPC interaction (default: 0)
"ugliness": 0,       // Ugliness of the trait for purposes of NPC interaction (default: 0)
"initial_ma_styles": [ "style_crane" ], // (optional) A list of ids of martial art styles of which the player can choose one when starting a game.
"mixed_effect": false, // Wheather the trait has both positive and negative effects. This is purely declarative and is only used for the user interface. (default: false)
"description": "Nothing gets you down!" // In-game description
"starting_trait": true, // Can be selected at character creation (default: false)
"valid": false,      // Can be mutated ingame (default: true)
"category": ["MUTCAT_BIRD", "MUTCAT_INSECT"], // Categories containing this mutation
"prereqs": ["SKIN_ROUGH"], // Needs these mutations before you can mutate toward this mutation
"cancels": ["ROT1", "ROT2", "ROT3"], // Cancels these mutations when mutating
"changes_to": ["FASTHEALER2"], // Can change into these mutations when mutating further
"leads_to": [], // Mutations that add to this one
"wet_protection":[{ "part": "HEAD", // Wet Protection on specific bodyparts
                    "good": 1 } ] // "neutral/good/ignored" // Good increases pos and cancels neg, neut cancels neg, ignored cancels both
```
###VEHICLE GROUPS
```C++
"id":"city_parked",            // Unique ID. Must be one continuous word, use underscores if necessary
"vehicles":[                 // List of potential vehicle ID's. Chance of a vehicle spawning is X/T, where
  ["suv", 600],           //    X is the value linked to the specific vehicle and T is the total of all
  ["pickup", 400],          //    vehicle values in a group
  ["car", 4700],
  ["road_roller", 300]
]
```
###VEHICLE PARTS
```C++
"id": "wheel",                // Unique identifier
"name": "wheel",              // Displayed name
"symbol": "0",                // ASCII character displayed when part is working
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
]
```
###VEHICLE PLACEMENT
```C++
"id":"road_straight_wrecks",  // Unique ID. Must be one continuous word, use underscores if necessary
"locations":[ {               // List of potential vehicle locations. When this placement is used, one of those locations will be chosen at random.
  "x" : [0,19],               // The x placement. Can be a single value or a range of possibilities.
  "y" : 8,                    // The y placement. Can be a single value or a range of possibilities.
  "facing" : [90,270]         // The facing of the vehicle. Can be a single value or an array of possible values.
} ]
```
###VEHICLE SPAWN
```C++
"id":"default_city",            // Unique ID. Must be one continuous word, use underscores if necessary
"spawn_types":[ {       // List of spawntypes. When this vehicle_spawn is applied, it will choose from one of the spawntypes randomly, based on the weight.
  "description" : "Clear section of road",           //    A description of this spawntype
  "weight" : 33,          //    The chance of this spawn type being used.
  "vehicle_function" : ""jack-knifed_semi" // This is only needed if the spawntype uses a built-in c++ function.
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
###VEHICLES
```C++
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
#raw/items jsons

###AMMO
```C++
"type" : "AMMO",      // Defines this as ammo
"id" : "shot_bird",   // Unique ID. Must be one continuous word, use underscores if necessary
"price" : 500,        // Used when bartering with NPC's
"name" : "birdshot",  // In-game name displayed
"symbol" : "=",       // ASCII character used in-game
"color" : "red",      // ASCII character color
"description" : "Weak shotgun ammunition. Designed for hunting birds and other small game, its applications in combat are very limited.", // In-game description
"material" : "plastic", // Material types.  See materials.json for possible options
"volume" : 2,         // Volume, measured in 1/4 liters
"weight" : 34,        // Weight, measured in grams
"bashing" : 1,        // Bashing damage caused by using it as a melee weapon
"cutting" : 0,        // Cutting damage caused by using it as a melee weapon
"to_hit" : 0,         // To-hit bonus if using it as a melee weapon
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
###GENERIC ITEMS
```C++
"type" : "GENERIC",   // Defines this as some generic item
"id" : "socks",       // Unique ID. Must be one continuous word, use underscores if necessary
"name" : "socks",     // The name appearing in the examine box.  Can be more than one word separated by spaces
"weight" : 350,       // Weight of the item in grams
"color" : "blue",     // ASCII character color
"to_hit" : 0,         // To-hit bonus if using it as a melee weapon (whatever for?)
"symbol" : "[",       // ASCII character used in-game
"description" : "Socks. Put 'em on your feet.", // Description of the item
"price" : 100,        // Used when bartering with NPCs
"material" : ["COTTON"],    // Material types, can be as many as you want.  See materials.json for possible options
"volume" : 1,         // Volume, measured in 1/4 liters
"cutting" : 0,        // Cutting damage caused by using it as a melee weapon
"phase" : "solid",    // What phase it is
"bashing" : -5,       // Bashing damage caused by using it as a melee weapon
"flags" : ["VARSIZE"] // Indicates special effects, see JSON_FLAGS.md
```
###ARMOR
Armor can be define like this:
```C++
"type" : "ARMOR",     // Defines this as armor
...                   // same entries as above for the generic item.
                      // additional some armor specific entries:
"covers" : ["FEET"],  // Where it covers.  Possible options are TORSO, HEAD, EYES, MOUTH, ARMS, HANDS, LEGS, FEET
"storage" : 0,        // How many volume storage slots it adds
"warmth" : 10,        // How much warmth clothing provides
"environmental_protection" : 0,  // How much environmental protection it affords
"encumbrance" : 0,   // Base encumbrance (unfitted value)
"coverage" : 80,      // What percentage of body part
"material_thickness" : 1  // Thickness of material, in millimetre units (approximately).  Generally ranges between 1 - 5, more unusual armor types go up to 10 or more
"power_armor" : false, // If this is a power armor item (those are special).
```
Alternately, every item (book, tool, gun, even food) can be used as armor if it has armor_data:
```C++
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
###BOOKS
Books can be define like this:
```C++
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
```C++
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

####Color key
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

###COMESTIBLES
```C++
"type" : "COMESTIBLE", // Defines this as a COMESTIBLE
"id" : "crack",       // Unique ID. Must be one continuous word, use underscores if necessary
"name" : "crack",     // In-game name displayed
"weight" : 1,         // Weight, measured in grams
"color" : "white",    // ASCII character color
"addiction_type" : "crack", // Addiction type
"spoils_in" : 0,      // How long a comestible is good for. 0 = no spoilage
"use_action" : "CRACK", // What effects a comestible has when used, see special definitions below
"stim" : 40,          // Stimulant effect
"container" : "null", // What container stores this
"to_hit" : 0,         // To-hit bonus if using it as a melee weapon
"comestible_type" : "MED", // Comestible type, used for inventory sorting
"symbol" : "!",       // ASCII character used in-game
"quench" : 0,         // Thirst quenched
"heal" : -2,          // Health effects (used for sickness chances)
"addiction_potential" : 80, // Ability to cause addictions
"nutrition" : 0,      // Hunger satisfied
"description" : "Refined cocaine, incredibly addictive.", // In-game description
"price" : 420,        // Used when bartering with NPCs
"material" : "powder", // Material types.  See materials.json for possible options
"tool" : "apparatus", // Tool required to be eaten/drank
"volume" : 1,         // Volume, measured in 1/4 liters
"cutting" : 0,        // Cutting damage caused by using it as a melee weapon
"phase" : "solid",    // What phase it is
"charges" : 4,        // Number of uses when spawned
"stack_size" : 8,     // (Optional) How many uses are in the above-defined volume. If omitted, is the same as 'charges'
"bashing" : 0,        // Bashing damage caused by using it as a melee weapon
"fun" : 50,            // Morale effects when used
```
###CONTAINERS
```C++
"type": "CONTAINER",  // Defines this as a container
...                   // same data as for the generic item (see above).
"contains": 200,      // How much volume this container can hold
"seals": false,       // Can be resealed, this is a required for it to be used for liquids. (optional, default: false)
"watertight": false,  // Can hold liquids, this is a required for it to be used for liquids. (optional, default: false)
"rigid": false,       // Volume of the item does not include volume of the content. Without that flag the volume of the contents are added to the volume of the container. (optional, default: false)
"preserves": false,   // Contents do not spoil. (optional, default: false)
```
Alternately, every item can be used as container:
```C++
"type": "ARMOR",      // Any type is allowed here
...                   // same data as for the type
"container_data" : {  // The container specific data goes here.
    "contains": 200,
}
```
This defines a armor (you need to add all the armor specific entries), but makes it usable as container.
It could also be written as a generic item ("type": "GENERIC") with "armor_data" and "container_data" entries.
###MELEE
```C++
"id": "hatchet",      // Unique ID. Must be one continuous word, use underscores if necessary
"symbol": ";",        // ASCII character used in-game
"color": "light_gray", // ASCII character color
"name": "hatchet",    // In-game name displayed
"description": "A one-handed hatchet. Makes a great melee weapon, and is useful both for cutting wood, and for use as a hammer.", // In-game description
"price": 95,          // Used when bartering with NPCs
"material": ["iron", "wood"], // Material types.  See materials.json for possible options
"weight": 907,        // Weight, measured in grams
"volume": 6,          // Volume, measured in 1/4 liters
"bashing": 12,        // Bashing damage caused by using it as a melee weapon
"cutting": 12,        // Cutting damage caused by using it as a melee weapon
"flags" : ["CHOP"],   // Indicates special effects
"to_hit": 1           // To-hit bonus if using it as a melee weapon
```
###GUN
Guns can be define like this:
```C++
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
```C++
"type" : "TOOL",      // Or any other item type
...                   // same entries as for the type (e.g. same entries as for any tool),
"gun_data" : {       // additionally the same gun data like above
    "skill": ...,
    "recoil": ...,
    ...
}
```
###TOOLS
```C++
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
"charges_per_use": 1, // Charges consumed per tool use
"turns_per_charge": 20, // Charges consumed over time
"ammo": "NULL",       // Ammo type used for reloading
"revert_to": "torch_done", // Transforms into item when charges are expended
"use_action": "TORCH_LIT" // Action performed when tool is used, see special definition below
```
###SPAWN DATA
Every item type can have optional spawn data:
```
"spawn_data" : {
    "container": "can"  // The id of a container item, new item will be put into that container (optional, default: no container)
}
```

###SEED DATA
Every item type can have optional seed data, if the item has seed data, it's considered a seed and can be planted:
```
"seed_data" : {
    "fruits": "weed", // The item id of the fruits that this seed will produce.
    "seeds": false, // (optional, default is true). If true, harvesting the plant will spawn seeds (the same type as the item used to plant). If false only the fruits are spawned, no seeds.
    "byproducts": ["withered", "straw_pile"], // A list of further items that should spawn upon harvest.
    "plant_name": "sunflower", // The name of the plant that grows from this seed. This is only used as information displayed to the user.
    "grow" : 91 // Time it takes for a plant to fully mature. Based around a 91 day season length (roughly a real world season) to give better accuracy for longer season lengths
                // Note that growing time is later converted based upon the season_length option, basing it around 91 is just for accuracy purposes
                // A value 91 means 3 full seasons, a value of 30 would mean 1 season.
}
```

###ARTIFACT DATA
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

#### charge_type
(optional, default: "ARTC_NULL") How the item is recharged. For this to work, the item needs to be a tool that consumes charges upon invocation and has non-zero max_charges. Possible values (see src/artifact.h for an up-to-date list):
- "ARTC_NULL" Never recharges!
- "ARTC_TIME" Very slowly recharges with time
- "ARTC_SOLAR" Recharges in sunlight
- "ARTC_PAIN" Creates pain to recharge
- "ARTC_HP" Drains HP to recharge

#### effects_carried
(optional, default: empty list) Effects of the artifact when it's in the inventory (main inventory, wielded, or worn) of the player. Possible values (see src/enums.h for an up-to-date list):
- "AEP_STR_UP" Strength + 4
- "AEP_DEX_UP" Dexterity + 4
- "AEP_PER_UP" Perception + 4
- "AEP_INT_UP" Intelligence + 4
- "AEP_ALL_UP" All stats + 2
- "AEP_SPEED_UP" +20 speed
- "AEP_IODINE" Reduces radiation
- "AEP_SNAKES" Summons friendly snakes when you're hit
- "AEP_INVISIBLE" Makes you invisible
- "AEP_CLAIRVOYANCE" See through walls
- "AEP_SUPER_CLAIRVOYANCE" See through walls to a great distance
- "AEP_STEALTH" Your steps are quieted
- "AEP_EXTINGUISH" May extinguish nearby flames
- "AEP_GLOW" Four-tile light source
- "AEP_PSYSHIELD" Protection from stare attacks
- "AEP_RESIST_ELECTRICITY" Protection from electricity
- "AEP_CARRY_MORE" Increases carrying capacity by 200
- "AEP_SAP_LIFE" Killing non-zombie monsters may heal you
- "AEP_HUNGER" Increases hunger
- "AEP_THIRST" Increases thirst
- "AEP_SMOKE" Emits smoke occasionally
- "AEP_EVIL" Addiction to the power
- "AEP_SCHIZO" Mimicks schizophrenia
- "AEP_RADIOACTIVE" Increases your radiation
- "AEP_MUTAGENIC" Mutates you slowly
- "AEP_ATTENTION" Draws netherworld attention slowly
- "AEP_STR_DOWN" Strength - 3
- "AEP_DEX_DOWN" Dex - 3
- "AEP_PER_DOWN" Per - 3
- "AEP_INT_DOWN" Int - 3
- "AEP_ALL_DOWN" All stats - 2
- "AEP_SPEED_DOWN" -20 speed
- "AEP_FORCE_TELEPORT" Occasionally force a teleport
- "AEP_MOVEMENT_NOISE" Makes noise when you move
- "AEP_BAD_WEATHER" More likely to experience bad weather
- "AEP_SICK" Decreases health over time

#### effects_worn
(optional, default: empty list) Effects of the artifact when it's worn (it must be an armor item to be worn). Possible values are the same as for effects_carried.

#### effects_wielded
(optional, default: empty list) Effects of the artifact when it's wielded. Possible values are the same as for effects_carried.

#### effects_activated
(optional, default: empty list) Effects of the artifact when it's activated (which require it to have a `"use_action": "ARTIFACT"` and it must have a non-zero max_charges value). Possible values (see src/artifact.h for an up-to-date list):
- "AEA_STORM" Emits shock fields
- "AEA_FIREBALL" Targeted
- "AEA_ADRENALINE" Adrenaline rush
- "AEA_MAP" Maps the area around you
- "AEA_BLOOD" Shoots blood all over
- "AEA_FATIGUE" Creates interdimensional fatigue
- "AEA_ACIDBALL" Targeted acid
- "AEA_PULSE" Destroys adjacent terrain
- "AEA_HEAL" Heals minor damage
- "AEA_CONFUSED" Confuses all monsters in view
- "AEA_ENTRANCE" Chance to make nearby monsters friendly
- "AEA_BUGS" Chance to summon friendly insects
- "AEA_TELEPORT" Teleports you
- "AEA_LIGHT" Temporary light source
- "AEA_GROWTH" Grow plants, a la triffid queen
- "AEA_HURTALL" Hurts all monsters!
- "AEA_RADIATION" Spew radioactive gas
- "AEA_PAIN" Increases player pain
- "AEA_MUTATE" Chance of mutation
- "AEA_PARALYZE" You lose several turns
- "AEA_FIRESTORM" Spreads minor fire all around you
- "AEA_ATTENTION" Attention from sub-prime denizens
- "AEA_TELEGLOW" Teleglow disease
- "AEA_NOISE" Loud noise
- "AEA_SCREAM" Noise & morale penalty
- "AEA_DIM" Darkens the sky slowly
- "AEA_FLASH" Flashbang
- "AEA_VOMIT" User vomits
- "AEA_SHADOWS" Summon shadow creatures

###SOFTWARE DATA
Every item type can have software data, it does not have any behavior:
```
"software_data" : {
    "type": "USELESS", // unused
    "power" : 91 // unused
}
```

###USE ACTIONS
The contents of use_action fields can either be a string indicating a built-in function to call when the item is activated (defined in iuse.cpp), or one of several special definitions that invoke a more structured function.
```C++
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
    "type: : "auto_transform", // Like transform, but it transforms automatically when a condition is met.
    "when_underwater" : "The candle is extinguished.", // Message to display if the item goes underwater, also cause the item to transform when it goes underwater.
    "non_interactive_message" " "You can not deactivate the lightstrip.",  // Message to display if the player tries to activate the item, also prevents activation by player from working.
},
"use_action": {
    "type": "explosion", // An item that explodes when it runs out of charges.
    "sound_volume": 0, // Volume of a sound the item makes every turn.
    "sound_msg": "Tick.", // Message describing sound the item makes every turn.
    "no_deactivate_msg": "You've already pulled the %s's pin, try throwing it instead.", // Message to display if the player tries to activate the item, prevents activation from succeeding if defined.
    "explosion_power": 12, // Power of the resulting explosion.
    "explosion_shrapnel": 28, // Power of shrapnel produced by explosion.
    "explosion_fire" : 33, // Power of flames produced by explosion.
    "explosion_blast" : 22, // Power of blast from explosion.
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
    "type": "ups_based_armor", // Armor that can be activated and uses power from an UPS, needs additional C++ code to work
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
    "draw_speed": 10, // Base move cost per unit volume when wielding the contained item
    "skills": ["pistol", "shotgun"], // Guns using any of these skills can be holstered
    "flags": ["SHEATH_KNIFE", "SHEATH_SWORD"] // Items with any of these flags set can be holstered
},
"use_action": {
    "type": "reveal_map", // reveal specific terrains on the overmap
    "radius": 180, // radius around the player where things are revealed. A single overmap is 180x180 tiles.
    "terrain": ["hiway", "road"], // ids of overmap terrain types that should be revealed (as many as you want).
    "message": "You add roads and tourist attractions to your map." // Displayed after the revelation.
}
```
###Random descriptions
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

#json jsons

### FURNITURE

```JSON
{
    "type": "furniture",
    "id": "f_toilet",
    "name": "toilet",
    "symbol": "&",
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

#### "type"
Fixed string, must be "furniture" to identify the JSON object as such.

#### "id", "name", "symbol", "color", "bgcolor", "max_volume", "open", "close", "bash", "deconstruct", "examine_action", "flags"
Same as for terrain, see below in the chapter "Common to furniture and terrain".

#### "move_cost_mod"
Movement cost modifier (-10 = impassable, 0 = no change). This is added to the movecost of the underlying terrain.

#### "required_str"
Strength required to move the furniture around. Negative values indicate an unmovable furniture.

#### "crafting_pseudo_item"
(Optional) Id of an item (tool) that will be available for crafting when this furniture is range (the furniture acts as an item of that type).

###TERRAIN

```JSON
{
    "type": "terrain",
    "id": "t_spiked_pit",
    "name": "spiked pit",
    "symbol": "0",
    "color": "ltred",
    "move_cost": 10,
    "trap": "spike_pit",
    "max_volume": 4000,
    "flags": ["TRANSPARENT", "DIGGABLE"],
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

#### "type"
Fixed string, must be "terrain" to identify the JSON object as such.

#### "id", "name", "symbol", "color", "bgcolor", "max_volume", "open", "close", "bash", "deconstruct", "examine_action", "flags"
Same as for furniture, see below in the chapter "Common to furniture and terrain".

#### "move_cost"
Move cost to move through. A value of 0 means it's impassable (e.g. wall). You should not use negative values. The positive value is multiple of 50 move points, e.g. value 2 means the player uses 2*50 = 100 move points when moving across the terrain.

#### "trap"
(Optional) Id of the build-in trap of that terrain. For example the terrain "t_pit" has the build in trap "tr_pit". Every tile in the game that has the terrain "t_pit" has therefor also an implicit trap "tr_pit" on it. Both are inseparable (the player can not deactivate the build-in trap, but changing the terrain will also deactivate the built-in trap).

A built-in trap prevents adding any other trap explicitly (by the player nor through mapgen).

#### "harvestable"
(Optional) If defined, the terrain is harvestable. This entry defines the item type of the harvested fruits (or similar). To make this work, you also have to set one of the "harvest_*" examine_action functions.

#### "transforms_into"
(Optional) Used for various transformation of the terrain. If defined, it must be a valid terrain id. Used for example:
- When harvesting fruits (to change into the harvested form of the terrain).
- In combination with the "HARVESTED" flag and "harvest_season" to change the harvested terrain back into a terrain with fruits.

#### "harvest_season"
(Optional) On of "SUMMER", "AUTUMN", "WINTER", "SPRING", used in combination with the "HARVESTED" flag to revert the terrain back into a terrain that can be harvested.

#### "roof"
(Optional) The terrain of the terrain on top of this (the roof).

### Common to furniture and terrain
Some values can/must be set for terrain and furniture. They have the same meaning in each case.

#### "id"
Id of the object, this should be unique among all object of that type (all terrain or all furniture types). By convention (but technically not needed), the id should have the "f_" prefix for furniture and the "t_" prefix for terrain. This is not translated. It must not be changed later as that would break save compatibility.

#### "name"
Displayed name of the object. This will be translated.

#### "flags"
(Optional) Various additional flags, see "doc/JSON_FLAGS.md".

#### "symbol"
ASCII symbol of the object as it appears in the game. The symbol string must be exactly one character long. This can also be an array of 4 strings, which define the symbol during the different seasons. The first entry defines the symbol during spring. If it's not an array, the same symbol is used all year round.

#### "color" or "bgcolor"
Color of the object as it appears in the game. "color" defines the the foreground color (no background color), "bgcolor" defines a solid background color. You must use EXACTLY ONE of "color" or "bgcolor". As with the "symbol" value, this can be an array with 4 entries, each entry being the color during the different seasons.

#### "max_volume"
(Optional) Maximal volume that can be used to store items here.

#### "examine_action"
(Optional) The C++ function that is called when the object is examined. See "src/iexamine.h".

#### "close" and "open"
(Optional) The value should be a terrain id (inside a terrain entry) or a furniture id (in a furniture entry). If either is defined, the player can open / close the object. Opening / closing will change the object at the affected tile to the given one. For example one could have object "safe_c", which "open"s to "safe_o" and "safe_o" in turn "close"s to "safe_c". Here "safe_c" and "safe_o" are two different terrain (or furniture) types that have different properties.

#### "bash"
(Optional) Defines whether the object can be bashed and if so, what happens. See "map_bash_info".

#### "deconstruct"
(Optional) Defines whether the object can be deconstructed and if so, what the results shall be. See "map_deconstruct_info".

### map_bash_info
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

#### "str_min", "str_max", "str_min_blocked", "str_max_blocked", "str_min_supported", "str_max_supported"
TODO

#### "sound", "sound_fail", "sound_vol", "sound_fail_vol"
(Optional) Sound and volume of the sound that appears upon destroying the bashed object or upon unsuccessfully bashing it (failing). The sound strings are translated (and displayed to the player).

#### "furn_set", "ter_set"
The terrain / furniture that will be set when the original is destroyed. This is mandatory for bash entries in terrain, but optional for entries in furniture (it defaults to no furniture).

#### "explosive"
(Optional) If greater than 0, destroying the object causes an explosion with this strength (see `game::explosion`).

#### "destroy_only"
TODO

#### "bash_below"
TODO

#### "tent_centers", "collapse_radius"
(Optional) For furniture that is part of tents, this defines the id of the center part, which will be destroyed as well when other parts of the tent get bashed. The center is searched for in the given "collapse_radius" radius, it should match the size of the tent.

#### "items"
(Optional) An item group (inline) or an id of an item group, see doc/ITEM_SPAWN.md. The default subtype is "collection". Upon successful bashing, items from that group will be spawned.

### map_deconstruct_info

```JSON
{
    "furn_set": "f_safe",
    "ter_set": "t_dirt",
    "items": "deconstructed_item_result_group"
}
```

#### "furn_set", "ter_set"
The terrain / furniture that will be set after the original has been deconstructed. "furn_set" is optional (it defaults to no furniture), "ter_set" is only used upon "deconstruct" entries in terrain and is mandatory there.

#### "items"
(Optional) An item group (inline) or an id of an item group, see doc/ITEM_SPAWN.md. The default subtype is "collection". Upon deconstruction the object, items from that group will be spawned.

###SCENARIO

```JSON
{
    "type": "scenario",              // Must always be "scenario"
    "ident": "schools_out",          // Internal ident of the scenario, must be unique
    "name": "School",                // The displayed name, can be gender specific:
    "name": {
        "male": "...",
        "female": "..."
    },
    "points" : -1,                   // How much points it costs (or if negative, how much it gives)
    "description": "...",            // Some description of the scenario, can be anything you like, can also be gender specific:
        "scen_desc_male": "...",     // Gender specific descriptions are optional.
        "scen_desc_female": "...",
    "allowed_locs" : ["school"],     // Starting locations (see start_locations.json) that can be chosen when using this scenario.
    "professions" : ["student"],     // Professions (see professions.json) that can be chosen when using this scenario. The first entry is the default profession.
    "start_name": "School",          // TODO
    "items": {                       // Additional starting items, see professions for the format.
        "both": [ ... ],
        "female": [ ... ],
        "male": [ ... ]
    },
    "flags": [ "some flag" ],        // Flags, see JSON_FLAGS.
    "traits": [ "PROF_MED" ],        // Allowed starting traits (see mutations.json). The player still has to choose them if they like, only useful if the listed trait can actually be chosen at game start.
    "forced_traits": [ "PROF_MED" ], // Traits that are automatically added to the new character.
    "forbidden_traits": [ "PROF_MED" ], // Traits that can explicitly not be chosen at start.
    "map_special": "mx_helicopter",  // (optional) Add a map special to the starting location, see JSON_FLAGS for the possible specials.
}
```
