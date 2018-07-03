# Monster types

Monster types are specified as JSON object with "type" member set to "MONSTER":
```JSON
{
    "type": "MONSTER",
    "id": "mon_foo",
    ...
}
```

The id member should be the unique id of the type. It can be any string, by convention it has the prefix "mon_". This id can be referred to in various places, like monster groups or in mapgen to spawn specific monsters.

Monster types support the following properties (mandatory, except if noted otherwise):

## "name", "name_plural"
(string)

Name (singular) displayed in-game, and optional the plural name, defaults to singular name + "s".

## "description"
(string)

In-game description for the monster.

## "species"
(array of strings, optional)

A list of species ids. One can add or remove entries in mods via "add:species" and "remove:species", see Modding below. Properties (currently only triggers) from species are added to the properties of each monster that belong to the species.

TODO: document species.

## "categories"
(array of strings, optional)

Monster categories. Can be NULL, CLASSIC (only mobs found in classic zombie movies) or WILDLIFE (natural animals). If they are not CLASSIC or WILDLIFE, they will not spawn in classic mode.  One can add or remove entries in mods via "add:flags" and "remove:flags".

## "symbol", "color"
(string)

Symbol and color representing monster in-game. The symbol must be a UTF-8 string, that is exactly one console cell width (may be several Unicode characters). The color must be a valid color id, see TODO: find or create a list of color ids.

## "size"
(string, optional)

Size flag, see JSON_FLAGS.md.

## "material"
(array of strings, optional)

The materials the monster is primarily composed of. Must contain valid material ids. An empty array (which is the default) is also allowed, the monster is made of no specific material. One can add or remove entries in mods via "add:material" and "remove:material".

## "phase"
(string, optional)

TODO: describe this. Is this even used in-game?

## "default_faction"
(string)

The id of the faction the monster belongs to, this affects what other monsters it will fight. See Monster factions.

## "attack_cost"
(integer, optional)

Number of moves per regular attack.

## "diff"
(integer, optional)

Monster difficulty. Impacts the shade used to label the monster, and if it is above 30 a kill will be recorded in the memorial log. Some example values: (Zombie, 3) (Mi-go, 26) (Zombie Hulk, 50).

## "aggression"
(integer, optional)

Defines how aggressive the monster is. Ranges from -99 (totally passive) to 100 (guaranteed hostility on detection)

## "morale"
(integer, optional)

Monster morale. TODO: describe this better.

## "speed"
(integer)

Monster speed. 100 is the normal speed for a human being - higher values are faster and lower values are slower.

## "melee_skill"
(integer, optional)

Monster melee skill, ranges from 0 - 10, with 4 being an average mob. See GAME_BALANCE.txt for more examples

## "dodge"
(integer, optional)

Monster dodge skill. See GAME_BALANCE.txt for an explanation of dodge mechanics.

## "melee_damage"
(integer, optional)

TODO: describe this.

## "melee_dice", "melee_dice_sides"
(integer, optional)

Number of dices and their sides that are rolled on monster melee attack. This defines the amount of bash damage.

## "melee_cut"
(integer, optional)

Amount of cutting damage added to die roll on monster melee attack.

## "armor_bash", "armor_cut", "armor_stab", "armor_acid", "armor_fire"
(integer, optional)

Monster protection from bashing, cutting, stabbing, acid and fire damage.

## "vision_day", "vision_night"
(integer, optional)

Vision range in full daylight and in total darkness.

## "luminance"
(integer, optional)

Amount of light passively output by monster. Ranges from 0 to 10.

## "hp"
(integer)

Monster hit points.

## "death_drops"
(string or item group, optional)

An item group that is used to spawn items when the monster dies. This can be an inlined item group, see ITEM_SPAWN.md. The default subtype is "distribution".

## "death_function"
(array of strings, optional)

How the monster behaves on death. See JSON_FLAGS.md for a list of possible functions. One can add or remove entries in mods via "add:death_function" and "remove:death_function".

## "special_attack"
(array of special attack definitions, optional)

Monster's special attacks. This should be an array, each element of it should be an object (new style) or an array (old style).

The old style array should contain 2 elements: the id of the attack (see JSON_FLAGS.md for a list) and the cooldown for that attack. Example (grab attack every 10 turns):
```JSON
"special_attack": [ [ "GRAB", 10 ] ]
```

The new style object should contain at least a "type" member (string) and "cooldown" member (integer). It may contain additional members as required by the specific type. Possible types are listed below. Example:
```JSON
"special_attack": [
    { "type": "leap", "cooldown": 10, "max_range": 4 }
]
```

"special_attack" may contain any mixture of old and new style entries:
```JSON
"special_attack": [
    [ "GRAB", 10 ],
    { "type": "leap", "cooldown": 10, "max_range": 4 }
]
```

One can add entries with "add:death_function", which takes the same content as the "special_attack" member and remove entries with "remove:death_function", which requires an array of attack types. Example:
```JSON
"remove:special_attack": [ "GRAB" ],
"add:special_attack": [ [ "SHRIEK", 20 ] ]
```

## "flags"
(array of strings, optional)

Monster flags. See JSON_FLAGS.md for a full list. One can add or remove entries in mods via "add:flags" and "remove:flags".

## "fear_triggers", "anger_triggers", "placate_triggers"
(array of strings, optional)

What makes the monster afraid / angry / what calms it. See JSON_FLAGS.md for a full list

## "revert_to_itype"
(string, optional)

If not empty and a valid item id, the monster can be converted into this item by the player, when it's friendly. This is usually used for turrets and similar to revert the turret monster back into the turret item, which can be picked up and placed elsewhere.

## "starting_ammo"
(object, optional)

An object containing ammo that newly spawned monsters start with. This is useful for a monster that has a special attack that consumes ammo. Example:
```JSON
"starting_ammo": { "9mm": 100, "40mm_frag": 100 }
```

## "upgrades"
(boolean or object, optional)

Controls how this monster is upgraded over time. It can either be the single value `false` (which is the default and disables upgrading) or an object describing the upgrades.

Example:
```JSON
"upgrades": {
    "into_group": "GROUP_ZOMBIE_UPGRADE",
    "half_life": 28
}
```

The upgrades object may have the following members:

### "half_life"
TODO: describe this.

### "into_group"
TODO: describe this.

### "into"
TODO: describe this.

## "special_when_hit"
(array, optional)

A special defense attack, triggered when the monster is attacked. It should contain an array with the id of the defense (see Monster defense attacks in JSON_FLAGS.md) and the chance for that defense to be actually triggered. Example:
```JSON
"special_when_hit": [ "ZAPBACK", 100 ]
```

## "attack_effs"
(array of objects, optional)

A set of effects that may get applied to the attacked creature when the monster successfully attacks. Example:
```JSON
"attack_effs": [
    {
        "id": "paralyzepoison",
        "duration": 33,
        "chance": 50
    }
]
```

Each element of the array should be an object containing the following members:

### "id"
(string)

The id of the effect that is to be applied.

### "duration"
(integer, optional)

How long (in turns) the effect should last.

### "affect_hit_bp"
(boolean, optional)

Whether the effect should be applied to the hit body part instead of the one set below.

### "bp"
(string, optional)

The body part that where the effect is applied. The default is to apply the effect to the whole body. Note that some effects may require a specific body part (e.g. "hot") and others may require the whole body (e.g. "meth").

### "permanent"
(boolean, optional)

Whether the effect is permanent, in which case the "duration" will be ignored. The default is non-permanent.

### "chance"
(integer, optional)

The chance of the effect getting applied.



# Modding

Monster types can be overridden or modified in mods. To do so, one has to add an "edit-mode" member, which can contain either:
- "create" (the default if the member does not exist), an error will be shown if a type with the given id already exists.
- "override", an existing type (if any) with the given id will be removed and the new data will be loaded as a completely new type.
- "modify", an existing type will be modified. If there is no type with the given id, an error will be shown.

Mandatory properties (all that are not marked as optional) are only required if edit mode is "create" or "override".

Example (rename the zombie monster, leaves all other properties untouched):
```JSON
{
    "type": "MONSTER",
    "edit-mode": "modify",
    "id": "mon_zombie",
    "name": "clown"
}
```
The default edit mode ("create") is suitable for new types, if their id conflicts with the types from other mods or from the core data, an error will be shown. The edit mode "override" is suitable for re-defining a type from scratch, it ensures that all mandatory members are listed and leaves no traces of the previous definitions. Edit mode "modify" is for small changes, like adding a flag or removing a special attack.

Modifying a type overrides the properties with the new values, this example sets the special attacks to contain *only* the "SHRIEK" attack:
```JSON
{
    "type": "MONSTER",
    "edit-mode": "modify",
    "id": "mon_zombie",
    "special_attack": [ [ "SHRIEK", 20 ] ]
}
```
Some properties allow adding and removing entries, as documented above, usually via members with the "add:"/"remove:" prefix.



# Monster special attack types
The listed attack types can be as monster special attacks (see "special_attack").

## "leap"
Makes the monster leap a few tiles. It supports the following additional properties:

### "max_range"
(Required) Maximal range to consider for leaping.

### "min_range"
TODO: describe this.

### "allow_no_target"
TODO: describe this.

### "move_cost"
TODO: describe this.

#### "min_consider_range", "max_consider_range"
TODO: describe this.

## "bite"
TODO: describe this.

### "damage_max_instance"
TODO: describe this.

### "min_mul", "max_mul"
TODO: describe this.

### "move_cost"
TODO: describe this.

### "accuracy"
TODO: describe this.

### "no_infection_chance"
TODO: describe this.

## "gun"
Fires a gun at a target. If friendly, will avoid harming the player.

### "gun_type"
(Required) Valid item id of a gun that will be used to perform the attack.

### "ammo_type"
(Required) Valid item id of the ammo the gun will be loaded with.
Monster should also have a "starting_ammo" field with this ammo.
For example
```
"ammo_type" : "50bmg",
"starting_ammo" : {"50bmg":100}
```

### "max_ammo"
Cap on ammo. If ammo goes above this value for any reason, a debug message will be printed.

### "fake_str"
Strength stat of the fake NPC that will execute the attack. 8 if not specified.

### "fake_dex"
Dexterity stat of the fake NPC that will execute the attack. 8 if not specified.

### "fake_int"
Intelligence stat of the fake NPC that will execute the attack. 8 if not specified.

### "fake_per"
Perception stat of the fake NPC that will execute the attack. 8 if not specified.

### "fake_skills"
Array of 2 element arrays of skill id and skill level pairs.

### "move_cost"
Move cost of executing the attack

// If true, gives "grace period" to player
### "require_targeting_player"
If true, the monster will need to "target" the player,
wasting `targeting_cost` moves, putting the attack on cooldown and making warning sounds,
unless it attacked something that needs to be targeted recently.

### "require_targeting_npc"
As above, but with npcs.

### "require_targeting_monster"
As above, but with monsters.

### "targeting_timeout"
Targeting status will be applied for this many turns.
Note that targeting applies to turret, not targets.

### "targeting_timeout_extend"
Successfully attacking will extend the targeting for this many turns. Can be negative.

### "targeting_cost"
Move cost of targeting the player. Only applied if attacking the player and didn't target player within last 5 turns.

### "laser_lock"
If true and attacking a creature that isn't laser-locked but needs to be targeted,
the monster will act as if it had no targeting status (and waste time targeting),
the target will become laser-locked, and if the target is the player, it will cause a warning.

Laser-locking affects the target, but isn't tied to specific attacker.

### "range"
Maximum range at which targets will be acquired.

### "range_no_burst"
Maximum range at which targets will be attacked with a burst (if applicable).

### "burst_limit"
Limit on burst size.

### "description"
Description of the attack being executed if seen by the player.

### "targeting_sound"
Description of the sound made when targeting.

### "targeting_volume"
Volume of the sound made when targeting.

### "no_ammo_sound"
Description of the sound made when out of ammo.
