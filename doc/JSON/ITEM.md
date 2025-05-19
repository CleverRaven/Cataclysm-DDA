- [Generic Items](#generic-items)
   * [To hit object](#to-hit-object)
- [Ammo](#ammo)
- [Ammo Effects](#ammo-effects)
- [Magazine](#magazine)
- [Armor](#armor)
   * [Armor Portion Data](#armor-portion-data)
      + [Encumbrance](#encumbrance)
      + [Encumbrance_modifiers](#encumbrance_modifiers)
      + [breathability](#breathability)
      + [Layers](#layers)
      + [rigid_layer_only](#rigid_layer_only)
      + [Coverage](#coverage)
      + [Covers](#covers)
      + [Specifically Covers](#specifically-covers)
      + [Part Materials](#part-materials)
      + [Armor Data](#armor-data)
   * [Guidelines for thickness: ####](#guidelines-for-thickness-)
   * [Armor inheritance](#armor-inheritance)
- [Pet Armor](#pet-armor)
- [Books](#books)
   * [Conditional Naming](#conditional-naming)
   * [Color Key](#color-key)
   * [CBMs](#cbms)
- [Comestibles](#comestibles)
- [Containers](#containers)
- [Melee](#melee)
- [Memory Cards](#memory-cards)
- [Gun](#gun)
- [Gunmod](#gunmod)
- [Batteries](#batteries)
- [Tools](#tools)
- [Seed](#seeds)
- [Brewables](#brewables)
   * [`Effects_carried`](#effects_carried)
   * [`effects_worn`](#effects_worn)
   * [`effects_wielded`](#effects_wielded)
   * [`effects_activated`](#effects_activated)
- [Compostables](#compostables)
- [Milling](#milling)
- [Use Actions](#use-actions)
- [Drop Actions](#drop-actions)
- [Tick Actions](#tick-actions)
   * [Delayed Item Actions](#delayed-item-actions)

Items are any entity in the game you can pick up. They all share a set of JSON fields, but to help preserve game memory some of their JSON fields are split into multiple `subtypes`:
- TOOL - can have tool qualities and use energy
- ARMOR - can define protection against incoming damage
- GUN - shoots projectiles of any kind
- GUNMOD - is installed on a gun, if it has required slot
- AMMO - consumed by another tools or guns
- MAGAZINE - stores AMMO to be used in GUN, TOOL, GUNMOD or TOOLMOD; in this case item should have not a pocket of type MAGAZINE, but pocket of type MAGAZINE_WELL
- COMESTIBLE - can be eaten and can rot
- BOOK - can be read
- PET_ARMOR - defines pet armor
- BIONIC_ITEM - item is a CBM, if installed, you get the bionic effect with the same id as the item name
- TOOLMOD - this is a toolmod, and can be installed to modify tool in some way (mainly battery slot changes)
- ENGINE - defines the properties of a vehicle engine if installed in a transport
- WHEEL - defines the properties of a vehicle wheel if installed in a transport
- SEED - is a plant seed
- BREWABLE - can be brewed
- COMPOSTABLE - can be composted
- MILLING - can be milled

With the exception of PET_ARMOR and ARMOR, any of the subtypes can be combined to load the respective subtype's field, demonstrated in the examples below. A subtype can be included without any of its associated fields defined to indicate that the item is e.g. a "BOOK" to the corresponding C++ function `is_book()`.

### Generic Items

These fields can be read by any ITEM regardless of subtypes:

```C++
"type": "ITEM",                   // Defines this as some generic item; no subtypes defined
"id": "socks",                    // Unique ID. Must be one continuous word, use underscores if necessary
"name": {
    "ctxt": "clothing",           // Optional translation context. Useful when a string has multiple meanings that need to be translated differently in other languages.
    "str": "pair of socks",       // The name appearing in the examine box.  Can be more than one word separated by spaces
    "str_pl": "pairs of socks"    // Optional. If a name has an irregular plural form (i.e. cannot be formed by simply appending "s" to the singular form), then this should be specified. "str_pl" may also be needed if the unit test cannot determine if the correct plural form can be formed by simply appending "s". "str_sp" should be used instead of "str" or "str_pl" if the singular and plural forms are the same.
},
"conditional_names": [ {          // Optional list of names that will be applied in specified conditions (see Conditional Naming section for more details).
    "type": "COMPONENT_ID",       // The condition type.
    "condition": "leather",       // The condition to check for.
    "name": { "str": "pair of leather socks", "str_pl": "pairs of leather socks" } // Name field, same rules as above.
} ],
"container": "null",             // What container (if any) this item should spawn within
"repairs_like": "scarf",          // If this item does not have recipe, what item to look for a recipe for when repairing it.
"color": "blue",                 // Color of the item symbol.
"symbol": "[",                   // The item symbol as it appears on the map. Must be a Unicode string exactly 1 console cell width.
"looks_like": "sheet_cotton",              // hint to tilesets if this item has no tile, use the looks_like tile
"description": "Socks. Put 'em on your feet.", // Description of the item
"snippet_category": "snippet_category",        // Can be used instead of description, if author want to have multiple ways to describe an item. See #Snippets
"ascii_picture": "ascii_socks", // Id of the asci_art used for this item
"phase": "solid",                            // (Optional, default = "solid") What phase it is
"weight": "350 g",                           // Weight, weight in grams, mg and kg can be used - "50 mg", "5 g" or "5 kg". For stackable items (ammo, comestibles) this is the weight per charge.
"volume": "250 ml",                          // Volume, volume in ml and L can be used - "50 ml" or "2 L". For stackable items (ammo, comestibles) this is the volume of stack_size charges.
"integral_volume": "50 ml",                        // Volume added to base item when item is integrated into another (eg. a gunmod integrated to a gun). Volume in ml and L can be used - "50 ml" or "2 L". Default is the same as volume.
"integral_weight": "50 g",                        // Weight added to base item when item is integrated into another (eg. a gunmod integrated to a gun). Default is the same as weight.
"longest_side": "15 cm",                     // Length of longest item dimension. Default is cube root of volume.
"rigid": false,                              // For non-rigid items volume (and for worn items encumbrance) increases proportional to contents
"insulation": 1,                             // (Optional, default = 1) If container or vehicle part, how much insulation should it provide to the contents
"price": "1 USD",                                // Barter value of an item. Used as barter value only when price_postapoc isn't defined. For stackable items (ammo, comestibles) this is the price for stack_size charges. Can use string "cent", "cents", "dollar", "dollars", "USD", or "kUSD".
"price_postapoc": "1 USD",                       // Barter value of an item post-Cataclysm. Can use string "cent", "cents", "dollar", "dollars", "USD", or "kUSD".
"stackable": true,                           // This item can be stacked together, similarly to `charges`
"degradation_multiplier": 0.8,               // Controls how quickly an item degrades when taking damage. 0 = no degradation. Defaults to 1.0.
"solar_efficiency": 0.3,                     // Efficiency of solar energy conversion for solarpacks; require SOLARPACK_ON to generate electricity; default 0
"source_monster": "mon_zombie",               // This item is corpse of this monster (so it has weight and volume of this monster), and revive into this monster; require COPRSE flag
"thrown_damage": [ { "damage_type": "bash", "amount": 15 } ], // Damage, that would be dealt when you throw this item; lack of this field fall back to use melee damage, including player's str bonus applied to melee attack
"material": [                                // Material types, can be as many as you want.  See materials.json for possible options
  { "type": "cotton", "portion": 9 },        // type indicates the material's ID, portion indicates proportionally how much of the item is composed of that material
  { "type": "plastic" }                      // portion can be omitted and will default to 1. In this case, the item is 90% cotton and 10% plastic.
],
"repairs_with": [ "plastic" ],               // Material types that this item can be repaired with. Defaults to all the item materials.
"weapon_category": [ "WEAPON_CAT1" ],        // (Optional) Weapon categories this item is in for martial arts.
"melee_damage": {                            // (Optional) Damage caused by using it as a melee weapon.  These values cannot be negative.
  "bash": 0,
  "cut": 0
},
"to_hit" {                                   // (Optional) To hit bonus values, omit if item isn't suited to be used as a melee weapon, see [GAME_BALANCE.md](/doc/design-balance-lore/GAME_BALANCE.md#to-hit-value) for individual value breakdowns.
  "grip": "solid",                           // the item's grip value
  "length": "long",                          // the item's length value
  "surface": "point",                        // the item's striking surface value
  "balance": "neutral"                       // the item's balance value
},
"to_hit": 1,                                 // (Optional, legacy, default = -2) To-hit bonus if using it as a melee weapon.
"variant_type": "gun"      // Possible options: "gun", "generic" - controls which options enable/disable seeing the variants of this item.
"variants": [              // Cosmetic variants this item can have
  {
    "id": "variant_a",                           // id used in spawning to spawn this variant specifically
    "name": { "str": "Variant A" },             // The name used instead of the default name when this variant is selected
    "description": "A fancy variant A",         // The description used instead of the default when this variant is selected
    "ascii_picture": "valid_ascii_art_id",      // An ASCII art picture used when this variant is selected. If there is none, the default (if it exists) is used.
    "symbol": "/",                              // Valid unicode character to replace the item symbol. If not specified, no change will be made.
    "color": "red",                             // Replacement color of item symbol. If not specified, no change will be made.
    "weight": 2,                                // The relative chance of this variant being selected over other variants when this item is spawned with no explicit variant. Defaults to 1. If it is 0, this variant will not be selected
    "append": true,                             // If this description should just be appended to the base item description instead of completely overwriting it.
    "expand_snippets": true                     // Allows to use snippet tags, see #Snippets
  }
],
"flags": ["VARSIZE"],                        // Indicates special effects, see JSON_FLAGS.md
"environmental_protection_with_filter": 6,   // the resistance to environmental effects if an item (for example a gas mask) requires a filter to operate and this filter is installed. Used in combination with use_action 'GASMASK' and 'DIVE_TANK'
"magazine_well": 0,                          // Volume above which the magazine starts to protrude from the item and add extra volume
"magazines": [                               // Magazines types for each ammo type (if any) which can be used to reload this item
    [ "9mm", [ "glockmag" ] ]                // The first magazine specified for each ammo type is the default
    [ "45", [ "m1911mag", "m1911bigmag" ] ],
],
"milling": {                                 // Optional. If given, the item can be milled in a water/wind mill.
  "into": "flour",                           // The item id of the result of the milling.
  "recipe": "paste_nut_mill_10_1"            // Reference to the recipe that performs the task. The syntax is <product name>_mill_<source amount>_<product amount>. The recipe is then defined as a normal recipe for the source with the product as its result and an id_suffix of "mill_X_Y". 
                                             // See data/json/recipes/food/milling.json for such recipes. Can also use "milling": { "into": "null", "recipe": "" } to override milling from a copied base item.
},
"explode_in_fire": true,                     // Should the item explode if set on fire
"nanofab_template_group": "nanofab_recipes", // This item is nanofabricator recipe, and point to itemgroup with items, that it could possibly contain; require nanofab_template_group
"template_requirements": "nanofabricator",   // `requirement`, that needed to craft any of this templates; used as "one full requirememt per 250 ml of item's volume" - item with volume 750 ml would require three times of `requirement`, item of 2L - eight times of `requirement`
"explosion": {                               // Physical explosion data
    "power": 10,                             // Measure of explosion power in grams of TNT equivalent explosive, affects damage and range.
    "distance_factor": 0.9,                  // How much power is retained per traveled tile of explosion. Must be lower than 1 and higher than 0.
    "max_noise": 25,                         // Maximum amount of (auditory) noise the explosion might produce.
    "fire": true,                            // Should the explosion leave fire
    "shrapnel": 200,                         // Total mass of casing, rest of fragmentation variables set to reasonable defaults.
    "shrapnel": {
        "casing_mass": 200,                  // Total mass of casing, casing/power ratio determines fragment velocity.
        "fragment_mass": 0.05,               // Mass of each fragment in grams. Large fragments hit harder, small fragments hit more often.
        "recovery": 10,                      // Percentage chance to drop an item at landing point.
        "drop": "nail"                       // Which item to drop at landing point.
    }
},
```

#### To hit object
For additional clarity, an item's `to_hit` bonus can be encoded as string of 4 fields.  All the fields are mandatory:

```jsonc
"to_hit": {
    "grip": "weapon",      // one of "bad", "none", "solid", or "weapon"
    "length": "hand",      // one of "hand", "short", or "long"
    "surface": "any",      // one of "point", "line", "any", or "every"
    "balance": "neutral"   // one of "clumsy", "uneven", "neutral", or "good"
}
```
See [GAME_BALANCE.md](/doc/design-balance-lore/GAME_BALANCE.md#to-hit-value)

### Ammo

```jsonc
{
  "id": "223",            // ID of the ammo
  "type": "ITEM",
  "subtypes": [ "AMMO" ], // Allows the below AMMO fields to be read in addition to generic ITEM fields
  "ammo_type": "shot",    // Determines where the items can be loaded in. Requires a proper `"ammunition_type"` to be declared (see below). In this case, the `223` rounds can be loaded into magazines that accept `shot`-type ammo
  "damage": {             // Ranged damage when fired
    "damage_type": "bullet",  // Type of the damage that would be dealt
    "amount": 39,             // Amount of damage to be dealt
    "armor_penetration": 2,   // Flat armor penetration
    "barrels": [              // Replaces the `amount` when weapon has barrel length defined, allow to change the damage of the single round depending on the barrel length.
      { "barrel_length": "28 mm", "amount": 13 },  // If weapon has barrel lengh this or less, this amount of the damage would be applied
      { "barrel_length": "30 mm", "amount": 14 },
      { "barrel_length": "35 mm", "amount": 15 },
      { "barrel_length": "39 mm", "amount": 16 }
    ]
  },
  "range": 5,             // Range when fired
  "recovery_chance": 6,   // Percentage of chance to recover the ammo after firing
  "dispersion": 0,        // Inaccuracy of ammo, measured in 100ths of Minutes Of Angle (MOA)
  "shot_counter": 5,      // Increases amount of shots produced by gun by this amount. `"shot_counter": 5` means each shot will be counted as 6 shots (1 you actually perform + 5). Designed for using in suppressor mod breakage and for stuff like replaceable barrels, but not used anywhere at this moment
  "projectile_count": 5,  // Amount of pellets, that the ammo will shot, like in shotgun-like weapon. If used, `"shot_damage"` should be specified
  "shot_damage": { "damage_type": "bullet", "amount": 15 },  // (Optional) Specifies the damage caused by a single projectile fired from this round. If present, projectile_count must also be specified. Syntax is the same as `"damage"`
  "critical_multiplier": 4,  // If the hit is a critical hit, all ranged damage dealt will be multiplied by this
  "shot_spread": 100,     // (Optional) Specifies the additional dispersion of single projectiles. Only meaningful if shot_count is present.
  "recoil": 18,           // Recoil caused when firing. Roughly set to the projectile's muzzle energy in J, the same value the ammo's damage is derived from. See also the gun's min_cycle_recoil value
  "count": 25,            // Number of rounds that spawn together
  "stack_size": 50,       // (Optional) How many rounds are in the above-defined volume. If omitted, is the same as 'count'
  "show_stats": true,     // (Optional) Force stat display for combat ammo. (for projectiles lacking both damage and prop_damage)
  "loudness": 10,         // (Optional) Modifier that can increase or decrease base gun's noise when firing. If loudness value is not specified, then game calculates it automatically from ammo's range, damage, and armor penetration.
  "casing": "223_casing", // Casing of the ammo that would be left after shooting
  "effects": ["COOKOFF", "SHOT"]  // Ammo effcts, see below
},
```

### Ammo Effects

ammo_effects define what effect the projectile, that you shoot, would have. List of existing ammo effects, **including hardcoded one**, can be found at `data/json/ammo_effects.json`

```jsonc
{
  "id": "AE_NULL",           // id of an effect
  "type": "ammo_effect",     // define it is an ammo effect
  "aoe": {                   // this field would be spawned at the tile projectile hit
    "field_type": "fd_fog",  // field, that would be spawned around the center of projectile; default "fd_null"
    "intensity_min": 1,      // min intensity of the field; default 0
    "intensity_max": 3,      // max intensity of the field; default 0
    "radius": 5,             // radius of a field to spawn; default 1
    "radius_z": 1,           // radius across z-level; default 0
    "chance": 100,           // probability to spawn 1 unit of field, from 0 to 100; default 100
    "size": 0,               // seems to be the threshold, where autoturret stops shooting the weapon to prevent friendly fire;; default 0
    "check_passable": false, // if false, projectile is able to penetrate impassable terrains, if penetration is defined (like walls and windows); if true, projectile can't penetrate even the sheet of glass; default false
    "check_sees": false,     // if false, field can be spawned behind the opaque wall (for example, behind the concrete wall); if true, it can't; default false
    "check_sees_radius": 0   // if "check_sees" is true, and this value is smaller than "radius", this value is used as radius instead. The purpose nor reasoning is unknown, probably some legacy of mininuke, so just don't use it; default 0
  },
  "trail": {                 // this field would be spawned across whole projectile path
    "field_type": "fd_fog",  // field, that would be spawned; defautl "fd_null"
    "intensity_min": 1,      // min intensity of the field; default 0
    "intensity_max": 3,      // max intensity of the field; default 0
    "chance": 100            // probability to spawn 1 unit of field, from 0 to 100; default 100
  },
  "explosion": {             // explosion, that will happen at the tile that projectile hit
    "power": 0,              // mandatory; power of the explosion, in grams of tnt; pipebomb is about 300, grenade (without shrapnel) is 240
    "distance_factor": 0.8,  // how fast the explosion decay, closer to 1 mean lesser "power" loss per tile, 0.8 means 20% of power loss per tile; default 0.75, value should be bigger than 0, but lesser than 1
    "fire": false,           // explosion create a fire, related to it's power, distance and distance_factor
    "shrapnel": {            // explosion create a shrapnel, that deal the damage, calculated by gurney equasion
      "casing_mass": 0,      // total mass of casing, casing/power ratio determines fragment velocity.
      "fragment_mass": 0.0,  // Mass of each fragment in grams. Large fragments hit harder, small fragments hit more often.
      "recovery": 0,         // Percentage chance to drop an item at landing point.
      "drop": "null"         // Which item to drop at landing point
    }
  },
  "do_flashbang": false,     // Creates a one tile radius EMP explosion at the hit location; default false
  "do_emp_blast": false,     // Creates a hardcoded flashbang explosion; default false
  "foamcrete_build": false,  // Creates foamcrete fields and walls on the hit location, used in aftershock; default false
  "eoc": [ "EOC_CAUSE_PAIN", "EOC_CAUSE_VOMIT" ], // Runs EoC when hit the target. See EFFECT_ON_CONDITION.md#typical-alpha-and-beta-talkers-by-cases for more information
  "spell_data": { "id": "bear_trap" }, // Spell, that would be casted when projectile hits an enemy
  "spell_data": { "id": "release_the_deltas", "hit_self": true, "min_level": 10 }, // another example
  "always_cast_spell ": false // if spell_data is used, and this is true, spell would be casted even if projectile did not deal any damage. Default false.
}
```

### Magazine

```C++
"type": "ITEM",
"subtypes": [ "MAGAZINE" ],      // Allows the below MAGAZINE fields to be read in addition to generic ITEM fields
"ammo_type": [ "40", "357sig" ], // What types of ammo this magazine can be loaded with
"capacity" : 15,                 // Capacity of magazine (in equivalent units to ammo charges)
"count" : 0,                     // Default amount of ammo contained by a magazine (set this for ammo belts)
"default_ammo": "556",           // If specified override the default ammo (optionally set this for ammo belts)
"reload_time" : 100,             // How long it takes to load each unit of ammo into the magazine
"mag_jam_mult": 1.25,            // Multiplier for gun mechanincal malfunctioning from magazine, mostly when it's damaged; Values lesser than 1 reflect better quality of the magazine, that jam less; bigger than 1 result in gun being more prone to malfunction and jam at lesser damage level; zero mag_jam_mult (and zero gun_jam_mult in a gun) would remove any chance for a gun to malfunction. Only works if gun has any fault from gun_mechanical_simple group presented; Jam chances are described in Character::handle_gun_damage(); at this moment it is roughly: 0.027% for undamaged magazine, 5% for 1 damage (|\), 24% for 2 damage (|.), 96% for 3 damage (\.), and 250% for 4 damage (XX), then this and gun values are summed up. Rule of thumb: helical mags should have 3, drum mags should have 2, the rest can be tweaked case by case, but mostly doesn't worth emulating it
"linkage" : "ammolink"           // If set one linkage (of given type) is dropped for each unit of ammo consumed (set for disintegrating ammo belts)
```


### Armor

Armor can be defined like this:

```C++
"type": "ITEM",
"subtypes": [ "ARMOR" ],            // Allows the below ARMOR fields to be read in addition to generic ITEM fields
"covers" : [ "foot_l", "foot_r" ],  // Where it covers.  Use bodypart_id defined in body_parts.json
"warmth" : 10,                      //  (Optional, default = 0) How much warmth clothing provides
"environmental_protection" : 0,     //  (Optional, default = 0) How much environmental protection it affords
"encumbrance" : 0,                  // Base encumbrance (unfitted value)
"max_encumbrance" : 0,              // When a character is completely full of volume, the encumbrance of a non-rigid storage container will be set to this. Otherwise it'll be between the encumbrance and max_encumbrance following the equation: encumbrance + (max_encumbrance - encumbrance) * non-rigid volume / non-rigid capacity.  By default, max_encumbrance is encumbrance + (non-rigid volume / 250ml).
"sided": true,                      // (Optional, default false) If true, this is a sided armor. Sided armor is armor that even though it describes covering, both legs, both arms, both hands, etc. actually only covers one "side" at a time but can be moved back and forth between sides at will by the player.
"coverage": 80,                     // What percentage of body part is covered (in general)
"cover_melee": 60,                  // What percentage of body part is covered (against melee)
"cover_ranged": 45,                 // What percentage of body part is covered (against ranged)
"cover_vitals": 10,                 // What percentage of critical hit damage is mitigated
"material_thickness" : 1,           // Thickness of material, in millimeter units (approximately).  Ordinary clothes range from 0.1 to 0.5. Particularly rugged cloth may reach as high as 1-2mm, and armor or protective equipment can range as high as 10 or rarely more.
"power_armor" : false,              // If this is a power armor item (those are special).
"max_worn": 10,                     // How many of these items can be worn.  Defaults to 2.
"energy_shield_max_hp" : false,     // Determines the inital value for the `ENERGY_SHIELD_HP` and `ENERGY_SHIELD_MAX_HP` item variables used by energy shields. The field has no effect for armor pieces without the `ENERGY_SHIELD` flag.
"non_functional" : "destroyed",     //this is the itype_id of an item that this turns into when destroyed. Currently only works for ablative armor.
"damage_verb": "makes a crunch, something has shifted", // if an item uses non-functional this will be the description when it turns into its non functional variant.
"valid_mods" : ["steel_padded"],    // List of valid clothing mods. Note that if the clothing mod doesn't have "restricted" listed, this isn't needed.
"armor": [ /* ... */ ]
```

#### Armor Portion Data
Encumbrance and coverage can be defined on a piece of armor as such:

```jsonc
"armor": [
  {
    "encumbrance": [ 2, 8 ],
    "breathability": "AVERAGE",
    "layers": [ "SKINTIGHT" ],
    "rigid_layer_only": true,
    "coverage": 95,
    "cover_melee": 95,
    "cover_ranged": 50,
    "cover_vitals": 5,
    "covers": [ "torso" ],
    "specifically_covers": [ "torso_upper", "torso_neck", "torso_lower" ],
    "material": [
      { "type": "cotton", "covered_by_mat": 100, "thickness": 0.2 },
      { "type": "plastic", "covered_by_mat": 15, "thickness": 0.8, "ignore_sheet_thickness": "true" }
    ]
  },
  {
    "encumbrance": 2,
    "volume_encumber_modifier": 0.5,
    "coverage": 80,
    "cover_melee": 80,
    "cover_ranged": 70,
    "cover_vitals": 5,
    "activity_noise": { "volume": 8, "chance": 60 },
    "covers": [ "arm_r", "arm_l" ],
    "specifically_covers": [ "arm_shoulder_r", "arm_shoulder_l" ],
    "material": [
      { "type": "cotton", "covered_by_mat": 100, "thickness": 0.2 }
    ]
  }
]
```

##### Encumbrance
(integer, or array of 2 integers)
The value of this field (or, if it is an array, the first value in the array) is the base encumbrance (unfitted) of this item.
When specified as an array, the second value is the max encumbrance - when the pockets of this armor are completely full of items, the encumbrance of a non-rigid item will be set to this. Otherwise it'll be between the first value and the second value following this the equation: first value + (second value - first value) * non-rigid volume / non-rigid capacity.  By default, the max encumbrance is the encumbrance + (non-rigid volume / 250ml).
`volume_encumber_modifier` is the more modern way to do it is to set a scaling factor on the armor itself. This is much easier to read and quickly parse (not requiring mental math) and is a direct scaling on that 250ml constant. So if I set a "volume_encumber_modifier" of .25 it means that it's one additional encumbrance per 1000ml (250ml/.25).

##### Encumbrance_modifiers
Experimental feature for having an items encumbrance be generated by weight instead of a fixed number. Takes an array of "DESCRIPTORS" described in the code. If you don't need any descriptors put "NONE". This overrides encumbrance putting it as well will make it be ignored. Currently only works for head armor.

##### breathability

Overrides the armor breathability, which is driven by armor material. Can be `IMPERMEABLE` (0%), `POOR` (30%), `AVERAGE` (50%), `GOOD` (80%), `MOISTURE_WICKING` (110%), `SECOND_SKIN` (140%)

##### Layers

What layer this piece of armor occupy. Can be `PERSONAL`, `SKINTIGHT`, `NORMAL`, `WAIST`, `OUTER`, `BELTED`, `AURA`, see [ARMOR_BALANCE_AND_DESIGN.md#layers](/doc/design-balance-lore/ARMOR_BALANCE_AND_DESIGN.md#layers) for details

##### rigid_layer_only

If true, this armor portion is rigid, and conflict with another rigid clothes on the same layers.

##### Coverage
(integer)
What percentage of time this piece of armor will be hit (and thus used as armor) when an attack hits the body parts in `covers`.

`cover_melee` and `cover_ranged` represent the percentage of time this piece of armor will be hit by melee and ranged attacks respectively. Usually these would be the same as `coverage`.

`cover_vitals` represents the percentage of critical hit damage is absorbed. Only the excess damage on top of normal damage is mitigated, so a vital coverage value of 100 means that critical hits would do the same amount as normal hits.

##### Covers
(array of strings)
What body parts this section of the armor covers. See the bodypart_ids defined in body_parts.json for valid values.

##### Specifically Covers
(array of strings)
What sub body parts this section of the armor covers. See the sub_bodypart_ids defined in body_parts.json for valid values.
These are used for wearing multiple armor pieces on a single layer without gaining encumbrance penalties. They are not mandatory
if you don't specify them it is assumed that the section covers all the body parts it covers entirely.
strapped layer items, and outer layer armor should always have these specified otherwise it will conflict with other pieces.

##### Part Materials
(array of objects)
The type, coverage and thickness of the materials that make up this portion of the armor.
- `type` indicates the material ID.
- `covered_by_mat` (_optional_) indicates how much (%) of this armor portion is covered by said material. Defaults to 100.
- `thickness` (_optional_) indicates the thickness of said material for this armor portion. Defaults to 0.0.
The portion coverage and thickness determine how much the material contributes towards the armor's resistances.
**NOTE:** These material definitions do not replace the standard `"material"` tag. Instead they provide more granularity for controlling different armor resistances.
- `ignore_sheet_thickness` (_optional, default false_) materials that come in a specific thickness, if you don't use a multiple of the allowed thickness the game throws an error

`covered_by_mat` should not be confused with `coverage`. When specifying `covered_by_mat`, treat it like the `portion` field using percentage instead of a ratio value. For example:

```jsonc
"armor": [
  {
    "covers" : [ "arm_r", "arm_l" ],
    "material": [
      {
        "type": "cotton",
        "covered_by_mat": 100,
        "thickness": 0.2
      },
      {
        "type": "plastic",
        "covered_by_mat": 15,
        "thickness": 0.6
      }
    ],
    // ...
  }
]
```
The case above describes a portion of armor that covers the arms. This portion is 100% covered by cotton, so a hit to the arm part of the armor will definitely impact the cotton. That portion is also 15% covered by plastic. This means that during damage absorption, the cotton material contributes 100% of its damage absorption, while the plastic material only contributes 15% of its damage absorption. Damage absorption is also affected by `thickness`, so thickness and material cover both provide positive effects for protection.

#### Guidelines for thickness: ####
According to <https://propercloth.com/reference/fabric-thickness-weight/>, dress shirts and similar fine clothing range from 0.15mm to 0.35mm.
According to <https://leathersupreme.com/leather-hide-thickness-in-leather-jackets/>:
* Fashion leather clothes such as thin leather jackets, skirts, and thin vests are 1.0mm or less.
* Heavy leather clothes such as motorcycle suits average 1.5mm.

From [this site](https://cci.one/site/marine/design-tips-fabrication-overview/tables-of-weights-and-measures/), an equivalency guideline for fabric weight to mm:

| Cloth                         | oz/yd2 | g/m2  | Inches | mm   |
| -----                         | ------ | ----- | ------ | ---- |
| Fiberglass (plain weave)      |    2.3 |    78 |  0.004 | 0.10 |
| Fiberglass (plain weave)      |    6.0 |   203 |  0.007 | 0.17 |
| Kevlar (TM) (plain weave)     |    5.0 |   170 |  0.010 | 0.25 |
| Carbon Fiber (plain weave)    |    5.8 |   197 |  0.009 | 0.23 |
| Carbon Fiber (unidirectional) |    9.0 |   305 |  0.011 | 0.28 |

Chart cobbled together from several sources for more general materials:

| Fabric     | oz/yd2  | Max g/m2   | Inches      | mm to use  |
| ---------- | ------- | ---------- | ----------- | ---------- |
| Very light |     0-4 |        136 | 0.006-0.007 |       0.15 |
| Light      |     4-7 |        237 |       0.008 |        0.2 |
| Medium     |    7-11 |        373 | 0.009-0.011 |       0.25 |
| Heavy      |   11-14 |        475 | 0.012-0.014 |        0.3 |

Shoe thicknesses are outlined at <https://secretcobbler.com/choosing-leather/>; TL;DR: upper 1.2 - 2.0mm, lining 0.8 - 1.2mm, for a total of 2.0 - 3.2mm.

For turnout gear, see <https://web.archive.org/web/20220331215535/http://bolivar.mo.us/media/uploads/2014/09/2014-06-bid-fire-gear-packet.pdf>.

#### Armor inheritance

Inheritance of one armor using another allows to copy all its values, including layers, thicknesses, and another. Additionally, the `replace_materials` could be used to replace one material in parent armor to another material in child node, in format `"replace_materials": { "material_1": "material_2" }`. For example: 
```jsonc
{
  "id": "skeleton_plate",
  "type": "TOOL_ARMOR",
  "copy-from": "armor_lc_plate",
  "name": { "str": "skeletal plate" },
  "description": "A full body multilayered suit of skeletal plate armor.",
  "replace_materials": { "lc_steel": "bone", "lc_steel_chain": "flesh" },
  "extend": { "flags": [ "INTEGRATED", "UNBREAKABLE", "NO_SALVAGE" ] },
  "proportional": { "encumbrance": 0.3 }
}
```
In this json, the item inherited all stats of `armor_lc_plate`, but replaced `lc_steel` material with `bone`, and `lc_steel_chain` with `flesh`. Multiple additional flags were added using `extend`, and encumbrance was `proportional`ly decreased to 30% of original.

### Pet Armor
Pet armor can be defined like this:

```C++
"type": "ITEM",
"subtypes": [ "PET_ARMOR" ], // Allows the below PET_ARMOR fields to be read in addition to generic ITEM fields
"environmental_protection" : 0,  //  (Optional, default = 0) How much environmental protection it affords
"material_thickness" : 1,  // Thickness of material, in millimeter units (approximately).  Generally ranges between 1 - 5, more unusual armor types go up to 10 or more
"pet_bodytype":        // the body type of the pet that this monster will fit. See MONSTERS.md
"max_pet_vol":          // the maximum volume of the pet that will fit into this armor. Volume in ml or L can be used - "50 ml" or "2 L".
"min_pet_vol":          // the minimum volume of the pet that will fit into this armor. Volume in ml or L can be used - "50 ml" or "2 L".
"power_armor" : false,  // If this is a power armor item (those are special).
```

### Books

Books can be defined like this:

```C++
"type": "ITEM",
"subtypes": [ "BOOK" ], // Allows the below BOOK fields to be read in addition to generic ITEM fields
"max_level" : 5,      // Maximum skill level this book will train to
"intelligence" : 11,  // Intelligence required to read this book without penalty
"time" : "35 m",      // Time a single read session takes. An integer will be read in minutes or a time string can be used.
"read_fun" : -2,           // Morale bonus/penalty for reading
"read_skill" : "computer", // Skill raised
"chapters" : 4,       // Number of chapters (for fun only books), each reading "consumes" a chapter. Books with no chapters left are less fun (because the content is already known to the character).
"generic": false,     // This book counts chapters by item instance instead of by type (this book represents a generic variety of books, like "book of essays")
"required_level" : 2, // Minimum skill level required to learn
"martial_art": "style_mma", // Martial art learned from this book; incompatible with `skill`
"proficiencies": [    // Having this book mitigate lack of proficiency, required for crafting 
  { 
    "proficiency": "prof_fermenting", // id of proficiency
    "time_factor": 0.1,               // slowdown for using this book proficiency - slowdown from lack of proficiency is multiplied on this value, so for `0.75`, if recipe adds 10 hours for lack of proficiency,  with book it would be [ 10 * ( 1 - 0.75 ) = ] 2.5 hours; multiple books stacks, but in logarithmic way, meaning having more books of the same proficiency is better than having one book, but never would be better than learning the proficiency
    "fail_factor": 0.25               // works same as `time_factor`
  },
  { "proficiency": "prof_brewing", "time_factor": 0.25, "fail_factor": 0.5 },
  { "proficiency": "prof_winemaking", "time_factor": 0.1, "fail_factor": 0.25 }
],

```
It is possible to omit the `max_level` field if the book you're creating contains only recipes and it's not supposed to level up any skill. In this case the `skill` field will just refer to the skill required to learn the recipes.

Since many book names are proper names, it's often necessary to explicitly specify
the plural forms. The following is the game's convention on plural names of books:

1. For non-periodical books (textbooks, manuals, spellbooks, etc.),
    1. If the book's singular name is a proper name, then the plural name is `copies of (singular name)`. For example, the plural name of `Lessons for the Novice Bowhunter` is `copies of Lessons for the Novice Bowhunter`.
    2. Otherwise, the plural name is the usual plural of the singular name. For example, the plural name of `tactical baton defense manual` is `tactical baton defense manuals`
2. For periodicals (magazines and journals),
    1. If the periodical's singular name is a proper name, and doesn't end with "Magazine", "Weekly", "Monthly", etc., the plural name is `issues of (singular name)`. For example, the plural name of `Archery for Kids` is `issues of Archery for Kids`.
    2. Otherwise, the periodical's plural name is the usual plural of the singular name. For example, the plural name of `Crafty Crafter's Quarterly` is `Crafty Crafter's Quarterlies`.
3. For board games (represented internally as book items),
    1. If the board game's singular name is a proper name, the plural is `sets of (singular name)`. For example, the plural name of `Picturesque` is `sets of Picturesque`.
    2. Otherwise the plural name is the usual plural. For example, the plural of `deck of cards` is `decks of cards`.

#### Conditional Naming

The `conditional_names` field allows defining alternate names for items that will be displayed instead of (or in addition to) the default name, when specific conditions are met. Take the following (incomplete) definition for `sausage` as an example of the syntax:

```jsonc
{
  "name": "sausage",
  "conditional_names": [
    {
      "type": "FLAG",
      "condition": "CANNIBALISM",
      "name": "Mannwurst"
    },
    {
      "type": "COMPONENT_ID_SUBSTRING",
      "condition": "mutant",
      "name": { "str_sp": "sinister %s" }
    },
    {
      "type": "COMPONENT_ID",
      "condition": "mutant",
      "name": { "str_sp": "sinister %s" }
    },
    {
      "type": "VAR",
      "condition": "DISPLAY_NAME_MORALE",
      "name": { "str_sp": "%s (morale)" },
      "value" : "true"
    },
    {
      "type": "SNIPPET_ID",
      "condition": "test",
      "value":"one",
      "name": { "str_sp": "Report 1" } }
  ]
}
```

You can list as many conditional names for a given item as you want. Each conditional name must consist of 3 elements:
1. The condition type:
    - `COMPONENT_ID_SUBSTRING` searches all the components of the item (and all of *their* components, and so on) for an item with the condition string in their ID. The ID only needs to *contain* the condition, not match it perfectly (though it is case sensitive). For example, supplying a condition `mutant` would match `mutant_meat`.
    - `COMPONENT_ID` Similar to `COMPONENT_ID_SUBSTRING`, but search the exact component match
    - `FLAG` which checks if an item has the specified flag (exact match).
    - `VITAMIN` which checks if an item has the specified vitamin (exact match).
    - `VAR` which checks if an item has a variable with the given name (exact match) and value = `value`.
    - `SNIPPET_ID`which checks if an item has a snippet id variable set by an effect_on_condition with the given name (exact match) and snippets id = `value`.
2. The condition you want to look for.
3. The name to use if a match is found. Follows all the rules of a standard `name` field, with valid keys being `str`, `str_pl`, and `ctxt`. You may use %s here, which will be replaced by the name of the item. Conditional names defined prior to this one are taken into account.

So, in the above example, if the sausage is made from mutant humanoid meat, and therefore both has the `CANNIBALISM` flag, *and* has a component with `mutant` in its ID:
1. First, the item name is entirely replaced with "Mannwurst" if singular, or "Mannwursts" if plural.
2. Next, it is replaced by "sinister %s", but %s is replaced with the name as it was before this step, resulting in "sinister Mannwurst" or "sinister Mannwursts".

NB: If `"str": "sinister %s"` was specified instead of `"str_sp": "sinister %s"`, the plural form would be automatically created as "sinister %ss", which would become "sinister Mannwurstss" which is of course one S too far. Rule of thumb: If you are using %s in the name, always specify an identical plural form unless you know exactly what you're doing!


#### Color Key

When adding a new book, please use this color key:

* Magazines: `pink`
* “Paperbacks” Short enjoyment books (including novels): `light_cyan`
* “Hardbacks” Long enjoyment books (including novels): `light_blue`
* “Small textbook” Beginner level textbooks, guides and martial arts books: `green`
* “Large textbook” Advanced level textbooks and advanced guides: `blue`
* Religious books: `dark_gray`
* “Printouts” (including spiral-bound, binders, and similar) Technical documents, (technical?) protocols, (lab) journals, personal diaries: `light_green`
* Other reading material/non-books (use only if every other category does not apply): `light_gray`

A few exceptions to this color key may apply, for example for books that don’t are what they seem to be.
Never use `yellow` and `red`, those colors are reserved for sounds and infrared vision.

#### CBMs

CBMs can be defined like this:

```jsonc
"type" : "BIONIC_ITEM",         // Defines this as a CBM
// ...                          // same entries as above for the generic item.
                                // additional some CBM specific entries:
"bionic_id" : "bio_advreactor", // ID of the installed bionic if not equivalent to "id"
"difficulty" : 11,              // Difficulty of installing CBM
"is_upgrade" : true,            // Whether the CBM is an upgrade of another bionic.
"installation_data" : "AID_bio_advreactor" // ID of the item which allows for almost guaranteed installation of corresponding bionic.
```

### Comestibles

```C++
"type": "ITEM",
"subtypes": [ "COMESTIBLE" ], // Allows the below COMESTIBLE fields to be read in addition to generic ITEM fields
"spoils_in" : 0,            // A time duration: how long a comestible is good for. 0 = no spoilage.
"use_action" : [ "CRACK" ], // What effects a comestible has when used, see special definitions below
"stim" : 40,                // Stimulant effect
"sleepiness_mod": 3,        // How much sleepiness this comestible removes. (Negative values add sleepiness)
"comestible_type" : "MED",  // Comestible type, used for inventory sorting. One of 'FOOD', 'DRINK', 'MED', or 'INVALID' (consider using a different "type" than COMESTIBLE instead of using INVALID)
"consumption_effect_on_conditions" : [ "EOC_1" ],  // Effect on conditions to run after consuming.  Inline or string id supported
"quench" : 0,               // Thirst quenched
"healthy" : -2,             // Health effects (used for sickness chances)
"addiction_potential" : 80, // Default strength for this item to cause addictions
"addiction_type" : [ "crack", { "addiction": "cocaine", "potential": 5 } ], // Addiction types (if no potential is given, the "addiction_potential" field is used to determine the strength of that addiction)
"monotony_penalty" : 0,     // (Optional, default: 2 or 0 depending on material) Fun is reduced by this number for each one you've consumed in the last 48 hours.
                            // Comestibles with any material of junk food (id: "junk") default to 0. All other comestibles default to 2 when unspecified.
                            // Can't drop fun below 0, unless the comestible also has the "NEGATIVE_MONOTONY_OK" flag.
"calories" : 0,             // Hunger satisfied (in kcal)
"tool" : "apparatus",       // Tool required to be eaten/drank
"charges" : 4,              // Number of uses when spawned
"stack_size" : 8,           // (Optional) How many uses are in the above-defined volume. If omitted, is the same as 'charges'
"fun" : 50                  // Morale effects when used
"freezing_point": 32,       // (Optional) Temperature in C at which item freezes, default is water (32F/0C)
"cooks_like": "meat_cooked",         // (Optional) If the item is used in a recipe, replaces it with its cooks_like
"parasites": 10,            // (Optional) one_in(x) chance of becoming parasitized when eating
"contamination": [ { "disease": "bad_food", "probability": 5 } ],         // (Optional) List of diseases carried by this comestible and their associated probability. Values must be in the [0, 100] range.
"vitamins": [ [ "calcium", "60 mg" ], [ "iron", 12 ] ],         // Vitamins provided by consuming a charge (portion) of this.  Some vitamins ("calcium", "iron", "vitC") can be specified with the weight of the vitamins in that food.  Vitamins specified by weight can be in grams ("g"), milligrams ("mg") or micrograms ("μg", "ug", "mcg").  If a vitamin is not specified by weight, it is specified in "units", with meaning according to the vitamin definition.  Nutrition vitamins ("calcium", "iron", "vitC") are an integer percentage of ideal daily value average.  Vitamins array keys include the following: calcium, iron, vitC, mutant_toxin, bad_food, blood, and redcells.
"material": [                     // All materials (IDs) this food is made of
  { "type": "flesh", "portion": 3 }, // See Generic Item attributes for type and portion details
  { "type": "wheat", "portion": 5 }
],
"primary_material": "meat",       // Overwrites generic item "material" field. Materials determine specific heat.
"rot_spawn": {                    // Defines what creature would be spawned when this item rots away. Primarily used for eggs
  "group": "GROUP_EGG_CHICKEN",   // id of monster group that would be spawned. Cannot be used with "monster"
  "monster": "mon_moose_calf",    // id of a monster that would be spawned. Cannot be used with "group"
  "amount": 2,                    // if "monster" is used, defines how many instances of this monster will be spawned; 
  "amount": [ 1, 3 ],             // also can be an array, then the game will roll the number between two values
  "chance": 70                    // chance for the monster to spawn from a single item
}
"smoking_result": "dry_meat",     // Food that results from drying this food in a smoker
"petfood": [ "FUNGALFRUIT", "MIGOFOOD" ] // (Optional) Pet food categories this item is in.
```


### Containers

Any Item can be a container. To add the ability to contain things to an item, you need to add pocket_data. The below example is a typical container (shown with optional default values, or mandatory if the value is mandatory)

```jsonc
"pocket_data": [
  {
    "pocket_type": "CONTAINER",             // Typical container pocket. Pockets can also be MAGAZINE.
    "max_contains_volume": /* mandatory */, // Maximum volume this pocket can hold, totaled among all contained items.  For example "2 L" or "2000 ml" would hold two liters of items.
    "max_contains_weight": /* mandatory */, // Maximum weight this pocket can hold, totaled among all container items.  For example "6 kg" is about enough to contain a bowling ball.
    "min_item_volume": "0 ml",              // Minimum volume of item that can be placed into this pocket.  Items smaller than this cannot be placed in the pocket.
    "max_item_volume": "0 ml",              // Maximum volume of item that can fit through the opening into this pocket.  For example, a 2-liter bottle has a "17 ml" opening.
    "max_item_length": "0 mm",              // Maximum length of items that can fit in this pocket, by their longest_side.  Default is the diagonal opening length assuming volume is a cube (cube_root(vol)*square_root(2))
    "min_item_length": "0 mm",              // Minimum length of the item that can fit int this pocket
    "spoil_multiplier": 1.0,                // How putting an item in this pocket affects spoilage.  Less than 1.0 and the item will be preserved longer; 0.0 will preserve indefinitely.
    "weight_multiplier": 1.0,               // The items in this pocket magically weigh less inside than outside.  Nothing in vanilla should have a weight_multiplier.
    "volume_multiplier": 1.0,               // The items in this pocket have less volume inside than outside.  Can be used for containers that'd help in organizing specific contents, such as cardboard rolls for duct tape.
    "volume_encumber_modifier": 1,          // Default 1. How much this pocket contributes to enumbrance compared to an average item; Works same as volume_encumber_modifier from armor data, see JSON_INFO.md#armor-portion-data
    "moves": 100,                           // Indicates the number of moves it takes to remove an item from this pocket, assuming best conditions.
    "rigid": false,                         // Default false. If true, this pocket's size is fixed, and does not expand when filled.  A glass jar would be rigid, while a plastic bag is not.
    "forbidden": true,                      // Default false. If true, this pocket cannot be used by players. 
    "magazine_well": "0 ml",                // Amount of space you can put items in the pocket before it starts expanding.  Only works if rigid = false.
    "watertight": false,                    // Default false. If true, can contain liquid.
    "airtight": false,                      // Default false. If true, can contain gas.
    "ablative": false,                      // Default false. If true, this item holds a single ablative plate. Make sure to include a flag_restriction on the type of plate that can be added.
    "holster": false,                       // Default false. If true, only one stack of items can be placed inside this pocket, or one item if that item is not count_by_charges.
    "open_container": false,                // Default false. If true, the contents of this pocket will spill if this item is worn by a character or placed into another item.
    "fire_protection": false,               // Default false. If true, the pocket protects the contained items from exploding if tossed into a fire.
    "transparent": false,                   // Default false. If true, the pocket is transparent, as you can see items inside it afar; in the future this would be used for light also
    "extra_encumbrance": 3,                 // Additional encumbrance given to character, if this pocket is used
    "ripoff": 3,                            // Default 0, as can't be ripped; Chance this pockets contents get ripped off when escaping a grab - random number between 0 and strength of the grab (20 for generic zed, for example) against random number between 0 and ten times of ripoff
    "activity_noise": {                     // Define the noise generated, if you walk, and this container is not empty 
        "volume": 8,                        // How loud the noise would be
        "chance": 60                        // Chance to generate a noise per move, from 0 to 100
      }, 
    "default_magazine": "medium_battery_cell",       // Define the default magazine this item would have when spawned. Can be overwritten by item group
    "ammo_restriction": { "ammotype": /* count */ }, // Restrict pocket to a given ammo type and count.  This overrides mandatory volume, weight, watertight and airtight to use the given ammo type instead.  A pocket can contain any number of unique ammo types each with different counts, and the container will only hold one type (as of now).  If this is left out, it will be empty.
    "flag_restriction": [ "FLAG1", "FLAG2" ],        // Items can only be placed into this pocket if they have a flag that matches one of these flags.
    "item_restriction": [ "item_id" ],               // Only these item IDs can be placed into this pocket. Overrides ammo and flag restrictions.
    "material_restriction": [ "material_id" ],       // Only items that are mainly made of this material can enter.
	// If multiple of "flag_restriction", "material_restriction" and "item_restriction" are used simultaneously then any item that matches any of them will be accepted.

    "sealed_data": { "spoil_multiplier": 0.0 },   // If a pocket has sealed_data, it will be sealed when the item spawns.  The sealed version of the pocket will override the unsealed version of the same datatype.

    "inherits_flags": true // Items in this pocket pass their flags to the parent item.
  }
]
```

### Melee

```jsonc
"id": "hatchet",       // Unique ID. Must be one continuous word, use underscores if necessary
"symbol": ";",         // ASCII character used in-game
"color": "light_gray", // ASCII character color
"name": "hatchet",     // In-game name displayed
"description": "A one-handed hatchet. Makes a great melee weapon, and is useful both for cutting wood, and for use as a hammer.", // In-game description
    "price": "95 cent",           // Used when bartering with NPCs.  Can use string "cent" "USD" or "kUSD".
"material": [          // Material types.  See materials.json for possible options
  { "type": "iron", "portion": 2 }, // See Generic Item attributes for type and portion details
  { "type": "wood", "portion": 3 }
],
"weight": 907,         // Weight, measured in grams
"volume": "1500 ml",   // Volume, volume in ml and L can be used - "50 ml" or "2 L"
"melee_damage": {      // Damage caused by using it as a melee weapon
  "bash": 12,
  "cut": 12
},
"flags" : ["CHOP"],    // Indicates special effects
"to_hit" {             // (Optional) To hit bonus values, omit if item isn't suited to be used as a melee weapon, see [GAME_BALANCE.md](/doc/design-balance-lore/GAME_BALANCE.md#to-hit-value) for individual value breakdowns.
  "grip": "solid",                           
  "length": "long",                          
  "surface": "point",                        
  "balance": "neutral"                       
}
```
### Memory Cards

Memory card information can be defined on any GENERIC item by adding an object named `memory_card`, this field does not support `extend`/`remove`, only override.

```jsonc
"id": "memory_card_unread",
"name": "memory card (unread)",
// ...
"memory_card": {
  "data_chance": 0.5,                  // 50% chance to contain data
  "on_read_convert_to": "memory_card", // converted to this itype_id on read
  "photos_chance": 0.33,               // 33% chance to contain new photos
  "photos_amount": 3,                  // contains between 1 and 3 new photos
  "songs_chance": 0.33,                // 33% chance to contain new songs
  "songs_amount": 4,                   // contains between 1 and 4 new songs
  "recipes_chance": 0.33,              // 33% chance to contain new recipes
  "recipes_amount": 5,                 // contains between 1 and 5 new recipes
  "recipes_level_min": 4,              // recipes will have at least level 4
  "recipes_level_max": 8,              // recipes will have at most level 8
  "recipes_categories": [ "CC_FOOD" ], // (Optional) Array, defaults `CC_FOOD`. Memory card can contain recipes from any of these categories.
  "secret_recipes": true               // (Optional) Boolean, default false. If true, can contain recipes with the `SECRET` flag.
}
```

### Gun

Guns can be defined like this:

```C++
"type": "ITEM",
"subtypes": [ "GUN" ], // Allows the below GUN fields to be read in addition to generic ITEM fields
"skill": "pistol",         // Skill used for firing
"ammo": [ "357", "38" ],   // Ammo types accepted for reloading
"ranged_damage": 0,        // Ranged damage when fired (see #ammo)
"range": 0,                // Range when fired
"dispersion": 32,          // Inaccuracy of gun, measured in 100ths of Minutes Of Angle (MOA)
// When sight_dispersion and aim_speed are present in a gun mod, the aiming system picks the "best"
// sight to use for each aim action, which is the fastest sight with a dispersion under the current
// aim threshold.
"min_strength": 8,         // Minimal strength required to use this gun. Mostly used in different bows
"sight_dispersion": 10,    // Inaccuracy of gun derived from the sight mechanism, measured in 100ths of Minutes Of Angle (MOA)
"recoil": 0,               // Recoil caused when firing, measured in 100ths of Minutes Of Angle (MOA)
"durability": 8,           // Resistance to damage/rusting, also determines misfire chance
"gun_jam_mult": 1.25       // Multiplier for gun mechanincal malfunctioning, mostly when it's damaged; Values lesser than 1 reflect better quality of the gun, that jam less; bigger than 1 result in gun being more prone to malfunction and jam at lesser damage level; zero gun_jam_mult (and zero mag_jam_mult if magazine is presented) would remove any chance for a gun to malfunction. Only apply if gun has any fault from gun_mechanical_simple group presented; Jam chances are described in Character::handle_gun_damage(); at this moment it is roughly: 0.05% for undamaged gun, 3% for 1 damage (|\), 15% for 2 damage (|.), 45% for 3 damage (\.), and 80% for 4 damage (XX), then this and magazine values are summed up
"blackpowder_tolerance": 8,// One in X chance to get clogged up (per shot) when firing blackpowder ammunition (higher is better). Optional, default is 8.
"min_cycle_recoil": 0,     // Minimum ammo recoil for the gun to be able to fire more than once per attack (to cycle), else shooting it results in a cycling failure. Set at 90% of the base ammo recoil, or 75% of the value if the weapon is known to cycle with blackpowder. This is to prevent the weapon from cycling with any kind of ammo
"clip_size": 100,          // Maximum amount of ammo that can be loaded
"faults": [                                     // Type of faults, that can be applied to this item; usually are inherited from single abstract like rifle_base, but exceptions exist
  { "fault": "fault_gun_dirt" },                // `fault` defines this fault can be applied to this item
  { "fault": "fault_stovepipe", "weight": 20 }  // `weight` defines the chance of this specific fault to be applied. default weight is 100 
  { "fault_group": "handles" },                 // `fault_group` defines the fault group that would be applied to this item
  { "fault_group": "handles", "weight_override": 20 } // if weight_override is used, the weight of an entire fault group is assigned to this value, and original weight is deleted
  { "fault_group": "handles", "weight_add": -5, "weight_mult": 0.8 } // weight_add and weight_mult both modify the weight of each fault in fault group. Default add is 0, default mult is 1
  ],                                            // Faults are applied from the code side
"handling": 10             // handling of the weapon; better handling means less recoil
"energy_drain": "2 kJ",    // Additionally to the normal ammo (if any), a gun can require some electric energy. Drains from battery in gun. Use flags "USE_UPS" and "USES_BIONIC_POWER" to drain other sources. This also works on mods. Attaching a mod with energy_drain will add/increase drain on the weapon.
"heat_per_shot": 10,       // Each shot from this weapon adds this amount of heat
"cooling_value": 3,        // Amount of heat value, that is reduced every turn
"overheat_threshold": 100, // Heat value, at which fault may occur, see #Item faults; values below zero mean item won't be able to fault
"ammo_to_fire" 1,          // Amount of ammo used
"modes": [ [ "DEFAULT", "semi-auto", 1 ], [ "AUTO", "auto", 4 ] ], // Firing modes on this gun, DEFAULT,AUTO, or MELEE followed by the name of the mode displayed in game, and finally the number of shots of the mod.
"reload": 450,             // Amount of time to reload, 100 = 1 second = 1 "turn"
"reload_noise": "Ping!",   // Sound, that would be produced, when the gun is reloaded; seems to not work
"reload_noise_volume": 4,  // how loud the reloading is
"built_in_mods": ["m203"], //An array of mods that will be integrated in the weapon using the IRREMOVABLE tag.
"default_mods": ["m203"]   //An array of mods that will be added to a weapon on spawn.
"barrel_volume": "30 mL",  // Amount of volume lost when the barrel is sawn. Approximately 250 ml per inch is a decent approximation.
"valid_mod_locations": [ [ "brass catcher", 1 ], [ "grip", 1 ] ],  // The valid locations for gunmods and the mount of slots for that location.
"loudness": 10             // Amount of noise produced by this gun when firing. If no value is defined, then it's calculated based on loudness value from loaded ammo. Final loudness is calculated as gun loudness + gunmod loudness + ammo loudness. If final loudness is 0, then the gun is completely silent.
```

### Gunmod

Gun mods can be defined like this:

```C++
"type": "ITEM",
"subtypes": [ "GUNMOD" ],      // Allows the below GUNMOD fields to be read in addition to generic ITEM fields
"location": "stock",           // Mandatory. Where is this gunmod is installed?
"mod_targets": [ "crossbow" ], // Mandatory. What kind of weapons can this gunmod be used with?
"install_time": "30 s",        // Mandatory. How long does installation take? An integer will be read as moves or a time string can be used.
"acceptable_ammo": [ "9mm" ],  // Optional filter restricting mod to guns with those base (before modifiers) ammo types
"ammo_modifier": [ "57" ],     // Optional field which if specified modifies parent gun to use these ammo types
"magazine_adaptor": [ [ "223", [ "stanag30" ] ] ], // Optional field which changes the types of magazines the parent gun accepts
"pocket_mods": [ { "pocket_type": "MAGAZINE_WELL", "item_restriction": [ "ai_338mag", "ai_338mag_10" ] } ], // Optional field, alters the original pockets of the weapon ; share the syntax with pocket_data; pocket type MAGAZINE and MAGAZINE_WELL are always overwritten, pocket type CONTAINER is always added to existing pockets; for MAGAZINE and MAGAZINE_WELL both ammo_modifier and magazine_adaptor fields are required to correctly migrate ammo type; type: TOOLMOD can use this field also
"damage_modifier": { "damage_type": "bullet", "amount": -1 }, // Optional field increasing or decreasing base gun damage
"dispersion_modifier": 15,     // Optional field increasing or decreasing base gun dispersion
"loudness_modifier": 4,        // Optional field increasing or decreasing base guns loudness
"loudness_multiplier": 0.5     // Optional field increasing or decreasing base guns loudness as multiplier. 0.5 would halve the gun loudness, 2 would double it
"range_modifier": 2,           // Optional field increasing or decreasing base gun range
"range_multiplier": 1.2,       // Optional field multiplying base gun range
"integral_longest_side": "5 cm", // Length that would be added to a gun when this mod is installed
"overwrite_min_cycle_recoil": 1350, // Using this field will overwrite gun's min_cycle_recoil
"reload_noise": "chuk chuk.",   // Message, that would be produced when you reload a gun with this mod; Seems to not work 
"reload_noise_volume": 2,       // Amount of noise produced, when you reload a gun with this mod
"aim_speed_modifier": -2,       // Changes how fast you aim a gun with this mod
"add_mod": [ [ "grip", 1 ], [ "sights", 1 ] ], // adds this amoutn of gunmods to gun, if this gunmod is installed
"energy_drain_multiplier": 1.2, // if weapon uses `energy_drain`, multiplies it on this amount
"field_of_view": 270,           // #53180 has an image of it, but it represent how big FoV of the scope - when characters start to aim, it doesn't use the scope whatsoever, aiming into "general direction", and then transfer to using scope to pinpoint the target. The bigger FoV is, the sooner character would be able to use the scope (target acquisition with higher power scopes is very very difficult); put simple: the bigger FoV, the faster player can aim, to some degree; measured in MOA (minutes of angle)
"min_skills": [ [ "weapon", 3 ], [ "gun", 4 ] ], // minimal skill level required to install this gunmod
"shot_spread_multiplier_modifier": -0.8, // For shotguns, changes the spread of the pellets. Given a default multiplier of 1.0(100%), a multiplier modifier of -0.8 results in 0.2(20%) shot spread. Multipliers from all mods are summed up, but in vanilla game, only choke should be able to manipulate with shot spread - **shotgun barrel length doesn't affect pellet spread**
"energy_drain_modifier": "200 kJ",  // Optional field increasing or decreasing base gun energy consumption (per shot) by adding given value. This addition is not multiplied by energy_drains_multiplier.
"energy_drains_multiplier": 2.5, // Optional field increasing or decreasing base gun energy consumption (per shot) by multiplying by given value.
"reload_modifier": -10,        // Optional field increasing or decreasing base gun reload time in percent
"min_str_required_mod": 14,    // Optional field increasing or decreasing minimum strength required to use gun
"aim_speed": 3,                // A measure of how quickly the player can aim, in moves per point of dispersion.
"ammo_effects": [ "BEANBAG" ], // List of IDs of ammo_effect types
"consume_chance": 5000,        // Odds against CONSUMABLE mod being destroyed when gun is fired (default 1 in 10000)
"consume_divisor": 10,         // Divide damage against mod by this amount (default 1)
"handling_modifier": 4,        // Improve gun handling. For example a forward grip might have 6, a bipod 18
"mode_modifier": [ [ "AUTO", "auto", 4 ] ], // Modify firing modes of the gun, to give AUTO or REACH for example
"barrel_length": "45 mm"       // Specify a direct barrel length for this gun mod. If used only the first mod with a barrel length will be counted
"overheat_threshold_modifier": 100,   // Add a flat amount to gun's "overheat_threshold"; if the threshold is 100, and the modifier is 10, the result is 110; if the modifier is -25, the result is 75
"overheat_threshold_multiplier": 1.5, // Multiply gun's "overheat_threshold" by this number; if the threshold is 100, and the multiplier is 1.5, the result is 150; if the multiplier is 0.8, the result is 80
"cooling_value_modifier": 2,          // Add a flat amount to gun's "cooling_value"; works the same as overheat_threshold_modifier
"cooling_value_multiplier": 0.5,      // Multiply gun's "cooling_value" by this number; works the same as overheat_threshold_multiplier
"heat_per_shot_modifier":  -2,        //  Add a flat amount to gun's "heat_per_shot"; works the same as overheat_threshold_modifier
"heat_per_shot_multiplier": 2.0,      // Multiply the gun's "heat_per_shot" by this number; works the same as overheat_threshold_multiplier
"is_bayonet": true,     // Optional, if true, the melee damage of this item is added to the base damage of the gun. Defaults to false.
"blacklist_slot": [ "rail", "underbarrel" ],      // prevents installation of the gunmod if the specified slot(s) are present on the gun.
"blacklist_mod": [ "m203", "m320" ],      // prevents installation of the gunmod if the specified mods(s) are present on the gun.
```

### Batteries
```C++
"type": "ITEM",
"subtypes": [ "BATTERY" ],   // Allows the below BATTERY fields to be read in addition to generic ITEM fields
"max_energy": "30 kJ" // Mandatory. Maximum energy quantity the battery can hold
```

### Tools

```C++
"type": "ITEM",
"subtypes": [ "TOOL" ], // Allows the below TOOL fields to be read in addition to generic ITEM fields
"id": "torch_lit",    // Unique ID. Must be one continuous word, use underscores if necessary
"symbol": "/",        // ASCII character used in-game
"color": "brown",     // ASCII character color
"name": "torch (lit)", // In-game name displayed
"description": "A large stick, wrapped in gasoline soaked rags. This is burning, producing plenty of light", // In-game description
"price": "0 cent",           // Used when bartering with NPCs.  Can use string "cent" "USD" or "kUSD".
"material": [ { "type": "wood", "portion": 1 } ], // Material types.  See materials.json for possible options. Also see Generic Item attributes for type and portion details
"techniques": [ "FLAMING" ], // Combat techniques used by this tool
"flags": [ "FIRE" ],      // Indicates special effects
"weight": 831,        // Weight, measured in grams
"volume": "1500 ml",  // Volume, volume in ml and L can be used - "50 ml" or "2 L"
"melee_damage": {     // Damage caused by using it as a melee weapon
  "bash": 12,
  "cut": 0
},
"to_hit" {             // (Optional) To hit bonus values, omit if item isn't suited to be used as a melee weapon, see [GAME_BALANCE.md](/doc/design-balance-lore/GAME_BALANCE.md#to-hit-value) for individual value breakdowns.
  "grip": "solid",                           
  "length": "long",                          
  "surface": "point",                        
  "balance": "neutral"                       
}
"turns_per_charge": 20, // Charges consumed over time, deprecated in favor of power_draw
"fuel_efficiency": 0.2, // When combined with being a UPS this item will burn fuel for its given energy value to produce energy with the efficiency provided. Needs to be > 0 for this to work
"use_action": [ "firestarter" ], // Action performed when tool is used, see special definition below
"qualities": [ [ "SCREW", 1 ] ], // Inherent item qualities like hammering, sawing, screwing (see tool_qualities.json)
"charged_qualities": [ [ "DRILL", 3 ] ], // Qualities available if tool has at least charges_per_use charges left
// Only TOOL type items may define the following fields:
"tool_ammo": [ "NULL" ],        // Ammo types used for reloading
"charge_factor": 5,        // this tool uses charge_factor charges for every charge required in a recipe; intended for tools that have a "sub" field but use a different ammo that the original tool
"charges_per_use": 1,      // Charges consumed per tool use
"initial_charges": 75,     // Charges when spawned
"max_charges": 75,         // Maximum charges tool can hold
"power_draw": "50 mW",     // Energy consumption per second
"revert_to": "torch_done", // Transforms into item when charges are expended
"revert_msg": "The torch fades out.", // Message, that would be printed, when revert_to is used
"sub": "hotplate",         // optional; this tool has the same functions as another tool
"etransfer_rate": "30 MB"  // units::ememory, electronic transfer rate per second supported by this e-device
"e_port": "USB-A",         // String defining connection type for fast file transfer; NOTE: if you want to use this for general connections, make a more general system, this is *only* for electronic devices that handle files
"e_port_banned": ["USB-A"],  // String array defining banned connection types; see above
"variables": {
  "vehicle_name": "Wheelchair",         // this tool is a foldable vehicle, that could bypass the default foldability rules; this is the name of the vehicle that would be unfolded 
  "folded_parts": "folded_parts_syntax" // this is the parts that this vehice has -it uses it's own syntax, different from `"type": "vehicle"`, so better to read the examples in `unfoldable.json`
}
```

### Seed

```C++
"type": "ITEM",
"subtypes": [ "SEED", "COMESTIBLE" ],
"id": "seed_blackberries",
"copy-from": "seed_fruit",
"name": { "str_sp": "blackberry seeds" },
"description": "Some blackberry seeds.",
"flags": [ "PLANTABLE_SEED", "NUTRIENT_OVERRIDE", "INEDIBLE" ],
"plant_name": "blackberry",  // The name of the plant that grows from this seed. This is only used as information displayed to the user.
"fruit": "blackberries",  // The item id of the fruits that this seed will produce.
"seeds": false,  // (optional, default is true). If true, harvesting the plant will spawn seeds (the same type as the item used to plant). If false only the fruits are spawned, no seeds.
"byproducts": [ "withered" ],  // A list of further items that should spawn upon harvest.
"grow": "91 days",  // A time duration: how long it takes for a plant to fully mature. Based around a 91 day season length (roughly a real world season) to give better accuracy for longer season lengths
// Note that growing time is later converted based upon the season_length option, basing it around 91 is just for accuracy purposes
// A value 91 means 3 full seasons, a value of 30 would mean 1 season.
"fruit_div": 2, // (optional, default is 1). Final amount of fruit charges produced is divided by this number. Works only if fruit item is counted by charges.
"required_terrain_flag": "PLANTABLE" // A tag that terrain and furniture would need to have in order for the seed to be plantable there.
// Default is "PLANTABLE", and using this will cause any terain the plant is wrown on to turn into dirt once the plant is planted, unless furniture is used.
// Using any other tag will not turn the terrain into dirt.
```

### Brewables

If an item is BREWABLE, it can be placed in a vat and will ferment into a different item type. Currently only vats can only accept and produce liquid items.

```C++
"type": "ITEM",
"subtypes": [ "BREWABLE", "COMESTIBLE" ], //Most BREWABLE items are also COMESTIBLE, so the COMESTIBLE fields are omitted here
"id": "brew_whiskey",
"looks_like": "wash_whiskey",
"name": { "str_sp": "whiskey wort" },
"description": "Unfermented whiskey.  The base of a fine drink.  Not the traditional preparation, but you don't have the time.  You need to put it in a fermenting vat to brew it.",
"brew_time": "7 days",  // A time duration: how long the fermentation will take.
"brew_results": [ "wash_whiskey", "yeast" ], // IDs with a multiplier for the amount of results per charge of the brewable items.
...
//comestible fields
```

### Compostables

```C++
"type": "ITEM",
"subtypes": [ "COMPOSTABLE", "COMESTIBLE" ]
"id": "fermentable_liquid_mixture",
"name": { "str_sp": "fermentable liquid mixture" },
"volume": "500 ml",
"weight": "500 g",
"description": "A mixture of various organic wastes and water.  Wait about two months for them to be fermented in a tank.  Also, biogas production will start after one month.",
"copy-from": "fertilizer_liquid",
"price": "0 cent",
"price_postapoc": "0 cent",
"flags": [ "NUTRIENT_OVERRIDE", "TRADER_AVOID" ],
"charges": 1,
"compost_time": "60 days", // the amount of time required to fully compost this item
"compost_results": { "fermented_fertilizer_liquid": 1, "biogas": 250 }, //item IDs and quantities resulting from compost
...
//COMESTIBLE fields
```

### Milling

```C++
"type": "ITEM",
"subtypes": [ "MILLING", "COMESTIBLE" ] //most MILLING items are also COMESTIBLE
"id": "pistachio_shelled",
"name": { "str_sp": "shelled pistachios" },
"weight": "30 g",
"color": "green",
"container": "bag_plastic_small",
"symbol": "%",
"description": "A handful of nuts from a pistachio tree, with the shells removed.",
"price": "25 cent",
"price_postapoc": "150 cent",
"material": [ "nut" ],
"volume": "62 ml",
"flags": [ "EDIBLE_FROZEN", "NUTRIENT_OVERRIDE", "RAW", "SMOKABLE" ],
"into": "paste_nut", // resulting item ID for milling this item
"recipe": "paste_nut_mill_10_1", //associated recipe for milling this item
...
//COMESTIBLE fields
```

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
- `AEP_PSYSHIELD` Protection from fear paralyze attack
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

Effects of the artifact when it's activated (which require it to have a `"use_action": [ "ARTIFACT" ]` and it must have a non-zero max_charges value).

Possible values (see src/artifact.h for an up-to-date list):

- `AEA_STORM` Emits shock fields
- `AEA_FIREBALL` Targeted
- `AEA_ADRENALINE` Adrenaline rush
- `AEA_MAP` Maps the area around you
- `AEA_BLOOD` Shoots blood all over
- `AEA_SLEEPINESS` Creates interdimensional sleepiness
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
- `AEA_FUN` Temporary morale bonus
- `AEA_SPLIT` Split between good and bad
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
- `AEA_STAMINA_EMPTY` Empties most of the player's stamina gauge

### Use Actions

The contents of `use_action` fields can either be a string indicating a built-in function to call when the item is activated (defined in iuse.cpp), or one of several special definitions that invoke a more structured function.

```jsonc
"use_action": {
  "type": "transform",                      // The type of method, in this case one that transforms the item
  "target": "gasoline_lantern_on",          // The item to transform to
  "target_group": "twisted_geometry",       // If used, target is a random item from itemgroup
  "variant_type": "condom_plain",           // (optional) Defaults to `<any>`. Specific variant type to set for the transformed item. Special string `<any>` will pick a random variant from all available variants, based on the variant's defined weight
  "active": true,                           // Whether the item is active once transformed
  "ammo_scale": 0,                          // For use when an item automatically transforms into another when its ammo drops to 0, or to allow guns to transform with 0 ammo
  "msg": "You turn the lamp on.",           // Message to display when activated
  "need_fire": 1,                           // Whether fire is needed to activate
  "need_fire_msg": "You need a lighter!",   // Message to display if there is no fire
  "need_charges": 1,                        // Number of charges the item needs to transform
  "need_charges_msg": "The lamp is empty.", // Message to display if there aren't enough charges
  "need_empty": true,                       // Whether the item must be empty to be transformed; false by default
  "need_worn": true,                        // Whether the item must be worn to be transformed, cast spells, or use EOCs; false by default
  "need_wielding": true,                    // Whether the item must be wielded to be transformed, cast spells, or use EOCs; false by default
  "qualities_needed": { "WRENCH_FINE": 1 }, // Tool qualities needed, e.g. "fine bolt turning 1"
  "target_charges": 3,                      // Number of charges the transformed item has
  "rand_target_charges": [ 10, 15, 25 ],    // Randomize the charges the transformed item has. This example has a 50% chance of rng(10, 15) charges and a 50% chance of rng(15, 25) (endpoints are included)
  "ammo_qty": 3,                            // If zero or positive set remaining ammo of transformed item to this
  "random_ammo_qty": [ 1, 5 ],              // If this has values, set remaining ammo of transformed item to one of them chosen at random
  "ammo_type": "tape",                      // If both this and ammo_qty are specified then set transformed item to this specific ammo
  "container": "jar",                       // Container holding the target item
  "sealed": true,                           // Whether the transformed container is sealed; true by default
  "menu_text": "Lower visor",               // (optional) Text displayed in the activation screen. Defaults to "Turn on"
  "moves": 500                              // Moves required to transform the item in excess of a normal action
},
"use_action": {
  "type": "explosion",               // An item that explodes when it runs out of charges
  "explosion": {                     // (optional) Physical explosion data
                                     // Specified like `"explosion"` field in generic items
  },
  "draw_explosion_radius": 5,        // How large to draw the radius of the explosion
  "draw_explosion_color": "blue",    // The color to use when drawing the explosion
  "do_flashbang": true,              // Whether to do the flashbang effect
  "flashbang_player_immune": true,   // Whether the player is immune to the flashbang effect
  "fields_radius": 3,                // The radius of spread for fields produced
  "fields_type": "fd_tear_gas",      // The type of fields produced
  "fields_min_intensity": 3,         // Minimum intensity of field generated by the explosion
  "fields_max_intensity": 3,         // Maximum intensity of field generated by the explosion
  "emp_blast_radius": 4,             // The radius of EMP blast created by the explosion
  "scrambler_blast_radius": 4        // The radius of scrambler blast created by the explosion
},
"use_action": {
  "type": "change_scent",              // Change the scent type of the user
  "scent_typeid": "sc_fetid",          // The scenttype_id of the new scent
  "charges_to_use": 2,                 // Charges consumed when the item is used. Default is 1
  "scent_mod": 150,                    // Modifier added to the scent intensity. Default is 0
  "duration": "6 m",                   // How long does the effect last
  "effects": [ { "id": "fetid_goop", "duration": 360, "bp": "torso", "permanent": true } ],  // List of effects with their ID, duration, bodyparts, and permanent bool
  "waterproof": true,                  // Is the effect waterproof. (Default: false)
  "moves": 500                         // Number of moves required in the process
},
"use_action": {
  "type": "consume_drug",                 // A drug the player can consume
  "activation_message": "You smoke your crack rocks.  Mother would be proud.", // Message, ayup
  "effects": { "high": 15 },              // Effects and their duration
  "damage_over_time": [
    {
      "damage_type": "pure",              // Type of damage
      "duration": "1 m",                  // For how long this damage will be applied
      "amount": -10,                      // Amount of damage applied every turn, negative damage heals
      "bodyparts": [ "torso", "head", "arm_l", "leg_l", "arm_r", "leg_r" ] // Body parts hit by the damage
    }
  ]
  "stat_adjustments": { "hunger": -10 },   // Adjustment to make to player stats
  "fields_produced": { "cracksmoke": 2 },  // Fields to produce, mostly used for smoke
  "charges_needed": { "fire": 1 },         // Charges to use in the process of consuming the drug
  "tools_needed": { "apparatus": -1 },     // Tool needed to use the drug
  "moves": 50,                             // Number of moves required in the process, default value is 100
  "vitamins": [                            // What and how much vitamin is given by this drug
    [ "mutagen_alpha", 225 ],
    [ "mutagen", 125 ]
  ] 
},
"use_action": {
  "type": "place_monster",          // Place a turret / manhack / whatever monster on the map
  "monster_id": "mon_manhack",      // Monster ID, see monsters.json
  "difficulty": 4,                  // Difficulty for programming it (manhacks have 4, turrets 6, etc.)
  "hostile_msg": "It's hostile!",   // (optional) Message when programming the monster failed and it's hostile
  "friendly_msg": "Good!",          // (optional) Message when the monster is programmed properly and it's friendly
  "place_randomly": true,           // Places the monster randomly around the player or lets the player decide where to put it (default: false)
  "skills": [ "unarmed", "throw" ], // (optional) Array of skill IDs. Higher skill level means more likely to place a friendly monster
  "moves": 60,                      // How many move points the action takes
  "is_pet": false                   // Specifies if the spawned monster is a pet. The monster will only spawn as a pet if it is spawned as friendly, hostile monsters will never be pets
},
"use_action": {
  "type": "place_npc",                     // Place NPC of specific class on the map
  "npc_class_id": "true_foodperson",       // NPC ID, see npcs/npc.json
  "summon_msg": "You summon a food hero!", // (optional) Message when summoning the NPC
  "place_randomly": true,                  // Places NPC randomly around the player or lets the player decide where to put it (default: false)
  "radius": 1,                             // Maximum radius for random NPC placement
  "moves": 50                              // How many move points the action takes
},
"use_action": {
  "type": "link_up",       // Connect item to a vehicle or appliance, such as plugging a chargeable device into a power source
                           // If the item has the CABLE_SPOOL flag, it has special behaviors available, like connecting vehicles together
  "cable_length": 4,       // (optional) Maximum length of the cable. Defaults to the item type's maximum charges
                           // If extended by other cables, will use the sum of all cables' lengths
  "charge_rate": "60 W",   // (optional) The charge rate of the plugged-in device's batteries in watts. Defaults to "0 W"
                           // A positive value will charge the device's chargeable batteries at the expense of the connected power grid
                           // A negative value will charge the connected electrical grid's batteries at the expense of the device's
                           // A value of 0 won't charge the device's batteries, but will still let the device operate off of the connected power grid
  "efficiency": 0.85,      // (optional) This out of 1.0 chance to successfully add 1 charge every charge interval. Defaults to 0.85f, AKA 85% efficiency
                           // A value less than 0.001 means the cable won't transfer any electricity at all
                           // If extended by other cables, will use the product of all cable's efficiencies multiplied together
  "menu_text":             // (optional) Text displayed in the activation screen. Defaults to Plug in / Manage cables
  "move_cost": 50,         // (optional) Move cost of attaching the cable. Defaults to 5
  "targets": [             // (optional) Array of link_states that are valid connection points of the cable. Defaults to only allowing disconnection
    "no_link",             // Must be included to allow letting the player manually disconnect the cable
    "vehicle_port",        // Can connect to a vehicle's cable ports / electrical controls or an appliance
    "vehicle_battery",     // Can connect to a vehicle's battery or an appliance
    "vehicle_tow",         // Can be used as a tow cable between two vehicles
    "bio_cable",           // Can connect to a cable system bionic
    "ups",                 // Can link to a UPS
    "solarpack"            // Can link to a worn solar pack
  ],
  "can_extend": [          // (optional) Array of cable items that can be extended by this one. Defaults to none
    "extension_cable",
    "long_extension_cable",
    "ELECTRICAL_DEVICES"   // "ELECTRICAL_DEVICES" is a special keyword that lets this cable extend all electrical devices that have link_up actions
  ]
},
"use_action": {
  "type": "deploy_furn",
  "furn_type": "f_foo"          // What furniture this item will be transmuted into
},
"use_action": {
  "type": "deploy_appliance",
  "base": "item_id"             // Base item of the appliance this item will turn into
},
"use_action": {
  "type": "delayed_transform",  // Like transform, but it will only transform when the item has a certain age
  "transform_age": 600,         // The minimal age of the item. Items that are younger won't transform. In turns (60 turns = 1 minute)
  "not_ready_msg": "The yeast has not been done The yeast isn't done culturing yet." // A message, shown when the item is not old enough
},
"use_action": {
  "type": "firestarter",  // Start a fire, like with a lighter.
  "moves": 15,            // Number of moves it takes to start the fire. This is reduced by survival skill
  "moves_slow": 1500,     // Number of moves it takes to start a fire on something that is difficult to ignite. This is reduced by survival skill
  "need_sunlight": true   // Whether the character needs to be in direct sunlight, e.g. to use magnifying glasses
},
"use_action": {
  "type": "unpack",                 // Unpack this item
  "group": "gobag_contents",        // Item group this unpacks into
  "items_fit": true,                // Do the armor items in this fit? Defaults to false
  "filthy_volume_threshold": "10 L" // If the items unpacked from this item have volume, and this item is filthy, at what amount of held volume should they become filthy
},
"use_action": {
  "type": "salvage",       // Try to salvage base materials from an item, e.g. cutting up cloth to get rags or leather
  "moves_per_part": 25,    // (optional) Number of moves it takes
  "material_whitelist": [  // List of material IDs (not item IDs!) that can be salvage from
    "cotton",              // The list here is the default list, used when there is no explicit martial list given
    "leather",             // If the item (that is to be cut up) has any material not in the list, it can not be cut up
    "fur",
    "wood",
    "wool"
  ]
},
"use_action": {
  "type": "inscribe",          // Inscribe a message on an item or on the ground
  "on_items": true,            // Whether the item can inscribe on an item
  "on_terrain": false,         // Whether the item can inscribe on the ground
  "material_restricted": true, // Whether the item can only inscribe on certain item materials. Not used when inscribing on the ground
  "material_whitelist": [      // List of material IDs (not item IDs!) that can be inscribed on
    "wood",                    // Only used when inscribing on an item, and only when material_restricted is true
    "plastic",                 // The list here is the default that is used when no explicit list is given.
    "glass",
    "chitin",
    "steel",
    "silver"
  ]
},
"use_action": {
  "type": "fireweapon_off",               // Activate a fire based weapon
  "target_id": "firemachete_on",          // The item type to transform this item into
  "success_message": "Your No. 9 glows!", // A message that is shows if the action succeeds
  "failure_message": "",                  // (optional) A message that is shown if the action fails, for whatever reason. If not given, no message will be printed
  "lacks_fuel_message": "Out of fuel",    // Message that is shown if the item has no charges
  "noise": 0,                             // (optional) The noise it makes to active the item. 0 means no sound at all
  "moves": 0,                             // The number of moves it takes the character to even try this action (independent of the result)
  "success_chance": 0                     // How likely it is to succeed the action. Default is to always succeed. Try numbers in the range of 0-10
},
"use_action": {
  "type": "fireweapon_on",                          // Function for active (burning) fire based weapons
  "noise_chance": 1,                                // The chance (one in X) that the item will make a noise, rolled on each turn
  "noise": 0,                                       // The sound volume it makes, if it makes a noise at all. If 0, no sound is made, but the noise message is still printed
  "noise_message": "Your No. 9 hisses.",            // The message / sound description (if noise is > 0), that appears when the item makes a sound
  "voluntary_extinguish_message": "Your No. 9 goes dark.", // Message that appears when the item is turned of by player
  "charges_extinguish_message": "Out of ammo!",     // Message that appears when the item runs out of charges
  "water_extinguish_message": "Your No. 9 hisses in the water and goes out.", // Message that appears if the character walks into water and the fire of the item is extinguished
  "auto_extinguish_chance": 0,                      // If > 0, this is the (one in X) chance that the item goes out on its own
  "auto_extinguish_message": "Your No. 9 cuts out!" // Message that appears if the item goes out on its own (only required if auto_extinguish_chance is > 0)
},
"use_action": {
  "type": "musical_instrument",   // The character plays an instrument (this item) while walking around.
  "speed_penalty": 10,            // This is subtracted from the characters speed.
  "volume": 12,                   // Volume of the sound of the instrument.
  "fun": -5,                      // Together with fun_bonus, this defines how much morale the character gets from playing the instrument. They get `fun + fun_bonus * <character-perception>` morale points out of it. Both values and the result may be negative
  "fun_bonus": 2,
  "description_frequency": 20,    // Once every Nth turn, a randomly chosen description (from the that array) is displayed
  "player_descriptions": [
    "You play a little tune on your flute.",
    "You play a beautiful piece on your flute.",
    "You play a piece on your flute that sounds harmonious with nature."
  ]
},
"use_action": {
  "type": "holster",                          // Holster or draw a weapon
  "holster_prompt": "Holster item",           // Prompt to use when selecting an item
  "holster_msg": "You holster your %s",       // Message to show when holstering an item
  "max_volume": "1500 ml",                    // Maximum volume of each item that can be holstered. Volume in ml and L can be used - "50 ml" or "2 L"
  "min_volume": "750 ml",                     // Minimum volume of each item that can be holstered or 1/3 max_volume if unspecified. Volume can be in ml or L: "50 ml" or "2 L"
  "max_weight": 2000,                         // Maximum weight of each item. If unspecified no weight limit is imposed
  "multi": 1,                                 // Total number of items that holster can contain
  "draw_cost": 10,                            // Base move cost per unit volume when wielding the contained item
  "skills": [ "pistol", "shotgun" ],          // Guns using any of these skills can be holstered
  "flags": [ "SHEATH_KNIFE", "SHEATH_SWORD" ] // Items with any of these flags set can be holstered
},
"use_action": {
  "type": "bandolier",       // Store ammo and later reload using it
  "capacity": 10,            // Total number of rounds that can be stored
  "ammo": [ "shot", "9mm" ]  // What types of ammo can be stored?
},
"use_action": {
  "type": "reveal_map",             // Reveal specific terrains on the overmap
  "radius": 180,                    // Radius around the player where things are revealed. A single overmap is 180x180 tiles
  "terrain": [ "hiway", "road" ],   // IDs of overmap terrain types that should be revealed (as many as you want)
  "message": "You add roads and tourist attractions to your map." // Displayed after the revelation
},
"use_action": {
  "type": "heal",               // Heal damage, possibly some statuses
  "limb_power": 10,             // How much hp to restore when healing limbs? Mandatory value
  "head_power": 7,              // How much hp to restore when healing head? If unset, defaults to 0.8 * limb_power
  "torso_power": 15,            // How much hp to restore when healing torso? If unset, defaults to 1.5 * limb_power
  "bleed": 4,                   // How many bleed effect intensity levels can be reduced by it. Base value
  "disinfectant_power": 4,      // Quality of disinfection. Antiseptic is 4, alcohol wipe is 2; float
  "bite": 0.95,                 // Chance to remove bite effect
  "infect": 0.1,                // Chance to remove infected effect
  "move_cost": 250,             // Cost in moves to use the item
  "limb_scaling": 1.2,          // How much extra limb hp should be healed per first aid level. Defaults to 0.25 * limb_power
  "head_scaling": 1.0,          // How much extra limb hp should be healed per first aid level. Defaults to (limb_scaling / limb_power) * head_power
  "torso_scaling": 2.0,         // How much extra limb hp should be healed per first aid level. Defaults to (limb_scaling / limb_power) * torso_power
  "effects": [                  // Effects to apply to patient on finished healing. Same syntax as in consume_drug effects
    { "id": "pkill1", "duration": 120 }
  ],
  "used_up_item": "rag_bloody"  // Item produced on successful healing. If the healing item is a tool, it is turned into the new type. Otherwise a new item is produced
},
"use_action": {
  "type": "place_trap",                // Places a trap
  "allow_underwater": false,           // (optional) Allow placing this trap when the player character is underwater
  "allow_under_player": false,         // (optional) Allow placing the trap on the same square as the player character (e.g. for benign traps)
  "needs_solid_neighbor": false,       // (optional) Trap must be placed between two solid tiles (e.g. for tripwire)
  "needs_neighbor_terrain": "t_tree",  // (optional) If non-empty: a terrain id, the trap must be placed adjacent to that terrain. Default is empty
  "outer_layer_trap": "tr_blade",      // (optional) If non-empty: a trap id, makes the game place a 3x3 field of traps. The center trap is the one defined by "trap", the 8 surrounding traps are defined by this (e.g. tr_blade for blade traps). Default is empty
  "bury_question": "",                 // (optional) If non-empty: a question that will be asked if the player character has a shoveling tool and the target location is diggable. It allows to place a buried trap. If the player answers the question (e.g. "Bury the X-trap?") with yes, the data from the "bury" object will be used.
  "bury": {                            // If the bury_question was answered with yes, data from this entry will be used instead of outer data
                                       // This json object should contain "trap", "done_message", "practice" and "moves", with the same meaning as below
  },
  "trap": "tr_engine",                             // The trap to place.
  "done_message": "Place the beartrap on the %s.", // The message that appears after the trap has been placed. %s is replaced with the terrain name of the place where the trap has been put
  "practice": 4,                                   // How much practice to the "traps" skill placing the trap gives
  "moves": 10                                      // (optional) The move points that are used by placing the trap. Default is 100
},
"use_action": {
  "type": "sew_advanced",  // Modify clothing
  "materials": [           // Materials to deal with
    "cotton",
    "leather"
  ],
  "skill": "tailor",       // Skill used
  "clothing_mods": [       // Clothing mods to deal with
    "leather_padded",
    "kevlar_padded"
  ]
},
"use_action": {
  "type": "effect_on_conditions",          // Activate effect_on_conditions
  "menu_text": "Infuse saline",            // (optional) Text displayed in the activation screen. Defaults to "Activate item"
  "description": "This debugs the game",   // Usage description
  "effect_on_conditions": [ "test_cond" ]  // IDs of the effect_on_conditions to activate
},
"use_action": {
  "type": "message",       // Displays message text
  "message": "Read this.", // Message that is shown
  "name": "Light fuse"     // (optional) Name for the action. Default "Activate"
},
"use_action": {
  "type": "sound",            // Makes sound
  "name": "Turn on",          // Optional name for the action. Default "Activate"
  "sound_message": "Bzzzz.",  // message shown to player if they are able to hear the sound. %s is replaced by item name
  "sound_id": "misc",         // ID of the audio to be played. Default "misc". See SOUNDPACKS.md for more details
  "sound_variant": "default", // Default is "default"
  "sound_volume": 5           // Loudness of the noise
},
"use_action": {
  "type": "manualnoise",              // Makes sound. Includes ammo checks and may take moves from player
  "use_message": "You do the thing",  // Shown to player who activated it
  "noise_message": "Bzzz",            // Shown if player can hear the sound. Default "hsss"
  "noise_id": "misc",                 // ID of the audio to be played. Default is "misc". See SOUNDPACKS.md for more details
  "noise_variant": "default",         // Default is "default"
  "noise": 6,                         // Loudness of the noise. Default is 0
  "moves": 40                         // How long the action takes. Default is 0
},
"use_action": {
  "type": "cast_spell",       // Casts the following spell. See MAGIC.md for more details
  "spell_id": "mana_doping",  // ID of the spell to cast
  "no_fail": true,            // If the spell is allowed to fail when casted (e.g. high difficulty may cause the cast to fail)
  "level": 1,                 // Level of the spell
  "need_worn": true,          // (optional) If the item has to be worn to cast the spell
  "need_wielding": true,      // (optional) If the item has to be wielded to cast the spell
  "mundane": true             // (optional) This spell uses magic-related features, but is not magic itself. The description is changed from "This item casts spell_name at level spell_level" to "This item when activated: spell_name"
},
```

### Drop Actions

Similar to use_action, this drop_actions would be triggered when you throw the item

```jsonc
"drop_action": {                  
  "type": "emit_actor",           // allow to emit a specific field when thrown
  "emits": [ "emit_acid_drop" ],  // id of emit to spread
  "scale_qty": true               // if true, throwing more than one charge of item with emit_actor increases the size of emission
  }
```

### Tick Actions

`"tick_action"` of active tools is executed once on every turn. This action can be any use action or iuse but some of them may not work properly when not executed by player.

If `"tick_action"` is defined as array of multiple actions they all are executed in order. Multiple use actions of same type cannot be used at once.
  
#### Delayed Item Actions

Item use actions can be used with a timer delay.

Item `"transform"` action can set and start the timer. This timer starts when the player activates the item.
```jsonc
"use_action": {
    "type": "transform",
    "target": "grenade_act",
    "target_timer": "5 seconds" // Sets timer on item to this
}
```

Timer inherent to the item itself can be set by defining `"countdown_interval"` in item json. This timer is started at the birth of the item.

```jsonc
    "id": "migo_plate_undergrown",
    "name": { "str": "undergrown iridescent carapace plate" },
    "countdown_interval": "24 hours",
```

Once the duration of the timer has passed the `"countdown_action"` is executed. This action can be any use action but many actions do not behave well when they are not triggered by the player.

```jsonc
"countdown_action": {
    "type": "explosion",
    "explosion": { "power": 240, "shrapnel": { "casing_mass": 217, "fragment_mass": 0.08 } }
}
```

Additionally `"revert_to"` can be defined in item definitions (not in use action). The item is deactivated and turned to this type after the `"countdown_action"`. If no revert_to is specified the item is destroyed.


### Item Properties

Properties are bound to item's type definition and code checks for them for special behaviour,
for example the property below makes a container burst open when filled over 75% and it's thrown.

```jsonc
  {
    "properties": { "burst_when_filled": "75" }
  }
```

### Item Variables

Item variables are plain strings of data stored within item and used to serialize special behaviour,
for example folding a vehicle serializes the folded vehicle's name and list of parts
(part type ids, part damage, degradation etc) into json string for use when unfolding.

Alternatively item variables may also originate from the item's prototype. Specifying them
can be done in the item's definition, add the `variables` key and inside write a key-value
map.

all values of variable are written as string
Example:
```jsonc
    "variables": { "variable_name": "value" }
```
```jsonc
    "variables": { "browsed": "false" }
```
```jsonc
    "variables": { "water_per_tablet": "4" }
```

This will make any item instantiated from that prototype get assigned this variable, once
the item is spawned the variables set on the prototype no longer affect the item's variables,
a migration can clear out the item's variables and reassign the prototype ones if reset_item_vars
flag is set.

Also item variables can be assigned and read by effect_on_conditions
```jsonc
{ "math": [ "n_variablename = rand(5, 100)" ] } // assuming beta talker, aka n_, is item

{ "math": [ "n_variablename > 50 ? u_pain()++ : u_pain()--" ] }
```

```jsonc
{
  "set_string_var": [ "foo", "bar", "xyz", "qwe", "rty" ],
  "target_var": { "npc_val": "variablename" }
}
```
