# How to add special attacks to monsters
- [Monster special Attacks](#monster-special-attacks)
    - [TODO](#todo)
    - [Adding `special_attacks` to monsters](#adding-special_attacks-to-monsters)
      - [Old style array](#old-style-array)
      - [New style object](#new-style-object)
      - [Combination](#combination)
    - [Hardcoded special attacks](#hardcoded-special-attacks)
    - [JSON special attacks](#json-special-attacks)
        - [`bite`](#bite)
    - [Non-melee special attacks](#non-melee-special-attacks)
        - [`gun`](#gun)
        - [`spell`](#spell)
        - [`leap`](#leap)
    - [Monster defensive attacks](#monster-defensive-attacks)        

# Monster special attacks

Monster creatures in C:DDA not only can do simple melee attacks, they have a wide array of attack-type actions.  

Depending on the intended effect, these can be valid `use_actions` for tools and weapons, hardcoded special attacks, any normal physical attack or even spells.  Also, depending on the kind of attack, these can be cooldown-based, conditioned or occur on death.


## TODO

- Descriptions for [Hardcoded special attacks](#hardcoded-special-attacks) could stand to be more descriptive of exactly what the attack does.


## Adding `special_attacks` to monsters

Monster special attacks can be defined either as old style arrays, new style objects, or a combination of both.  However, it is recommended to avoid the use of the old style in favor of the new style.


### Old style array

Generally [hardcoded special attacks](#hardcoded-special-attacks) are declared this way, although it can also be used for [JSON-declared attacks](/data/json/monster_special_attacks) too.  It contains 2 elements, the `id` of the attack and the cooldown:

```JSON
"special_attacks": [ [ "ACID", 10 ] ]
```

### New style object

It contains either: 
* An `id` of a [hardcoded special attack](#hardcoded-special-attacks), or a [JSON-declared attacks](/data/json/monster_special_attacks).
* A `type` member (string) plus a `cooldown` member (integer) pair, for [partially hardcoded special attacks](#partially-hardcoded-special-attacks).
* A spell (see [MAGIC.md](MAGIC.md)).

Depending on the kind of attack, it may contain additional required members.  Example:

```JSON
"special_attacks": [
    { "type": "leap", "cooldown": 10, "max_range": 4 }
]
```
In the case of separately defined attacks the object has to contain at least an `id` member.  In this case the attack will use the default attack data defined in `monster_attacks.json`, if a field is additionally defined it will overwrite those defaults.  These attacks have the common type `monster_attack`.  Example:

```JSON
"special_attacks": [
    { "id": "impale" }
]
```


### Combination

`special_attacks` may contain a mixture of the old and new style:

```JSON
"special_attacks": [
    [ "ACID", 10 ],
    { "type": "leap", "cooldown": 8, "max_range": 4 },
    { "id": "impale", "cooldown": 5, "min_mul": 1, "max_mul": 3 }
]
```
This monster can attempt a grab every ten turns, a leap with a maximum range of 4 every eight and an impale attack with 1-3x damage multiplier every five turns.


## Hardcoded special attacks

These special attacks are mostly hardcoded in C++ and are generally not configurable with JSON (unless otherwise documented).  JSON-declared replacements are mentioned when available.

- ```ABSORB_ITEMS``` Consumes objects it moves over to gain HP.  A movement cost per ml consumed can be enforced with `absorb_move_cost_per_ml` (default 0.025). The movement cost can have a minimum and maximum value specified by `absorb_move_cost_min` (default 1) and `absorb_move_cost_max` (default -1 for no limit). The volume in milliliters the monster must consume to gain 1 HP can be specified with `absorb_ml_per_hp` (default 250 ml). A list of materials that the monster can absorb can be specified with `absorb_materials` (can be string or string array, not specified = absorb all materials).
- ```ABSORB_MEAT``` Absorbs adjacent meat items (maximum absorbable item volume depends on the monster's volume), regenerating health in the process.
- ```ACID``` Spits acid.
- ```ACID_ACCURATE``` Shoots acid that is accurate at long ranges, but less so up close.
- ```ACID_BARF``` Barfs corroding, blinding acid.
- ```BIO_OP_BIOJUTSU``` Attacks with any of the below martial art attacks.
- ```BIO_OP_DISARM``` Disarming attack, does no damage.
- ```BIO_OP_IMPALE``` Stabbing attack, deals heavy damage and has a chance to cause bleeding.
- ```BIO_OP_TAKEDOWN``` Takedown attack, bashes either the target's head or torso and inflicts `downed`.
- ```BLOW_WHISTLE``` Blow a whistle creating a sound of volume 40 from the position of the monster.
- ```BOOMER_GLOW``` Spits glowing bile.
- ```BOOMER``` Spits bile.
- ```BRANDISH``` Brandishes a knife at the player.
- ```BREATHE``` Spawns a `breather`.  Note: `breather hub` only!
- ```CALLBLOBS``` Calls 2/3 of nearby blobs to defend this monster, and sends 1/3 of nearby blobs after the player.
- ```CHICKENBOT``` Robot can attack with tazer, M4, or MGL depending on distance.  Note: Legacy special attack.
- ```COPBOT``` Cop-bot warns then tazes the player.
- ```DANCE``` Monster dances.
- ```DARKMAN``` Can cause darkness and wraiths to spawn.
- ```DERMATIK``` Attempts to lay dermatik eggs in the player.
- ```DISAPPEAR``` Hallucination (or other unusual monster) disappears.
- ```DOGTHING``` The dog _thing_ spawns into a tentacle dog.
- ```EAT_CARRION``` The monster will nibble on organic corpses, including zombies and plants, damaging them and filling its stomach if it has the EATS flag.
- ```EAT_CROP``` The monster eats an adjacent planted crop.
- ```EAT_FOOD``` The monster eats an adjacent non-seed food item (apart from their own eggs and food with fun < -20). If paired with the EATS flag, this will fill its stomach.
- ```EVOLVE_KILL_STRIKE``` Damages the target's torso (damage scales with monster's melee dice), if it succeeds in killing a fleshy target the monster will upgrade to its next evolution.
- ```FEAR_PARALYZE``` Paralyzes the player with fear.
- ```FLAMETHROWER``` Shoots a stream of fire.
- ```FLESH_GOLEM``` Attacks the player with 5-10 bash, has a chance to inflict `downed` if the attack connects.  Also roars menacingly for some reason.
- ```FLESH_TENDRIL``` Spawns gangrenous impalers or crawlers, pulls targets close when 4 > range > 1, either flings or grabs them when adjacent.
- ```FORMBLOB``` Attacks a neighboring tile, effect depends on the tile's inhabitant: spawns small slimes depending on its speed if empty, slimes players/NPCs, speeds up friendly slimes, heals brain slimes, converts nonfriendly flesh/veggy non-huge monsters to slimes of appropriate size.  Decreases in size if it did any of those and its current speed is below a threshold.
- ```FUNGUS``` Releases fungal spores and attempts to infect the player.
- ```FUNGUS_BIG_BLOSSOM``` Spreads fire suppressing fungal haze.
- ```FUNGUS_BRISTLE``` Performs a barbed tendril attack that can cause fungal infections.
- ```FUNGUS_CORPORATE``` Used solely by Crazy Cataclysm. This will cause runtime errors if used without, and spawns SpOreos on top of the creature.
- ```FUNGUS_FORTIFY``` Grows fungal hedgerows, and advances player on the Mycus threshold path.
- ```FUNGUS_GROWTH``` Grows a young fungaloid into an adult.
- ```FUNGUS_HAZE``` Spawns fungal fields.
- ```FUNGUS_INJECT``` Performs a needle attack that can cause fungal infections.
- ```FUNGUS_SPROUT``` Grows a fungal wall.
- ```FUNGAL_TRAIL``` Spreads fungal terrain.
- ```GENE_STING``` Shoots a dart at the player that causes a mutation if it connects.
- ```GENERATOR``` Regenerates health, hums.
- ```GRENADIER``` Deploys tear gas/pacification/flashbang/c4 hacks from its ammo.
- ```GRENADIER_ELITE``` Deploys grenade/flashbang/c4/mininuke hacks from its ammo.
- ```GROWPLANTS``` Spawns underbrush, or promotes it to `> young tree > tree`.  Can destroy bashable terrain or do damage if it hits something.
- ```GROW_VINE``` Grows creeper vines.
- ```HOWL``` "an ear-piercing howl!".
- ```IMPALE``` Stabbing attack against the target's torso, with a chance to inflict `downed`.  Note: superseded by the JSON-defined `"impale"`.
- ```JACKSON``` Converts zombies into zombie dancers (until its death).
- ```KAMIKAZE``` Detonates its defined ammo after a countdown (calculated automatically to hopefully almost catch up to a running player).
- ```LEECH_SPAWNER``` Spawns root runners or root drones, low chance of upgrading itself into a leech stalk.
- ```LONGSWIPE``` Claw attack with 3-10 cut damage, which can even hit 3 tiles away.  If targeting an adjacent enemy it always hits the head and causes heavy bleeding.  JSON equivalents of the two elements are `"longswipe"` and `"cut_throat"` respectively.
- ```LUNGE``` Performs a jumping attack from some distance away, which can inflict `downed` to the target.
- ```MON_LEECH_EVOLUTION``` Evolves a leech plant into a leech blossom if no other blossoms are in sight.
- ```MULTI_ROBOT``` Robot can attack with tazer, flamethrower, M4, MGL, or 120mm cannon depending on distance.
- ```NONE``` No special attack.
- ```PAID_BOT```  For creature with the `PAY_BOT` flag, removes the ally status when the pet effect runs out.
- ```PARA_STING``` Shoots a paralyzing dart at the player.
- ```PARROT``` Parrots the speech defined in `speech.json`, picks one of the lines randomly.  `speaker` points to a monster id.
- ```PARROT_AT_DANGER``` Performs the same function as `PARROT`, but only if the creature sees an angry monster from a hostile faction.
- ```PHOTOGRAPH``` Photographs the player.  Causes a robot attack?
- ```PLANT``` Fungal spores take seed and grow into a fungaloid.
- ```PULL_METAL_WEAPON``` Pulls any weapon that's made of iron or steel from the player's hand.
- ```RATKING``` Inflicts disease `rat`.
- ```RATTLE``` "a sibilant rattling sound!".
- ```RESURRECT``` Revives the dead (again).
- ```RIOTBOT``` Sprays teargas or relaxation gas, can handcuff players, and can use a blinding flash.
- ```SCIENCE``` Various science/technology related attacks (e.g. manhacks, radioactive beams, etc.).
- ```SEARCHLIGHT``` Tracks targets with a searchlight.
- ```SHOCKING_REVEAL``` Shoots bolts of lightning, and reveals a SHOCKING FACT! Very fourth-wall breaking.  Used solely by Crazy Cataclysm.
- ```SHOCKSTORM``` Shoots bolts of lightning.
- ```SHRIEK``` "a terrible shriek!".
- ```SHRIEK_ALERT``` "a very terrible shriek!", louder than `SHRIEK`.
- ```SHRIEK_STUN``` "a stunning shriek!", causes a small bash, can cause a stun.
- ```SLIMESPRING``` Can provide a morale boost to the player, and cure bite and bleed effects.
- ```SMASH``` Smashes the target, sending it flying for a number of tiles equal to (`melee_dice` * `melee_dice_sides` * 3) / 10.  JSON equivalent is `"smash"`.
- ```SPIT_SAP``` Spits sap (acid damage, 12 range).
- ```SPLIT``` Creates a copy of itself if it has twice the maximum HP that it should normally have.  This can be achieved by combining this with `ABSORB_ITEMS`.
- ```STARE``` Stares at the player and inflicts ramping debuffs (`taint>tindrift`).
- ```STRETCH_ATTACK``` Ranged (3 tiles) piercing attack, dealing 5-10 damage.  JSON equivalent is `"stretch_attack"`
- ```STRETCH_BITE``` Ranged (3 tiles) bite attack, dealing stab damage and potentially infecting without grabbing.  JSON equivalent (without the chance of deep bites) is `"stretch_bite"`.
- ```SUICIDE``` Dies after attacking.
- ```TAZER``` Shocks the player.
- ```TENTACLE``` Lashes a tentacle at an enemy, doing bash damage at 3 tiles range. JSON equivalent is `"tentacle"`.
- ```TINDALOS_TELEPORT``` Spawns afterimages, teleports to corners nearer to its target.
- ```TRIFFID_GROWTH``` Young triffid grows into an adult.
- ```TRIFFID_HEARTBEAT``` Grows and crumbles root walls around the player, and spawns more monsters.
- ```UPGRADE``` Upgrades one of the non-hostile surrounding monsters, gets angry if it finds no targets to upgrade.
- ```VINE``` Attacks with vine.
- ```ZOMBIE_FUSE``` Absorbs an adjacent creature, healing and becoming less likely to fuse for 10 days.


## JSON special attacks

These special attacks are defined in [JSON](/data/json/monster_special_attacks), and belong to the `monster_attack` type, `melee` attack_type.  These don't have to be declared in the monster's attack data, the `id` of the desired attack can be used instead.  All fields beyond `id` are optional.

| field                       | description
| ---                         | ---
| `cooldown`			      | Integer, amount of turns between uses.
| `damage_max_instance`       | Array of objects.  See also [MONSTERS.md#melee_damage](MONSTERS.md#melee_damage).
| `min_mul`, `max_mul`        | Sets the bounds on the range of damage done.  For each attack, the above defined amount of damage will be multiplied by a
|						      | randomly rolled multiplier between the values `min_mul` and `max_mul`.  Default 0.5 and 1.0, meaning each attack will do at least half of the defined damage.
| `move_cost`                 | Integer, moves needed to complete special attack.  Default 100.
| `accuracy`                  | Integer, if defined the attack will use a different accuracy from monster's regular melee attack.
| `body_parts`			      | List, If empty the regular melee roll body part selection is used.  If non-empty, a body part is selected from the map to be targeted using the provided weights.
|						      | targeted with a chance proportional to the value.
| `condition`                 | Object, dialog conditions enabling the attack - see `NPC.md` for the potential conditions - note that `u` refers to the monster, `npc` to the attack target, and for `x_has_flag` conditions targeting monsters only take effect flags into consideration, not monster flags.
| `attack_upper`		      | Boolean, default true. If false the attack can't target any bodyparts with the `UPPER_LIMB` flag with the regular attack rolls (provided the bodypart is not explicitly targeted).
| `range`       		      | Integer, range of the attack in tiles (Default 1, this equals melee range). Melee attacks require unobstructed straight paths.
| `grab`                      | Boolean, default false. Denotes this attack as a grabbing one. See `grabs` for further information
| `grab_data`                 | Array, grab data of the attack. Read only if `grab: true`, see `grabs` for the possible variables.
| `hitsize_min`               | Integer, lower bound of limb size this attack can target (if no bodypart targets are explicitly defined)
| `hitsize_max`               | Integer, upper bound of limb size this attack can target.
| `no_adjacent`			      | Boolean, default false. The attack can't target adjacent creatures.
| `dodgeable`                 | Boolean, default true. The attack can be dodged normally.
| `uncanny_dodgeable`         | Boolean, defaults to the value of `dodgeable`. The attack can be dodged by the Uncanny Dodge bionic or by characters having the `UNCANNY_DODGE` character flag.  Uncanny dodging takes precedence over normal dodging.
| `blockable`                 | Boolean, default true.  The attack can be blocked (after the dodge checks).
| `effects_require_dmg`       | Boolean, default true.  Effects will only be applied if the attack successfully damaged the target.
| `effects`				      | Array, defines additional effects for the attack to add.  See [MONSTERS.md](MONSTERS.md#attack_effs) for the exact syntax. Duration is in turns, not in movement points
| `self_effects_always`        | Array of `effects` the monster applies to itself when doing this attack.
| `self_effects_onhit`         | Array of `effects` the monster applies to itself when successfully hitting with the attack.
| `self_effects_ondmg`         | Array of `effects` the monster applies to itself when damaging its target.
| `throw_strength`		      | Integer, if larger than 0 the attack will attempt to throw the target, every 10 strength equals one tile of distance thrown.
| `miss_msg_u`			      | String, message for missed attack against the player.
| `miss_msg_npc`		      | String, message for missed attack against an NPC.
| `hit_dmg_u`                 | String, message for successful attack against the player.
| `hit_dmg_npc`			      | String, message for successful attack against an NPC.
| `no_dmg_msg_u`	          | String, message for a 0-damage attack against the player.
| `no_dmg_msg_npc`            | String, message for a 0-damage attack against an NPC.
| `throw_msg_u`		          | String, message for a flinging attack against the player.
| `throw_msg_npc`		      | String, message for a flinging attack against an NPC.


### `bite`

Under the hood an attack with `monster_attack` type, `bite` attack_type - if you want to define multiple separate bites for a monster you'll need to do a proper definition using an `id` as well. Makes monster use teeth to bite opponent, uses the same fields as "monster_attack" attacks. Monster bites can give infections, and for humanoid enemies (`human` bodytype) require the target being grabbed.
If `hitsize_min` is undefined it will default to 1 (disqualifying bites on the eyes and mouth).

| Field                       | Description                                                                          |
| ---                         | ---                                                                                  |
| `infection_chance`          | Chance to give infection in a percentage.  Exact chance is `infection_chance` / 100. |

### Grab attacks

Any melee/bite-type JSON attack can function as a grab, which leads to special behavior in addition to the base attack functions:  Grabs are not allowed to hit an already-grabbed bodypart, filtered by any effect on the BP having the `GRAB` flag, instead they will attempt to retarget to another limb.  A successful (not-dodged) grab will apply an instance of its `grab_effect` (which should have the `GRAB` effect flag) with the intensity defined by `grab_strength` and will gain the target bodypart's `grabbing_effect` to facilitate targeted grab removal (see later).
The attack's behavior is determined by the `grab_data` array, using the below variables:

| field                       | description
| ---                         | ---
| `grab_strength`             | Optional integer.  The strength of the grab effect applied on a successful grab, defaults to the monster's own `grab_strength`.
| `grab_effect`               | Optional string.  ID of the effect to apply on a successful grab (defaults to `grabbed`).  The effect id `null` will apply a null effect without a bug message, allowing for e.g. pull attacks without grab effects and is exempt from the usual retargeting/grab_filter application rules.
| `exclusive_grab`            | Optional bool, default `false`.  If true the attack will attempt to break all `GRAB`-flagged effect one by one ( on a `puller grab_strength / 2 in effect intensity` roll), and fail if it can't remove all - used for ranged pulls and grabbing.
| `drag_distance`             | optional int, default 0.  If higher the monster will attempt to drag on a successful attack - moving away and moving their target to its previous position as many times as the defined distance with it. This will be disabled by seatbelts unless `respect_seatbelts` is false.  Drag movement costs the same amount of move points as it would as normal movement, but happens in the same turn as part of the attack.
| `drag_deviation`            | optional int, default 0. Deviation of the monster's pathing from their target's opposite tile - 0 always drags directly opposite, 1 chooses randomly from the monster's and opposite tile's common neighbors, 2 lets the monster drag towards any of its neighboring tiles not occupied by itself or its target. Dragging direction is randomized every drag step.
| `drag_grab_break_distance`  | optional int, default 0. Drag steps after which the dragged character gets a chance to break the grab using the normal grab break calculations.
| `drag_movecost_mod`         | optional float, default 1.0. Move cost modifier for every step of dragging - per default the monster spends moves for each drag step, but will still complete the drag, leading to them "catching up" on their move debt afterwards. Internally a move cost of 0.0 will *prevent* movement, so use a slightly larger value.
| `respect_seatbelts`         | Optional bool, default true.  When false drags/pulls can tear you out of your seatbelt, damaging the part in question.
| `pull_chance`               | Optional integer.  Percent chance for a connecting attack to initiate a pull, moving the target adjacent.  Pulls are prevented by seatbelts.
| `pull_weight_ratio`         | Optional float.  Ratio of weight the monster can successfully pull when succeeding the `pull_chance` roll or when dragging (requires a non-zero `drag_distance` ).  Default 0.75.
| `pull_msg_u/npc`            | Optional strings.  Message to print on a successful pull.
| `pull_fail_msg_u/npc`       | Optional strings.  Message to print on a failed pull attempt - either because of too high target weight or because of other grabs holding them back.

`GRAB`-flagged effects prevent movement for monsters and characters as well, and might have additional debuffs like every effect.  Grab break attempts happen effect by effect on non-attack movement or waiting in place, with the chance affected by limb scores, stats, and the existence of grab break MA techniques.  A successful grab removal removes the effect in question from the limb, as well as the bodypart's `grabbing_effect` from whichever monster has the latter.  If all grabs are broken movement is allowed on the same turn.

### `gun`

The monster fires a gun at a target.  If the monster is friendly, it will avoid harming the player.

| Field                       | Description                                                                                                           |
| ---                         | --------------------------------------------------------------------------------------------------------------------- |
| `gun_type`                  | (Required) Valid item id of a gun that will be used to perform the attack.                                            |
| `ammo_type`                 | (Required) Valid **item** id (**not** `ammo_type`) of the ammo the gun will be loaded with.  Note: the monster itself should also have a `starting_ammo` field with this ammo.  For example: the monster is defined with `"starting_ammo": {"50bmg": 100}` so it can shoot `"ammo_type": "50bmg"` when using the gun attack. |
| `max_ammo`                  | Cap on ammo. If ammo goes above this value for any reason, a debug message will be printed.                           |
| `fake_str`                  | Strength stat of the fake NPC that will execute the attack.  8 if not specified.                                      |
| `fake_dex`                  | Dexterity stat of the fake NPC that will execute the attack.  8 if not specified.                                     |
| `fake_int`                  | Intelligence stat of the fake NPC that will execute the attack.  8 if not specified.                                  |
| `fake_per`                  | Perception stat of the fake NPC that will execute the attack.  8 if not specified.                                    |
| `fake_skills`               | Array of 2 element arrays of skill id and skill level pairs.                                                          |
| `move_cost`                 | Move cost of executing the attack.                                                                                    |
| `condition`                 | Object, dialogue conditions enabling the attack.  See `NPCs.md` for the possible conditions, `u` refers to the monster.
| `require_targeting_player`  | If true, the monster will need to "target" the player, wasting `targeting_cost` moves, putting the attack on cooldown and making warning sounds, unless it attacked something that needs to be targeted recently.  Gives "grace period" to player.                                                               |
| `require_targeting_npc`     | As above, but with NPCs.                                                                                              |
| `require_targeting_monster` | As above, but with monsters.                                                                                          |
| `target_moving_vehicles`    | If true, the monster will "target" moving vehicles even if it cannot see the player.
| `targeting_timeout`         | Targeting status will be applied for this many turns.  Note that targeting applies to turret, not targets.            |
| `targeting_timeout_extend`  | Successfully attacking will extend the targeting for this many turns.  Can be negative.                               |
| `targeting_cost`            | Move cost of targeting the player. Only applied if attacking the player and didn't target player within last 5 turns. |
| `laser_lock`                | If true and attacking a creature that isn't laser-locked but needs to be targeted, the monster will act as if it had no targeting status (and waste time targeting), the target will become laser-locked, and if the target is the player, it will cause a warning.  Laser-locking affects the target, but isn't tied to specific attacker. |
| `range`                     | Maximum range at which targets will be acquired.                                                                      |
| `range_no_burst`            | Maximum range at which targets will be attacked with a burst (if applicable).                                         |
| `description`               | Description of the attack being executed if seen by the player.                                                       |
| `targeting_sound`           | Description of the sound made when targeting.                                                                         |
| `targeting_volume`          | Volume of the sound made when targeting.                                                                              |
| `no_ammo_sound`             | Description of the sound made when out of ammo.                                                                       |

### "spell" Monster Spells


Casts a separately-defined spell at the monster's target.  Spells with `target_self: true` will only target the casting monster, and will still be casted only if the monster has a hostile target.

| Identifier                     | Description                                                                                             |
| ---                            | ------------------------------------------------------------------------------------------------------- |
| `spell_data`                   | List of spell properties for the attack.                                                                |
| `min_level`                    | The level at which the spell is cast. Spells cast by monsters do not gain levels like player spells.    |
| `cooldown `                    | How often the monster can cast this spell.
| `monster_message`              | Message to print when the spell is cast, replacing the `message` in the spell definition. Dynamic fields correspond to `<Monster Display Name> / <Spell Name> / <Target name>`. |
| `condition`                    | Object, dialogue conditions enabling the attack.  See `NPCs.md` for the possible conditions, `u` refers to the casting monster and `npc` to the target unless the spell allows no target (in which case only self-conditions can be defined).
| `allow_no_target`              | Bool, default `false`. If `true` the monster will cast it even without a hostile target.                |


### "leap"

Makes the monster leap a few tiles over passable terrain as long as it can see its destination. It supports the following additional properties:

| Field                   | Description                                                                                          |
| ---                     | ---------------------------------------------------------------------------------------------------- |
| `max_range`             | (Required) Float, maximal range of the jump.  Respects circular distance setting!                    |
| `min_range`             | (Required) Float, minimal range of the jump.  Respects circular distance setting!                    |
| `prefer_leap`           | Leap even when adjacent to target, will still choose the closest acceptable destination.             |
| `random_leap`           | Disregard target location entirely when leaping, leading to completely random jumps.                 |
| `ignore_dest_terrain`   | Leap even if the destination is terrain that it doesn't usually move on.                             |
| `ignore_dest_danger`    | Leap even if the destination is tiles that it would usually avoid, such fire, traps.                 |
| `allow_no_target`       | Default `false` prevents monster from using the ability without a hostile target at its destination. |
| `move_cost`             | Moves needed to complete special attack. 100 move_cost with 100 speed is equal to 1 second/turn.     |
| `min_consider_range`    | Minimal distance to target to consider for using specific attack.                                    |
| `max_consider_range`    | Maximal distance to target to consider for using specific attack.        
| `condition`             | Object, dialogue conditions enabling the attack.  See `NPCs.md` for the possible conditions, `u` refers to the monster.
| `self_effects`          | Array of `effects` to apply after a successful leap.                                                 |
| `message`               | String, message to print when the player sees the monster jump (or land).                            |

## Monster defensive attacks

A special defense attack, triggered when the monster is attacked.  It should contain an array with the `id` of the defense (see a full list below) and the chance for that defense to be actually triggered.  Example:

```JSON
"special_when_hit": [ "ZAPBACK", 100 ]
```

- ```ACIDSPLASH``` Splashes acid on the attacker.
- ```NONE``` No special attack to the attacker.
- ```ZAPBACK``` Shocks attacker on hit.
