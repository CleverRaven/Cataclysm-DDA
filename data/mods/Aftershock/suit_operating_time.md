# Powered Armor Balance

These are guidelines for designing all sorts of Aftershock power armor.

## Operating time 

We define `operating time` as the amount of time the suit/gear-piece takes to consume 1000 charges of batteries. The following factors, totaled, determine how long the suit should work between battery swaps.


#### Additional factors
 |      Extra Hours      |     Condition                                                           | 
 |-----------------------|-------------------------------------------------------------------------|
 |        4 hours        | Base operating time. All suits work for at least 4 hours per full charge |
 |-----------------------|-------------------------------------------------------------------------|
 | 1hour per every 5 enc | Count only the most encumbered part                                     |
 |        1 hours        | Less than 40 armor against all hazards                                  |
 |        1 hours        | Less than 20 armor against all hazards                                  |
 |        6 hours        | Less than 10 armor against all hazards                                  |
 |        2 hours        | Has the fragile tag or is made from a very fragile material (ex: glass) |
 |        4 hours        | Less than 90% coverage of the most encumbered part                      |
 |        4 hours        | No protection against gases                                             |
 |        12 hours       | No protection against the cold                                          |
 |        2 hours        | No protection against other environmental factors (ex: acid, electricity, radiation)  |
 |        4 hours        | Gear doesn't give passive bonuses to physical attributes (ex: dex or str) or skills (ex: athletics) and won't grant physical-based proficiencies|
 |        4 hours        | Doesn't grant any combat/movement related spells or enchantments         |
 |        4 hours        | Gear doesn't give passive bonuses to mental attributes (per or int) or skills (ex: applied science) and won't grant knowledge based proficiencies |
 |        1 hours        | Doesn't grant any mental/knowledge related spells or enchantments        |
 |        2 hours        | Has a non-swappable integral battery.                                   |
 |        2 hours        | Won't grant night/heat-vision or clairvoyance effects.                   |

Once you have determined the operating time using the table above, simply divide the number 277777.78 by the indicated number of hours to obtain your new suit's `power_draw` json value.

For player convenience, the power draw and battery capacity of all clothing pieces that comprise a single matching outfit (e.g., the Magellan exosuit and its helmet) should be the same. 

#### Example Calculation

Consider the Magellan exosuit, which has the following definition:
```
  {
    "id": "afs_magellan_suit",
    "type": "TOOL_ARMOR",
    "category": "armor",
    "name": { "str": "Magellan exosuit" },
    "description": "A high-quality, civilian grade EVA suit often employed by well-established frontier research and exploration associations.  Designed to support the exploration of challenging terrain, it offers respectable protection against common environmental hazards like extreme temperatures, inhospitable atmospheres, and light radiation.  It leaves arms and hands relatively unencumbered to aid the manipulation of scientific instruments.\n\nAn integral battery allows the suit to operate for up to 34 hours, but complicates field recharging.",
    "weight": "7800 g",
    "volume": "14 L",
    "price": "4 kUSD",
    "material": [ "nomex", "steel" ],
    "symbol": "[",
    "looks_like": "robofac_enviro_suit",
    "color": "light_gray",
    "ammo": "battery",
    "charges_per_use": 1,
    "use_action": {
      "type": "transform",
      "msg": "You activate your %s.",
      "target": "afs_magellan_suit_on",
      "active": true,
      "need_charges": 1,
      "need_charges_msg": "The %s's batteries are dead."
    },
    "armor": [
      { "covers": [ "torso" ], "coverage": 100, "encumbrance": 25 },
      { "covers": [ "leg_l", "leg_r" ], "coverage": 100, "encumbrance": 25 },
      { "covers": [ "arm_l", "arm_r" ], "coverage": 100, "encumbrance": 15 },
      { "covers": [ "hand_l", "hand_r" ], "coverage": 100, "encumbrance": 10 },
      { "covers": [ "foot_l", "foot_r" ], "coverage": 100, "encumbrance":  15 }
    ],
    "pocket_data": [
      { "pocket_type": "MAGAZINE", "rigid": true, "ammo_restriction": { "battery": 1000 } }
    ],
    "warmth": 20,
    "material_thickness": 2,
    "valid_mods": [ "steel_padded" ],
    "environmental_protection": 15,
    "flags": [ "VARSIZE", "WATERPROOF", "GAS_PROOF", "POCKETS", "RAINPROOF", "STURDY", "RAD_RESIST",  "RECHARGE",  "OUTER", "NO_RELOAD", "NO_UNLOAD" ]
  },

```
The active version provides no additional bonuses except for cold protection.

Evaluating the performance time using the table above, the result should be:

| Suit Qualifies |      Extra Hours      |     Condition                                                           | 
|----------------|-----------------------|-------------------------------------------------------------------------|
|      yes       |        4 hours        | Base operating time. All suits work for at least 4 hours per full charge |
|----------------|-----------------------|-------------------------------------------------------------------------|
|      yes       |        5 hours        | Count only the most encumbered part                                     |
|      yes       |        1 hours        | Less than 40 armor against all hazards                                  |
|      yes       |        1 hours        | Less than 20 armor against all hazards                                  |
|      yes       |        6 hours        | Less than 10 armor against all hazards                                  |
|      no        |        2 hours        | Has the fragile tag or is made from a very fragile material (ex: glass) |
|      no        |        4 hours        | Less than 90% coverage of the most encumbered part                      |
|      no        |        4 hours        | No protection against gases                                             |
|      no        |        12 hours       | No protection against the cold                                          |
|      no        |        2 hours        | No protection against other environmental factors (ex: acid, electricity, radiation)  |
|      yes       |        4 hours        |Gear doesn't give passive bonuses to physical attributes (ex: dex or str) or skills (ex: athletics) and won't grant physical-based proficiencies|
|      yes       |        4 hours        | Doesn't grant any combat/movement related spells or enchantments         |
|      yes       |        4 hours        | Gear doesn't give passive bonuses to mental attributes (per or int) or skills (ex: applied science) and won't grant knowledge based proficiencies  |
|      yes       |        1 hours        | Doesn't grant any mental/knowledge related spells or enchantments        |
|      yes       |        2 hours        | Has a non-swappable integral battery.                                   |
|      yes       |        2 hours        | Won't grant night/heat-vision or clairvoyance effects,                  |
|----------------|-----------------------|-------------------------------------------------------------------------|
| Total hours    |       34 hours        |                                                                         |

Due to the fulfilled conditions, the suit should operate for 34 hours when connected to a 1000 charge battery. To calculate its final `power_draw` value, we divide 277777.78 by 34 to obtain a `power_draw` of 8170.