- [Special Attacks](#special-attacks)
    - [TODO](#todo)
    - [Adding "special_attacks" to monsters](#adding-special_attacks-to-monsters)
    - [Hardcoded Special Attacks](#hardcoded-special-attacks)
    - [JSON Special Attacks](#json-special-attacks)
        - ["monster_attack"](#monster_attack)
        - ["bite"](#bite)
        - ["leap"](#leap)
        - ["gun"](#gun)
    - [Monster Defense Attacks](#monster-defense-attacks)        

# Special Attacks
Some special attacks are also valid use actions for tools and weapons.
See `monsters.json` for examples on how to use these attacks.
Also see `monster_attacks.json` for more special attacks, for example, impale and scratch.

## TODO

- Descriptions for `Hardcoded special attacks` could stand to be more descriptive of exactly what the attack does.

## Adding "special_attacks" to monsters
(array of special attack definitions, optional)

Monster's special attacks. This should be an array, each element of it should be an object (new style) or an array (old style).

The old style array should contain 2 elements: the id of the attack (see [Hardcoded special attacks](#hardcoded-special-attacks) for a list) and the cooldown for that attack. Example:

```JSON
"special_attacks": [ [ "GRAB", 10 ] ]
```

The new style object can contain a "type" member (string) - "cooldown" member (integer) pair for the three types listed below, the "id" of an explicitly defined monster attack (from monster_attacks.json) or a spell (see [MAGIC.md]). It may contain additional members as required by the specific attack. Possible types are listed below. Example:

```JSON
"special_attacks": [
    { "type": "leap", "cooldown": 10, "max_range": 4 }
]
```
In the case of separately defined attacks the object has to contain at least an "id" member. In this case the attack will use the default attack data defined in monster_attacks.json, if a field is additionally defined it will overwrite those defaults. These attacks have the common "type": "monster_attack", see below for possible fields. Example:

```JSON
"special_attacks": [
    { "id": "impale" }
]
```

"special_attacks" may contain any mixture of old and new style entries:

```JSON
"special_attacks": [
    [ "GRAB", 10 ],
    { "type": "leap", "cooldown": 8, "max_range": 4 },
    { "id": "impale", "cooldown": 5, "min_mul": 1, "max_mul": 3 }
]
```
This monster can attempt a grab every ten turns, a leap with a maximum range of 4 every eight and an impale attack with 1-3x damage multiplier every five turns.

See [JSON special attacks](#json-special-attacks) for a full list.

## Hardcoded Special Attacks

These special attacks are mostly hardcoded in C++ and are generally not configurable with JSON (unless otherwise documented).

- ```ABSORB_ITEMS``` Consumes objects it moves over to gain HP.  A movement cost per ml consumed can be enforced with `absorb_move_cost_per_ml` (default 0.025). The movement cost can have a minimum and maximum value specified by `absorb_move_cost_min` (default 1) and `absorb_move_cost_max` (default -1 for no limit). The volume in milliliters the monster must consume to gain 1 HP can be specified with `absorb_ml_per_hp` (default 250 ml). A list of materials that the monster can absorb can be specified with `absorb_materials` (can be string or string array, not specified = absorb all materials).
- ```ABSORB_MEAT``` Absorbs adjacent meat items (maximal absorbable item volume depends on the monster's volume), regenerating health in the process.
- ```ACID``` Spits acid.
- ```ACID_ACCURATE``` Shoots acid that is accurate at long ranges, but less so up close.
- ```ACID_BARF``` Barfs corroding, blinding acid.
- ```ANTQUEEN``` Hatches/grows: `egg > ant > soldier`.
- ```BIO_OP_BIOJUTSU``` Attack with any of the below martial art attacks.
- ```BIO_OP_TAKEDOWN``` Takedown attack, bashes either the target's head or torso and inflicts `downed`.
- ```BIO_OP_DISARM``` Disarming attack, does no damage.
- ```BIO_OP_IMPALE``` Stabbing attack, deals heavy damage and has a chance to cause bleeding.
- ```BITE``` Bite attack that can cause deep infected wounds. If the attacker is humanoid, the target must be `grabbed` before BITE can trigger.
- ```BOOMER_GLOW``` Spit glowing bile.
- ```BOOMER``` Spit bile.
- ```BRANDISH``` Brandish a knife at the player.
- ```BREATHE``` Spawns a `breather` - `breather hub` only!
- ```CALLBLOBS``` Calls 2/3 of nearby blobs to defend this monster, and sends 1/3 of nearby blobs after the player.
- ```CHICKENBOT``` LEGACY - Robot can attack with tazer, M4, or MGL depending on distance.
- ```COPBOT``` Cop-bot warns then tazes the player.
- ```DANCE``` Monster dances.
- ```DARKMAN``` Can cause darkness and wraiths to spawn.
- ```DERMATIK_GROWTH``` Dermatik larva grows into an adult - obsoleted by them using `age_grow`.
- ```DERMATIK``` Attempts to lay dermatik eggs in the player.
- ```DISAPPEAR``` Hallucination (or other unusual monster) disappears.
- ```DOGTHING``` The dog _thing_ spawns into a tentacle dog.
- ```EAT_CROP``` The monster eats an adjacent planted crop.
- ```EAT_FOOD``` The monster eats an adjacent non-seed food item (apart from their own eggs and food with fun < -20).
- ```EVOLVE_KILL_STRIKE``` Damages the target's torso (damage scales with monster's melee dice), if it succeeds in killing a fleshy target the monster will upgrade to its next evolution.
- ```FEAR_PARALYZE``` Paralyze the player with fear.
- ```FLAMETHROWER``` Shoots a stream of fire.
- ```FLESH_GOLEM``` Attack the player with 5-10 bash, has a chance to inflict `downed` if the attack connects. Also roars menacingly for some reason.
- ```FLESH_TENDRIL``` Spawns gangrenous impalers or crawlers, pulls targets close when 4 > range > 1, either flings or grabs them when adjacent.
- ```FORMBLOB``` Attacks a neighboring tile, effect depends on the tile's inhabitant: spawns small slimes depending on its speed if empty, slimes players/NPCs, speeds up friendly slimes, heals brain slimes, converts nonfriendly flesh/veggy non-huge monsters to slimes of appropriate size.  Decreases in size if it did any of those and its current speed is below a threshold.
- ```FUNGAL_TRAIL``` Spreads fungal terrain.
- ```FUNGUS_BIG_BLOSSOM``` Spreads fire suppressing fungal haze.
- ```FUNGUS_BRISTLE``` Perform barbed tendril attack that can cause fungal infections.
- ```FUNGUS_CORPORATE``` Used solely by Crazy Cataclysm. This will cause runtime errors if used without, and spawns SpOreos on top of the creature.
- ```FUNGUS_FORTIFY``` Grows fungal hedgerows, and advances player on the Mycus threshold path.
- ```FUNGUS_GROWTH``` Grows a young fungaloid into an adult.
- ```FUNGUS_HAZE``` Spawns fungal fields.
- ```FUNGUS_INJECT``` Perform needle attack that can cause fungal infections.
- ```FUNGUS_SPROUT``` Grows a fungal wall.
- ```FUNGUS``` Releases fungal spores and attempts to infect the player.
- ```GENERATOR``` Regenerates health, hums.
- ```GENE_STING``` Shoot a dart at the player that causes a mutation if it connects.
- ```GRAB_DRAG``` GRAB the target, and drag it around - dragging is resistible depending on the size difference and the melee dice of the attacker.
- ```GRAB``` Grabs the player, slowing on hit, making movement and dodging impossible and blocking harder.
- ```GROWPLANTS``` Spawns underbrush, or promotes it to `> young tree > tree`. Can destroy bashable terrain or do damage if it hits something.
- ```GROW_VINE``` Grows creeper vines.
- ```GRENADIER``` Deploys tear gas/pacification/flashbang/c4 hacks from its ammo.
- ```GRENADIER_ELITE``` Deploys grenade/flashbang/c4/mininuke hacks from its ammo.
- ```HOWL``` "an ear-piercing howl!"
- ```IMPALE``` Stabbing attack against the target's torso, with a chance to down (superseded by the JSON-defined `impale` attack)
- ```JACKSON``` Converts zombies into zombie dancers (until its death).
- ```KAMIKAZE``` Detonates its defined ammo after a countdown (calculated automatically to hopefully almost catch up to a running player).
- ```LEECH_SPAWNER``` Spawns root runners or root drones, low chance of upgrading itself into a leech stalk.
- ```LONGSWIPE``` Claw attack with 3-10 cut damage, which can even hit 3 tiles away. If targeting an adjacent enemy it always hits the head and causes heavy bleeding. JSON equivalents of the two elements are `"id": "longswipe"` and `"id": "cut_throat"` respectively.
- ```LUNGE``` Perform a jumping attack from some distance away, which can down the target.
- ```MON_LEECH_EVOLUTION``` Evolves a leech plant into a leech blossom if no other blossoms are in sight.
- ```MULTI_ROBOT``` Robot can attack with tazer, flamethrower, M4, MGL, or 120mm cannon depending on distance.
- ```NONE``` No special attack.
- ```PARA_STING``` Shoot a paralyzing dart at the player.
- ```PARROT``` Parrots the speech defined in `speech.json`, picks one of the lines randomly. "speaker" points to a monster id.
- ```PARROT_AT_DANGER``` Performs the same function as PARROT, but only if the creature sees an angry monster from a hostile faction.
- ```PAID_BOT```  For creature with PAY_BOT flag, removes the ally status when the pet effect runs out.
- ```PHOTOGRAPH``` Photograph the player. Causes a robot attack?
- ```PLANT``` Fungal spores take seed and grow into a fungaloid.
- ```PULL_METAL_WEAPON``` Pulls any weapon that's made of iron or steel from the player's hand.
- ```RANGED_PULL``` Pulls targets towards attacker from 3 tiles range, dodgable but not resistible.
- ```RATKING``` Inflicts disease `rat`
- ```RATTLE``` "a sibilant rattling sound!"
- ```RESURRECT``` Revives the dead--again.
- ```RIOTBOT``` Sprays teargas or relaxation gas, can handcuff players, and can use a blinding flash.
- ```SCIENCE``` Various science/technology related attacks (e.g. manhacks, radioactive beams, etc.)
- ```SEARCHLIGHT``` Tracks targets with a searchlight.
- ```SHOCKING_REVEAL``` Shoots bolts of lightning, and reveals a SHOCKING FACT! Very fourth-wall breaking. Used solely by Crazy Cataclysm.
- ```SHOCKSTORM``` Shoots bolts of lightning.
- ```SHRIEK_ALERT``` "a very terrible shriek!" - louder than for SHRIEK
- ```SHRIEK_STUN``` "a stunning shriek!", causes a small bash, can cause a stun.
- ```SHRIEK``` "a terrible shriek!"
- ```SLIMESPRING``` Can provide a morale boost to the player, and cure bite and bleed effects.
- ```SMASH``` Smashes the target, sending it flying for a number of tiles equal to `("melee_dice" * "melee_dice_sides" * 3) / 10`. JSON equivalent is `id: smash`.
- ```SPIT_SAP``` Spit sap (acid damage, 12 range).
- ```SPLIT``` Creates a copy of itself if it has 2 the maximum HP that it should normally have. This can be achieved by combining this ability with the `ABSORB_ITEMS` ability.
- ```STARE``` Stare at the player and inflict ramping debuffs (`taint>tindrift`).
- ```STRETCH_ATTACK``` Ranged (3 tiles) piercing attack, doing 5-10 damage. JSON equivalent is `id: stretch_attack`
- ```STRETCH_BITE``` Ranged (3 tiles) bite attack, doing stab damage and potentially infecting without grabbing. JSON equivalent (without the chance of deep bites) is `id: stretch_bite`.
- ```SUICIDE``` Dies after attacking.
- ```TAZER``` Shock the player.
- ```TENTACLE``` Lashes a tentacle at an enemy, doing bash damage at 3 tiles range. JSON equivalent is `id: tentacle`.
- ```TINDALOS_TELEPORT``` Spawns afterimages, teleports to corners nearer to its target.
- ```TRIFFID_GROWTH``` Young triffid grows into an adult.
- ```TRIFFID_HEARTBEAT``` Grows and crumbles root walls around the player, and spawns more monsters.
- ```UPGRADE``` Upgrades one of the non-hostile surrounding monsters, gets angry if it finds no targets to upgrade.
- ```VINE``` Attacks with vine.
- ```ZOMBIE_FUSE``` Absorbs an adjacent creature, healing and becoming less likely to fuse for 10 days.

## JSON Special Attacks

### "monster_attack"

The common type for JSON-defined attacks. Note, you don't have to declare it in the monster attack data, use the "id" of the desired attack instead. All fields beyond `id` are optional.

| field                 | description
| ---                   | ---
| `cooldown`			| Integer, amount of turns between uses.
| `damage_max_instance` | Array of objects, see ### "melee_damage"
| `min_mul`, `max_mul`  | Sets the bounds on the range of damage done. For each attack, the above defined amount of damage will be multiplied by a
|						| randomly rolled multiplier between the values min_mul and max_mul. Default 0.5 and 1.0, meaning each attack will do at least half of the defined damage.
| `move_cost`           | Integer, moves needed to complete special attack. Default 100.
| `accuracy`            | Integer, if defined the attack will use a different accuracy from monster's regular melee attack.
| `body_parts`			| List, If empty the regular melee roll body part selection is used. If non-empty, a body part is selected from the map to be targeted using the provided weights.
|						| targeted with a chance proportional to the value.
| `attack_chance`		| Integer, percent chance of the attack being successfully used if a monster attempts it. Default 100.
| `forbidden_effects_any` | Array of effect ids, if the monster has any one the attack can't trigger.
| `forbidden_effects_all` | Array of effect ids, if the monster has every effect the attack can't trigger.
| `required_effects_any` | Array of effect ids, the monster needs any one for the attack to trigger.
| `required_effects_all` | Array of effect ids, the monster needs every effect for the attack to trigger.
| `attack_upper`		| Boolean, default true. If false the attack can't target any bodyparts with the `UPPER_LIMB` flag with the regular attack rolls(provided the bodypart is not explicitly targeted).
| `range`       		| Integer, range of the attack in tiles (Default 1, this equals melee range). Melee attacks require unobstructed straight paths.
| `hitsize_min`         | Integer, lower bound of limb size this attack can target ( if no bodypart targets are explicitly defined )
| `hitsize_max`         | Integer, upper bound of limb size this attack can target.
| `no_adjacent`			| Boolean, default false. The attack can't target adjacent creatures.
| `dodgeable`           | Boolean, default true. The attack can be dodged normally.
| `uncanny_dodgeable`   | Boolean, defaults to the value of `dodgeable`. The attack can be dodged by the Uncanny Dodge bionic or by characters having the `UNCANNY_DODGE` character flag. Uncanny dodging takes precedence over normal dodging.
| `blockable`           | Boolean, default true. The attack can be blocked (after the dodge checks).
| `effects_require_dmg` | Boolean, default true. Effects will only be applied if the attack successfully damaged the target.
| `effects`				| Array, defines additional effects for the attack to add. See [MONSTERS.md](MONSTERS.md#"attackeffs") for the exact syntax.
| `self_effect_always`  | Array of `effects` the monster applies to itself when doing this attack.
| `self_effect_onhit`   | Array of `effects` the monster applies to itself when successfully hitting with the attack.
| `self_effect_ondmg`   | Array of `effects` the monster applies to itself when damaging its target.
| `throw_strength`		| Integer, if larger than 0 the attack will attempt to throw the target, every 10 strength equals one tile of distance thrown.
| `miss_msg_u`			| String, message for missed attack against the player.
| `miss_msg_npc`		| String, message for missed attack against an NPC.
| `hit_dmg_u`			| String, message for successful attack against the player.
| `hit_dmg_npc`			| String, message for successful attack against an NPC.
| `no_dmg_msg_u`		| String, message for a 0-damage attack against the player.
| `no_dmg_msg_npc`		| String, message for a 0-damage attack against an NPC.
| `throw_msg_u`		    | String, message for a flinging attack against the player.
| `throw_msg_npc`		| String, message for a flinging attack against an NPC.

### "bite"

Makes monster use teeth to bite opponent, uses the same fields as "monster_attack" attacks. Monster bites can give infections if the target is grabbed at the same time.

| field                 | description
| ---                   | ---
| `infection_chance`    | Chance to give infection in a percentage. Exact chance is infection_chance / 100.


### "leap"

Makes the monster leap a few tiles. It supports the following additional properties:

| field                | description
| ---                  | ---
| `max_range`          | (Required) Maximal range of attack.
| `min_range`          | (Required) Minimal range needed for attack.
| `allow_no_target`    | This prevents monster from using the ability on empty space.
| `move_cost`          | Turns needed to complete special attack. 100 move_cost with 100 speed is equal to 1 second/turn.
| `min_consider_range` | Minimal range to consider for using specific attack.
| `max_consider_range` | Maximal range to consider for using specific attack.
| `forbidden_effects_any` | Array of effect ids, if the monster has any one the attack can't trigger.
| `forbidden_effects_all` | Array of effect ids, if the monster has every effect the attack can't trigger.
| `required_effects_any` | Array of effect ids, the monster needs any one for the attack to trigger.
| `required_effects_all` | Array of effect ids, the monster needs every effect for the attack to trigger.


### "gun"

Fires a gun at a target. If friendly, will avoid harming the player.

| field                       | description
| ---                         | ---
| `gun_type`                  | (Required) Valid item id of a gun that will be used to perform the attack.
| `ammo_type`                 | (Required) Valid item id of the ammo the gun will be loaded with.  Monster should also have a "starting_ammo" field with this ammo. For example: `"ammo_type" : "50bmg", "starting_ammo" : {"50bmg":100}`
| `max_ammo`                  | Cap on ammo. If ammo goes above this value for any reason, a debug message will be printed.
| `fake_str`                  | Strength stat of the fake NPC that will execute the attack. 8 if not specified.
| `fake_dex`                  | Dexterity stat of the fake NPC that will execute the attack. 8 if not specified.
| `fake_int`                  | Intelligence stat of the fake NPC that will execute the attack. 8 if not specified.
| `fake_per`                  | Perception stat of the fake NPC that will execute the attack. 8 if not specified.
| `fake_skills`               | Array of 2 element arrays of skill id and skill level pairs.
| `move_cost`                 | Move cost of executing the attack
| `require_targeting_player`  | If true, the monster will need to "target" the player, wasting `targeting_cost` moves, putting the attack on cooldown and making warning sounds, unless it attacked something that needs to be targeted recently.  Gives "grace period" to player.
| `require_targeting_npc`     | As above, but with npcs.
| `require_targeting_monster` | As above, but with monsters.
| `targeting_timeout`         | Targeting status will be applied for this many turns.  Note that targeting applies to turret, not targets.
| `targeting_timeout_extend`  | Successfully attacking will extend the targeting for this many turns. Can be negative.
| `targeting_cost`            | Move cost of targeting the player. Only applied if attacking the player and didn't target player within last 5 turns.
| `laser_lock`                | If true and attacking a creature that isn't laser-locked but needs to be targeted, the monster will act as if it had no targeting status (and waste time targeting), the target will become laser-locked, and if the target is the player, it will cause a warning.  Laser-locking affects the target, but isn't tied to specific attacker.
| `range`                     | Maximum range at which targets will be acquired.
| `range_no_burst`            | Maximum range at which targets will be attacked with a burst (if applicable).
| `burst_limit`               | Limit on burst size.
| `description`               | Description of the attack being executed if seen by the player.
| `targeting_sound`           | Description of the sound made when targeting.
| `targeting_volume`          | Volume of the sound made when targeting.
| `no_ammo_sound`             | Description of the sound made when out of ammo.

## Monster Defense Attacks

A special defense attack, triggered when the monster is attacked. It should contain an array with the id of the defense (see a full list below) and the chance for that defense to be actually triggered. Example:

```JSON
"special_when_hit": [ "ZAPBACK", 100 ]
```

- ```ACIDSPLASH``` Splashes acid on the attacker
- ```NONE``` No special attack-back
- ```ZAPBACK``` Shocks attacker on hit
