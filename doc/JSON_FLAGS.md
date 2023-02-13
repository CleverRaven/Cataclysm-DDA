# JSON Flags

- [JSON Flags](#json-flags)
  - [Notes](#notes)
  - [Inheritance](#inheritance)
  - [TODO](#todo)
  - [Ammo](#ammo)
    - [Ammo type](#ammo-type)
    - [Effects](#ammo-effects)
    - [Flags](#flags)
  - [Traps](#traps)
  - [Armor](#armor)
    - [Covers](#covers)
    - [Specifically Covers](#specifically-covers)
    - [Flags](#flags-1)
  - [Bionics](#bionics)
  - [Bodyparts](#bodyparts)
  - [Books](#books)
  - [Character - (Bionic/Mutation/Effect)](#character)
  - [Comestibles](#comestibles)
    - [Comestible type](#comestible-type)
    - [Addiction type](#addiction-type)
    - [Use action](#use-action)
    - [Flags](#flags-2)
  - [Effects](#effects)
    - [Flags](#flags-3)
  - [Furniture and Terrain](#furniture-and-terrain)
    - [Flags](#flags-4)
    - [Fungal Conversions Only](#fungal-conversions-only)
    - [Furniture Only](#furniture-only)
  - [Generic](#generic)
    - [Flags](#flags-5)
  - [Guns](#guns)
    - [Firing modes](#firing-modes)
  - [Magazines](#magazines)
  - [Mapgen](#mapgen)
  - [Map Specials](#map-specials)
  - [Material Phases](#material-phases)
  - [Melee](#melee)
    - [Flags](#flags-6)
  - [Monster Groups](#monster-groups)
    - [Seasons](#seasons)
    - [Time of day](#time-of-day)
  - [Monsters](#monsters)
    - [Anger, Fear and Placation Triggers](#anger-fear-and-placation-triggers)
    - [Categories](#categories)
    - [Death Functions](#death-functions)
    - [Flags](#flags-7)
    - [Sizes](#sizes)
    - [Special attacks](#special-attacks)
  - [Mutations](#mutations)
      - [Flags](#flags-8)
    - [Categories](#categories-1)
  - [Overmap](#overmap)
    - [Overmap connections](#overmap-connections)
    - [Overmap specials](#overmap-specials)
      - [Flags](#flags-9)
    - [Overmap terrains](#overmap-terrains)
      - [Flags](#flags-10)
  - [Recipes](#recipes)
    - [Crafting recipes](#crafting-recipes)
      - [Flags](#flags-11)
    - [Camp building recipes](#camp-building-recipes)
      - [Flags](#flags-12)
  - [Scenarios](#scenarios)
    - [Flags](#flags-13)
      - [Profession Flags](#profession-flags)
      - [Starting Location Flags](#starting-location-flags)
  - [Skills](#skills)
    - [Tags](#tags)
  - [Techniques](#techniques)
  - [Tools](#tools)
    - [Flags](#flags-14)
    - [Flags that apply to items](#flags-that-apply-to-items)
    - [Use actions](#use-actions)
  - [Vehicle Parts](#vehicle-parts)
    - [Flags](#flags-15)
    - [Vehicle parts requiring other vehicle parts](#vehicle-parts-requiring-other-vehicle-parts)
    - [Fuel types](#fuel-types)
    - [Faults](#faults)
      - [Flags](#flags-16)
      - [Parameters](#parameters)

## Notes

- Some flags (items, effects, vehicle parts) have to be defined in `flags.json` or `vp_flags.json` (with type: `json_flag`) to work correctly.
- Many of the flags intended for one category or item type, can be used in other categories or item types. Experiment to see where else flags can be used.
- Offensive and defensive flags can be used on any item type that can be wielded.


## Inheritance

When an item is crafted, it can inherit flags from the components that were used to craft it. This requires that the flag to be inherited has the `"craft_inherit": true` entry. If you don't want a particular item to inherit flags when crafted, specify the member delete_flags, which is an array of strings. Flags specified there will be removed from the resultant item upon crafting. This will override flag inheritance, but will not delete flags that are part of the item type itself.


## TODO

- `Ammo type` table is very old and doesn't include many new ammo types. Consider updating it or removing altogether, as ammo types ain't no json flags at all.


## Ammo

### Ammo type

These are handled through `ammo_types.json`.  You can tag a weapon with these to have it chamber existing ammo, or make your own ammo there.  The first column in this list is the tag's "id", the internal identifier DDA uses to track the tag, and the second is a brief description of the ammo tagged.  Use the id to search for ammo listings, as ids are constant throughout DDA's code.  Happy chambering!  :-)

- ```120mm``` 120mm HEAT
- ```12mm``` 12mm
- ```20x66mm``` 20x66mm Shot (and relatives)
- ```223``` .223 Remington (and 5.56 NATO)
- ```22``` .22LR (and relatives)
- ```3006``` 30.06
- ```300``` .300 WinMag
- ```308``` .308 Winchester (and relatives)
- ```32``` .32 ACP
- ```36paper``` .36 cap & ball
- ```38``` .38 Special
- ```40``` 10mm
- ```40mm``` 40mm Grenade
- ```44``` .44 Magnum
- ```44paper``` .44 cap & ball
- ```454``` .454 Casull
- ```45``` .45 ACP (and relatives)
- ```46``` 46mm
- ```500``` .500 Magnum
- ```50``` .50 BMG
- ```57``` 57mm
- ```5x50``` 5x50 Dart
- ```66mm``` 66mm HEAT
- ```700nx``` .700 Nitro Express
- ```762R``` 7.62x54mm
- ```762``` 7.62x39mm
- ```762x25``` 7.62x25mm
- ```84x246mm``` 84x246mm HE
- ```8x40mm``` 8mm Caseless
- ```9mm``` 9x19mm Luger (and relatives)
- ```9x18``` 9x18mm
- ```BB``` BB
- ```RPG-7``` RPG-7
- ```UPS``` UPS charges
- ```ammo_flintlock``` Flintlock ammo
- ```ampoule``` Ampoule
- ```arrow``` Arrow
- ```battery``` Battery
- ```blunderbuss``` Blunderbuss
- ```bolt``` Bolt
- ```charcoal``` Charcoal
- ```components``` Components
- ```dart``` Dart
- ```diesel``` Diesel
- ```fish_bait``` Fish bait
- ```fishspear``` Speargun spear
- ```fusion``` Laser Pack
- ```gasoline``` Gasoline
- ```homebrew_rocket``` homebrew rocket
- ```lamp_oil``` Lamp oil
- ```laser_capacitor``` Charge
- ```m235``` M235 TPA (66mm Incendiary Rocket)
- ```metal_rail``` Rebar Rail
- ```money``` Cents
- ```muscle``` Muscle
- ```nail``` Nail
- ```pebble``` Pebble
- ```plasma``` Plasma
- ```plutonium``` Plutonium Cell
- ```rebreather_filter``` Rebreather filter
- ```shot``` Shotshell
- ```signal_flare``` Signal Flare
- ```tape``` Duct tape
- ```thread``` Thread
- ```thrown``` Thrown
- ```unfinished_char``` Semi-charred fuel
- ```water``` Water
- ```paper``` Paper

### Ammo effects

- ```ACIDBOMB``` Leaves a pool of acid on detonation.
- ```ACT_ON_RANGED_HIT``` The item should activate when thrown or fired, then immediately get processed if it spawns on the ground.
- ```APPLY_SAP``` Applies sap-coated effect on hit.
- ```BEANBAG``` Stuns the target.
- ```BLACKPOWDER``` May clog up the gun with blackpowder fouling, which will also cause rust.
- ```BLINDS_EYES``` Blinds the target if it hits the head (ranged projectiles can't actually hit the eyes at the moment).
- ```BOUNCE``` Inflicts target with `bounced` effect and rebounds to a nearby target without this effect.
- ```BURST``` Spills the contents on hit.
- ```COOKOFF``` Explodes when lit on fire.
- ```CUSTOM_EXPLOSION``` Explosion as specified in ```"explosion"``` field of used ammo. See `JSON_INFO.md`.
- ```DRAW_AS_LINE``` Doesn't go through regular bullet animation; instead draws a line and the bullet on its end for one frame.
- ```DRAW_LASER_BEAM``` Creates a trail of laser (the field type).
- ```EMP``` Damages "electronic" terrain types (such as consoles or card readers) In rare cases might make card readers open doors. Damages and destroys robotic enemies. Drains bionic power and power from any electronic equipment in player possession.
- ```EXPLOSIVE```                           -
- ```EXPLOSIVE_120mmHEAT```                 -
- ```EXPLOSIVE_20x66```                     -
- ```EXPLOSIVE_66mmHEAT```                  -
- ```EXPLOSIVE_84x246HE```                  -
- ```EXPLOSIVE_84x246HEDP```                -
- ```EXPLOSIVE_ATGMHEAT```                  -
- ```EXPLOSIVE_BIG```                       -
- ```EXPLOSIVE_GRENADE```                   -
- ```EXPLOSIVE_HESHOT```                    -
- ```EXPLOSIVE_HOMEMADE```                  - Explosions of various power, with or without shrapnel, see `ammo_effects.json` for exact values.
- ```EXPLOSIVE_HOMEMADE_GRENADE_1```        -
- ```EXPLOSIVE_HOMEMADE_GRENADE_2```        -
- ```EXPLOSIVE_HUGE```                      -
- ```EXPLOSIVE_m430a1```                    -
- ```EXPLOSIVE_m433```                      -
- ```EXPLOSIVE_OG7V```                      -
- ```EXPLOSIVE_PG7VL```                     -
- ```EXPLOSIVE_PG7VR```                     -
- ```EXPLOSIVE_RAUFOSS```                   -
- ```EXPLOSIVE_SMALL```                     -
- ```EXPLOSIVE_SMALL_HOMEMADE_GRENADE_1```  -
- ```EXPLOSIVE_SMALL_HOMEMADE_GRENADE_2```  -
- ```EXPLOSIVE_TBG7V```                     -
- ```FLAME``` Very small explosion that lights fires.
- ```FLARE``` Lights the target tile on fire.
- ```FLASHBANG``` Blinds and deafens nearby targets.
- ```FOAMCRETE``` Applies foamcrete effect on hit.
- ```FRAG``` Small explosion that spreads shrapnel ("power": 185, "shrapnel": { "casing_mass": 212, "fragment_mass": 0.025}).
- ```FRAG_20x66``` Small explosion that spreads shrapnel ("power": 40, "shrapnel": { "casing_mass": 15, "fragment_mass": 0.08 }).
- ```GAS_FUNGICIDAL``` Creates a cloud of fungicidal gas on hit.
- ```GAS_INSECTICIDAL``` Creates a cloud of insecticidal gas on hit.
- ```HEAVY_HIT``` Creates a loud sound on hit.
- ```INCENDIARY``` Lights target on fire.
- ```IGNITE``` Lights target on intense and longer-lasting fire.
- ```JET``` Draws a `*` symbol as a flying projectile (unlike usual `#` symbol).
- ```LARGE_BEANBAG``` Heavily stuns the target.
- ```LASER``` Creates a trail of laser (the field type).
- ```LIGHTNING``` Creates a trail of lightning.
- ```magic``` Always best possible hit, do nothing special, no damage mults, nothing.
- ```MININUKE_MOD``` Small thermo-nuclear detonation that leaves behind radioactive fallout.
- ```MUZZLE_SMOKE``` Generate a small cloud of smoke at the source.
- ```NAPALM``` Explosion that spreads fire.
- ```NAPALM_BIG``` Large explosion that spreads fire.
- ```NAPALM_TBG7V``` Creates a large fire on hit.
- ```NEVER_MISFIRES``` Firing ammo without this flag may trigger a misfiring, this is independent of the weapon flags.
- ```NO_DAMAGE_SCALING``` Always set 100% damage due to hit in the weakpoint.
- ```NO_EMBED``` When an item would be spawned from the projectile, it will always be spawned on the ground rather than in a monster's inventory. Implied for active thrown items. Doesn't do anything on projectiles that do not drop items.
- ```NOGIB``` Prevents overkill damage on the target (target won't explode into gibs, see also the monster flag `NOGIB`).
- ```NO_ITEM_DAMAGE``` Will not damage items on the map even when it otherwise would try to.
- ```NON_FOULING``` This ammo does not cause dirtying or blackpowder fouling on the gun when fired.
- ```NO_OVERSHOOT``` Projectiles with this effect won't fly farther than player's set target tile.
- ```NO_PENETRATE_OBSTACLES``` Prevents a projectile from going through a tile with obstacles, such as chainlink fences or dressers.
- ```NPC_AVOID``` NPCs won't use guns or gunmods loaded with ammo with this effect.
- ```NULL_SOURCE``` Projectiles with this effect doesn't have a creature who fired them; applied only to explosives' shrapnel.
- ```PARALYZEPOISON``` Applies paralyzing poison effect on damaging hit.
- ```PLASMA``` Creates a trail of superheated plasma.
- ```PLASMA_BUBBLE``` Creates a cloud of superheated plasma.
- ```PYROPHORIC``` Large explosion that spreads fire of high intensity.
- ```ROBOT_DAZZLE``` Applies sensor-stunning effect to robots.
- ```RECOVER_[X]``` Has a (X-1/X) chance to create a single charge of the used ammo at the point of impact.
- ```RECYCLED``` (For handmade ammo) causes the gun to misfire sometimes; this is independent of the weapon flags.
- ```SHATTER_SELF``` Destroys itself and creates shards on hit.
- ```SHOT``` Multiple smaller pellets; less effective against armor but increases chance to hit and no point-blank penalty.
- ```SMOKE``` Generates a cloud of smoke at the target.
- ```SMOKE_BIG``` Generates a large cloud of smoke at the target.
- ```STREAM``` Leaves a trail of fire fields.
- ```STREAM_BIG``` Leaves a trail of intense fire fields.
- ```STREAM_GAS_FUNGICIDAL``` Leaves a trail of fungicidal gas.
- ```STREAM_GAS_INSCENTICIDAL``` Leaves a trail of insecticidal gas.
- ```TANGLE``` When this projectile hits a target, it has a chance to tangle them up and immobilize them.
- ```TOXICGAS``` Creates a cloud of toxic gas on hit.
- ```TRAIL``` Creates a trail of smoke.
- ```WIDE``` Prevents `HARDTOSHOOT` monster flag from having any effect. Implied by ```SHOT``` or liquid ammo.

### Flags

- ```CASELESS_ROUNDS``` Caseless rounds that cannot be disassembled or reloaded.

## Traps

- ```AVATAR_ONLY``` Only the player character will trigger this trap.
- ```CONVECTS_TEMPERATURE``` This trap convects temperature, like lava.
- ```PIT``` This trap is a version of the pit terrain.
- ```SONAR_DETECTABLE``` This trap can be identified with ground-penetrating SONAR.
- ```UNCONSUMED``` If this trap is a spell type it will not be removed after activation.
- ```UNDODGEABLE``` This trap can't be dodged.

## Armor

### Covers

- ```ARMS``` ... same as ```ARM_L``` and ```ARM_R```
- ```ARM_L```
- ```ARM_R```
- ```EYES```
- ```FEET``` ... same as ```FOOT_L``` and ```FOOT_R```
- ```FOOT_L```
- ```FOOT_R```
- ```HANDS``` ... same as ```HAND_L``` and ```HAND_R```
- ```HAND_L```
- ```HAND_R```
- ```HEAD```
- ```LEGS``` ... same as ```LEG_L``` and ```LEG_R```
- ```LEG_L```
- ```LEG_R```
- ```MOUTH```
- ```TORSO```

### Specifically Covers

- ```torso_upper```
- ```torso_neck```
- ```torso_lower```
- ```torso_hanging_front```
- ```torso_hanging_back```
- ```arm_shoulder_r```
- ```arm_upper_r```
- ```arm_elbow_r```
- ```arm_lower_r```
- ```arm_shoulder_l```
- ```arm_upper_l```
- ```arm_elbow_l```
- ```arm_lower_l```
- ```leg_hip_r```
- ```leg_upper_r```
- ```leg_knee_r```
- ```leg_lower_r```
- ```leg_hip_l```
- ```leg_upper_l```
- ```leg_knee_l```
- ```leg_lower_l```


### Flags

Some armor flags, such as `WATCH` and `ALARMCLOCK` are compatible with other item types. Experiment to find which flags work elsewhere.

- ```ABLATIVE_LARGE``` This item fits in large ablative pockets.
- ```ABLATIVE_MEDIUM``` This item fits in medium ablative pockets.
- ```ACTIVE_CLOAKING``` While active, drains UPS to provide invisibility.
- ```ALARMCLOCK``` Has an alarm-clock feature.
- ```ALLOWS_NATURAL_ATTACKS``` Doesn't prevent any natural attacks or similar benefits from mutations, fingertip razors, etc., like most items covering the relevant body part would.
- ```AURA``` This item goes in the outer aura layer, intended for metaphysical effects.
- ```BAROMETER``` This gear is equipped with an accurate barometer (which is used to measure atmospheric pressure).
- ```BELTED``` Layer for backpacks and things worn over outerwear.
- ```BLIND``` Blinds the wearer while worn, and provides nominal protection v. flashbang flashes.
- ```BLOCK_WHILE_WORN``` Allows worn armor or shields to be used for blocking attacks.
- ```BULLET_IMMUNE``` Wearing an item with this flag makes you immune to bullet damage
- ```CANT_WEAR``` This item can't be worn directly.
- ```COLLAR``` This piece of clothing has a wide collar that can keep your mouth warm.
- ```COMBAT_TOGGLEABLE``` This item is meant to be toggled during combat. Used by NPCs to determine if they will toggle it on during combat. This only supports simple "transform" actions.
- ```DECAY_EXPOSED_ATMOSPHERE``` Consumable will go bad once exposed to the atmosphere (such as MREs).
- ```DEAF``` Makes the player deaf.
- ```MUTE``` Makes the player mute.
- ```ELECTRIC_IMMUNE``` This gear completely protects you from electric discharges.
- ```FANCY``` Wearing this clothing gives a morale bonus if the player has the `Stylish` trait.
- ```FIX_FARSIGHT``` This gear corrects farsightedness.
- ```FIX_NEARSIGHT``` This gear corrects nearsightedness.
- ```FLOTATION``` Prevents the player from drowning in deep water. Also prevents diving underwater.
- ```FRAGILE``` This gear is less resistant to damage than normal.
- ```HOOD``` Allow this clothing to conditionally cover the head, for additional warmth or water protection, if the player's head isn't encumbered
- ```HYGROMETER``` This gear is equipped with an accurate hygrometer (which is used to measure humidity).
- ```INTEGRATED``` This item represents a part of you granted by mutations or bionics.  It will always fit, cannot be unequipped (aside from losing the source), and won't drop on death, but otherwise behaves like normal armor with regards to function, encumbrance, layer conflicts and so on.
- ```NORMAL``` Items worn like normal clothing. This is assumed as default.
- ```NO_TAKEOFF``` Item with that flag can't be taken off.
- ```NO_QUICKDRAW``` Don't offer to draw items from this holster when the fire key is pressed whilst the players hands are empty
- ```NO_WEAR_EFFECT``` This gear doesn't provide any effects when worn (most jewelry).
- ```ONLY_ONE``` You can wear only one.
- ```OUTER```  Outer garment layer.
- ```OVERSIZE``` Can always be worn no matter what encumbrance/mutations/bionics/etc, but prevents any other clothing being worn over this.
- ```PADDED``` This armor counts as comfortable even if none of the specific materials are soft.
- ```PARTIAL_DEAF``` Reduces the volume of sounds to a safe level.
- ```PERSONAL``` This item goes in the personal aura layer, intended for metaphysical effects.
- ```POCKETS``` Increases warmth for hands if the player's hands are cold and the player is wielding nothing.
- ```POWERARMOR_COMPATIBLE``` Makes item compatible with power armor despite other parameters causing failure.
- ```PSYSHIELD_PARTIAL``` 25% chance to protect against `fear_paralyze` monster attack when worn.
- ```RAD_PROOF``` This piece of clothing completely protects you from radiation.
- ```RAD_RESIST``` This piece of clothing partially (75%) protects you from radiation.
- ```RAINPROOF``` Prevents the covered body-part(s) from getting wet in the rain.
- ```REQUIRES_BALANCE``` Gear that requires a certain balance to be steady with. If the player is hit while wearing, they have a chance to be downed.
- ```RESTRICT_HANDS``` Prevents the player from wielding a weapon two-handed, forcing one-handed use if the weapon permits it.
- ```ROLLER_ONE``` A less stable and slower version of `ROLLER_QUAD`, still allows the player to move faster than walking speed.
- ```ROLLER_QUAD```The medium choice between `ROLLER_INLINE` and `ROLLER_ONE`, while it is more stable, and moves faster, it also has a harsher non-flat terrain penalty than `ROLLER_ONE`.
- ```ROLLER_INLINE``` Faster, but less stable overall, the penalty for non-flat terrain is even harsher.
- ```SEMITANGIBLE``` Prevents the item from participating in the encumbrance system when worn.
- ```SKINTIGHT``` Undergarment layer.
- ```SLOWS_MOVEMENT``` This piece of clothing multiplies move cost by 1.1.
- ```SLOWS_THIRST``` This piece of clothing multiplies the rate at which the player grows thirsty by 0.70.
- ```STURDY``` This clothing is a lot more resistant to damage than normal.
- ```SUN_GLASSES``` Prevents glaring when in sunlight.
- ```SUPER_FANCY``` Gives an additional moral bonus over `FANCY` if the player has the `Stylish` trait.
- ```SWIM_GOGGLES``` Allows you to see much further underwater.
- ```THERMOMETER``` This gear is equipped with an accurate thermometer (which is used to measure temperature).
- ```VARSIZE``` Can be made to fit via tailoring.
- ```WAIST``` Layer for belts other things worn on the waist.
- ```WATCH``` Acts as a watch and allows the player to see actual time.
- ```WATERPROOF``` Prevents the covered body-part(s) from getting wet in any circumstance.
- ```WATER_FRIENDLY``` Prevents the item from making the body part count as unfriendly to water and thus reducing morale from being wet.


## Bionics

- ```BIONIC_ARMOR_INTERFACE``` This bionic can provide power to powered armor.
- ```BIONIC_FAULTY``` This bionic is a "faulty" bionic.
- ```BIONIC_GUN``` This bionic is a gun bionic and activating it will fire it.  Prevents all other activation effects.
- ```BIONIC_NPC_USABLE``` The NPC AI knows how to use this CBM, and it can be installed on an NPC.
- ```BIONIC_POWER_SOURCE``` This bionic is a power source bionic.
- ```BIONIC_SLEEP_FRIENDLY``` This bionic won't prompt the user to turn it off if they try to sleep while it's active.
- ```BIONIC_TOGGLED``` This bionic only has a function when activated, else it causes its effect every turn.
- ```BIONIC_WEAPON``` This bionic is a weapon bionic and activating it will create (or destroy) bionic's fake_item in user's hands.  Prevents all other activation effects.
- ```BIONIC_SHOCKPROOF``` This bionic can't be incapacitated by electrical attacks.
- ```USES_BIONIC_POWER``` If present, items attached to this bionic will inherit the `USES_BIONIC_POWER` flag automatically.


## Bodyparts

- ```ALWAYS_BLOCK``` This nonstandard bodypart is always eligible to block in unarmed combat even if your martial arts don't allow such blocks.
- ```ALWAYS_HEAL``` This bodypart regenerates every regen tick (5 minutes, currently) regardless if the part would have healed normally.
- ```HEAL_OVERRIDE``` This bodypart will always regenerate its `heal_bonus` HP instead of it modifying the base healing step. Without `ALWAYS_HEAL` this still only happens when the part would have healed non-zero amount of damage.
- ```IGNORE_TEMP``` This bodypart is ignored for temperature calculations
- ```LIMB_LOWER``` This bodypart is close to the ground, and as such has a higher chance to be attacked by small monsters - hitsize is tripled for creatures that can't attack upper limbs.
- ```LIMB_UPPER``` This bodypart is high off the ground, and as such can't be attacked by small monsters - unless they have the `FLIES` or have `ATTACK_UPPER` flags`
- ```MEND_LIMB``` This bodypart can heal from being broken without needing a splint.
- ```NONSTANDARD_BLOCK``` This limb is different enough that martial arts' arm/leg blocks aren't applicable - blocking with this limb is unlocked by reaching the MA's `nonstandard_block` level, unless the limb also has `ALWAYS_BLOCK`. Either block flag is **required** for non-arm / non-leg limbs to be eligible to block.


## Books

- ```INSPIRATIONAL``` Reading this book grants bonus morale to characters with the `SPIRITUAL` trait.
- ```BINDER_ADD_RECIPE``` Add recipe to a book binder.


## Character

- ```BLEED_IMMUNE``` Immune to bleeding.
- ```COLDBLOOD``` For heat dependent mutations.
- ```COLDBLOOD2``` For very heat dependent mutations.
- ```COLDBLOOD3``` For cold-blooded mutations.
- ```ECTOTHERM``` For ectothermic mutations, like `COLDBLOOD4` and `DRAGONBLOOD3` (Black Dragon from Magiclysm).
- ```HEAT_IMMUNE``` Immune to very hot temperatures.
- ```NO_DISEASE``` This mutation grants immunity to diseases.
- ```NO_THIRST``` Your thirst is not modified by food or drinks.
- ```NO_RADIATION``` This mutation grants immunity to radiations.
- ```NO_MINIMAL_HEALING``` This mutation disables the minimal healing of 1 hp a day.
- ```INFECTION_IMMUNE``` This mutation grants immunity to infections, including infection from bites and tetanus.
- ```SUPER_HEARING``` You can hear much better than a normal person.
- ```IMMUNE_HEARING_DAMAGE``` Immune to hearing damage from loud sounds.
- ```CANNIBAL``` Butcher humans, eat foods with the `CANNIBALISM` and `STRICT_HUMANITARIANISM` flags without a morale penalty
- ```CLIMB_NO_LADDER``` Capable of climbing up single-level walls without support.
- ```DEAF``` Makes you deaf.
- ```BLIND``` Makes you blind.
- ```EYE_MEMBRANE``` Lets you see underwater.
- ```NO_SCENT``` You have no scent.
- ```STOP_SLEEP_DEPRIVATION``` Stops Sleep Deprivation while awake and boosts it while sleeping.
- ```GLARE_RESIST``` Protect your eyes from glare like sunglasses.
- ```HYPEROPIC``` You are far-sighted - close combat is hampered and reading is impossible without glasses.
- ```MYOPIC``` You are nearsighted - vision range is severely reduced without glasses.
- ```MYOPIC_IN_LIGHT``` You are nearsighted in light, but can see normally in low-light conditions.
- ```MEND_ALL``` You need no splint to heal broken bones.
- ```NIGHT_VISION``` You can see in the dark.
- ```INFRARED``` You can see infrared, aka heat vision.
- ```SEESLEEP``` You can see while sleeping, and aren't bothered by light when trying to fall asleep.
- ```ELECTRIC_IMMUNE``` You are immune to electric damage.
- ```COLD_IMMUNE``` You are immune to cold damage.
- ```BIO_IMMUNE``` You are immune to biological damage.
- ```BASH_IMMUNE``` You are immune to bashing damage.
- ```CUT_IMMUNE``` You are immune to cutting damage.
- ```STAB_IMMUNE``` You are immune to stabbing damage.
- ```ACID_IMMUNE``` You are immune to acid damage.
- ```BULLET_IMMUNE``` You are immune to bullet damage.
- ```WATCH``` You always know what time it is.
- ```ALARMCLOCK``` You always can set alarms.
- ```PARAIMMUNE``` You are immune to parasites.
- ```IMMUNE_SPOIL``` You are immune to negative outcomes from spoiled food.
- ```FEATHER_FALL``` You are immune to fall damage.
- ```INVISIBLE``` You can't be seen.
- ```DIMENSIONAL_ANCHOR``` You can't be teleported.
- ```PORTAL_PROOF``` You are immune to personal portal storm effects.
- ```CLIMATE_CONTROL``` You are resistant to extreme temperatures.
- ```HEATSINK``` You are resistant to extreme heat.
- ```THERMOMETER``` You always know what temperature it is.
- ```CBQ_LEARN_BONUS``` You learn CBQ from the bionic bio_cqb faster.
- ```GILLS``` You can breathe underwater.
- ```HARDTOHIT``` Whenever something attacks you, RNG gets rolled twice, and you get the better result.
- ```HUGE``` Changes your size to `creature_size::huge`.  Checked last of the size category flags, if no size flags are found your size defaults to `creature_size::medium`.
- ```LARGE``` Changes your size to `creature_size::large`.  Checked third of the size category flags.
- ```PSYCHOPATH``` Butcher humans without a morale penalty
- ```PRED1``` Small morale bonus from foods with the `PREDATOR_FUN` flag.  Lower morale penalty from the guilt mondeath effect.
- ```PRED2``` Learn combat skills with double catchup modifier.  Resist skill rust on combat skills. Small morale bonus from foods with the `PREDATOR_FUN` flag.  Lower morale penalty from the guilt mondeath effect.
- ```PRED3``` Learn combat skills with double catchup modifier.  Resist skill rust on combat skills. Medium morale bonus from foods with the `PREDATOR_FUN` flag.  Immune to the guilt mondeath effect.
- ```PRED4``` Learn combat skills with triple catchup modifier.  Learn combat skills without spending focus.  Resist skill rust on combat skills. Large morale bonus from foods with the `PREDATOR_FUN` flag.  Immune to the guilt mondeath effect.
- ```SAPIOVORE``` Butcher humans without a morale penalty
- ```SMALL``` Changes your size to `creature_size::small`.  Checked second of the size category flags.
- ```STEADY``` Your speed can never go below base speed, bonuses from effects etc can still apply.
- ```STRICT_HUMANITARIAN``` You can eat foodstuffs tagged with `STRICT_HUMANITARIANISM` without morale penalties.
- ```TINY``` Changes your size to `creature_size::tiny`.  Checked first of the size category flags.
- ```WEBBED_FEET``` You have webbings on your feet, supporting your swimming speed if not wearing footwear.
- ```WEBBED_HANDS``` You have webbings on your hands, supporting your swimming speed.
- ```WEB_RAPPEL``` You can rappel down staircases and sheer drops of any height.
- ```WALL_CLING``` You can ascend/descend sheer cliffs as long as the tile above borders at least one wall. Chance to slip and fall each step.
- ```WALL_CLING_FOURTH``` Same as `WALL_CLING`, but you need four instances of the flag for it to function (ex. four bodyparts with the flag).
- ```WINGS_1``` You have 50% chance to ignore falling traps (including ledges).
- ```WINGS_2``` You have 100% chance to ignore falling traps (including ledges).  Requires two flag instances.


## Comestibles

### Comestible type

- ```DRINK```
- ```FOOD```
- ```MED```

### Addiction type

- ```alcohol```
- ```amphetamine```
- ```caffeine```
- ```cocaine```
- ```crack```
- ```nicotine```
- ```opiate```
- ```sleeping pill```

### Use action

- ```ALCOHOL_STRONG``` Greatly increases drunkenness. Adds disease `drunk`.
- ```ALCOHOL_WEAK``` Slightly increases drunkenness. Adds disease `drunk`
- ```ALCOHOL``` Increases drunkenness. Adds disease `drunk`.
- ```ANTIBIOTIC``` Helps fight infections. Removes disease `infected` and adds disease `recover`.
- ```BANDAGE``` Stop bleeding.
- ```BIRDFOOD``` Makes a small bird friendly.
- ```BLECH``` Causes vomiting, adds disease `poison`, adds pain and hurts torso.
- ```BLECH_BECAUSE_UNCLEAN``` Causes warning.
- ```CATFOOD``` Makes a cat friendly.
- ```CATTLEFODDER``` Makes a large herbivore friendly.
- ```CHEW``` Displays message "You chew your %s.", but otherwise does nothing.
- ```CIG``` Alleviates nicotine cravings. Adds disease `cig`.
- ```COKE``` Decreases hunger. Adds disease `high`.
- ```CRACK``` Decreases hunger. Adds disease `high`.
- ```DISINFECTANT``` Prevents infections.
- ```DOGFOOD``` Makes a dog friendly.
- ```FIRSTAID``` Heals.
- ```FLUMED``` Adds disease `took_flumed`.
- ```FLUSLEEP``` Adds disease `took_flumed` and increases fatigue.
- ```FUNGICIDE``` Kills fungus and spores. Removes diseases `fungus` and `spores`.
- ```HALLU``` Adds disease `hallu`.
- ```HONEYCOMB``` Spawns wax.
- ```INHALER``` Removes disease `asthma`.
- ```IODINE``` Adds disease `iodine`.
- ```MARLOSS``` "As you eat the berry, you have a near-religious experience, feeling at one with your surroundings..."
- ```METH``` Adds disease `meth`.
- ```NONE``` "You can't do anything of interest with your [x]."
- ```PKILL``` Reduces pain. Adds disease `pkill[n]` where `[n]` is the level of flag `PKILL_[n]` used on this comestible.
- ```PLANTBLECH``` Activates `BLECH` iuse action if player does not have plant mutations.
- ```POISON``` Adds diseases `poison` and `foodpoison`.
- ```PROZAC``` Adds disease `took_prozac` if not currently present, otherwise acts as a minor stimulant. Rarely has the `took_prozac_bad` adverse effect.
- ```PURIFIER``` Removes random number of negative mutations.
- ```SEWAGE``` Causes vomiting.
- ```SLEEP``` Greatly increases fatigue.
- ```THORAZINE``` Removes diseases `hallu`, `visuals`, `high`. Additionally removes disease `formication` if disease `dermatik` isn't also present. Has a chance of a negative reaction which increases fatigue.
- ```VITAMINS``` Increases healthiness (not to be confused with HP).
- ```WEED``` Makes you roll with Cheech & Chong. Adds disease `weed_high`.
- ```XANAX``` Alleviates anxiety. Adds disease `took_xanax`.

### Flags

- ```ACID``` When consumed using the `BLECH` function, penalties are reduced if character has `ACIDPROOF` or `ACIDBLOOD` traits.
- ```CARNIVORE_OK``` Can be eaten by characters with the Carnivore mutation.
- ```CANT_HEAL_EVERYONE``` This med can't be used by everyone, it requires a special mutation. See `can_heal_with` in mutation.
- ```CORROSIVE``` when consumed using the `BLECH` function, causes the same penalties as `ACID` but is not affected by `ACIDPROOF` or `ACIDBLOOD` traits.
- ```EATEN_COLD``` Morale bonus for eating cold.
- ```EATEN_HOT``` Morale bonus for eating hot.
- ```EDIBLE_FROZEN``` Being frozen doesn't prevent eating it. No morale bonus.
- ```INEDIBLE``` Inedible by default, enabled to eat when in conjunction with (mutation threshold) flags: `BIRD`, `CATTLE`.
- ```FERTILIZER``` Works as fertilizer for farming, of if this consumed with the `PLANTBLECH` function penalties will be reversed for plants.
- ```FREEZERBURN``` First thaw is `MUSHY`, second is rotten.
- ```FUNGAL_VECTOR``` Will give a fungal infection when consumed.
- ```HIDDEN_HALLU``` Food causes hallucinations, visible only with a certain survival skill level.
- ```HIDDEN_POISON``` Food displays as poisonous with a certain survival skill level. Note that this doesn't make items poisonous on its own, consider adding `"use_action": [ "POISON" ]` as well, or using `FORAGE_POISON` instead.
- ```MELTS``` Provides half fun unless frozen. Edible when frozen.
- ```MILLABLE``` Can be placed inside a mill, to turn into flour.
- ```MUTAGEN_CATALYST``` Injecting it will jumpstart mutation.
- ```MUTAGEN_PRIMER``` Injecting it will prime your body for mutation.
- ```MYCUS_OK``` Can be eaten by post-threshold Mycus characters. Only applies to Mycus fruits by default.
- ```NEGATIVE_MONOTONY_OK``` Allows ```negative_monotony``` property to lower comestible fun to negative values.
- ```NO_AUTO_CONSUME``` Consumables with this flag would not get consumed in auto-eat/auto-drink zone.
- ```NO_INGEST``` Administered by some means other than oral intake.
- ```NUTRIENT_OVERRIDE``` When you craft an item, game checks if it's a comestible, and if it is, it stores the components the item was created from. The "NUTRIENT_OVERRIDE" flag will skip this step.
- ```PKILL_1``` Minor painkiller.
- ```PKILL_2``` Moderate painkiller.
- ```PKILL_3``` Heavy painkiller.
- ```PKILL_L``` Slow-release painkiller.
- ```RAD_STERILIZED``` Irradiated food that is safe to eat, but is not edible forever (such as MREs).
- ```RAD_STERILIZED_EDIBLE_FOREVER``` Irradiated food that is safe to eat and remains edible forever.
- ```RAW``` Reduces kcal by 25%, until cooked (that is, used in a recipe that requires a heat source). Should be added to *all* uncooked food, unless that food derives more than 50% of its calories from sugars (i.e. many fruits, some veggies) or fats (i.e. butchered fat, coconut). TODO: Make a unit test for these criteria after fat/protein/carbs are added.
- ```SMOKABLE``` Accepted by smoking rack.
- ```SMOKED``` Not accepted by smoking rack (product of smoking).
- ```USE_EAT_VERB``` "You drink your %s." or "You eat your %s."
- ```USE_ON_NPC``` Can be used on NPCs (not necessarily by them).
- ```ZOOM``` Zoom items can increase your overmap sight range.


## Effects
Effect flags. These are checked by hardcode for monsters (introducing new flags will require C++ changes), but for characters are considered "character flags", meaning new ones can be implemented in JSON alone - see Character Flags.

### Flags

- ```DISABLE_FLIGHT``` Monsters affected by an effect with this flag will never count as flying (even if they have the `FLIES` flag).
- ```EFFECT_IMPEDING``` Character affected by an effect with this flag can't move until they break free from the effect.  Breaking free requires a strength check: `x_in_y( STR * limb lifting score * limb grip score, 6 * get_effect_int( eff_id )`

## Furniture and Terrain

List of known flags, used in both `terrain.json` and `furniture.json`.

### Flags

- ```ALARMED``` Sets off an alarm if smashed.
- ```ALIGN_WORKBENCH``` (only for furniture) A hint to the tiles display that the sprite for this furniture should face toward any adjacent tile with a workbench quality.
- ```ALLOW_FIELD_EFFECT``` Apply field effects to items inside `SEALED` terrain/furniture.
- ```ALLOW_ON_OPEN_AIR``` Don't warn when this furniture is placed on `t_open_air` or similar 'open air' terrains which lack a floor.
- ```AMMOTYPE_RELOAD``` Furniture reloads by ammotype so player can choose from more than one fuel type.
- ```AUTO_WALL_SYMBOL``` (only for terrain) The symbol of this terrain will be one of the line drawings (corner, T-intersection, straight line etc.) depending on the adjacent terrains.

    Example: `-` and `|` are both terrain with the `CONNECT_WITH_WALL` flag. `O` does not have the flag, while `X` and `Y` have the `AUTO_WALL_SYMBOL` flag.

    `X` terrain will be drawn as a T-intersection (connected to west, south and east), `Y` will be drawn as horizontal line (going from west to east, no connection to south).
    ```
    -X-    -Y-
     |      O
    ```

- ```BARRICADABLE_DOOR_DAMAGED```
- ```BARRICADABLE_DOOR_REINFORCED_DAMAGED```
- ```BARRICADABLE_DOOR_REINFORCED```
- ```BARRICADABLE_DOOR``` Door that can be barricaded.
- ```BARRICADABLE_WINDOW_CURTAINS```
- ```BARRICADABLE_WINDOW``` Window that can be barricaded.
- ```BLOCK_WIND``` This terrain will block the effects of wind.
- ```BURROWABLE``` Burrowing monsters can travel under this terrain, while most others can't (e.g. graboid will traverse under the chain link fence, while ordinary zombie will be stopped by it).
- ```BUTCHER_EQ``` Butcher's equipment - required for full butchery of corpses.
- ```CAN_SIT``` Furniture the player can sit on. Player sitting near furniture with the `FLAT_SURF` tag will get mood bonus for eating.
- ```CHIP``` Used in construction menu to determine if wall can have paint chipped off.
- ```COLLAPSES``` Has a roof that can collapse.
- ```CONNECT_WITH_WALL``` (only for terrain) This flag has been superseded by the JSON entries `connect_group` and `connects_to`, but is retained for backward compatibility.
- ```CONSOLE``` Used as a computer.
- ```CONTAINER``` Items on this square are hidden until looted by the player.
- ```CURRENT``` This water is flowing.
- ```DEEP_WATER``` This is water that can submerge the player.
- ```DESTROY_ITEM``` Items that land here are destroyed. See also `NOITEM`.
- ```DIFFICULT_Z``` Most zombies will not be able to follow you up this terrain (i.e a ladder).
- ```DIGGABLE_CAN_DEEPEN``` Diggable location can be dug again to make deeper (e.g. shallow pit to deep pit).
- ```DIGGABLE``` Digging monsters, seeding monster, digging with shovel, etc.
- ```DOOR``` Can be opened (used for NPC path-finding).
- ```EASY_DECONSTRUCT``` Player can deconstruct this without tools.
- ```ELEVATOR``` Terrain with this flag will move player, NPCs, monsters, and items up and down when player activates nearby `elevator controls`.
- ```FIRE_CONTAINER``` Stops fire from spreading (brazier, wood stove, etc).
- ```FISHABLE``` You can try to catch fish here.
- ```FLAMMABLE_ASH``` Burns to ash rather than rubble.
- ```FLAMMABLE_HARD``` Harder to light on fire, but still possible.
- ```FLAMMABLE``` Can be lit on fire.
- ```FLAT_SURF``` Furniture or terrain with a flat hard surface (e.g. table, but not chair; tree stump, etc.).
- ```FLAT``` Player can build and move furniture on.
- ```FORAGE_HALLU``` This item can be found with the `HIDDEN_HALLU` flag when found through foraging.
- ```FORAGE_POISION``` This item can be found with the `HIDDEN_POISON` flag when found through foraging.
- ```FRESH_WATER``` Source of fresh water. Will spawn fresh water (once) on terrains with `SPAWN_WITH_LIQUID` flag.
- ```GOES_DOWN``` Can use <kbd>></kbd> to go down a level.
- ```GOES_UP``` Can use <kbd><</kbd> to go up a level.
- ```GROWTH_SEEDLING``` This plant is in its seedling stage of growth.
- ```GROWTH_MATURE``` This plant is in a mature stage of a growth.
- ```GROWTH_HARVEST``` This plant is ready for harvest.
- ```HARVESTED``` Marks the harvested version of a terrain type (e.g. harvesting an apple tree turns it into a harvested tree, which later becomes an apple tree again).
- ```HIDE_PLACE``` Creatures on this tile can't be seen by creatures not standing on adjacent tiles.
- ```INDOORS``` Has a roof over it; blocks rain, sunlight, etc.
- ```LADDER``` This piece of furniture that makes climbing easy.
- ```LIQUIDCONT``` Furniture that contains liquid, allows for contents to be accessed in some checks even if `SEALED`.
- ```LIQUID``` Blocks movement, but isn't a wall (lava, water, etc.)
- ```MINEABLE``` Can be mined with a pickaxe/jackhammer.
- ```MOUNTABLE``` Suitable for guns with the `MOUNTED_GUN` flag.
- ```MURKY``` Liquid taken from tiles with this flag is badly poisoned (almost on par with sewage).
- ```NOCOLLIDE``` Feature that simply doesn't collide with vehicles at all.
- ```NOITEM``` Items cannot be added here but may overflow to adjacent tiles. See also `DESTROY_ITEM`.
- ```NO_FLOOR``` Things should fall when placed on this tile.
- ```NO_PICKUP_ON_EXAMINE``` Examining this tile (<kbd>e</kbd> by default) won't open Pick Up menu even if there are items here.
- ```NO_SIGHT``` Creature on this tile have their sight reduced to one tile.
- ```NO_SCENT``` This tile cannot have scent values, which prevents scent diffusion through this tile.
- ```NO_SHOOT``` Terrain with this flag cannot be damaged by ranged attacks, and ranged attacks will not pass through it.
- ```NO_SPOIL``` Items placed in this tile do not spoil.
- ```OPENCLOSE_INSIDE``` If it's a door (with an 'open' or 'close' field), it can only be opened or closed if you're inside.
- ```PAINFUL``` May cause a small amount of pain.
- ```PERMEABLE``` Permeable for gases.
- ```PICKABLE``` This terrain/furniture could be picked with lockpicks.
- ```PIT_FILLABLE``` This terrain can be filled with dirt like a shallow pit.
- ```PLACE_ITEM``` Valid terrain for `place_item()` to put items on.
- ```PLANT``` A 'furniture' that grows and fruits.
- ```PLANTABLE``` This terrain or furniture can have seeds planted in it.
- ```PLOWABLE``` Terrain can be plowed.
- ```RAMP_END``` Technical flag for proper work of ramps mechanics.
- ```RAMP``` Can be used to move up a z-level.
- ```REDUCE_SCENT``` Reduces scent diffusion (not total amount of scent in area); only works if also bashable.
- ```ROAD``` Flat and hard enough to drive or skate (with rollerblades) on.
- ```ROUGH``` May hurt the player's feet.
- ```RUG``` Enables the `Remove Carpet` Construction entry.
- ```SALT_WATER``` Source of salt water (works for terrains with examine action "water_source").
- ```SEALED``` Can't use <kbd>e</kbd> to retrieve items; must smash them open first.
- ```SEEN_FROM_ABOVE``` Visible from a higher level (provided the tile above has no floor)
- ```SHALLOW_WATER``` This is water that is not deep enough to submerge the player.
- ```SHARP``` May do minor damage to players/monsters passing through it.
- ```SHORT``` Feature too short to collide with vehicle protrusions. (mirrors, blades).
- ```SIGN``` Show written message on examine.
- ```SMALL_PASSAGE``` This terrain or furniture is too small for large or huge creatures to pass through.
- ```SPAWN_WITH_LIQUID``` This terrain will place liquid (once) on its own spawn. Type of liquid is defined by other flags. For example, it spawns fresh water via `FRESH_WATER` flag.
- ```SUN_ROOF_ABOVE``` This furniture (terrain is not supported currently) has a "fake roof" above, that blocks sunlight. Special hack for #44421, to be removed later.
- ```SUPPORTS_ROOF``` Used as a boundary for roof construction.
- ```SUPPRESS_SMOKE``` Prevents smoke from fires; used by ventilated wood stoves, etc.
- ```SWIMMABLE``` Player and monsters can swim through it.
- ```THIN_OBSTACLE``` ```SPEAR``` attacks can go through this to hit something on the other side.
- ```TINY``` Feature too short to collide with vehicle undercarriage. Vehicles drive over them with no damage, unless a wheel hits them.
- ```TOILET_WATER``` Liquid taken from tiles with this flag is rather dirty and may poison you.
- ```TRANSPARENT``` Players and monsters can see through/past it. Also sets ter_t.transparent.
- ```TRANSPARENT_FLOOR``` This terrain allows light to the z-level below.
- ```UNSTABLE``` Walking here cause the bouldering effect on the character.
- ```USABLE_FIRE``` This terrain or furniture counts as a nearby fire for crafting.
- ```WALL``` This terrain is an upright obstacle. Used for fungal conversion, and also implies `CONNECT_WITH_WALL`.
- ```WINDOW``` This terrain is a window, though it may be closed, broken, or covered up.  Used by the tiles code to align furniture sprites away from the window.
- ```WIRED_WALL``` This terrain is a wall with electric wires inside. Allows the `Reveal wall wirings` construction.
- ```WORKOUT_LEGS``` This furniture is for training your legs. Needed for checks like `is_limb_broken()`.
- ```WORKOUT_ARMS``` This furniture is for training your arms. Needed for checks like `is_limb_broken()`.

### Fungal Conversions Only

- ```FLOWER``` This furniture is a flower.
- ```FUNGUS``` Fungal covered.
- ```ORGANIC``` This furniture is partly organic.
- ```SHRUB``` This terrain is a shrub.
- ```TREE``` This terrain is a tree.
- ```YOUNG``` This terrain is a young tree.

### Furniture Only

- ```ACTIVE_GENERATOR``` This furniture is considered to be an active power source for the purpose of certain monster special attacks (e.g. milspec searchlight's SEARCHLIGHT)
- ```AUTODOC``` This furniture can be an Autodoc console, it also needs the `autodoc` examine action.
- ```AUTODOC_COUCH``` This furniture can be a couch for a furniture with the `autodoc` examine action.
- ```BLOCKSDOOR``` This will boost map terrain's resistance to bashing if `str_*_blocked` is set (see `map_bash_info`).
- ```BRIDGE``` If this furniture is placed over water tiles, it prevents player from becoming wet.


## Generic

These flags can be applied via JSON item definition to most items.  Not to be confused with the set of flags listed under Tools > Flags that apply to items, which cannot be assigned via JSON.

### Flags

- ```BIONIC_NPC_USABLE``` ... Safe CBMs that NPCs can use without extensive NPC rewrites to utilize toggle CBMs.
- ```BIONIC_TOGGLED``` ... This bionic only has a function when activated, instead of causing its effect every turn.
- ```BIONIC_POWER_SOURCE``` ... This bionic is a source of bionic power.
- ```BIONIC_SHOCKPROOF``` ... This bionic can't be incapacitated by electrical attacks.
- ```BIONIC_FAULTY``` ... This bionic is a "faulty" bionic.
- ```BIONIC_WEAPON``` ... This bionic is a weapon bionic and activating it will create (or destroy) its fake_item in the user's hands. Prevents all other activation effects.
- ```BIONIC_ARMOR_INTERFACE``` ... This bionic can provide power to powered armor.
- ```BIONIC_SLEEP_FRIENDLY``` ... This bionic won't provide a warning if the player tries to sleep while it's active.
- ```BIONIC_GUN``` ... This bionic is a gun bionic and activating it will fire it. Prevents all other activation effects.
- ```CONDUCTIVE``` ... Item is considered as conducting electricity, even if material it's made of is non-conductive. Opposite of `NONCONDUCTIVE`.
- ```CORPSE``` ... Flag used to spawn various human corpses during the mapgen.
- ```CRUTCHES``` ... Item with this flag helps characters not to fall down if their legs are broken.
- ```DANGEROUS``` ... NPCs will not accept this item. Explosion iuse actor implies this flag. Implies `NPC_THROW_NOW`.
- ```DETERGENT``` ... This item can be used as a detergent in a washing machine.
- ```DURABLE_MELEE``` ... Item is made to hit stuff and it does it well, so it's considered to be a lot tougher than other weapons made of the same materials.
- ```ELECTRONIC``` ... This item contain sensitive electronics which can be fried by nearby EMP blast.
- ```FAKE_MILL``` ... Item is a fake item, to denote a partially milled product by @ref Item::process_fake_mill, where conditions for its removal are set.
- ```FAKE_SMOKE``` ... Item is a fake item generating smoke, recognizable by @ref item::process_fake_smoke, where conditions for its removal are set.
- ```FIREWOOD``` ... This item can serve as a firewood. Items with this flag are sorted out to "Loot: Wood" zone
- ```FRAGILE_MELEE``` ... Fragile items that fall apart easily when used as a weapon due to poor construction quality and will break into components when broken.
- ```GAS_DISCOUNT``` ... Discount cards for the automated gas stations.
- ```ITEM_BROKEN``` ... Item was broken and won't activate anymore.
- ```IS_PET_ARMOR``` ... Is armor for a pet monster, not armor for a person.
- ```LEAK_ALWAYS``` ... Leaks (may be combined with `RADIOACTIVE`).
- ```LEAK_DAM``` ... Leaks when damaged (may be combined with `RADIOACTIVE`).
- ```MISSION_ITEM``` ... This item's chance to spawn isn't affected by world item spawn scaling factor.
- ```MOP``` ... This item could be used to mop up spilled liquids like blood or water.
- ```NEEDS_UNFOLD``` ... Has an additional time penalty upon wielding. For melee weapons and guns this is offset by the relevant skill. Stacks with `SLOW_WIELD`.
- ```NO_PACKED``` ... This item is not protected against contamination and won't stay sterile.  Only applies to CBMs.
- ```NO_REPAIR``` ... Prevents repairing of this item even if otherwise suitable tools exist.
- ```NO_SALVAGE``` ... Item cannot be broken down through a salvage process. Best used when something should not be able to be broken down (i.e. base components like leather patches).
- ```NO_STERILE``` ... This item is not sterile.  Only applies to CBMs.
- ```NPC_ACTIVATE``` ... NPCs can activate this item as an alternative attack. Currently done by throwing it right after activation. Implied by `BOMB`.
- ```NPC_ALT_ATTACK``` ... Shouldn't be set directly. Implied by `NPC_ACTIVATE` and `NPC_THROWN`.
- ```NPC_THROWN``` ... NPCs will throw this item (without activating it first) as an alternative attack.
- ```NPC_THROW_NOW``` ... NPCs will try to throw this item away, preferably at enemies. Implies `TRADER_AVOID` and `NPC_THROWN`.
- ```OLD_CURRENCY``` ... Paper bills and coins that used to be legal tender before the Cataclysm and may still be accepted by some automated systems.
- ```PERFECT_LOCKPICK``` ... Item is a perfect lockpick. Takes only 5 seconds to pick a lock and never fails, but using it grants only a small amount of lock picking xp. The item should have `LOCKPICK` quality of at least 1.
- ```PRESERVE_SPAWN_OMT``` ... This item will store the OMT that it spawns in, in the `spawn_location_omt` item var.
- ```PSEUDO``` ... Used internally to mark items that are referred to in the crafting inventory but are not actually items. They can be used as tools, but not as components. Implies `TRADER_AVOID`.
- ```RADIOACTIVE``` ... Is radioactive (can be used with `LEAK_*`).
- ```RAIN_PROTECT``` ... Protects from sunlight and from rain when wielded.
- ```REDUCED_BASHING``` ... Gunmod flag; reduces the item's bashing damage by 50%.
- ```REDUCED_WEIGHT``` ... Gunmod flag; reduces the item's base weight by 25%.
- ```REQUIRES_TINDER``` ... Requires tinder to be present on the tile this item tries to start a fire on.
- ```SINGLE_USE``` ... This item is deleted after being used. Items that count by charge do not need this as they are deleted when charges run out.
- ```SLEEP_AID``` ... This item helps in sleeping.
- ```SLEEP_AID_CONTAINER``` ... This item allows sleep aids inside of it to help in sleeping. (E.g. this is a pillowcase).
- ```SLEEP_IGNORE``` ... This item is not shown as before-sleep warning.
- ```SLOW_WIELD``` ... Has an additional time penalty upon wielding. For melee weapons and guns this is offset by the relevant skill. Stacks with `NEEDS_UNFOLD`.
- ```TACK``` ... Item can be used as tack for a mount.
- ```TARDIS``` ... Container item with this flag bypasses internal checks for pocket data, so inside it could be bigger than on the outside, and could hold items that otherwise won't fit its dimensions.
- ```TIE_UP``` ... Item can be used to tie up a creature.
- ```TINDER``` ... This item can be used as tinder for lighting a fire with a `REQUIRES_TINDER` flagged firestarter.
- ```TRADER_AVOID``` ... NPCs will not start with this item. Use this for active items (e.g. flashlight (on)), dangerous items (e.g. active bomb), fake items or unusual items (e.g. unique quest item).
- ```TRADER_KEEP``` ... NPCs will not trade this item away under any circumstances.
- ```TRADER_KEEP_EQUIPPED``` ... NPCs will only trade this item if they aren't currently wearing or wielding it.
- ```UNBREAKABLE``` ... This item can not be damaged, be that directly, while worn as armor, or when used as a melee weapon.
- ```UNBREAKABLE_MELEE``` ... Never gets damaged when used as melee weapon.
- ```UNRECOVERABLE``` ... Cannot be recovered from a disassembly.
- ```WATER_BREAK``` ... Item is broken in water.
- ```WATER_BREAK_ACTIVE``` ... Item can get wet and is broken in water if active.
- ```WATER_DISSOLVE``` ... Item is dissolved in water.
- ```ZERO_WEIGHT``` Normally items with zero weight will generate an error. Use this flag to indicate that zero weight is intentional and suppress that error.


## Guns

- ```BACKBLAST``` Causes a small explosion behind the person firing the weapon. Currently not implemented?
- ```BIPOD``` Handling bonus only applies on `MOUNTABLE` map/vehicle tiles. Does not include wield time penalty (see `SLOW_WIELD`).
- ```CHARGE``` Has to be charged to fire. Higher charges do more damage.
- ```COLLAPSIBLE_STOCK``` Reduces weapon volume proportional to the base size of the gun (excluding any mods). Does not include wield time penalty (see `NEEDS_UNFOLD`).
- ```CONSUMABLE``` Makes a gunpart have a chance to get damaged depending on ammo fired, and definable fields 'consume_chance' and 'consume_divisor'.
- ```DISABLE_SIGHTS``` Prevents use of the base weapon sights.
- ```FIRE_TWOHAND``` Gun can only be fired if player has two free hands.
- ```IRREMOVABLE``` Makes so that the gunmod cannot be removed.
- ```MECH_BAT``` This is an exotic battery designed to power military mechs.
- ```MOUNTED_GUN``` Gun can only be used on terrain / furniture with the `MOUNTABLE` flag.
- ```NEVER_JAMS``` Never malfunctions.
- ```NO_TURRET``` Prevents generation of a vehicle turret prototype for this gun.
- ```NO_UNLOAD``` Cannot be unloaded.
- ```PRIMITIVE_RANGED_WEAPON``` Allows using non-gunsmith tools to repair (but not reinforce) it.
- ```PUMP_ACTION``` Gun has rails on its pump action, allowing to install only mods with `PUMP_RAIL_COMPATIBLE flag` on underbarrel slot.
- ```PUMP_RAIL_COMPATIBLE``` Mod can be installed on underbarrel slot of guns with rails on their pump action.
- ```RELOAD_AND_SHOOT``` Firing automatically reloads and then shoots.
- ```RELOAD_EJECT``` Ejects shell from gun on reload instead of when fired.
- ```RELOAD_ONE``` Only reloads one round at a time.
- ```STR_DRAW``` Range with this weapon is reduced unless character has at least twice the required minimum strength.
- ```STR_RELOAD``` Reload speed is affected by strength.
- ```UNDERWATER_GUN``` Gun is optimized for usage underwater, does perform badly outside of water.
- ```WATERPROOF_GUN``` Gun does not rust and can be used underwater.
- ```NON_FOULING``` Gun does not become dirty or blackpowder fouled.

### Firing modes

- ```MELEE``` Melee attack using properties of the gun or auxiliary gunmod.
- ```NPC_AVOID``` NPCs will not attempt to use this mode.
- ```SIMULTANEOUS``` All rounds fired concurrently (not sequentially) with recoil added only once (at the end).


## Magazines

- ```MAG_BULKY``` Can be stashed in an appropriate oversize ammo pouch (intended for bulky or awkwardly shaped magazines).
- ```MAG_COMPACT``` Can be stashed in an appropriate ammo pouch (intended for compact magazines).
- ```MAG_DESTROY``` Magazine is destroyed when the last round is consumed (intended for ammo belts). Has precedence over `MAG_EJECT`.
- ```MAG_EJECT``` Magazine is ejected from the gun/tool when the last round is consumed.
- ```SPEEDLOADER``` Acts like a magazine, except it transfers rounds to the target gun instead of being inserted into it.

    
## Mapgen
    
- ```ERASE_ALL_BEFORE_PLACING_TERRAIN``` Clear items, traps, or furniture before placing terrain tile. See also [`remove_all`](https://github.com/CleverRaven/Cataclysm-DDA/blob/master/doc/MAPGEN.md#remove-everything-with-remove_all). Mutually exclusive with `ALLOW_TERRAIN_UNDER_OTHER_DATA `.
- ```ALLOW_TERRAIN_UNDER_OTHER_DATA``` Keep items, traps, or furniture before placing terrain tile. Mutually exclusive with `ERASE_ALL_BEFORE_PLACING_TERRAIN`.
- ```NO_UNDERLYING_ROTATE``` The map won't be rotated even if the underlying tile is.
- ```AVOID_CREATURES``` If a creature is present on terrain, furniture and traps won't be placed.

    
## Map Specials

- ```mx_bandits_block``` ...  Road block made by bandits from tree logs, caltrops, or nailboards.
- ```mx_burned_ground``` ... Fire has ravaged this place.
- ```mx_point_burned_ground``` ... Fire has ravaged this place. (partial application)
- ```mx_casings``` ... Several types of spent casings (solitary, groups, entire overmap tile).
- ```mx_city_trap``` ... A spinning blade trap with a loudspeaker to attract zombies.
- ```mx_clay_deposit``` ... A small surface clay deposit.
- ```mx_clearcut``` ... All trees become stumps.
- ```mx_collegekids``` ... Corpses and items.
- ```mx_corpses``` ... Up to 5 corpses with everyday loot.
- ```mx_crater``` ... Crater with rubble (and radioactivity).
- ```mx_drugdeal``` ... Corpses and some drugs.
- ```mx_dead_vegetation``` ... Kills all plants. (aftermath of acid rain etc.)
- ```mx_exocrash_1``` ... Area of glassed sand created by a crashed pod of space travelers. Populated by zomborgs.
- ```mx_exocrash_2``` ... Area of glassed sand created by a crashed pod of space travelers. Populated by zomborgs.
- ```mx_point_dead_vegetation``` ... Kills all plants. (aftermath of acid rain etc.) (partial application)
- ```mx_fallen_shed``` ... A collapsed shed.
- ```mx_grove``` ... All trees and shrubs become a single species of tree.
- ```mx_grass``` ... A meadow with tall grass.
- ```mx_grass_2``` ... A meadow with tall grass.
- ```mx_grave``` ... A grave in the open field, with a corpse and some everyday loot.
- ```mx_helicopter``` ... Metal wreckage and some items.
- ```mx_house_spider``` ... A house with wasps, dermatiks, and walls converted to paper.
- ```mx_house_wasp``` ... A house with spiders, webs, eggs and some rare loot.
- ```mx_jabberwock``` ... A *chance* of a jabberwock.
- ```mx_looters``` ... Up to 5 bandits spawn in the building.
- ```mx_marloss_pilgrimage``` A sect of people worshiping fungaloids.
- ```mx_mass_grave``` ... Mass grave with zombies and everyday loot.
- ```mx_mayhem``` ... Several types of road mayhem (firefights, crashed cars etc).
- ```mx_military``` ... Corpses and some military items.
- ```mx_minefield``` ... A military roadblock at the entry of the bridges with landmines scattered in the front of it.
- ```mx_nest_dermatik``` ... Dermatik nest.
- ```mx_nest_wasp``` ... Wasp nest.
- ```mx_null``` ... No special at all.
- ```mx_pond``` ... A small pond.
- ```mx_pond_forest``` ... A small basin.
- ```mx_pond_forest_2``` ... A small basin.
- ```mx_pond_swamp``` ... A small bog.
- ```mx_pond_swamp_2``` ... A small bog.
- ```mx_portal_in``` ... Another portal to neither space.
- ```mx_portal``` ... Portal to neither space, with several types of surrounding environment.
- ```mx_prison_bus``` ... Prison bus with zombie cops and zombie prisoners.
- ```mx_prison_van``` ... Traces of a violent escape near a prison van.
- ```mx_reed``` ... Extra water vegetation.
- ```mx_roadblock``` ... Roadblock furniture with turrets and some cars.
- ```mx_roadworks``` ... Partially closed damaged road with chance of work equipment and utility vehicles.
- ```mx_science``` ... Corpses and some scientist items.
- ```mx_shia``` ... A *chance* of Shia, if Crazy Cataclysm is enabled.
- ```mx_shrubbery``` ... All trees and shrubs become a single species of shrub.
- ```mx_spider``` ... A big spider web, complete with spiders and eggs.
- ```mx_supplydrop``` ... Crates with some military items in it.
- ```mx_Trapdoor_spider_den``` ... Chunk of a forest with a spider spawning out of nowhere.
- ```mx_trees``` ... A small chunk of forest with puddles with fresh water.
- ```mx_trees_2``` ... A small chunk of forest with puddles with fresh water.



## Material Phases

- ```GAS```
- ```LIQUID```
- ```NULL```
- ```PLASMA```
- ```SOLID```


## Melee

### Flags

- ```ALWAYS_TWOHAND``` Item is always wielded with two hands. Without this, the items volume and weight are used to calculate this.
- ```BIONIC_WEAPON``` Cannot wield this item normally. It has to be attached to a bionic and equipped through activation of the bionic.
- ```DIAMOND``` Diamond coating adds 30% bonus to cutting and piercing damage.
- ```MESSY``` Creates more mess when pulping.
- ```NO_CVD``` Item can never be used with a CVD machine.
- ```NO_RELOAD``` Item can never be reloaded (even if has a valid ammo type).
- ```NO_UNWIELD``` Cannot unwield this item. Fake weapons and tools wielded from bionics will automatically have this flag added.
- ```NONCONDUCTIVE``` Item doesn't conduct electricity thanks to some feature (nonconductive material of handle or entire item) and thus can be safely used against electricity-themed monsters without the risk of zapback. Opposite of `CONDUCTIVE`.
- ```POLEARM``` Item is clumsy up close and does 70% of normal damage against adjacent targets.  Should be paired with `REACH_ATTACK`.  Simple reach piercing weapons like spears should not get this flag.
- ```REACH_ATTACK``` Allows performing a melee attack on 2-tile distance.
- ```REACH3``` Allows performing a melee attack on 3-tile distance.
- ```SHEATH_AXE``` Item can be sheathed in an axe sheath.
- ```SHEATH_GOLF``` Item can be sheathed in a golf bag.
- ```SHEATH_KNIFE``` Item can be sheathed in a knife sheath, it's applicable to small/medium knives (with volume not bigger than 2).
- ```SHEATH_SWORD``` Item can be sheathed in a sword scabbard.
- ```SPEAR``` When making reach attacks intervening `THIN_OBSTACLE` terrain is not an obstacle.  Should be paired with `REACH_ATTACK`.
- ```STAB``` Changes item's damage type from cutting to stabbing.
- ```UNARMED_WEAPON``` Fighting while wielding this item still counts as unarmed combat.
- ```WHIP``` Has a chance of disarming the opponent.


## Monster Groups

The condition flags limit when monsters can spawn.

### Seasons

Multiple season conditions will be combined together so that any of those conditions become valid time of year spawn times.

- ```AUTUMN```
- ```SPRING```
- ```SUMMER```
- ```WINTER```

### Time of day

Multiple time of day conditions will be combined together so that any of those conditions become valid time of day spawn times.

- ```DAWN```
- ```DAY```
- ```DUSK```
- ```NIGHT```


## Monsters

Flags used to describe monsters and define their properties and abilities.

### Anger, Fear and Placation Triggers

- ```FIRE``` Triggers if there's a fire within 3 tiles, the strength of the effect equals 5 * the field intensity of the fire.
- ```FRIEND_ATTACKED``` Triggers if the monster sees another monster of a friendly faction being attacked; strength = 15.  **Requires** an instance of the trigger on the attacked monster as well(the trigger type need not match, just the trigger itself)!  Always triggers character aggro see [MONSTERS.md](MONSTERS.md#aggrocharacter).
- ```FRIEND_DIED``` Triggers if the monster sees another monster of a friendly faction dying; strength = 15.  **Requires** an instance of the trigger on the attacked monster as well(the trigger type need not match, just the trigger itself)!  Always triggers character aggro.
- ```HURT``` Triggers when the monster is hurt, strength equals 1 + (damage / 3 ).  Always triggers character aggro.
- ```NULL``` Source use only?
- ```PLAYER_CLOSE``` Triggers when a potential enemy is within 5 tiles range - Anger/fear trigger only!  Triggers character aggro `<anger>%` of the time.
- ```PLAYER_WEAK``` Strength = `10 - (percent of hp remaining / 10)` if a non-friendly critter has less than 70% hp remaining .  Triggers character aggro `<anger>%` of the time.
- ```PLAYER_NEAR_BABY``` Increases monster aggression by 8 and morale by 4 if **the player** comes within 3 tiles of its offspring (defined by the baby_monster field in its reproduction data)- Anger trigger only!  Always triggers character aggro.
- ```SOUND``` Not an actual trigger, monsters above 10 aggression and 0 morale will wander towards, monsters below 0 morale will wander away from the source of the sound for 1 turn (6, if they have the `GOODHEARING` flag).
- ```STALK``` Raises monster aggression by 1, triggers 20% of the time each turn if aggression > 5 - Anger trigger only!
- ```HOSTILE_SEEN``` Increases aggression/ decreases morale by a random amount between 0-2 for every potential enemy it can see, up to 20 aggression - Anger/fear trigger only!  Triggers character aggro `<anger/2>%` of the time.
- ```MATING_SEASON``` Increases aggression by 3 if a potential enemy is within 5 tiles range and the season is the same as the monster's mating season (defined by the baby_flags field in its reproduction data) - Anger trigger only!  Triggers character aggro `<anger>%` of the time.

### Categories

- ```CLASSIC``` Only monsters we expect in a classic zombie movie.
- ```NULL``` No category.
- ```WILDLIFE``` Natural animals.

### Death Functions

Multiple death functions can be used. Not all combinations make sense.

- ```ACID``` Acid instead of a body. Not the same as the `ACID_BLOOD flag`. In most cases you want both.
- ```AMIGARA``` Removes hypnosis if the last one.
- ```BLOBSPLIT``` Creates more blobs.
- ```BOOMER``` Explodes in vomit.
- ```BROKEN``` Spawns a broken robot item; its id is calculated like this: the prefix "mon_" is removed from the monster id, then the prefix "broken_" is added. Example: mon_eyebot -> broken_eyebot.
- ```DISAPPEAR``` Hallucination disappears.
- ```DISINTEGRATE``` Falls apart.
- ```EXPLODE``` Damaging explosion.
- ```FIREBALL``` 10 percent chance to explode in a fireball.
- ```FLAME_EXPLOSION``` guaranteed to explode and starts fires.
- ```FUNGUS``` Explodes in spores.
- ```GAMEOVER``` Game over man! Game over! Defense mode.
- ```GUILT``` Morale penalty. There is also a flag with a similar effect.
- ```KILL_BREATHERS``` All breathers die.
- ```KILL_VINES``` Kill all nearby vines.
- ```MELT``` Normal death, but melts.
- ```NORMAL``` Drop a body, leave gibs.
- ```RATKING``` Cure verminitis.
- ```SMOKEBURST``` Explode like a huge smoke bomb.
- ```TEARBURST``` Explode like a huge tear gas bomb.
- ```THING``` Turn into a full thing.
- ```TRIFFID_HEART``` Destroys all roots.
- ```VINE_CUT``` Kill adjacent vine if it's cut.
- ```WORM``` Spawns 2 half-worms.

### Flags

Other monster flags.

- ```ACIDPROOF``` Immune to acid.
- ```ACIDTRAIL``` Leaves a trail of acid.
- ```ACID_BLOOD``` Makes monster bleed acid. Does not automatically dissolve in a pool of acid on death.
- ```ALL_SEEING``` Can see every creature within its vision (highest of day/night vision counts) on the same Z-level.
- ```ALWAYS_VISIBLE``` This monster can always be seen regardless of line of sight or light level.
- ```ANIMAL``` Is an _animal_ for purposes of the `Animal Empathy` trait.
- ```AQUATIC``` Confined to water.
- ```ARTHROPOD_BLOOD``` Forces monster to bleed hemolymph.
- ```ATTACKMON``` Attacks other monsters regardless of faction relations when pathing through their space.
- ```ATTACK_UPPER``` Even though this monster is small in size it can attack upper limbs.
- ```ATTACK_LOWER``` Even though this monster is large in size it can't attack upper limbs.
- ```BADVENOM``` Attack may **severely** poison the player.
- ```BASHES``` Bashes down doors.
- ```BILE_BLOOD``` Makes monster bleed bile.
- ```BORES``` Tunnels through just about anything (15x bash multiplier: dark wyrms' bash skill 12->180).
- ```CAN_DIG``` Will dig on any diggable terrain the same way ```DIGS``` does, however, will walk normally over non-diggable terrain.
- ```CAN_OPEN_DOORS``` Can open doors on its path.
- ```CAMOUFLAGE``` Stays invisible up to (current Perception, + base Perception if the character has the Spotting proficiency) tiles away, even in broad daylight. Monsters see it from the lower of `vision_day` and `vision_night` ranges.
- ```CANPLAY``` This creature can be played with if it's a pet.
- ```CLIMBS``` Can climb over fences or similar obstacles quickly.
- ```COLDPROOF``` Immune to cold damage.
- ```DESTROYS``` Bashes down walls and more. (2.5x bash multiplier, where base is the critter's max melee bashing)
- ```DIGS``` Digs through the ground. Will not travel through non-diggable terrain such as roads.
- ```DOGFOOD``` Can be ordered to attack with a dog whistle.
- ```DRIPS_GASOLINE``` Occasionally drips gasoline on move.
- ```DRIPS_NAPALM``` Occasionally drips napalm on move.
- ```DROPS_AMMO``` This monster drops ammo. Should not be set for monsters that use pseudo ammo.
- ```ELECTRIC``` Shocks unarmed attackers.
- ```ELECTRIC_FIELD``` This monster is surrounded by an electrical field that ignites flammable liquids near it. It also deals damage to other monsters with this flag, with "The %s's disabled electrical field reverses polarity!" message.
- ```ELECTRONIC``` e.g. a robot; affected by emp blasts and other stuff.
- ```FILTHY``` Any clothing it drops will be filthy.  The squeamish trait prevents wearing clothing with this flag, one can't craft anything from filthy components, and wearing filthy clothes may result in infection if hit in melee.
- ```FIREPROOF``` Immune to fire.
- ```FIREY``` Burns stuff and is immune to fire.
- ```FLAMMABLE``` Monster catches fire, burns, and spreads fire to nearby objects.
- ```FLIES``` Can fly (over water, etc.)
- ```GOODHEARING``` Pursues sounds more than most monsters.
- ```GRABS``` Its attacks may grab you!
- ```GROUP_BASH``` Gets help from monsters around it when bashing, adding their strength together.
- ```GROUP_MORALE``` More courageous when near friends.
- ```HARDTOSHOOT``` It's one size smaller for ranged attacks, no less than the `TINY` flag.
- ```HEARS``` It can hear you.
- ```HIT_AND_RUN``` Flee for several turns after a melee attack.
- ```INSECTICIDEPROOF``` It's immune to insecticide even though it's made of bug flesh ("iflesh").
- ```HUMAN``` It's a live human, as long as it's alive.
- ```CONSOLE_DESPAWN``` Despawns when a nearby console is properly hacked.
- ```IMMOBILE``` Doesn't move (e.g. turrets).
- ```ID_CARD_DESPAWN``` Despawns when a science ID card is used on a nearby console.
- ```INTERIOR_AMMO``` Monster contains ammo inside itself, no need to load on launch. Prevents ammo from being dropped on disable.
- ```KEENNOSE``` Keen sense of smell.
- ```LARVA``` Creature is a larva. Currently used for gib and blood handling.
- ```LOUDMOVES``` Makes move noises as if ~2 sizes louder, even if flying.
- ```MECH_RECON_VISION``` This mech grants you night-vision and enhanced overmap sight radius when piloted.
- ```MECH_DEFENSIVE``` This mech can protect you thoroughly when piloted.
- ```MILITARY_MECH``` Is a military-grade mech.
- ```MILKABLE``` Produces milk when milked.
- ```NEMESIS``` Tags Nemesis enemies for the HAS_NEMESIS mutation.
- ```NIGHT_INVISIBILITY``` Monster becomes invisible if it's more than one tile away and the lighting on its tile is LL_LOW or less. Visibility is not affected by night vision.
- ```NOT_HALLUCINATION``` This monster does not appear while the player is hallucinating.
- ```NOGIB``` Does not leave gibs / meat chunks when killed with huge damage.
- ```NOHEAD``` Headshots not allowed!
- ```NO_BREATHE``` Creature can't drown and is unharmed by gas, smoke or poison.
- ```NO_BREED``` Creature doesn't reproduce even though it has reproduction data - useful when using copy-from to make child versions of adult creatures.
- ```NO_FUNG_DMG``` This monster can't be damaged by fungal spores and can't be fungalized either.
- ```NO_NECRO``` This monster can't be revived by necros. It will still rise on its own.
- ```PATH_AVOID_DANGER_1``` This monster will path around some dangers instead of through them.
- ```PATH_AVOID_DANGER_2``` This monster will path around most dangers instead of through them.
- ```PATH_AVOID_FIRE``` This monster will path around heat-related dangers instead of through them.
- ```PATH_AVOID_FALL``` This monster will path around cliffs instead of off of them.
- ```PAY_BOT``` Creature can be turned into a pet for a limited time in exchange of e-money.
- ```PET_MOUNTABLE``` Creature can be ridden or attached to a harness.
- ```PET_HARNESSABLE``` Creature can be attached to a harness.
- ```PET_WONT_FOLLOW``` This monster won't follow the player automatically when tamed.
- ```PRIORITIZE_TARGETS``` This monster will prioritize targets depending on their danger levels.
- ```NULL``` Source use only.
- ```PACIFIST``` Monster will never do melee attacks. Useful for having them use grab without attacking the player.
- ```KEEP_DISTANCE``` Monster will try to keep `tracking_distance` number of tiles between it and its current target.
- ```PARALYZE``` Attack may paralyze the player with venom.
- ```PLASTIC``` Absorbs physical damage to a great degree.
- ```POISON``` Poisonous to eat.
- ```PUSH_MON``` Can push creatures out of its way.
- ```PUSH_VEH``` Can push vehicles out of its way.
- ```QUEEN``` When it dies, local populations start to die off too.
- ```RANGED_ATTACKER``` Monster has any sort of ranged attack.
- ```REVIVES``` Monster corpse will revive after a short period of time.
- ```REVIVES_HEALTHY``` When revived, this monster has full hitpoints and speed.
- ```RIDEABLE_MECH``` This monster is a mech suit that can be piloted.
- ```SEES``` It can see you (and will run/follow).
- ```SHEARABLE``` This monster can be sheared for wool.
- ```SHORTACIDTRAIL``` Leaves an intermittent trail of acid. See also `ACIDTRAIL`.
- ```SLUDGEPROOF``` Ignores the effect of sludge trails.
- ```SLUDGETRAIL``` Causes the monster to leave a sludge trap trail when moving.
- ```SMALLSLUDGETRAIL``` Causes the monster to occasionally leave a 1-tile sludge trail when moving.
- ```SMELLS``` It can smell you.
- ```STUMBLES``` Stumbles in its movement.
- ```STUN_IMMUNE``` This monster is immune to stun.
- ```SUNDEATH``` Dies in full sunlight.
- ```SWARMS``` Groups together and forms loose packs.
- ```SWIMS``` Treats water as 50 movement point terrain.
- ```VENOM``` Attack may poison the player.
- ```VERMIN``` Obsolete flag for inconsequential monsters, now prevents loading.
- ```WARM``` Warm blooded.
- ```WATER_CAMOUFLAGE``` If in water, stays invisible up to (current Perception, + base Perception if the character has the Spotting proficiency) tiles away, even in broad daylight. Monsters see it from the lower of `vision_day` and `vision_night` ranges. Can also make it harder to see in deep water or across Z-levels if it is underwater and the viewer is not.
- ```WEBWALK``` Doesn't destroy webs and won't get caught in them.

### Sizes

Monster physical sizes.

- ```HUGE``` Tank
- ```LARGE``` Cow
- ```MEDIUM``` Human
- ```SMALL``` Dog
- ```TINY``` Squirrel

### Special attacks
Special attacks have been moved to [MONSTER_SPECIAL_ATTACKS.md](MONSTER_SPECIAL_ATTACKS.md) as they have all been migrated away from flags.

## Mutations

#### Flags

- ```UNARMED_BONUS``` You get a bonus to unarmed bash and cut damage equal to unarmed_skill/2 up to 4.

### Categories

These branches are also the valid entries for the categories of `dreams` in `dreams.json`

- ```MUTCAT_ALPHA``` "You feel...better. Somehow."
- ```MUTCAT_BEAST``` "Your heart races and you see blood for a moment."
- ```MUTCAT_BIRD``` "Your body lightens and you long for the sky."
- ```MUTCAT_CATTLE``` "Your mind and body slow down. You feel peaceful."
- ```MUTCAT_CEPHALOPOD``` "Your mind is overcome by images of eldritch horrors...and then they pass."
- ```MUTCAT_CHIMERA``` "You need to roar, bask, bite, and flap. NOW."
- ```MUTCAT_ELFA``` "Nature is becoming one with you..."
- ```MUTCAT_FISH``` "You are overcome by an overwhelming longing for the ocean."
- ```MUTCAT_INSECT``` "You hear buzzing, and feel your body harden."
- ```MUTCAT_LIZARD``` "For a heartbeat, your body cools down."
- ```MUTCAT_MEDICAL``` "Your can feel the blood rushing through your veins and a strange, medicated feeling washes over your senses."
- ```MUTCAT_PLANT``` "You feel much closer to nature."
- ```MUTCAT_RAPTOR``` "Mmm...sweet bloody flavor...tastes like victory."
- ```MUTCAT_RAT``` "You feel a momentary nausea."
- ```MUTCAT_SLIME``` "Your body loses all rigidity for a moment."
- ```MUTCAT_SPIDER``` "You feel insidious."
- ```MUTCAT_TROGLOBITE``` "You yearn for a cool, dark place to hide."


## Overmap

### Overmap connections

- ```ORTHOGONAL``` The connection generally prefers straight lines, avoids turning wherever possible.

### Overmap specials

#### Flags

- ```BEE``` Location is related to bees. Used to classify location.
- ```BLOB``` Location should "blob" outward from the defined location with a chance to be placed in adjacent locations.
- ```CLASSIC``` Location is allowed when classic zombies are enabled.
- ```FARM```
- ```FUNGAL``` Location is related to fungi. Used to classify location.
- ```LAKE``` Location is placed on a lake and will be ignored for placement if the overmap doesn't contain any lake terrain.
- ```MI-GO``` Location is related to mi-go.
- ```SAFE_AT_WORLDGEN``` Location will not spawn overmap monster groups during worldgen (does not affect monsters spawned by mapgen).
- ```TRIFFID``` Location is related to triffids. Used to classify location.
- ```UNIQUE``` Location is unique and will only occur once per overmap. `occurrences` is overridden to define a percent chance (e.g. `"occurrences" : [75, 100]` is 75%)
- ```GLOBALLY_UNIQUE``` Location will only occur once per world. `occurrences` is overridden to define a percent chance (e.g. `"occurrences" : [75, 100]` is 75%)
- ```URBAN```
- ```WILDERNESS```
- ```MAN_MADE``` - For location, created by human. For Innawood mod purposes

### Overmap terrains

#### Flags

- ```KNOWN_DOWN``` There's a known way down.
- ```KNOWN_UP``` There's a known way up.
- ```LINEAR``` For roads etc, which use ID_straight, ID_curved, ID_tee, ID_four_way.
- ```NO_ROTATE``` The terrain can't be rotated (ID_north, ID_east, ID_south, and ID_west instances will NOT be generated, just ID).
- ```SHOULD_NOT_SPAWN``` The terrain should not be expected to spawn.  This  might be because it exists only for testing purposes, or it is part of a partially completed feature where more work is required before it can start spawning.
- ```RIVER``` It's a river tile.
- ```SIDEWALK``` Has sidewalks on the sides adjacent to roads.
- ```IGNORE_ROTATION_FOR_ADJACENCY``` When mapgen for this OMT performs neighbor checks, the directions will be treated as absolute, rather than rotated to account for the rotation of the mapgen itself.  Probably only useful for hardcoded mapgen.
- ```REQUIRES_PREDECESSOR``` Mapgen for this will not start from scratch; it will update the mapgen from the terrain it replaced.  This allows the corresponding json mapgen to use the `expects_predecessor` feature.
- ```LAKE``` Consider this location to be a valid lake terrain for mapgen purposes.
- ```LAKE_SHORE``` Consider this location to be a valid lake shore terrain for mapgen purposes.
- ```SOURCE_FUEL``` For NPC AI, this location may contain fuel for looting.
- ```SOURCE_FOOD``` For NPC AI, this location may contain food for looting.
- ```SOURCE_FARMING``` For NPC AI, this location may contain useful farming supplies for looting.
- ```SOURCE_FABRICATION``` For NPC AI, this location may contain fabrication tools and components for looting.
- ```SOURCE_GUN``` For NPC AI, this location may contain guns for looting.
- ```SOURCE_AMMO``` For NPC AI, this location may contain ammo for looting.
- ```SOURCE_BOOKS``` For NPC AI, this location may contain books for looting.
- ```SOURCE_WEAPON``` For NPC AI, this location may contain weapons for looting.
- ```SOURCE_FORAGE``` For NPC AI, this location may contain plants to forage.
- ```SOURCE_COOKING``` For NPC AI, this location may contain useful tools and ingredients to aid in cooking.
- ```SOURCE_TAILORING``` For NPC AI, this location may contain useful tools for tailoring.
- ```SOURCE_DRINK``` For NPC AI, this location may contain drink for looting.
- ```SOURCE_VEHICLES``` For NPC AI, this location may contain vehicles/parts/vehicle tools, to loot.
- ```SOURCE_ELECTRONICS``` For NPC AI, this location may contain useful electronics to loot.
- ```SOURCE_CONSTRUCTION``` For NPC AI, this location may contain useful tools/components for construction.
- ```SOURCE_CHEMISTRY``` For NPC AI, this location may contain useful chemistry tools/components.
- ```SOURCE_CLOTHING``` For NPC AI, this location may contain useful clothing to loot.
- ```SOURCE_SAFETY``` For NPC AI, this location may be safe/sheltered and a good place for a base.
- ```SOURCE_ANIMALS``` For NPC AI, this location may contain useful animals for farming/riding.
- ```SOURCE_MEDICINE``` For NPC AI, this location may contain useful medicines for looting.
- ```SOURCE_LUXURY``` For NPC AI, this location may contain valuable/feel-good items to sell/keep.
- ```SOURCE_PEOPLE``` For NPC AI, this location may have other survivors.
- ```RISK_HIGH``` For NPC AI, this location has a high risk associated with it - labs/superstores etc.
- ```RISK_LOW``` For NPC AI, this location is secluded and remote, and appears to be safe.
- ```GENERIC_LOOT``` This is a place that may contain any of the above, but at a lower frequency - usually a house.

## Recipes

### Crafting recipes
These flags apply to crafting recipes, i.e. those that fall within the following categories:

- ```CC_AMMO```
- ```CC_ARMOR```
- ```CC_CHEM```
- ```CC_DRINK```
- ```CC_ELECTRONIC```
- ```CC_FOOD```
- ```CC_MISC```
- ```CC_WEAPON```

#### Flags

- ```ALLOW_ROTTEN``` Explicitly allow rotten components when crafting non-perishables.
- ```BLIND_EASY``` Easy to craft with little to no light.
- ```BLIND_HARD``` Possible to craft with little to no light, but difficult.
- ```SECRET``` Not automatically learned at character creation time based on high skill levels.
- ```UNCRAFT_LIQUIDS_CONTAINED``` Spawn liquid items in its default container.
- ```NEED_FULL_MAGAZINE``` If this recipe requires magazines, it needs one that is full.
- ```FULL_MAGAZINE``` Crafted or deconstructed items from this recipe will have fully-charged magazines.

### Camp building recipes
These flags apply only to camp building recipes (hubs and expansions), i.e. those that have category `CC_BUILDING`.

#### Flags
The purpose of these flags is to allow reuse of blueprints to create the "same" facility oriented differently. Mirroring takes place before
rotation, and it is an error to try to apply mirroring multiple times with the same orientation, as well as to try to apply multiple
rotations. It is permitted to apply different versions of the flags if they apply to different directions (and it is indeed the primary
intended usage).

- ```MAP_MIRROR_HORIZONTAL``` Causes the building recipe to mirror both the location and contents of the blueprint(s) used by the recipe.
- ```MAP_MIRROR_VERTICAL``` Causes the building recipe to mirror both the location and contents of the blueprint(s) used by the recipe.
- ```MAP_ROTATE_[X]``` X has to be one of 90, 180, or 270 and requests the blueprint to be rotated by the given number of degrees before being applied.
- ```MAP_MIRROR_HORIZONTAL_IF_[Y]``` Similar to MAP_MIRROR_HORIZONTAL, but is applied only if the tile the expansion is on is Y. The legal values for Y are "NW", "N", "NE", "E", "SE", "S", SW", and "W".
- ```MAP_MIRROR_VERTICAL_IF_[Y]``` The vertical version of the previous flag.
- ```MAP_ROTATE_[X]_IF_[Y]``` The expansion location dependent version of "MAP_ROTATE_X", with Y having the same legal values as the two sets of flags above.

## Scenarios

### Flags

- ```BORDERED``` Initial start location is bordered by an enormous wall of solid rock.
- ```CHALLENGE``` Game won't choose this scenario in random game types.
- ```CITY_START``` Scenario is available only when city size value in world options is more than 0.
- ```FIRE_START``` Player starts the game with fire nearby.
- ```HELI_CRASH``` Player starts the game with various limbs wounds.
- ```LONE_START``` This scenario won't spawn a fellow NPC on game start.

#### Profession Flags

- ```SCEN_ONLY``` Profession can be chosen only as part of the appropriate scenario.

#### Starting Location Flags

- ```ALLOW_OUTSIDE``` Allows placing player outside of building, useful for outdoor start.
- ```BOARDED``` Start in boarded building (windows and doors are boarded, movable furniture is moved to windows and doors).

## Skills

### Tags

- ```combat_skill``` The skill is considered a combat skill. It's affected by `PACIFIST`, `PRED1`, `PRED2`, `PRED3`, and `PRED4` traits.
- ```contextual_skill``` The skill is abstract, it depends on context (an indirect item to which it's applied). Neither player nor NPCs can possess it.


## Techniques

Techniques may be used by tools, armors, weapons and anything else that can be wielded.

- See contents of `data/json/techniques.json`.
- Techniques are also used with martial arts styles, see `data/json/martialarts.json`.


## Tools

### Flags

Melee flags are fully compatible with tool flags, and vice versa.

- ```ACT_ON_RANGED_HIT```  The item should activate when thrown or fired, then immediately get processed if it spawns on the ground.
- ```ALLOWS_REMOTE_USE``` This item can be activated or reloaded from adjacent tile without picking it up.
- ```BELT_CLIP``` The item can be clipped or hooked on to a belt loop of the appropriate size (belt loops are limited by their max_volume and max_weight properties)
- ```BOMB``` It can be a remote controlled bomb.
- ```CABLE_SPOOL``` This item is a cable spool and must be processed as such. It has an internal "state" variable which may be in the states "attach_first" or "pay_out_cable" -- in the latter case, set its charges to `max_charges - dist(here, point(vars["source_x"], vars["source_y"]))`. If this results in 0 or a negative number, set its state back to "attach_first".
- ```CANNIBALISM``` The item is a food that contains human flesh, and applies all applicable effects when consumed.
- ```CHARGEDIM``` If illuminated, light intensity fades with charge, starting at 20% charge left.
- ```DIG_TOOL``` If wielded, digs thorough terrain like rock and walls, as player walks into them. If item also has ```POWERED``` flag, then it digs faster, but uses up the item's ammo as if activating it.
- ```FIRESTARTER``` Item will start fire with some difficulty.
- ```FIRE``` Item will start a fire immediately.
- ```FISH_GOOD``` When used for fishing, it's a good tool (requires that the matching use_action has been set).
- ```FISH_POOR``` When used for fishing, it's a poor tool (requires that the matching use_action has been set).
- ```HAS_RECIPE``` Used by the E-Ink tablet to indicate it's currently showing a recipe.
- ```IS_UPS``` Item is Unified Power Supply. Used in active item processing.
- ```LIGHT_[X]``` Illuminates the area with light intensity `[X]` where `[X]` is an intensity value. (e.x. `LIGHT_4` or `LIGHT_100`). Note: this flags sets `itype::light_emission` field and then is removed (can't be found using `has_flag`);
- ```MC_MOBILE```, ```MC_RANDOM_STUFF```, ```MC_SCIENCE_STUFF```, ```MC_USED```, ```MC_HAS_DATA``` Memory card related flags, see `iuse.cpp`
- ```NO_DROP``` Item should never exist on map tile as a discrete item (must be contained by another item).
- ```NO_UNLOAD``` Cannot be unloaded.
- ```POWERED``` If turned ON, item uses its own source of power, instead of relying on power of the user.
- ```RADIOCARITEM``` Item can be put into a remote controlled car.
- ```RADIOSIGNAL_1``` Activated per radio signal 1.
- ```RADIOSIGNAL_2``` Activated per radio signal 2.
- ```RADIOSIGNAL_3``` Activated per radio signal 3.
- ```RADIO_ACTIVATION``` Activated by a remote control (also requires `RADIOSIGNAL_*`).
- ```RADIO_CONTAINER``` It's a container of something that is radio controlled.
- ```RADIO_MODABLE``` Indicates the item can be made into a radio-activated item.
- ```RADIO_MOD``` The item has been made into a radio-activated item.
- ```RECHARGE``` Gain charges when placed in a cargo area with a recharge station.
- ```SAFECRACK``` This item can be used to unlock safes.
- ```USES_BIONIC_POWER``` The item has no charges of its own, and runs off of the player's bionic power.
- ```USE_PLAYER_ENERGY``` Item with `use_action` that `cast_spell` consumes the specified `base_energy_cost`.
- ```USE_UPS``` Item charges from an UPS / it uses the charges of an UPS instead of its own.
- ```WATER_EXTINGUISH``` Is extinguishable in water or under precipitation. Converts items (requires "reverts_to" or use_action "transform" to be set).
- ```WET``` Item is wet and will slowly dry off (e.g. towel).
- ```WIND_EXTINGUISH``` This item will be extinguished by the wind.
- ```WRITE_MESSAGE``` This item could be used to write messages on signs.

### Flags that apply to items

These flags **do not apply to item types**.

Those flags are added by the game code to specific items (for example, that specific thingamabob, not *all* thingamabob).  These flags are **not** assigned in JSON by content contributors, they are set programmatically.

- ```COLD``` Item is cold (see `EATEN_COLD`).
- ```DIRTY``` Item (liquid) was dropped on the ground and is now irreparably dirty.
- ```FIELD_DRESS_FAILED``` Corpse was damaged by unskillful field dressing. Affects butcher results.
- ```FIELD_DRESS``` Corpse was field dressed. Affects butcher results.
- ```FIT``` Reduces encumbrance by one.
- ```FROZEN``` Item is frozen solid (used by freezer).
- ```HIDDEN_ITEM``` This item cannot be seen in AIM.
- ```HOT``` Item is hot (see `EATEN_HOT`).
- ```LITCIG``` Marks a lit smoking item (cigarette, joint etc.).
- ```MUSHY``` `FREEZERBURN` item was frozen and is now mushy and tasteless and will go bad after freezing again.
- ```NO_PARASITES``` Invalidates parasites count set in food->type->comestible->parasites
- ```QUARTERED``` Corpse was quartered into parts. Affects butcher results, weight, volume.
- ```REVIVE_SPECIAL``` Corpses revives when the player is nearby.
- ```USE_UPS``` The tool has the UPS mod and is charged from an UPS.
- ```WARM``` A hidden flag used to track an item's journey to/from hot, buffers between HOT and cold.
- ```WET``` Item is wet and will slowly dry off (e.g. towel).
    
### Use actions

These flags apply to the `use_action` field, instead of the `flags` field.

- ```ACIDBOMB_ACT``` Get rid of it, or you'll end up like that guy in Robocop.
- ```ACIDBOMB``` Pull the pin on an acid bomb.
- ```AUTOCLAVE``` Sterilize one CBM by autoclaving it.
- ```BELL``` Ring the bell.
- ```BOLTCUTTERS``` Use your town key to gain access anywhere.
- ```BREAK_STICK``` Breaks long branch into two.
- ```C4``` Arm the C4.
- ```CABLE_ATTACH``` This item is a cable spool. Use it to try to attach to a vehicle.
- ```CAN_GOO``` Release a little blob buddy.
- ```CAPTURE_MONSTER_ACT``` Capture and encapsulate a monster. The associated action is also used for releasing it.
- ```CARVER_OFF``` Turn the carver on.
- ```CARVER_ON``` Turn the carver off.
- ```CHAINSAW_OFF``` Turn the chainsaw on.
- ```CHAINSAW_ON``` Turn the chainsaw off.
- ```COMBATSAW_OFF``` Turn the combat-saw on.
- ```COMBATSAW_ON``` Turn the combat-saw off
- ```CROWBAR``` Pry open doors, windows, man-hole covers and many other things that need prying.
- ```DIG``` Clear rubble.
- ```DIRECTIONAL_ANTENNA``` Find the source of a signal with your radio.
- ```DIVE_TANK``` Use compressed air tank to breathe.
- ```DOG_WHISTLE``` Dogs hate this thing; your dog seems pretty cool with it though.
- ```DOLLCHAT``` That creepy doll just keeps on talking.
- ```ELEC_CHAINSAW_OFF``` Turn the electric chainsaw on.
- ```ELEC_CHAINSAW_ON``` Turn the electric chainsaw off.
- ```EXTINGUISHER``` Put out fires.
- ```FIRECRACKER_ACT``` The saddest Fourth of July.
- ```FIRECRACKER_PACK_ACT``` Keep the change you filthy animal.
- ```FIRECRACKER_PACK``` Light an entire packet of firecrackers.
- ```FIRECRACKER``` Light a singular firecracker.
- ```FLASHBANG``` Pull the pin on a flashbang.
- ```GEIGER``` Detect local radiation levels.
- ```GRANADE_ACT``` Assaults enemies with source code fixes?
- ```GRANADE``` Pull the pin on Granade.
- ```GRENADE``` Pull the pin on a grenade.
- ```HACKSAW``` Cut metal into chunks.
- ```HAMMER``` Pry boards off of windows, doors and fences.
- ```HEATPACK``` Activate the heatpack and get warm.
- ```HEAT_FOOD``` Heat food around fires.
- ```HOTPLATE``` Use the hotplate.
- ```JACKHAMMER``` Bust down walls and other constructions.
- ```JET_INJECTOR``` Inject some jet drugs right into your veins.
- ```LAW``` Unpack the LAW for firing.
- ```LIGHTSTRIP``` Activates the lightstrip.
- ```LUMBER``` Cut logs into planks.
- ```MAKEMOUND``` Make a mound of dirt.
- ```MANHACK``` Activate a manhack.
- ```MATCHBOMB``` Light the matchbomb.
- ```MILITARYMAP``` Learn of local military installations, and show roads.
- ```MININUKE``` Set the timer and run. Or hit with a hammer (not really).
- ```MOLOTOV_LIT``` Throw it, but don't drop it.
- ```MOLOTOV``` Light the Molotov cocktail.
- ```MOP``` Mop up the mess.
- ```MP3_ON``` Turn the mp3 player off.
- ```MP3``` Turn the mp3 player on.
- ```NOISE_EMITTER_OFF``` Turn the noise emitter on.
- ```NOISE_EMITTER_ON``` Turn the noise emitter off.
- ```NONE``` Do nothing.
- ```PACK_CBM``` Put CBM in special autoclave pouch so that they stay sterile once sterilized.
- ```PHEROMONE``` Makes zombies ignore you.
- ```PICK_LOCK``` Pick a lock on a door. Speed and success chance are determined by skill, `LOCKPICK` item quality and `PERFECT_LOCKPICK` item flag
- ```PICKAXE``` Does nothing but berate you for having it (I'm serious).
- ```PLACE_RANDOMLY``` This is very much like the flag in the `manhack` iuse, it prevents the item from querying the player as to where they want the monster unloaded to, and instead chooses randomly.
- ```PORTABLE_GAME``` Play games.
- ```PORTAL``` Create portal traps.
- ```RADIO_OFF``` Turn the radio on.
- ```RADIO_ON``` Turn the radio off.
- ```RAG``` Stop the bleeding.
- ```RESTAURANTMAP``` Learn of local eateries, and show roads.
- ```ROADMAP``` Learn of local common points-of-interest and show roads.
- ```SCISSORS``` Cut up clothing.
- ```SEED``` Asks if you are sure that you want to eat the seed. As it is better to plant seeds.
- ```SEW``` Sew clothing.
- ```SHELTER``` Put up a full-blown shelter.
- ```SHOCKTONFA_OFF``` Turn the shocktonfa on.
- ```SHOCKTONFA_ON``` Turn the shocktonfa off.
- ```SIPHON``` Siphon liquids out of vehicle.
- ```SMOKEBOMB_ACT``` This may be a good way to hide as a smoker.
- ```SMOKEBOMB``` Pull the pin on a smoke bomb.
- ```SOLARPACK_OFF``` Fold solar backpack array.
- ```SOLARPACK``` Unfold solar backpack array.
- ```SOLDER_WELD``` Solder or weld items.
- ```SPRAY_CAN``` Graffiti the town.
- ```SURVIVORMAP``` Learn of local points-of-interest that can help you survive, and show roads.
- ```TAZER``` Shock someone or something.
- ```TELEPORT``` Teleport.
- ```TORCH``` Light a torch.
- ```TOURISTMAP``` Learn of local points-of-interest that a tourist would like to visit, and show roads.
- ```TOWEL``` Dry your character using the item as towel.
- ```TOW_ATTACH``` This is a tow cable, activate it to attach it to a vehicle.
- ```TURRET``` Activate a turret.
- ```WASH_ALL_ITEMS``` Wash items with ```FILTHY``` flag.
- ```WASH_HARD_ITEMS``` Wash hard items with ```FILTHY``` flag.
- ```WASH_SOFT_ITEMS``` Wash soft items with ```FILTHY``` flag.
- ```WATER_PURIFIER``` Purify water.

## Vehicle Parts

### Flags

- ```ADVANCED_PLANTER``` This planter doesn't spill seeds and avoids damaging itself on non-diggable surfaces.
- ```AIRCRAFT_REPAIRABLE_NOPROF``` Allows the player to safely remove part from an aircraft without any proficiency.
- ```AUTOPILOT``` This part will enable a vehicle to have a simple autopilot.
- ```AISLE_LIGHT``` This part lightens up surroundings.
- ```AISLE``` Player can move over this part with less speed penalty than normal.
- ```ALTERNATOR``` Recharges batteries installed on the vehicle. Can only be installed on a part with ```E_ALTERNATOR``` flag.
- ```ANCHOR_POINT``` Allows secure seatbelt attachment.
- ```ANIMAL_CTRL``` Can harness an animal, need HARNESS_bodytype flag to specify bodytype of animal.
- ```ARMOR``` Protects the other vehicle parts it's installed over during collisions.
- ```ATOMIC_LIGHT``` This part lightens up surroundings.
- ```BATTERY_MOUNT``` This part allows mounting batteries for quick change.
- ```HANDHELD_BATTERY_MOUNT``` Same as `BATTERY_MOUNT`, but for handheld battery mount.
- ```BED``` A bed where the player can sleep.
- ```BEEPER``` Generates noise when the vehicle moves backward.
- ```BELTABLE``` Seatbelt can be attached to this part.
- ```BIKE_RACK_VEH``` Can be used to merge an adjacent single tile wide vehicle, or split a single tile wide vehicle off into its own vehicle.
- ```BOARDABLE``` The player can safely move over or stand on this part while the vehicle is moving.
- ```CAMERA_CONTROL```This part allows for using the camera system installed on a vehicle.
- ```CAMERA``` Vehicle part which allows looking through the installed camera system.
- ```CAPTURE_MOSNTER_VEH``` Can be used to capture monsters when mounted on a vehicle.
- ```CARGO_LOCKING``` This cargo area is inaccessible to NPCs. Can only be installed on a part with ```LOCKABLE_CARGO``` flag.
- ```CARGO``` Cargo holding area.
- ```CHIMES``` Generates continuous noise when used.
- ```CIRCLE_LIGHT``` Projects a circular radius of light when turned on.
- ```CONE_LIGHT``` Projects a cone of light when turned on.
- ```CONTROLS``` Can be used to control the vehicle.
- ```CONTROL_ANIMAL``` These controls can only be used to control a vehicle pulled by an animal (e.g., reins and other tack).
- ```COOLER``` There is a separate command to toggle this part.
- ```COVERED``` Prevents items in cargo parts from emitting any light.
- ```CTRL_ELECTRONIC``` Controls electrical and electronic systems of the vehicle.
- ```CURTAIN``` Can be installed over a part flagged with ```WINDOW```, and functions the same as blinds found on windows in buildings.
- ```DOME_LIGHT``` This part lightens up surroundings.
- ```DOOR_MOTOR``` Can only be installed on a part with ```OPENABLE``` flag.
- ```ENGINE``` Is an engine and contributes towards vehicle mechanical power.
- ```EVENTURN``` Only on during even turns.
- ```EXTRA_DRAG``` Tells the vehicle that the part exerts engine power reduction.
- ```E_ALTERNATOR``` Is an engine that can power an alternator.
- ```E_COLD_START``` Is an engine that starts much slower in cold weather.
- ```E_COMBUSTION``` Is an engine that burns its fuel and can backfire or explode when damaged.
- ```E_HEATER``` Is an engine and has a heater to warm internal vehicle items when on.
- ```E_HIGHER_SKILL``` Is an engine that is more difficult to install as more engines are installed.
- ```E_STARTS_INSTANTLY``` Is an engine that starts instantly, like food pedals.
- ```FLAT_SURF``` Part with a flat hard surface (e.g. table).
- ```FREEZER``` Can freeze items in below zero degrees Celsius temperature.
- ```FRIDGE``` Can refrigerate items.
- ```FUNNEL``` If installed over a vehicle tank, can collect rainwater during rains.
- ```HALF_CIRCLE_LIGHT``` Projects a directed half-circular radius of light when turned on.
- ```HARNESS_bodytype``` Replace bodytype with `any` to accept any type, or with the targeted type.
- ```HORN``` Generates noise when used.
- ```INITIAL_PART``` When starting a new vehicle via the construction menu, this vehicle part will be the initial part of the vehicle (if the used item matches the item required for this part). The items of parts with this flag are automatically added as component to the vehicle start construction.
- ```INTERNAL``` Can only be installed on a part with ```CARGO``` flag.
- ```LOCKABLE_CARGO``` Cargo containers that are able to have a lock installed.
- ```MUFFLER``` Muffles the noise a vehicle makes while running.
- ```MULTISQUARE``` Causes this part and any adjacent parts with the same ID to act as a singular part.
- ```MUSCLE_ARMS``` Power of the engine with such flag depends on player's strength (it's less effective than ```MUSCLE_LEGS```).
- ```MUSCLE_LEGS``` Power of the engine with such flag depends on player's strength.
- ```NAILABLE``` Attached with nails.
- ```NEEDS_BATTERY_MOUNT``` Part with this flag needs to be installed over part with `BATTERY_MOUNT` flag.
- ```NEEDS_HANDHELD_BATTERY_MOUNT``` Same as `NEEDS_BATTERY_MOUNT`, but for handheld battery mount.
- ```NOINSTALL``` Cannot be installed.
- ```NO_INSTALL_PLAYER``` Cannot be installed by a player, but can be installed on vehicles.
- ```NO_MODIFY_VEHICLE``` Installing a part with this flag on a vehicle will mean that it can no longer be modified. Parts with this flag should not be installable by players.
- ```NO_UNINSTALL``` Cannot be uninstalled.
- ```NO_REPAIR``` Cannot be repaired.
- ```OBSTACLE``` Cannot walk through part, unless the part is also ```OPENABLE```.
- ```ODDTURN``` Only on during odd turns.
- ```ON_CONTROLS``` Can only be installed on a part with ```CONTROLS``` flag.
- ```ON_ROOF``` Parts with this flag could only be installed on a roof (parts with ```ROOF``` flag).
- ```OPAQUE``` Cannot be seen through.
- ```OPENABLE``` Can be opened or closed.
- ```OPENCLOSE_INSIDE```  Can be opened or closed, but only from inside the vehicle.
- ```OVER``` Can be mounted over other parts.
- ```PERPETUAL``` If paired with REACTOR, part produces electrical power without consuming fuel.
- ```PLANTER``` Plants seeds into tilled dirt, spilling them when the terrain underneath is unsuitable. It is damaged by running it over non-```DIGGABLE``` surfaces.
- ```PLOW``` Tills the soil underneath the part while active. Takes damage from unsuitable terrain at a level proportional to the speed of the vehicle.
- ```POWER_TRANSFER``` Transmits power to and from an attached thingy (probably a vehicle).
- ```PROTRUSION``` Part sticks out so no other parts can be installed over it.
- ```REACTOR``` When enabled, part consumes fuel to generate epower.
- ```REAPER``` Cuts down mature crops, depositing them on the square.
- ```RECHARGE``` Recharge items with the same flag. ( Currently only the rechargeable battery mod. )
- ```REMOTE_CONTROLS``` Once installed, allows using vehicle through remote controls.
- ```REVERSIBLE``` Removal has identical requirements to installation but is twice as quick.
- ```ROOF``` Covers a section of the vehicle. Areas of the vehicle that have a roof and roofs on surrounding sections, are considered inside. Otherwise they're outside.
- ```SCOOP``` Pulls items from underneath the vehicle to the cargo space of the part. Also mops up liquids.
- ```SEATBELT``` Helps prevent the player from being ejected from the vehicle during an accident. Can only be installed on a part with ```BELTABLE``` flag.
- ```SEAT``` A seat where the player can sit or sleep.
- ```SECURITY``` If installed, will emit a loud noise when the vehicle is smashed.
- ```SHARP``` Striking a monster with this part does cutting damage instead of bashing damage, and prevents stunning the monster.
- ```SHOCK_ABSORBER``` This part protects non-frame parts on the same tile from shock damage from collisions.  It doesn't provide protect against direct impacts or other attacks.
- ```SIMPLE_PART``` This part can be installed or removed from that otherwise prevent modification.
- ```SMASH_REMOVE``` When you remove this part, instead of getting the item back, you will get the bash results.
- ```SOLAR_PANEL``` Recharges vehicle batteries when exposed to sunlight. Has a 1 in 4 chance of being broken on car generation.
- ```SPACE_HEATER``` There is separate command to toggle this part.
- ```STABLE``` Similar to `WHEEL`, but if the vehicle is only a 1x1 section, this single wheel counts as enough wheels.
- ```UNSTABLE_WHEEL``` The opposite of `STABLE` - this will not provide for the wheeling needs of your vehicle if installed alone.
- ```STEERABLE``` This wheel is steerable.
- ```STEREO``` Allows playing music for increasing the morale.
- ```TRANSFORM_TERRAIN``` Transform terrain (using rules defined in ```transform_terrain```).
- ```TRACKED``` Contributes to steering effectiveness but doesn't count as a steering axle for install difficulty and still contributes to drag for the center of steering calculation.
- ```TRACK``` Allows the vehicle installed on to be marked and tracked on map.
- ```TURRET_CONTROLS``` If part with this flag is installed over the turret, it allows to set said turret's targeting mode to full auto. Can only be installed on a part with ```TURRET``` flag.
- ```TURRET_MOUNT``` Parts with this flag are suitable for installing turrets.
- ```TURRET``` Is a weapon turret. Can only be installed on a part with ```TURRET_MOUNT``` flag.
- ```UNMOUNT_ON_DAMAGE``` Part breaks off the vehicle when destroyed by damage. Item is new and typically undamaged.
- ```UNMOUNT_ON_MOVE``` Dismount this part when the vehicle moves. Doesn't drop the part, unless you give it special handling.
- ```VARIABLE_SIZE``` Has 'bigness' for power, wheel radius, etc.
- ```VISION``` Gives vision of otherwise unseen directions, e.g. mirrors.
- ```WASHING_MACHINE``` Can be used to wash filthy clothes en masse.
- ```DISHWASHER``` Can be used to wash filthy non-soft items en masse.
- ```WATER_WHEEL``` Recharges vehicle batteries when submerged in moving water.
- ```WHEEL``` Counts as a wheel in wheel calculations.
- ```WIDE_CONE_LIGHT``` Projects a wide cone of light when turned on.
- ```WINDOW``` Can see through this part and can install curtains over it.
- ```WIND_POWERED``` This engine is powered by wind (sails etc).
- ```WIND_TURBINE``` Recharges vehicle batteries when exposed to wind.
- ```WORKBENCH``` Can craft at this part, must be paired with a workbench json entry.
- ```NEEDS_WINDOW``` Can only be installed on a part with ```WINDOW``` flag.
- ```NEEDS_WHEEL_MOUNT_LIGHT``` Can only be installed on a part with ```WHEEL_MOUNT_LIGHT``` flag.
- ```NEEDS_WHEEL_MOUNT_MEDIUM``` Can only be installed on a part with ```WHEEL_MOUNT_MEDIUM``` flag.
- ```NEEDS_WHEEL_MOUNT_HEAVY``` Can only be installed on a part with ```WHEEL_MOUNT_HEAVY``` flag.

### Vehicle parts requiring other vehicle parts

The requirement for other vehicle parts is defined for a json flag by setting ```requires_flag``` for the flag. ```requires_flag``` is the other flag that a part with this flag requires.


### Fuel types

- ```animal``` Beast of burden.
- ```avgas``` I believe I can fly!
- ```battery``` Electrifying.
- ```biodiesel``` Homemade power.
- ```coal_lump``` Good ol' steampunk.
- ```charcoal``` Good ol' steampunk.
- ```diesel``` Refined dino.
- ```gasoline``` Refined dino.
- ```jp8``` Refined dino for military use.
- ```lamp_oil``` Let there be light!
- ```motor_oil``` Synthetic analogue of refined dino.
- ```muscle``` I got the power!
- ```plut_cell``` 1.21 Gigawatts!
- ```wind``` Wind powered.

### Faults

#### Flags

General fault flag:
- ```SILENT``` Makes the "faulty " text NOT appear next to item on general UI. Otherwise the fault works the same.

Vehicle fault flags:
- ```NO_ALTERNATOR_CHARGE``` The alternator connected to this engine does not work.
- ```BAD_COLD_START``` The engine starts as if the temperature was 20 F colder. Does not stack with multiples of itself.
- ```IMMOBILIZER``` Prevents engine from starting and makes it beep.
- ```BAD_FUEL_PUMP``` Prevents engine from starting and makes it stutter.
- ```BAD_STARTER``` Prevents engine from starting and makes click noise.
- ```DOUBLE_FUEL_CONSUMPTION``` Doubles fuel consumption of the engine. Does not stack with multiples of itself.
- ```EXTRA_EXHAUST``` Makes the engine emit more exhaust smoke. Does not stack with multiples of itself.
- ```REDUCE_ENG_POWER``` Multiplies engine power by 0.6. Does not stack with multiples of itself.
- ```ENG_BACKFIRE``` Causes the engine to backfire as if it had zero hp.

Gun fault flags:
- ```BLACKPOWDER_FOULING_DAMAGE``` Causes the gun to take random acid damage over time.
- ```NO_DIRTYING``` Prevents the gun from receiving `fault_gun_dirt` fault.
- ```JAMMED_GUN``` Stops burst fire. Adds delay on next shot.
- ```UNLUBRICATED``` Randomly causes screeching noise when firing and applies damage when that happens.
- ```BAD_CYCLING``` One in 16 chance that the gun fails to cycle when fired resulting in `fault_gun_chamber_spent` fault.

#### Parameters

- ```turns_into``` Causes this fault to apply to the item just mended.
- ```also_mends``` Causes this fault to be mended (in addition to fault selected) once that fault is mended.

