# Smashing

Smashing is a feature that allows dealing damage to furniture, terrain, and fields on the map.

The "hp" for these objects is determined by subtracting the appropriate `str_max` value from the `str_min` value.

When one of these is smashed, the damage types of the characters weapon are applied to the damage profile of the bashed object to determine damage points. For terrain and furniture, this damage is stored, and once enough damage has been done, it will be destroyed. For fields, it is a (damage/hp) chance.

## Damage Profile

A damage profile defines multipliers for converting each damage type into damage dealt in a smash action.

```jsonc
{
  "id": "my_profile",
  "type": "bash_damage_profile",
  "profile": { "bash": 1.0, "cut": 0.5, "stab": 2.0 }
}
```

Damage points are determined by multiplying the damage of each type of the weapon against the multiplier defined in `profile`, subtracting the `str_min`, then summing.

For damage types not specified, the multiplier is determined by the bash conversion factor specified by the damage type.
