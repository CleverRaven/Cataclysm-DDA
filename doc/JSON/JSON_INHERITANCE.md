# JSON Inheritance

To reduce duplication in the JSON data, it is possible for _some_ JSON `type`s to inherit from existing objects.  Some restraint should be used, see [Guidelines](#guidelines) section.  It is important to note that inheritance is not always completely standard between `type`s.  See the [Behavior](#behavior) section for further details.

## Examples

Items are one of the most common JSON types, so the "subtypes" field splits data into optional chunks. See [the item docs](ITEM.md#subtypes) for an explanation of how it works.

In the following example, `556` ammo is derived from `223` ammo via `copy-from`:

```jsonc
  {
    "id": "223",
    "type": "ITEM",
    "subtypes": [ "AMMO" ],
    "name": { "str_sp": ".223 Remington" },
    "description": ".223 Remington ammunition with a 55gr FMJ-BT bullet, patterned after the M193 loading.  The .223 round has been very popular with civilian shooters for almost a century, finding use in a wide variety of weapons.  The combination of high velocity with a thinly-jacketed, cannelured projectile led to unexpected yaw and fragmentation in soft tissue, increasing this round's performance.",
    "ascii_picture": "223",
    "weight": "12 g",
    "volume": "194 ml",
    "longest_side": "57 mm",
    "price": "2 USD 80 cent",
    "price_postapoc": "9 USD",
    "flags": [ "IRREPLACEABLE_CONSUMABLE" ],
    "material": [ "brass", "lead", "powder" ],
    "//": "`symbol` through `range` omitted for brevity",
    "damage": {
      "damage_type": "bullet",
      "amount": 39,
      "armor_penetration": 2,
      "barrels": [
        "... omitted for brevity"
      ]
    },
    "dispersion": 30,
    "dispersion_modifier": [ { "barrel_length": "337 mm", "dispersion": 30 }, { "barrel_length": "533 mm", "dispersion": 0 } ],
    "recoil": 1500,
    "effects": [ "COOKOFF" ]
  },
  {
    "id": "556",
    "copy-from": "223",
    "type": "ITEM",
    "subtypes": [ "AMMO" ],
    "name": { "str_sp": "5.56 NATO M855" },
    "material": [ "steel", "brass", "lead", "powder" ],
    "description": "5.56x45mm ammunition with a 62gr FMJ bullet containing a steel penetrator.  Originally designed for the M249 FN Minimi, M855 was designed to penetrate steel helmets and light cover at range, sacrificing terminal performance and accuracy in the process.  The limited yaw and fragmentation is worsened with shorter-barreled carbines shooting this round, and optimization of barrel twist for stabilization of the tracer variant of M855 has resulted in lackluster accuracy overall.",
    "ascii_picture": "556",
    "price": "2 USD 90 cent",
    "price_postapoc": "9 USD",
    "flags": [ "IRREPLACEABLE_CONSUMABLE" ],
    "relative": { 
      "damage": { "damage_type": "bullet", "amount": -3, "armor_penetration": 10 }, 
      "dispersion": 20 
    },
    "dispersion_modifier": [ { "barrel_length": "337 mm", "dispersion": 120 }, { "barrel_length": "533 mm", "dispersion": 0 } ],
    "proportional": { "recoil": 1.1 },
    "extend": { "effects": [ "NEVER_MISFIRES" ] }
  },
```

For `"type": "ITEM"`, the following rules apply:

* Missing fields, such as `weight`, `volume`, `material` and so on have the same value as the parent.
* Fields explicitly specified replace those of the parent type.  The above example replaces `name`, `description` and `price`.
* Numeric values may be specified `relative` to the parent.  For example `556` has less `damage` but more `armor_penetration` than `223` and will maintain this relationship if the definition for `223` is changed.
  * Note the syntax for fields that support objects: `damage` is still defined as an object for `relative`, with missing fields having the same value as the parent.
* Flags can be added via `extend`.  For example `556` is military ammo and gains the `NEVER_MISFIRES` ammo effect.  Any existing flags specified from `223` are preserved.
* The entry you copied from must be of the same `type` as the item you added or changed.  Not all `type`s are supported, and not all are supported in the same way.  See [Support](#support) and [Behavior](#behavior) below).


Another example.  Reloaded ammo is derived from the factory equivalent but with a 10% penalty to `damage` and `dispersion` and a chance to misfire.  Additional rules apply:

```jsonc
  {
    "id": "reloaded_556",
    "copy-from": "556",
    "type": "ITEM",
    "subtypes": [ "AMMO" ],
    "name": { "str_sp": "5.56 NATO, reloaded" },
    "proportional": { 
      "price": 0.7, 
      "damage": { "damage_type": "bullet", "amount": 0.9 }, 
      "dispersion": 1.1 
    },
    "extend": { "effects": [ "RECYCLED" ] },
    "delete": { "effects": [ "NEVER_MISFIRES" ], "flags": [ "IRREPLACEABLE_CONSUMABLE" ] }
  },
```

* Chained inheritance is possible.  For example, `reloaded_556` inherits from `556`, which is itself derived from `223`.
* Numeric values may be specified as `proportional` to the parent by via a decimal factor.  Here `reloaded_556` deals 90% of the damage of the factory equivalent with 110% dispersion.
* Flags can be deleted via `delete`.  It is not an error if the deleted flag does not exist in the parent.


Not all `type`s work the same.  For `"type": "MONSTER"`, `relative` uses different fields than `"type": "ITEM"`:

```jsonc
    "//": "MONSTER uses the same melee_damage field that ITEM does, but also has "melee_dice" and "melee_dice_sides",
    "relative": { 
      "melee_dice": 1, 
      "melee_dice_sides": 5, 
      "melee_damage": [ { "damage_type": "cut", "amount": 2 } ] 
	  },
```

Same as above, now with `proportional`:

```jsonc
    "proportional": { "hp": 1.5, "speed": 1.5, "attack_cost": 1.5, "melee_damage": [ { "damage_type": "cut", "amount": 0.8 } ] },
```


It is possible to define an `abstract` ID that exists only for other `type`s to inherit from and cannot itself be used in game.  This is done to facilitate maintenance and reduce line count.  In the following condensed example, `magazine_belt` provides values common to all implemented ammo belts:

```jsonc
{
    "abstract": "magazine_belt",
    "type": "ITEM",
    "subtypes": [ "ARMOR", "MAGAZINE" ],
    "name": { "str": "ammo belt" },
    "description": "An ammo belt consisting of metal linkages which disintegrate upon firing.",
    "volume": "0 ml",
    "price": "0 cent",
    "material": [ "steel" ],
    "symbol": "#",
    "color": "light_gray",
    "flags": [ "MAG_BELT", "MAG_DESTROY", "BELTED", "OVERSIZE", "WATER_FRIENDLY", "ZERO_WEIGHT" ],
    "armor": [
      {
        "material": [ { "type": "steel", "covered_by_mat": 100, "thickness": 0.1 } ],
        "encumbrance": 10,
        "coverage": 10,
        "covers": [ "torso" ],
        "specifically_covers": [ "torso_upper" ]
      }
    ]
  }
```

The following additional rules apply:

* Missing mandatory fields, such as `volume` do not result in errors as the `abstract` type is discarded after JSON loading completes.
* Missing optional fields are set to the usual defaults for that type.


## Support

The following `type`s currently support inheritance (non-exhaustive list):

```
effect_on_condition
furniture
harvest
ITEM
item_group
material
MONSTER
MONSTER_FACTION
monstergroup
mutation
overmap_terrain
recipe
scenario
SPELL
terrain
uncraft
vehicle_part
```

To find out if a type supports `copy-from`, you need to know if it has implemented generic_factory.  To find out if this is the case, do the following:
* Open [init.cpp](https://github.com/CleverRaven/Cataclysm-DDA/tree/master/src/init.cpp)
* Find the line that mentions your type, for example `add( "gate", &gates::load );`.
* Copy the load function, in this case it would be *gates::load*.
* Use this in [the search bar on github](https://github.com/CleverRaven/Cataclysm-DDA/search?q=%22gates%3A%3Aload%22&unscoped_q=%22gates%3A%3Aload%22&type=Code) to find the file that contains *gates::load* (Note, you cannot search for ":" in file finder.  The search will simply ignore this symbol.).
* In the search results you find [gates.cpp](https://github.com/CleverRaven/Cataclysm-DDA/tree/master/src/gates.cpp). open it.
* In gates.cpp, find the generic_factory line, it looks like this: `generic_factory<gate_data> gates_data( "gate type", "handle", "other_handles" );`.
* Since the generic_factory line is present, you can now conclude that it supports `copy-from`.
* If you don't find generic_factory present, it does not support copy-from, as is the case for type vitamin (repeat the above steps and find that [vitamin.cpp](https://github.com/CleverRaven/Cataclysm-DDA/tree/master/src/vitamin.cpp) does not contain generic_factory).


## Behavior

A common misconception is that every `type` that supports `copy-from` will also support `extend`, `delete`, `proportional` and `relative`, in the same way as one's most commonly seen object: `ITEM`.  As explained above, this is not the case; some types handle these fields manually (which is undesired), others use generic_factory.  `extend` and `delete` only apply to JSON members that have containers as their underlying C++ types, and `proportional`/`relative` handling must be implemented with corresponding C++ functions.  

In summary, there is no "default" JSON inheritance, only shared degrees of support; check the relevant C++ if `copy-from` behavior is uncertain, and looking for an existing (and functional!) example is always a good idea.  Some types that implement partial or non-standard `copy-from` support (non-exhaustive list):
* `monstergroup` extends by default.  This prevents mods copying monstergroup definitions from replacing vanilla monstergroup lists.  To replace the *entire* list, add `"override": true` to the monstergroup object.
* `SPELL` has limited support, given their behavior is governed by its spell `effect`.  Flags are not always inherited.  Testing is required to guarantee an inherited spell will behave as intended, use at your own risk.
* The `vitamins` field from `material` extends by default.


## Guidelines

Contributors are encouraged to not overuse `copy-from`, as it can decrease the human readability of the JSON.  Chained inheritance is especially likely to become unwieldy, essentially recreating the level of redundancy we'd like to eliminate.

In general, there are two situations where `copy-from` should be used in the core game:

* Two things are _nearly identical_ variants of each other.
  * If they're pretty much identical (e.g. everything except for their descriptions can be kept, there are only decorative differences, there are no mechanical differences, or in the case of [guns](/doc/GUN_NAMING_AND_INCLUSION.md#difference-threshold) minor value differences), they can be handled as [variants](JSON_INFO.md#snippets) (scroll down to the variant example).
* A group of entities always (not almost always, always) shares some set of properties, then one or two levels of abstracts can set up a very shallow and narrow hierarchy.
