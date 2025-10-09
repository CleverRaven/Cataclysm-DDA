# Wounds

Wound is a type, that affects specific bodyparts. It's similar in this to effects, but it can be applied only on specific bodyparts, and their ability to be aided/repaired using (at the moment of writing, yet to be added) wound_fix

### Wound

```jsonc
  {
    "id": "scratch",
    "type": "wound",
    "name": { "str": "scratch", "str_pl": "scratches" },
    "description": "Foobar.",
    "weight": 10, // weight of a wound, determines the chance of this specific wound to be picked when limb takes damage. Default 1
    "damage_types": [ "cut", "bash" ], // only taking these type of damage can apply wound. Mandatory
    "damage_required": [ 1, 1000 ], // smallest and highest damage that is required for this wound to be applied. Mandatory
    "pain": [ 1, 10 ], // when wound is applied, it would give character this random amount of pain rolled between this two numbers. Default 0
    "healing_time": [ "2 hours", "25 days" ], // how long this wound need time to be fully healed. Rolled randomly when applied, supposed to be adjusted by wound_fix.
            // Default infinite duration
    "whitelist_body_part_types": [ "leg", "arm" ], // if used, this wound can be applied only at bodyparts of this type. Possible values are: head, torso, sensor, mouth, arm, hand, leg, foot, wing, tail, other
    "blacklist_body_part_types": [ "torso", "sensor" ], // if used, this wound cannot be applied on bodyparts of this type.
    "whitelist_bp_with_flag": "LIMB_UPPER", // only body parts with this flag can receive the wound.
    "blacklist_bp_with_flag": "CYBERNETIC_OR_WHATEVER_IT_DOESNT_EXIST_YET", // Bodyparts with this flag cannot receive this wound.
  }
```

