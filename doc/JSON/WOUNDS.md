# Wounds

Wound is a type, that affects specific bodyparts. It's similar in this to effects, but it can be applied only on specific bodyparts, and their ability to be aided/repaired using wound_fix.
Bad wounds should, generally, affect a lot of limb_scores. Most open untreated wounds shouldn't heal and should get infected when the ability for wounds to get infected/turn into another wound with time is added.

Wounds should generally be tiered into: small/light, medium, bad. You should append _"tier" at the end of the id. The last tier is bad because small medium and bad do not neccesarily refer to how large the wound is. a bad stab wound be a deep stab wound, same for a bad cut.

A light wounds should generally have minimal effects, a medium would should hinder you but not too much, a single bad wound should be a significant hinderance. Some wounds like broken bones should hinder you extremely.

### wound

```jsonc
  {
    "id": "scratch",
    "type": "wound",
    "name": { "str": "scratch", "str_pl": "scratches" },
    "description": "Foobar.",
    "weight": 10, // weight of a wound, determines the chance of this specific wound to be picked when limb takes damage. Default 1
    "limit": 2, // limits amount of wounds of this type you can get per limb
    "damage_types": [ "cut", "bash" ], // taking these type of damage can apply wound
    "damage_required": [ 1, 1000 ], // smallest and highest damage that is required for this wound to be applied
    "pain": [ 1, 10 ], // when wound is applied, it would give character this random amount of pain rolled between this two numbers. Default 0
    "healing_time": [ "2 hours", "25 days" ], // how long this wound need time to be fully healed. Rolled randomly when applied, supposed to be adjusted by wound_fix.
            // Default infinite duration
    "wound_progression": [ { "id": "deep_scratch", "chance": 10 } ], // taking damage on bodypart with this limb may worsen it. chance is range from 0% to 100%
    "limb_scores": [ { "score": "lift", "value": 0.25 } ], // if wounded part has this limb score, it will be decreased by this amount
    "progress_timer": {   // this wound may transform into another wound over time
      "id": "scratch_medium",
      "time": [ "2 minutes", "4 minutes" ],
      "message": "Suddenly the scratch on your %s got wider!" // argument %s is body part name
    },
    "mod_values": [ // defines variety of effect that happen over time while wound is presented
      {
        "type": "PAIN", // type of said effect. Currently supported are PAIN (increases the pain value of wound until treated), BLEED (drips some blood on the ground) and VITAMIN (decreases specific vitamin in character body)
        "rate": "2 seconds", // how often said effect happens
        "amount": 3, // when happens, how big the change would be? +3 pain every 2 seconds, in case of this particulare object
        "cap_min": -20, // if used, value will not be decreased beyond this value. Default INT_MIN
        "cap_max": 20 // if used, value will not be increased beyond this value, Default INT_MAX
      },
      { "type": "VITAMIN", "vitamin_id": "redcells", "rate": "1 seconds", "amount": -1 } // if VITAMIN type, `vitamin_id` is used to specify the vitamin that should be modified
    ],
    "periodic_message": { // used to send messages into log when such wound is presented. Reserve for the most important one
      "message": "OH NO YOUR WOUND ON YOUR %s IS BLEEDING CLOSE IT CLOSE IT CLOSE IT!", // argument %s is body part name
      "rate": "2 seconds"
    },
    "whitelist_flags": [ "FLAG_A", "FLAG_B" ], // only character that has any of these flags can receive this wound
    "blacklist_flag": [ "FLAG_C", "FLAG_D" ], // if chartacter has any of these flags, they cannot receive this wound
    "whitelist_body_part_types": [ "leg", "arm" ], // if used, this wound can be applied only at bodyparts of this type. Possible values are: head, torso, sensor, mouth, arm, hand, leg, foot, wing, tail, other
    "blacklist_body_part_types": [ "torso", "sensor" ], // if used, this wound cannot be applied on bodyparts of this type.
    "whitelist_bp_with_flag": [ "LIMB_UPPER" ], // only body parts with this flags can receive the wound.
    "blacklist_bp_with_flag": [ "BIONIC_LIMB" ], // Bodyparts with this flags cannot receive this wound.
  }
```

### wound_fix

Wound fix is a way for wounds to be treated. You should append _fix to the end of a wound fix.

Wound fixes support everything recipes support.

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
    "mod_hp": 10, // if used, the HP of bodypart will be healed by this amount when wound fix is used; negative number will damage the limb instead 
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
