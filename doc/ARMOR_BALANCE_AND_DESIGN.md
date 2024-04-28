# Armor Balance And Design

## Armor Overview
Armor in cataclysm has varying levels of complexity to attempt a deep simulation of worn clothing. Compared with systems that try to compress down armor to one or two numbers like Pen And Paper game armor class systems or CRPGs with systems like DT and DR (flat and multiplicative reduction) cataclysm attempts to more accurately simulate different materials, their relative coverage of the body, and how that interacts with the player.  A lot of this is simplified or clarified to the player in a way that is digestible. We *shouldn't* hide info from the player but we should *avoid* information overload.

That said this document will practice no such restraint and is designed to be a complete guide to *everything* possible. This guide will cover the basic ideas of **how it works**, **what is necessary** for armor to function, **what is possible** with the armor system and **how to design armor** so that it is balanced and makes sense. An **FAQ** will also be included to cover some specifics people might have about armor or how to understand it as well as a section on things to be added **looking forward**.  Each section will also give specific example item ids you could investigate for examples (as well as code snippets) or for balance considerations.

## How it works
Characters in Cataclysm wear armor. This armor provides them with protection, warmth, encumbrance and depending on material helps them breathe when they sweat.

### protection
When you take a direct hit in Cataclysm the following happens:
1. When the player is hit at a specific body part, first a sub-location on the limb is generated from the weighted list of sub-limbs
2. For the torso location an additional secondary sub-limb is chosen for items hanging off the player (like backpacks)
3. Armor coverage determines whether any armor is hit, or an uncovered part of the player is hit (roll a single `1d100` against coverage).
4. Go through all worn armor from the most outward clothing piece inwards. Reducing the total damage as you apply the defense of each item.
5. If the armor doesn't cover the chosen sub body part skip it.
6. Depending on the attack the armors melee, ranged or general coverage (which is not scaled to the overall body part and instead just how much of the sub-limb it covers) is compared to the roll from above.
7. In the future Vitals Coverage will be used to scale incoming critical damage, reducing crit multipliers but it is not implemented yet.
8. If the above roll fails (ie roll value is above coverage), then the armor does not absorb any damage from the blow, neither does it become damaged.
9. If the above roll succeeds, the armor is hit, possibly absorbing some damage and possibly getting damaged in the process.
10. If the armor has ABLATIVE pockets (like ballistic vests) at this point the coverage of those plates is rolled to see if they possibly absorb some damage and possibly get damaged in the process. At most one ablative pocket on an item can apply to an attack and the coverage of the plates is scaled based on the coverage of the armor they are in. This is because if a plate covers 45% of the torso but the clothing its in only covers 50% of the torso, you already know the attack hit the jacket and the jacket is 90% (45/50) plate.
11. Armor protects against damage.  The values are determined by multiplying the armor thickness by the materials individual damage type resistance factor respectively, given in `materials.json`.
12. For simple definition armors: If the armor is made from more than 1 material types, then it divides the armors overall thickness based on proportions each is (assumes equal proportions by default). Giving a single protection value per type of damage.
13. For complex definition armors: Each material on each limb/sub-limb can have a specific thickness and also an amount of the overall armored portion that it covers (proportional_coverage). Each individual material is rolled against its proportional_coverage vs `1d100` to see if it applies to any given attack. That is why some armors have a red (worst case) percent, green (best case) percent and a median protection value now because an armor itself can have variable protection for any given attack. This is to simulate armor with padding or plating that don't fully cover it.

Specifically to ablative plates, some transform when damaged instead of damaging normally. The chance of transforming scales based on damage taken.


### warmth
Is a particularly arcane system helping to insulate the character keeping them warm. The more warm your armor the more you'll sweat and the hotter you'll be.

### encumbrance
Encumbrance applies to limbs and gives a number of effects based on the limb. Hands effect manipulation, torso effects chance to hit. It's an overall measure of how heavy and hard to move in an armor is.

Armor can conflict with other armor, this is to simulate things being uncomfortable or awkwardly pressing against each other. If this happens, the encumbrance of each piece of armor other than the most encumbering is added to your character. If any piece has an encumbrance less than 2 their penalty is 2.

For example if you were wearing 2 tank tops, at 1 encumbrance each your total torso encumbrance would be 1 + 1 + 2 = 4,  1 for each tank top and 2 for the conflict (since it will be at minimum 2).

## What is necessary
This is a basic overview of things armors basically **need** to have to work as expected, or at all.  Sometimes an advanced feature will circumvent or overwrite the things provided here.

All armors need:
* Everything a normal item does. I'm not explaining this
* ```"material"``` array: a list of the materials that make up the item in general.
* ```"armor"``` array: this will be described in depth later in **advanced armor definitions**.
* ```"warmth"``` value, usually between 0 - 50 based on how hot the item is to wear.
* ```"material_thickness"``` this is measured in mm, this is **the most important value to get right when designing armor**

here is an example armor to look at:
```json
{
  "id": "dress_shirt",
  "type": "ARMOR",
  "name": { "str": "dress shirt" },
  "description": "A white button-down shirt with long sleeves.  Looks professional!",
  "weight": "250 g",
  "volume": "750 ml",
  "price": "15 USD",
    "price_postapoc": "50 cent",
  "material": [ "cotton" ],
  "symbol": "[",
  "looks_like": "longshirt",
  "color": "white",
  "armor": [
    { "covers": [ "torso" ], "coverage": 90, "encumbrance": 4 },
    { "covers": [ "arm_l", "arm_r" ], "coverage": 90, "encumbrance": 4 }
  ],
  "warmth": 10,
  "material_thickness": 0.1,
}
```

## What is possible
This section is purely about what you can do. Basically no armor should have all of these features. Remember *the more complexity you add to something the harder it is to understand*.

Each subsection of this will consist of an **explanation of the feature**, an **example of how to use it**, and **further examples from the code**

### Advanced Armor Definitions
#### Explanation
Some items will use an old way of defining armor where their coverage and thickness are loose in the json. This is likely to be phased out. As such I would recommend **always using an advanced armor definition**. Each piece of this will be explained in detail further below but the simplest thing you can do is just define an array of objects containing a coverage value, encumbrance, and the covered locations.

#### Example
```json
"armor": [
  { "covers": [ "torso" ], "coverage": 95, "encumbrance": 4 },
  { "covers": [ "arm_l", "arm_r" ], "coverage": 90, "encumbrance": 2 }
]
```
#### Further Reading
Any item with an "armor" array easily searchable in the code.

### Layers
#### Explanation
Layers are a system in Cataclysm used to differentiate different types of clothes. Functionally things on separate layers will not conflict with each other. So you can wear underwear, jeans, knee pads and drop leg pouches without conflict. The layers are defined as flags, if you don't define a layer for an item it instead is assumed to be normal layer. The layers are (in order):
* PERSONAL - your personal aura, this is primarily for magiclysm and sci-fi stuff.
* SKINTIGHT - underwear, things that are incredibly thin, less than 1mm
* NORMAL - everyday clothes, pants, sweaters, loose shirts, jumpsuits.
* WAIST - things worn as belts, this is an old legacy solution designed for having belts not conflict with backpacks. Use it **only** for belts on torso.
* OUTER - outerwear, things like winter coats, trench coats, **all hard armor**
* BELTED - pouches and storage, things like drop pouches, rifle slings, holsters, backpacks all go on this layer
* AURA - outer aura, again magiclysm and sci-fi, not really used in core

Items can also be on multiple layers by including additional flags. The best example of this is an OUTER armor with a lot of pouches would also be BELTED.

#### Example
```json
"flags": [ "OUTER", "BELTED" ]
```

#### Further Reading
Basically all items have a layer (or omit it to be counted as NORMAL)
The Nomad Plate is an example of an item that is NORMAL + OUTER.

### Multi-layer
#### Explanation
Aside from a specific armor taking up multiple layers, different *parts* of an armor can take up different layers. This for example would be useful for a spacesuit, where the helmet is a large glass dome and would be OUTER but the suit itself is form fitting NORMAL layer so that you could wear armor over it. Another use case is for armor that has pouches on the torso but not on the limbs, you could specify OUTER + BELTED for torso and just OUTER for everywhere else. This is done by including the layer info in the advanced armor definition instead of with flags. If an entry doesn't have layers described it falls back to the item flags and if it doesn't have flags it falls back to NORMAL.

#### Example
```json
"flags": [ "STURDY", "OUTER" ],
"armor": [
  { "encumbrance": [ 4, 6 ], "coverage": 100, "cover_vitals": 90, "covers": [ "torso" ], "layers": [ "OUTER", "BELTED" ] },
  {
    "encumbrance": [ 4, 4 ],
    "coverage": 100,
    "cover_vitals": 90,
    "covers": [ "arm_l", "arm_r" ],
    "specifically_covers": [ "arm_shoulder_l", "arm_shoulder_r", "arm_upper_l", "arm_upper_r" ]
  },
  {
    "encumbrance": [ 4, 4 ],
    "coverage": 100,
    "cover_vitals": 90,
    "covers": [ "leg_l", "leg_r" ],
    "specifically_covers": [ "leg_hip_l", "leg_hip_r", "leg_upper_r", "leg_upper_l" ]
  }
]
```
#### Further Reading
The Phase Immersion suit is about as complicated as it gets.

### Sublocations
#### Explanation
Sub-locations are a new-ish feature of characters in Cataclysm. Sub-locations are sub divided pieces of characters limbs. For example a human arm consists of a lower arm, elbow, upper arm, and shoulder. All limbs have sub-locations and you can find the full list of them in the body part definitions in ```body_parts.json```. When you define specific sub-locations for armor it does the following:
* Makes it so that armor only conflicts with armor that shares sub-locations with it on it's layer
* Scales total coverage

So if you appropriately use specifically_covers a character can wear multiple pieces on the same limb and layer. For example a character could wear knee pads, and shin guards. Also when defining sub-locations and describing coverage you are describing the amount of the sub-locations the armor covers. So a pair of knee pads isn't 5% coverage, specifically covering the knees: its 90% coverage specifically covering the knees and the game is smart enough to know your knee is 5% of your leg. This is to make it simpler, just describe how much of the parts covered are covered, not how much of the overall limb.

The strapped layer also has additional sub-limbs that are used for hanging items. These currently only exist on the torso and in the json have ```"secondary": true```. They are for things hanging around your neck (like binoculars), your front (like a rifle on a sling), your back (like a backpack).

**if you don't define this for an armor it assumes that the armor covers every sub-location on that limb**

This will play more and more of a role as armor gets further developed.

#### Example
```json
"armor": [
  {
    "covers": [ "leg_l", "leg_r" ],
    "encumbrance": 0,
    "coverage": 90,
    "specifically_covers": [ "leg_knee_r", "leg_knee_l" ]
  }
]
```

#### Further Reading
Heavy Ballistic Vest uses this for different limb bits well.

### Sided Armor
#### Explanation
Sided armor is armor that even though it describes covering, both legs, both arms, both hands, etc. actually only covers one "side" at a time but can be moved back and forth between sides at will by the player. This works by setting ```"sided": true```

#### Example
```json
{
  "id": "bootsheath",
  "type": "ARMOR",
  "name": { "str": "ankle sheath" },
  "description": "A small concealed knife sheath worn on the ankle.  It is awkward to use without practice.  Activate to sheathe/draw a weapon.",
  "weight": "160 g",
  "volume": "500 ml",
  "price": "52 USD",
  "price_postapoc": "2 USD 50 cent",
  "to_hit": -1,
  "material": [ "leather" ],
  "symbol": "[",
  "looks_like": "sheath",
  "color": "brown",
  "sided": true,
  "material_thickness": 1,
  "pocket_data": [
    {
      "magazine_well": "200 ml",
      "holster": true,
      "flag_restriction": [ "SHEATH_KNIFE" ],
      "moves": 30,
      "max_contains_volume": "500 ml",
      "max_contains_weight": "1 kg",
      "max_item_length": "30 cm"
    }
  ],
  "use_action": { "type": "holster", "holster_prompt": "Sheath knife", "holster_msg": "You sheath your %s" },
  "flags": [ "BELTED", "OVERSIZE", "ALLOWS_NATURAL_ATTACKS", "WATER_FRIENDLY" ],
  "armor": [
    {
      "material": [ { "type": "leather", "covered_by_mat": 100, "thickness": 1 } ],
      "covers": [ "foot_l", "foot_r" ],
      "specifically_covers": [ "foot_ankle_l", "foot_ankle_r" ],
      "coverage": 25,
      "encumbrance": [ 2, 3 ],
      "layers": [ "BELTED" ]
    }
  ]
}
```
#### Further Reading
lots of holsters. The new plate armor is also sided per piece.

### Basic Materials
#### Explanation
Materials inform a lot of the information for an armor. What something is made of determines how likely it is to break, how much it protects you, how well it breaths (when you sweat). Materials can be defined in a number of ways going up in complexity. The simplest way is to specify an array of materials. When you do this it is assumed that each of the given materials is equally represented in the armor meaning if you had a 2mm thick armor and it was 2 materials it would be assumed each material was 1mm thick. You can expand this further and give each a portion value. So if you had 9 portion material A and 1 portion material B for a 1mm thick armor than it would be .9mm A and .1mm B.
#### Example
```json
  "material": [ { "type": "plastic", "portion": 1 }, { "type": "steel", "portion": 1 }, { "type": "nomex", "portion": 2 } ],
```
#### Further Reading
Kevlar Gambeson hood and sleeves.

### Multi Material Inheritance
#### Explanation
This is a trick you can do to shorten the amount of work you need. Once you have an abstract for an armor defined, with all of the coverage and encumbrance info, you could create variants with different materials by using proportional coverage and thickness. This can be a powerful and convenient short hand.

#### Example
```json
{
  "//": "Tier 2 mantle",
  "id": "robofac_nomex_mantle",
  "type": "ARMOR",
  "copy-from": "robofac_mantle",
  "name": "Hub 01 turnout mantle",
  "description": "3D printed upper body armor, it is well finished, lightweight and it seems a lot of thought went into the ergonomics.  This is made of 3D printed plastic combined with printed Nomex and a hardened metal core to give significant reinforcement.  Designed to clip to the chest and shoulders the interior side reads ATTACH TO MODULAR DEFENSE SYSTEM.",
  "material": [ { "type": "plastic", "portion": 1 }, { "type": "steel", "portion": 1 }, { "type": "nomex", "portion": 2 } ],
  "proportional": { "weight": 2.5 }
}
```

#### Further Reading
The robofac greaves, mantles, skirts and vambraces use this.

### Repairs
#### Explanation
Clothing repairs are inherited from their material type.  Needed materials and repair difficulty are based on these materials (you can make a repair with any material it is made of, but the difficulty is whatever is the hardest part of it to repair).  In cases where the repair difficulty does not match that of its materials the ``repairs-like`` flag can be used to specify an item from which the difficulty and required skills should be inherited instead. This flag does not change the materials required to repair the item, which is always derived from its composition.

#### Example
The leather belt repairs like a leather patch because steel is more difficult to repair than leather, but the steel belt buckle is exceedingly unlikely to sustain damage compared to the leather majority of the belt.
```json
{
  "id": "belt_leather",
  "repairs_like": "leather"
}
```

### Advanced Materials
#### Explanation
It is possible to be more verbose and specific with materials and armor. To do this you can specify an array of materials for each advanced armor definition array. Each material needs at least an id and thickness (in mm) however you can also optionally specify a covered_by_mat which is how much of the armor is covered by the material. that way for example if an armor was steel but had leather straps that only covered 5% of the armor you could represent the straps as well. **At least 1 material on any armor should have 100 proportional coverage**.

#### Example
```json
"armor": [
  {
    "material": [
      { "type": "lc_steel", "covered_by_mat": 95, "thickness": 4.8 },
      { "type": "lc_steel_chain", "covered_by_mat": 100, "thickness": 1.2 },
      { "type": "leather", "covered_by_mat": 1, "thickness": 1.0 }
    ],
    "covers": [ "torso" ],
    "coverage": 100,
    "encumbrance": 25
  }
]
```

#### Further Reading
The Plate armor uses this.

### Melee, Ranged and Vitals Coverage
#### Explanation
In the case of melee and ranged coverage they can be specified to differentiate an armors chance to absorb ranged vs. melee attacks. They behave identically to regular coverage taking it's place for those types of attacks. If not defined these default to the armors base coverage. **these are not a replacement for coverage** basic "coverage" is still used for stuff like, sun burn/wind. Primarily these can be used by modern gear that is designed to protect center of mass/practical front facing locations.

Vitals Coverage does not behave like normal coverage and a is a % reduction to the amount of bonus damage a character takes from critical hits. If you have 100% vitals protection you take no additional damage from critical hits. If you have 50% vitals protection you take half the additional damage.

#### Example
```json
{
  "material": [
    { "type": "nylon", "covered_by_mat": 100, "thickness": 3.0 },
    { "type": "kevlar_layered", "covered_by_mat": 100, "thickness": 3.0 }
  ],
  "encumbrance": 5,
  "//": "an enemy could get under the protection in melee but ranged shots should almost always be blocked",
  "coverage": 80,
  "cover_melee": 80,
  "cover_ranged": 95,
  "cover_vitals": 100,
  "covers": [ "arm_l", "arm_r" ],
  "specifically_covers": [ "arm_shoulder_l", "arm_shoulder_r", "arm_upper_l", "arm_upper_r" ],
}
```

#### Further Reading
Thassalonian Bronze armor, Heavy Ballistic Vest

### Pockets
#### Explanation
Armor can have pockets. They are defined as a Pocket array. A storage guide will be written separately to this. It's worth keeping in mind how pockets effect armor though.

When your armor has potential volume it can store the encumbrance of the armor defaults to scaling as the armor gets fuller and fuller. By default an armors encumbrance is equal to its empty encumbrance + (volume stored / 250ml). However this is the base encumbrance that poorly tying an item to yourself with rope would give; so any man made storage will have a better volume / encumbrance than this.

You can represent a better volume / encumbrance in 3 ways:
* The simplest is to specify the encumbrance as an array of two values "[x, y]" this means the encumbrance empty is "x" and if full the item will have encumbrance "y" between 0 and 100% volume it scales linearly.

* The more modern way to do it is to set a scaling factor on the armor itself. This is much easier to read and quickly parse (not requiring mental math) and is a direct scaling on that 250ml constant. So if I set a "volume_encumber_modifier" of .25 it means that it's one additional encumbrance per 1000ml (250ml/.25). This is defined in the advanced armor definition.

* To get really specific you can combine the above with similar "volume_encumber_modifier" but on any individual pocket of the armor. This has the same effect as above but also effects how much that pocket contributes to the overall encumbrance. An example use case would be a rifle on a tactical sling would be more encumbering per volume than magazines affixed to the same vest so you could represent it as such. This can also be used with a modifier of 0 to set it so a pocket does not contribute to encumbrance.

#### Example
simple:
```json
  "encumbrance": [4, 6]
```

complex:
```json
{
  "material": [
    { "type": "nylon", "covered_by_mat": 100, "thickness": 3.0 },
    { "type": "kevlar_layered", "covered_by_mat": 95, "thickness": 3.0 }
  ],
  "volume_encumber_modifier": 0.2,
  "encumbrance": 15,
  "coverage": 100,
  "cover_vitals": 90,
  "covers": [ "torso" ]
}
```

```json
{
  "pocket_type": "CONTAINER",
  "ablative": true,
  "volume_encumber_modifier": 0,
  "max_contains_volume": "1600 ml",
  "max_contains_weight": "5 kg",
  "moves": 200,
  "description": "Pocket for front plate",
  "flag_restriction": [ "ABLATIVE_LARGE" ]
}
```
#### Further Reading
The ammo pouches for Load Bearing Vests and the LBVs themselves make use of this heavily.


### Ablative Armor
#### Explanation
This is a unique armor system to represent, armored inserts for armor. The original use case is for ballistic armor plates that go in bullet proof vests. However other armors could use this features in a similar way it is all supported through JSON.

To use this feature set a pocket on an item to be ```"ablative": true```, and set restrictions on what can go in the pocket by flag.

Then create armor inserts like normal armor but make sure they have 2 flags, CANT_WEAR and the flag that is restricted to those pockets. The plates when in those pockets will provide protection.

Keep in mind **plates need to have less or equal coverage to the armor they are in**. If you have a vest that covers 90% of your upper torso but it has an armor plate that covers 95% of the upper torso that doesn't make any sense.


#### Example
pocket:
```json
"pocket_data": [
  {
    "pocket_type": "CONTAINER",
    "ablative": true,
    "volume_encumber_modifier": 0,
    "max_contains_volume": "1600 ml",
    "max_contains_weight": "5 kg",
    "moves": 200,
    "description": "Pocket for front plate",
    "flag_restriction": [ "ABLATIVE_LARGE" ]
  }
]
```

plate definition:
```json
{
  "id": "esapi_plate",
  "type": "ARMOR",
  "category": "armor",
  "name": { "str": "ESAPI ballistic plate" },
  "description": "A polygonal ceramic ballistic plate with a slightly concave profile.  Its inner surface is coated with Ultra High Molecular Weight Polyethylene, and is labeled \"TOP\", while its outer surface is labeled \"STRIKE FACE\".  This is intended to be worn in a ballistic vest and can withstand several high energy rifle rounds before breaking.",
  "weight": "2500 g",
  "volume": "1533 ml",
  "price": "600 USD",
  "price_postapoc": "1 USD",
  "material": [ "ceramic" ],
  "symbol": ",",
  "color": "dark_gray",
  "material_thickness": 25,
  "non_functional": "destroyed_large_ceramic_plate",
  "damage_verb": "makes a crunch, something has shifted",
  "flags": [ "ABLATIVE_LARGE", "CANT_WEAR" ],
  "armor": [ { "encumbrance": 2, "coverage": 45, "covers": [ "torso" ], "specifically_covers": [ "torso_upper" ] } ]
}
```

#### Further Reading
Look at the Plates and Vests and ballistic armor or anything with ABLATIVE_LARGE / ABLATIVE_MEDIUM.

### Transform vs Durability
#### Explanation
Normally armor degrades with use. This degradation is incremental and decreases the armors effectiveness and protection. However not all armor degrades, some instead will become completely compromised. For these items ```"non_functional": "ITEM_ID"``` can be added. Instead of the normal degradation rules, armor with non_functional will have a chance to transform into its non_functional version when struck. Specifically transform items are concerned with how much damage they take before reduction, rather than after reduction and as the damage scales towards their total resistance (and beyond) scales to a 33% chance to transform (and beyond). Example a 50 damage bullet hitting a 50 damage plate has a 33% chance to transform, a 25 damage bullet would only have a 16.5% chance to cause a transform, 100 damage a 66% chance.

Transforming items can also specify a custom destruction message with ```"damage_verb"``` which is what will be said when it is damaged.

#### Example
```json
{
  "id": "esapi_plate",
  "type": "ARMOR",
  "category": "armor",
  "name": { "str": "ESAPI ballistic plate" },
  "description": "A polygonal ceramic ballistic plate with a slightly concave profile.  Its inner surface is coated with Ultra High Molecular Weight Polyethylene, and is labeled \"TOP\", while its outer surface is labeled \"STRIKE FACE\".  This is intended to be worn in a ballistic vest and can withstand several high energy rifle rounds before breaking.",
  "weight": "2500 g",
  "volume": "1533 ml",
  "price": "600 USD",
  "price_postapoc": "1 USD",
  "material": [ "ceramic" ],
  "symbol": ",",
  "color": "dark_gray",
  "material_thickness": 25,
  "non_functional": "destroyed_large_ceramic_plate",
  "damage_verb": "makes a crunch, something has shifted",
  "flags": [ "ABLATIVE_LARGE", "CANT_WEAR" ],
  "armor": [ { "encumbrance": 2, "coverage": 45, "covers": [ "torso" ], "specifically_covers": [ "torso_upper" ] } ]
}
```

#### Further Reading
The MIGO ablative plates use this to slag off and grow back. As well as all other old school ceramic plates. Easily searchable in the code with "non_functional".

### Actions and Transforms
#### Explanation
Items with actions can be worn as armor. Usually you need to swap the type to ```TOOL_ARMOR```. Actions are outside the scope of this doc but a simple thing possible is a transform. The basic use case for this is something like a headlamp that can be turned on to provide light. Usually on versions of items will drain power but have additional flags and effects.

#### Example
```json
{
  "id": "survivor_light",
  "type": "TOOL_ARMOR",
  "category": "clothing",
  "symbol": "[",
  "color": "blue",
  "name": { "str": "survivor headlamp" },
  "description": "This is a custom-made LED headlamp reinforced to be more durable, brighter, and with a larger and more efficient battery pack.  The adjustable strap allows it to be comfortably worn on your head or attached to your helmet.  Use it to turn it on.",
  "price": "65 USD",
  "price_postapoc": "2 USD 50 cent",
  "material": [ "plastic", "aluminum" ],
  "flags": [ "OVERSIZE", "BELTED", "ALLOWS_NATURAL_ATTACKS" ],
  "weight": "620 g",
  "volume": "500 ml",
  "melee_damage": { "bash": 1 },
  "charges_per_use": 1,
  "ammo": "battery",
  "use_action": {
    "type": "transform",
    "msg": "You turn the survivor headlamp on.",
    "target": "survivor_light_on",
    "active": true,
    "need_charges": 1,
    "need_charges_msg": "The survivor headlamp batteries are dead."
  },
  "material_thickness": 1,
  "pocket_data": [
    {
      "pocket_type": "MAGAZINE_WELL",
      "rigid": true,
      "flag_restriction": [ "BATTERY_LIGHT", "BATTERY_ULTRA_LIGHT" ],
      "default_magazine": "light_plus_battery_cell"
    }
  ],
  "armor": [ { "coverage": 20, "covers": [ "head" ] } ]
},
{
  "id": "survivor_light_on",
  "copy-from": "survivor_light",
  "type": "TOOL_ARMOR",
  "name": { "str": "survivor headlamp (on)", "str_pl": "survivor headlamps (on)" },
  "description": "This is a custom-made LED headlamp reinforced to be more durable, brighter, and with a larger and more efficient battery pack.  The adjustable strap allows it to be comfortably worn on your head or attached to your helmet.  It is turned on, and continually draining batteries.  Use it to turn it off.",
  "flags": [ "LIGHT_350", "CHARGEDIM", "OVERSIZE", "BELTED", "ALLOWS_NATURAL_ATTACKS" ],
  "power_draw": "10 W",
  "revert_to": "survivor_light",
  "use_action": {
    "ammo_scale": 0,
    "type": "transform",
    "menu_text": "Turn off",
    "msg": "The %s flicks off.",
    "target": "survivor_light"
  }
}
```

#### Further Reading
Basically any "TOOL_ARMOR"

### Enchantments / Traits

#### Explanation
Some armor with custom abilities can be handled as enchantments. This is a new way of handling stuff that is more JSON friendly and doesn't require hard coded flags in C++. The best way to implement an effect on armor is to **not add the effect directly** instead it is for the armor to provide a trait which provides the effect. **this is so if you wear two of the armor it won't give double the effect**. As well it makes it obvious to the player that they have a new trait giving them power.

#### Example
```json
"relic_data": { "passive_effects": [ { "has": "WORN", "condition": "ALWAYS", "mutations": [ "well_distributed" ] } ] }
```

```json
{
  "type": "mutation",
  "id": "well_distributed",
  "name": { "str": "Well Distributed Gear" },
  "points": 0,
  "description": "You are wearing equipment that helps to distribute weight allowing you to carry more.",
  "valid": false,
  "purifiable": false,
  "types": [ "Equipment" ],
  "enchantments": [ { "values": [ { "value": "CARRY_WEIGHT", "multiply": 0.1 } ] } ]
}
```

#### Further Reading
The Nomad Jumpsuits use this to provide well distributed.

### Notable Flags
This is a hopefully exhaustive list of flags you may wish to use on items and what they do.

#### Layers
ID        | Description
--        | --
PERSONAL  | On this layer
SKINTIGHT | On this layer
NORMAL    | On this layer
WAIST     | On this layer
OUTER     | On this layer
BELTED    | On this layer
AURA      | On this layer


#### Clothing stuff
ID               | Description
--               | --
VARSIZE          | Item may not fit you, if it fits encumbrance values are halved compared to defined values.
STURDY           | Armor is much less likely to take damage and degrade when struck
NO_REPAIR        | Can't be repaired by the player using tools like the sewing kit, or welder
WATER_FRIENDLY   | Armor makes the covered body parts not feel bad to be wet
WATER_PROOF      | Makes the body parts immune to water
RAIN_PROOF       | Wont get wet in rain
HOOD             | Keeps head warm if nothing on it
POCKETS          | Keeps hands warm if they are free
BLOCK_WHILE_WORN | Can be used to block with while worn
COLLAR           | Keeps mouth warm if not heavily encumbered
ONLY_ONE         | Only one of this item can be worn
ONE_PER_LAYER    | Only one item can be worn on this clothing layer
FANCY            | Clothing is impractically fancy, (like a top hat)
SUPER_FANCY      | Even more fancy than fancy
FILTHY           | Disgusting dirty clothes
FRAGILE          | Breaks fast
SLOWS_MOVEMENT   | Slows you down to wear

#### Immunities/Defenses
**only some of these that are used**

ID                | Description
--                | --
ELECTRIC_IMMUNE   | Immunity
BIO_IMMUNE        | Immunity
BASH_IMMUNE       | Immunity
CUT_IMMUNE        | Immunity
BULLET_IMMUNE     | Immunity
ACID_IMMUNE       | Immunity
STAB_IMMUNE       | Immunity
HEAT_IMMUNE       | Immunity
GAS_PROOF         | Immunity
RAD_PROOF         | Immunity
PSYSHIELD_PARTIAL | Partial mind control protection
RAD_RESIST        | Partial rads protection
SUN_GLASSES       | Protects from suns glare

#### Mutation stuff
ID                     | Description
--                     | --
OVERSIZE               | Can be worn by larger Characters
UNDERSIZE              | Can be worn by smaller Characters
ALLOWS_TAIL            | People with tails can still wear it
ALLOWS_TALONS          | People with talons can still wear it
ALLOWS_NATURAL_ATTACKS | Wont hinder special attacks

#### Sci-fi
ID                    | Description
--                    | --
ACTIVE_CLOAKING       | Makes you invisible
CLIMATE_CONTROL       | Keeps the character at a safe temperature
NO_CLEAN              | Can't be cleaned no matter what you do (combine with filthy)
SEMITANGIBLE          | Can be worn with other stuff on layer
POWERARMOR_COMPATIBLE | Can be worn with power armor on
NANOFAB_REPAIR        | Can be repaired in a nanofabricator
DIMENSIONAL_ANCHOR    | Provides nether protection
PORTAL_PROOF          | Provides protection from portals
GNV_EFFECT            | Night Vision
IR_EFFECT             | Infrared Vision

#### Ablative
ID              | Description
--              | --
CANT_WEAR       | Armor can't be worn directly (**all ablative plates should have this**)
ABLATIVE_LARGE  | Large ablative plates that fit in the front of standard ballistic vests
ABLATIVE_MEDIUM | Side ablative plates that fit in the sides of standard ballistic vests
ABLATIVE_MANTLE | Shoulder, arm and torso armor used by Hub 01
ABLATIVE_SKIRT  | Hip and thigh armor used by Hub 01

#### Situational
ID               | Description
--               | --
DEAF             | Can't hear anything
FLASH_PROTECTION | Protects from flashbangs
FLOTATION        | Causes you to float
FIN              | Makes you swim faster
PARTIAL_DEAF     | Protects from loud sounds while you can hear other sounds
NO_WEAR_EFFECT   | Lets players know this item gives no benefits (used on jewelry)
PALS_SMALL       | Can be incorporated into a LBV / ballistic vest (takes 1 slot)
PALS_MEDIUM      | Can be incorporated into a LBV / ballistic vest (takes 2 slot)
PALS_LARGE       | Can be incorporated into a LBV / ballistic vest (takes 3 slot)
ROLLER_ONE       | Heelies
ROLLER_INLINE    | Inline roller blades
ROLLER_QUAD      | Quad roller blades
SWIM_GOGGLES     | Lets you see under water

## Making your own items
When you want to add a new item to Cataclysm there are a few things you should prioritize, focus on and consider.

### Balance/Considerations

#### The Golden Rule Of Balance
The core ideal when making any item but especially armor is that **weight, thickness and materials should be correct** if the item is real it should be exactly as it is in real life, if it is created for cataclysm it should be similar to other true to life armors. It does not matter if you think that your pet project leather armor *should* be better, if a leather jacket is 1.2mm thick and that gives it 4.4 bash protection and you think that is too low **you can not just make it thicker**. Armor should be based off real life thickness values. If after making something true to life it isn't behaving true to life then a discussion about how to solve it can be had. A new material might need to be defined (we have 3 types of Kevlar and 3-4 types of plastic) or perhaps your assumptions / materials are wrong. Any solution however should come with proper research since it will have long reaching effects. The reasoning for this golden rule is it consolidates balancing at the material level making it much easier to research and get info and leads to less favoritism and outliers.

#### The Golden Rule Of Design
Make things as simple as they possibly can be. When a player inspects a t-shirt they shouldn't have to inspect 35 lines of armor data that could be summarized as "basically no protection, keeps you kinda warm". Advanced and dedicated armor can be *very* complicated but try to keep clothing as complex as they need to be to be simulated correctly.

#### Subjective Values
With the above mentioned you should be able to intuit/research sensible materials, thicknesses, flags, and weight.  Your core variables that are more discretionary are warmth, encumbrance, layer and coverage.

#### Layer
Layer should be based on real world intuit however thickness also matters. The loose rule, erring on the side of generosity is (at least for arms, legs and torso)
* skintight 1mm or less
* normal 2mm or less
* outer no restrictions (within reason obviously)

#### Warmth
Warmth should be based on materials, look at armors similar to what you are making and go from there. Also it is worth considering how much of the body is covered.

#### Coverage
Coverage has a big rule on the core team of **No hard, powerful armors with 100% coverage**. This can be subverted slightly by using covered_by_mat to make it so the armors best protection doesn't cover 100% of the body but limbs need to bend so no part of the body can really have 100% coverage with a hard high protection material. Further any armor that **does** have 100% coverage with great protection from hard materials should either be super sci-fi, or made to order. Super covering protective plate mail was made tailored to a persons body in a way mass manufacture gear isn't.

Something a lot of people don't consider is that an armor with 95% coverage is **4 times better** than an armor with 80% coverage when it comes to protecting against attacks.

When balancing coverage try to describe what you would see. Doing multiple armor entries per sub-limb can help with this. "well this covers all the elbows and forearms and has some plastic covering 75% of that". Then give it 100% coverage on the elbow and lower arm sub-limbs with covered by mat plastic at 75. Then the game is smart enough to figure out how much of the overall arm that all is.


#### Encumbrance
This can be very subjective. The main things to keep in mind are.
1. if something is VARSIZE it's values in game will be halved, balance for that.
2. the further from your core, the torso, weight is the more encumbering it should be.
3. the more coverage something has the more encumbering it should be.
4. joints should be penalized more heavily than other locations.
5. the heavier something is the more encumbering it should be.
6. the more uncomfortable and poorly designed something is the more encumbering it should be.

So you will find lots of
* negligible encumbrance (< 2) clothing with almost no protection and 90% to 100% coverage
* low encumbrance (< 5) clothing made of soft materials with just okay protection and 90% to 100% coverage
* low - medium (< 10) encumbrance modern armor with good protection and low 80% coverage
* high (> 15) encumbrance traditional armor with good protection and high 95%+ coverage

Aside from super sci-fi outliers everything should realistically fall into those categories.

When doing encumbrance per volume you should use volume_encumber_modifier. Some good modifiers to use are:
* .4+ poorly designed/improvised storage
* .4 stuff not well fixed to you and hanging like rifles on slings
* .3 reasonable value for armor and strapped pouches designed for combat, but accessible.
* .2 very well affixed pouches (should have a higher base encumbrance on the item to compensate)
* 0 all ablative pockets (their encumbrance is added from their armor directly)

also keep in mind that, for example, if something covers the legs and the torso you can split that ".3" over the legs and torso so the torso becomes ".15" and the legs are each ".1". This probably should never be perfect unless it is an excellently designed storage system.

Also the above values assume things being focused around the torso, shoulders, hips and thighs for the same reason mentioned above the further out from your center something is the worse it should be.

### What 'Just Works'
Aside from what is described in this guide a lot of the behind the scenes calculations are just handled for you. So for example the info displayed in game per limb is an amalgamation of all of your sub-limb entries consolidated. This is just to simplify a single display to the player but is still very useful.

## FAQ
Space left for questions as they arise.

## Looking Forward
In the future ideally
* Warmth will be handled by material + thickness, or at least scaled by armor coverage.
* Encumbrance will be entirely handled by material, thickness, 1 - 2 descriptive words, sub-location coverage, coverage (using the 6 tenets from above).
* Characters will be able to wear human armor on non human limbs with scaling coverage based on how closely their limbs approximate the sub-limbs that the armor normally covers.
* All old armors will be overhauled to have better and more detailed information.
* More material refinement.
