# Armor Overview
Armor in cataclysm has varying levels of complexity to attempt a deep simulation of worn clothing. Compared with systems that try to compress down armor to one or two numbers like Pen And Paper game armor class systems or CRPGs with systems like DT and DR (flat and multiplicative reduction) cataclysm attempts to more accurately simulate different materials, their relative coverage of the body, and how that interacts with the player.  A lot of this is simplified or clarified to the player in a way that is digestible. We *shouldn't* hide info from the player but we should *avoid* information overload.

That said this document will practice no such restraint and is designed to be a complete guide to *everything* possible. If you want clear examples of the code in use you can look at: !!LIST SOME ITEM IDS!!. This guide will cover the basic ideas of **what is necessary** for armor to function, **what is possible** with the armor system and **how to design armor** so that it is balanced and makes sense. An **FAQ** will also be included to cover some specifics people might have about armor or how to understand it as well as a section on things to be added **looking forward**.  Each section will also give specific example item ids you could investigate for examples (as well as code snippets) or for balance considerations.

## What is necessary
This is a basic overview of things armors basically **need** to have to work as expected, or at all.  Sometimes an advanced feature will circumvent or overwrite the things provided here.

All armors need:
* Everything a normal item does. I'm not explaining this
* ```"material"``` array: a list of the materials that make up the item in general.
* ```"armor"``` array: this will be described in depth later.
* ```"warmth"``` value, usually between 0 - 50 based on how hot the item is to wear.
* ```"material_thickness"``` this is measured in mm, this **the most important value to get right when designing armor**

here is an example armor to look at:
``` json
{
  "id": "dress_shirt",
  "repairs_like": "tshirt",
  "type": "ARMOR",
  "name": { "str": "dress shirt" },
  "description": "A white button-down shirt with long sleeves.  Looks professional!",
  "weight": "250 g",
  "volume": "750 ml",
  "price": 1500,
  "price_postapoc": 50,
  "material": [ "cotton" ],
  "symbol": "[",
  "looks_like": "longshirt",
  "color": "white",
  "armor": [
    { "covers": [ "torso" ], "coverage": 90, "encumbrance": [ 4, 5 ] },
    { "covers": [ "arm_l", "arm_r" ], "coverage": 90, "encumbrance": [ 4, 4 ] }
  ],
  "warmth": 10,
  "material_thickness": 0.1,
}
```

## What is possible
This section is purely about what you can do. Basically no armor should have all of these features. Remember *the more complexity you add to something the harder it is to understand*.

Each subsection of this will consist of an **explanation of the feature**, an **example of how to use it**, and **further examples from the code**

### Layers
#### Explanation

#### Example

#### Further Reading

### Sublocations
#### Explanation
Sublocations are a new-ish feature of characters in Cataclysm. Sub-locations are sub divided pieces of characters limbs. For example a human arm consists of a lower arm, elbow, upper arm, and shoulder. All limbs have sublocations and you can find the full list of them in the bodypart definitions in ```body_parts.json```. When you define specific sublocations for armor it does the following:
* Makes it so that armor only conflicts with armor that shares sublocations with it on it's layer
* Scales total coverage

**if you don't define this for an armor it assumes that the armor covers every sublocation on that limb**

This will play more and more of a role as armor gets further developed.

#### Example

#### Further Reading

### Materials

### Notable Flags

### Advanced Armor Definitions
double material low coverage for multiplicative protection,


### Multi Material Inheritance

### Melee and Ranged Coverage

### Enchantments / Traits

### Pockets

### Ablative Armor

### Transform vs Durability

### Multi-layer


## Making your own items

### Balance
Sublimb distance, total coverage, thickness SERIOUSLY, material considerations.

### Considerations
Warmth, Relative vs. Total Coverage

### What 'Just Works'
Limb amalgamation

## FAQ

## Looking Forward
