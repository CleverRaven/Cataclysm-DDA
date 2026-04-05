# Wounds

Wound is a type, that affects specific bodyparts. It's similar in this to effects, but it can be applied only on specific bodyparts, and their ability to be aided/repaired using wound_fix

### wound

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
    "blacklist_bp_with_flag": "BIONIC_LIMB", // Bodyparts with this flag cannot receive this wound.
  }
```

### wound_fix

Wound fix is a way for wounds to be treated

```jsonc
  {
    "id": "bandage_the_bleeding",
    "type": "wound_fix",
    "name": "bandage the bleeding",
    "description": "Foobar.",
    "time": "2 hours", // how long it takes to apply this wound fix
    "skills": { "mechanics": 1 }, // skills required to apply fix
    "wounds_removed": [ "scratch" ], // wounds removed when this fix is applied
    "wounds_added": [ "scratch" ], // wounds added when fix is applied
    "success_msg": "Tada.",
    "requirements": [ [ "gun_cleaning", 1 ] ], // requirements that dictate tools and materials needed to apply this fix
    "proficiencies": [
      {
        "proficiency": "prof_wound_care", // id of prof required
        "time_save": 0.9, // multiplies the would fix time by this amount; default 1.0
        "is_mandatory": true  // if true, lacking this prof would prevent you from using this wound fix
      }
    ]
  }
```