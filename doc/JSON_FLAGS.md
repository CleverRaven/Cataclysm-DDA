# JSON Flags

- [JSON Flags](#json-flags)
  - [Notes](#notes)
  - [Inheritance](#inheritance)
  - [TODO](#todo)
  - [Ammo](#ammo)
  - [Armor](#armor)
    - [Covers](#covers)
    - [Specifically Covers](#specifically-covers)
  - [Bionics](#bionics)
  - [Bodyparts](#bodyparts)
  - [Books](#books)
  - [Character](#character)
    - [Mutation Categories](#mutation-categories)
  - [Comestibles](#comestibles)
    - [Comestible type](#comestible-type)
    - [Addiction type](#addiction-type)
    - [`use_action`](#use_action)
  - [Effects](#effects)
  - [Furniture and Terrain](#furniture-and-terrain)
    - [Fungal Conversions Only](#fungal-conversions-only)
    - [Furniture Only](#furniture-only)
  - [Generic](#generic)
  - [Guns](#guns)
    - [Firing modes](#firing-modes)
    - [Gun Faults](#gun-faults)
  - [Magazines](#magazines)
  - [Magic](#magic)
  - [Mapgen](#mapgen)
  - [Map Specials](#map-specials)
  - [Material Phases](#material-phases)
  - [Melee](#melee)
  - [Monsters](#monsters)
    - [Anger, Fear and Placation Triggers](#anger-fear-and-placation-triggers)
    - [Categories](#categories)
    - [Death Functions](#death-functions)
    - [Monster Groups](#monster-groups)
      - [Seasons](#seasons)
      - [Time of day](#time-of-day)
    - [Sizes](#sizes)
    - [Special attacks](#special-attacks)
  - [Mutations](#mutations)
  - [Overmap](#overmap)
    - [Overmap connections](#overmap-connections)
    - [Overmap specials](#overmap-specials)
    - [Overmap terrains](#overmap-terrains)
  - [Recipes](#recipes)
    - [Crafting recipes](#crafting-recipes)
    - [Camp building recipes](#camp-building-recipes)
      - [Blueprint reorientation flags](#blueprint-reorientation-flags)
  - [Scenarios](#scenarios)
    - [Profession](#profession)
    - [Starting Location](#starting-location)
  - [Skills](#skills)
    - [Tags](#tags)
  - [Technical flags](#technical-flags)
  - [Techniques](#techniques)
  - [Tools](#tools)
    - [`use_action`](#use_action)
  - [Traps](#traps)
  - [Vehicles](#vehicles)
    - [Fuel types](#fuel-types)
    - [Parts](#parts)
    - [Vehicle faults](#vehicle-faults)

## Notes

- Some flags (items, effects, vehicle parts) have to be defined in `flags.json` or `vp_flags.json` (with type: `json_flag`) to work correctly.
- Many of the flags intended for one category or item type can be used in other categories or item types.  Experiment to see where else flags can be used.
- Offensive and defensive flags can be used on any item type that can be wielded.


## Inheritance

When an item is crafted, it can inherit flags from the components that were used to craft it.  This requires that the flag to be inherited has the `"craft_inherit": true` entry.  If you don't want a particular item to inherit flags when crafted, specify the member delete_flags, which is an array of strings.  Flags specified there will be removed from the resultant item upon crafting.  This will override flag inheritance, but will not delete flags that are part of the item type itself.


## TODO

- `Ammo type` table is very old and doesn't include many new ammo types.  Consider updating it or removing altogether, as ammo types ain't no json flags at all.


## Ammo

These are handled through [ammo types](../data/json/items/ammo_types.json).  You can tag a weapon with these to have it chamber existing ammo, or make your own ammo there.  The first column in this list is the tag's "id", the internal identifier DDA uses to track the tag, and the second is a brief description of the ammo tagged.  Use the id to search for ammo listings, as ids are constant throughout DDA's code.  Happy chambering!  :-)

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


## Armor

Some armor flags, such as `WATCH` and `ALARMCLOCK` are compatible with other item types.  Experiment to find which flags work elsewhere.

- ```ABLATIVE_CHAINMAIL_ARMS``` item can be worn with chainmail armor without encumbrance penalty; specifically can be put in pocket for armor with this flag restriction.
- ```ABLATIVE_CHAINMAIL_ELBOWS``` item can be worn with chainmail armor without encumbrance penalty; specifically can be put in pocket for armor with this flag restriction.
- ```ABLATIVE_CHAINMAIL_KNEES``` item can be worn with chainmail armor without encumbrance penalty; specifically can be put in pocket for armor with this flag restriction.
- ```ABLATIVE_CHAINMAIL_LEGS``` item can be worn with chainmail armor without encumbrance penalty; specifically can be put in pocket for armor with this flag restriction.
- ```ABLATIVE_CHAINMAIL_TORSO``` item can be worn with chainmail armor without encumbrance penalty; specifically can be put in pocket for armor with this flag restriction.
- ```ABLATIVE_HELMET``` item can be worn with Hub 01 headgear without encumbrance penalty; specifically can be put in pocket for armor with this flag restriction.
- ```ABLATIVE_LARGE``` This item fits in large ablative pockets.
- ```ABLATIVE_MANTLE``` item can be worn with Hub 01 armor without encumbrance penalty; specifically can be put in pocket for armor with this flag restriction.
- ```ABLATIVE_MEDIUM``` This item fits in medium ablative pockets.
- ```ABLATIVE_SKIRT``` item can be worn with Hub 01 armor without encumbrance penalty; specifically can be put in pocket for armor with this flag restriction.
- ```ACTIVE_CLOAKING``` While active, drains UPS to provide invisibility.
- ```ALARMCLOCK``` Has an alarm-clock feature.
- ```ALLOWS_NATURAL_ATTACKS``` Doesn't prevent any natural attacks or similar benefits from mutations, fingertip razors, etc., like most items covering the relevant body part would.
- ```ALLOWS_TAIL``` You can wear this leg-covering item even if you have a tail.
- ```ALLOWS_TALONS``` People with talon mutations still can wear this armor, that cover arms.
- ```AURA``` This item goes in the outer aura layer, intended for metaphysical effects.
- ```BAROMETER``` This gear is equipped with an accurate barometer (which is used to measure atmospheric pressure).
- ```BELTED``` Layer for backpacks and things worn over outerwear.
- ```BLIND``` Blinds the wearer while worn, and provides nominal protection vs flashbang flashes.
- ```BLOCK_WHILE_WORN``` Allows worn armor or shields to be used for blocking attacks.
- ```BULLET_IMMUNE``` Wearing an item with this flag makes you immune to bullet damage.
- ```CANT_WEAR``` This item can't be worn directly.
- ```COLLAR``` This piece of clothing has a wide collar that can keep your mouth warm when it is mostly unencumbered.
- ```COMBAT_TOGGLEABLE``` This item is meant to be toggled during combat.  Used by NPCs to determine if they will toggle it on during combat.  This only supports simple `transform` actions.
- ```DEAF``` Makes the player deaf.
- ```DECAY_EXPOSED_ATMOSPHERE``` Consumable will go bad once exposed to the atmosphere (such as MREs).
- ```ELECTRIC_IMMUNE``` This gear completely protects you from electric discharges.
- ```EXTRA_PLATING``` Item can be worn over some armors, as additional layer of protection (like armor above brigandine); specifically can be put in pocket for armor with this flag restriction.
- ```FANCY``` Wearing this clothing gives a morale bonus if the player has the `Stylish` trait.
- ```FIN``` This item is swim fins aka diving fins aka flippets, and provide speed boost when you swim.
- ```FIX_FARSIGHT``` This gear corrects farsightedness.
- ```FIX_NEARSIGHT``` This gear corrects nearsightedness.
- ```FLASH_PROTECTION``` This item grants a protection against different light-related dangers, including flashbangs and dazzling light.
- ```FLOTATION``` Prevents the player from drowning in deep water.  Also prevents diving underwater.
- ```FRAGILE``` This gear is less resistant to damage than normal.
- ```GAS_PROOF``` This item will completely protect you from any dangerous gases.
- ```GNV_EFFECT``` Being worn, this item will give a night vision.  Using enchantment, that applies effect, that modifies character's night vision score, may be better alternative, and more flexible.
- ```HELMET_AVENTAIL``` Item can be worn with nasal helmet; specifically can be put in pocket for armor with this flag restriction.
- ```HELMET_BACK_POUCH``` Item can be worn with different hard hats, as attachment; specifically can be put in pocket for armor with this flag restriction.
- ```HELMET_EAR_ATTACHMENT``` Item can be worn with different hard hats, as attachment; specifically can be put in pocket for armor with this flag restriction.
- ```HELMET_FACE_SHIELD``` Item can be worn with different hard hats, as attachment; specifically can be put in pocket for armor with this flag restriction.
- ```HELMET_MANDIBLE_GUARD_STRAPPED``` Item can be worn with different hard helmets, as attachment; specifically can be put in pocket for armor with this flag restriction.
- ```HELMET_MANDIBLE_GUARD``` Item can be worn with different hard helmets, as attachment; specifically can be put in pocket for armor with this flag restriction.
- ```HELMET_NAPE_PROTECTOR``` Item can be worn with different hard helmets, as attachment; specifically can be put in pocket for armor with this flag restriction.
- ```HOOD``` Allow this clothing to conditionally cover the head, for additional warmth or water protection, if the player's head isn't encumbered.
- ```HYGROMETER``` This gear is equipped with an accurate hygrometer (which is used to measure humidity).
- ```INTEGRATED``` This item represents a part of you granted by mutations or bionics.  It will always fit, cannot be unequipped (aside from losing the source), and won't drop on death, but otherwise behaves like normal armor with regards to function, encumbrance, layer conflicts and so on.
- ```IR_EFFECT``` Being worn, this item will give an infrared vision.
- ```MUTE``` Makes the player mute.
- ```NORMAL``` Items worn like normal clothing.  This is assumed as default.
- ```NO_TAKEOFF``` Item with that flag can't be taken off.
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
- ```REQUIRES_BALANCE``` Gear that requires a certain balance to be steady with.  If the player is hit while wearing, they have a chance to be downed.
- ```RESTRICT_HANDS``` Prevents the player from wielding a weapon two-handed, forcing one-handed use if the weapon permits it.
- ```ROLLER_INLINE``` Faster, but less stable overall, the penalty for non-flat terrain is even harsher.
- ```ROLLER_ONE``` A less stable and slower version of `ROLLER_QUAD`, still allows the player to move faster than walking speed.
- ```ROLLER_QUAD```The medium choice between `ROLLER_INLINE` and `ROLLER_ONE`, while it is more stable, and moves faster, it also has a harsher non-flat terrain penalty than `ROLLER_ONE`.
- ```SEMITANGIBLE``` Prevents the item from participating in the encumbrance system when worn.
- ```SKINTIGHT``` Undergarment layer.
- ```SLOWS_MOVEMENT``` This piece of clothing multiplies move cost by 1.1.
- ```STAR_PLATE``` Item can be worn with ryūsei battle kit armor; specifically can be put in pocket for armor with this flag restriction.
- ```STAR_SHOULDER``` Item can be worn with ryūsei battle kit armor ; specifically can be put in pocket for armor with this flag restriction.
- ```STAR_SKIRT``` Item can be worn with ryūsei battle kit armor; specifically can be put in pocket for armor with this flag restriction.
- ```STURDY``` This clothing is a lot more resistant to damage than normal.
- ```SUN_GLASSES``` Prevents glaring when in sunlight.
- ```SUPER_FANCY``` Gives an additional moral bonus over `FANCY` if the player has the `Stylish` trait.
- ```SWIM_GOGGLES``` Allows you to see much further underwater.
- ```THERMOMETER``` This gear is equipped with an accurate thermometer (which is used to measure temperature).
- ```TOUGH_FEET``` This armor provide effect similar to wearing a proper boots (like scale on your legs), so you don't have a debuff from not wearing footwear.
- ```UNDERSIZE``` This clothes can be worn comfortably by mutants with Tiny or Unassuming.  Too small for anyone else.
- ```VARSIZE``` Can be made to fit via tailoring.
- ```WAIST``` Layer for belts other things worn on the waist.
- ```WATCH``` Acts as a watch and allows the player to see actual time.
- ```WATERPROOF``` Prevents the covered body-part(s) from getting wet in any circumstance.
- ```WATER_FRIENDLY``` Prevents the item from making the body part count as unfriendly to water and thus reducing morale from being wet.


### Covers

- ```ARMS``` same as `ARM_L` and `ARM_R`.
- ```ARM_L```
- ```ARM_R```
- ```EYES```
- ```FEET``` same as `FOOT_L` and `FOOT_R`.
- ```FOOT_L```
- ```FOOT_R```
- ```HANDS``` same as `HAND_L` and `HAND_R`.
- ```HAND_L```
- ```HAND_R```
- ```HEAD```
- ```LEGS``` same as `LEG_L` and `LEG_R`.
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


## Bionics

- ```BIONIC_ARMOR_INTERFACE``` This bionic can provide power to powered armor.
- ```BIONIC_FAULTY``` This bionic is a "faulty" bionic.
- ```BIONIC_GUN``` This bionic is a gun bionic and activating it will fire it.  Prevents all other activation effects including power draw by bionic.
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
- ```HEAL_OVERRIDE``` This bodypart will always regenerate its `heal_bonus` HP instead of it modifying the base healing step.  Without `ALWAYS_HEAL` this still only happens when the part would have healed non-zero amount of damage.
- ```IGNORE_TEMP``` This bodypart is ignored for temperature calculations.
- ```LIMB_LOWER``` This bodypart is close to the ground, and as such has a higher chance to be attacked by small monsters - hitsize is tripled for creatures that can't attack upper limbs.
- ```LIMB_UPPER``` This bodypart is high off the ground, and as such can't be attacked by small monsters - unless they have the `FLIES` or have `ATTACK_UPPER` flags`.
- ```MEND_LIMB``` This bodypart can heal from being broken without needing a splint.
- ```NONSTANDARD_BLOCK``` This limb is different enough that martial arts' arm/leg blocks aren't applicable - blocking with this limb is unlocked by reaching the MA's `nonstandard_block` level, unless the limb also has `ALWAYS_BLOCK`.  Either block flag is **required** for non-arm / non-leg limbs to be eligible to block.
- ```WING_ARM``` Counts as a wing for `Character::can_fly` if two instances of the flag are present.


## Books

- ```INSPIRATIONAL``` Reading this book grants bonus morale to characters with the `SPIRITUAL` trait.
- ```BINDER_ADD_RECIPE``` Add recipe to a book binder.


## Character

Character flags can be `trait_id`, `json_flag_id` or `flag_id`.  Some of these are hardcored, others can be edited and created via JSON.  The current trait/mutation list is at [mutations.json](../data/json/mutations/mutations.json).  For further information, see also [MUTATIONS.doc](#MUTATIONS.md#mutations).

- ```ACIDBLOOD``` Drip acid from wounds instead of blood
- ```ACID_IMMUNE``` You are immune to acid damage.
- ```ALARMCLOCK``` You always can set alarms.
- ```ALBINO``` Cause you to have painful sunburns.
- ```BASH_IMMUNE``` You are immune to bashing damage.
- ```BG_OTHER_SURVIVORS_STORY``` Given to NPC when it has other survival story.
- ```BG_SURVIVAL_STORY``` Given to NPC when it has a survival story.
- ```BIO_IMMUNE``` You are immune to biological damage.
- ```BLEED_IMMUNE``` Immune to bleeding.
- ```BLEEDSLOW``` When bleeding, lose blood at 2/3 of the normal rate.
- ```BLEEDSLOW2``` When bleeding, lose blood at 1/3 of the normal rate.
- ```BLIND``` Makes you blind.
- ```BULLET_IMMUNE``` You are immune to bullet damage.
- ```CANNIBAL``` Butcher humans, eat foods with the `CANNIBALISM` and `STRICT_HUMANITARIANISM` flags without a morale penalty.
- ```CBQ_LEARN_BONUS``` You learn CBQ from the bionic bio_cqb faster.
- ```CHANGING```This flag is silently given to player to detect it can mutate.
- ```CLAIRVOYANCE_PLUS``` Gives a clairvoyance effect, used for debug purposes.
- ```CLIMATE_CONTROL``` You are resistant to extreme temperatures.
- ```CLIMB_NO_LADDER``` Capable of climbing up single-level walls without support.
- ```COLDBLOOD2``` For very heat dependent mutations.
- ```COLDBLOOD3``` For cold-blooded mutations.
- ```COLDBLOOD``` For heat dependent mutations.
- ```COLD_IMMUNE``` You are immune to cold damage.
- ```CUT_IMMUNE``` You are immune to cutting damage.
- ```DEAF``` Makes you deaf.
- ```DIMENSIONAL_ANCHOR``` You can't be teleported.  Also protects you from any dangerous effects of portal storms.
- ```DOWNED_RECOVERY``` Always has 50% chance to recover from downing, regardless of limb scores / stats.
- ```ECTOTHERM``` For ectothermic mutations, like `COLDBLOOD4` and `DRAGONBLOOD3` (Black Dragon from Magiclysm).
- ```ETHEREAL```  You will not drop your items if you gain the `incorporeal` effect.
- ```ELECTRIC_IMMUNE``` You are immune to electric damage.
- ```EMP_IMMUNE``` You bionic power cannot be drained and your vulnerable electronics cannot be broken during an EMP blast.
- ```ENHANCED_VISION``` Increases the scouting range, similarly to `ZOOM` item flag.
- ```EYE_MEMBRANE``` Lets you see underwater.
- ```FEATHER_FALL``` You are immune to fall damage.
- ```GILLS``` You can breathe underwater.
- ```GLARE_RESIST``` Protect your eyes from glare like sunglasses.
- ```GLIDE``` You can glide from ledges without the use of wings, as if by magic.
- ```GLIDING``` You are in the process of gliding.
- ```HARDTOHIT``` Whenever something attacks you, RNG gets rolled twice, and you get the better result.
- ```HEATSINK``` You are resistant to extreme heat.
- ```HEAT_IMMUNE``` Immune to very hot temperatures.
- ```HUGE``` Changes your size to `creature_size::huge`.  Checked last of the size category flags, if no size flags are found your size defaults to `creature_size::medium`.
- ```HYPEROPIC``` You are far-sighted: close combat is hampered and reading is impossible without glasses.
- ```IMMUNE_HEARING_DAMAGE``` Immune to hearing damage from loud sounds.
- ```IMMUNE_SPOIL``` You are immune to negative outcomes from spoiled food.
- ```INFECTION_IMMUNE``` This mutation grants immunity to infections, including infection from bites and tetanus.
- ```INFRARED``` You can see infrared, aka heat vision.
- ```INSECTBLOOD``` Your body drip insect blood if wounded.
- ```INVERTEBRATEBLOOD``` Your body drip invertebrate blood if wounded
- ```INVISIBLE``` You can't be seen.
- ```LARGE``` Changes your size to `creature_size::large`.  Checked third of the size category flags.
- ```MEND_ALL``` You need no splint to heal broken bones.
- ```MUSCLE_VEH_BOOST``` Something, such as buzzing insect wings, is speeding you up when you use a muscle-powered vehicle.
- ```MYCUS_IMMUNE``` Critter is immune to fungal hase field (`fd_fungal_haze`)
- ```MYOPIC``` You are nearsighted: vision range is severely reduced without glasses.
- ```MYOPIC_IN_LIGHT``` You are nearsighted in light, but can see normally in low-light conditions.
- ```NIGHT_VISION``` You can see in the dark.
- ```NO_DISEASE``` This mutation grants immunity to diseases.
- ```NO_RADIATION``` This mutation grants immunity to radiations.
- ```NO_SCENT``` You have no scent.
- ```NO_SPELLCASTING``` Mutations with this flag blocks you from casting spells with the spellcasting menu.  No effect on other spells sources such as activated items, mutations and bionics.  Does not block spells with the `PSIONIC` flag.
- ```NO_PSIONICS``` You are unable to use any psionic power (spell with the `PSIONIC` flag).
- ```NO_THIRST``` Your thirst is not modified by food or drinks.
- ```NUMB``` Changes character's moral behaviour in some situations.
- ```NYCTOPHOBIA``` Apply some negative effects when the ambient light is too low.
- ```PAIN_IMMUNE``` Character don't feel pain.
- ```PARAIMMUNE``` You are immune to parasites.
- ```PLANTBLOOD``` Your body drip veggy blood if wounded.
- ```PORTAL_PROOF``` You are immune to personal portal storm effects.
- ```PRED1``` Small morale bonus from foods with the `PREDATOR_FUN` flag.  Lower morale penalty from the guilt mondeath effect.
- ```PRED2``` Learn combat skills with double catchup modifier.  Resist skill rust on combat skills.  Small morale bonus from foods with the `PREDATOR_FUN` flag.  Lower morale penalty from the guilt mondeath effect.
- ```PRED3``` Learn combat skills with double catchup modifier.  Resist skill rust on combat skills.  Medium morale bonus from foods with the `PREDATOR_FUN` flag.  Immune to the guilt mondeath effect.
- ```PRED4``` Learn combat skills with triple catchup modifier.  Learn combat skills without spending focus.  Resist skill rust on combat skills.  Large morale bonus from foods with the `PREDATOR_FUN` flag.  Immune to the `guilt` mondeath effect.
- ```PSYCHOPATH``` Butcher humans without a morale penalty.
- ```ROOTS2``` Gain enhanced effects from the Mycorrhizal Communion mutation.
- ```ROOTS3``` Gain enhanced effects from the Mycorrhizal Communion mutation (slightly faster than `ROOTS2`).
- ```SAPIOVORE``` Butcher humans without a morale penalty.
- ```SEESLEEP``` You can see while sleeping, and aren't bothered by light when trying to fall asleep.
- ```SLUDGE_IMMUNE``` Critter is immune to sludge trail field (`fd_sludge`)
- ```SMALL``` Changes your size to `creature_size::small`.  Checked second of the size category flags.
- ```SPIRITUAL``` Changes character's moral behaviour in some situations.
- ```STAB_IMMUNE``` You are immune to stabbing damage.
- ```STEADY``` Your speed can never go below base speed, bonuses from effects etc can still apply.
- ```STOP_SLEEP_DEPRIVATION``` Stops Sleep Deprivation while awake and boosts it while sleeping.
- ```STRICT_HUMANITARIAN``` You can eat foodstuffs tagged with `STRICT_HUMANITARIANISM` without morale penalties.
- ```SUNBURN``` TBD, probably related to `ALBINO`.
- ```SUPER_CLAIRVOYANCE``` Gives a super clairvoyance effect (works with multiple z-levels), used for debug purposes.
- ```SAFECRACK_NO_TOOL``` Allows to open safes without stethoscope.
- ```TELEPORT_LOCK``` You cannot teleport.  This has none of the protective effects of `DIMENSIONAL_ANCHOR`.
- ```THERMOMETER``` You always know what temperature it is.
- ```TINY``` Changes your size to `creature_size::tiny`.  Checked first of the size category flags.
- ```TREE_COMMUNION_PLUS``` Gain greatly enhanced effects from the Mycorrhizal Communion mutation.
- ```UNARMED_BONUS``` You get a bonus to unarmed bash and cut damage equal to unarmed_skill/2 up to 4.
- ```WALK_UNDERWATER``` your stamina burn is not increased when you swim, emulating you walking on the water bottom.
- ```WALL_CLING``` You can ascend/descend sheer cliffs as long as the tile above borders at least one wall.  Chance to slip and fall each step.
- ```WATCH``` You always know what time it is.
- ```WEBBED_FEET``` You have webbings on your feet, supporting your swimming speed if not wearing footwear.
- ```WEBBED_HANDS``` You have webbings on your hands, supporting your swimming speed.
- ```WEB_RAPPEL``` You can rappel down staircases and sheer drops of any height.
- ```WEB_WALKER``` Removes the movement speed demerit while walking through webs.
- ```WINGS_1``` You can slow your fall, effectively reducing the height of it by 1 level.
- ```WINGS_2``` You can slow your fall, effectively reducing the height of falls by 2 levels, and ignore pit-like traps.
- ```WING_ARMS``` Two instances of this flag enable you to glide and ignore pit traps if not above 50% carryweight or 4 lift strength. 
- ```WINGGLIDE``` You can glide using some part of your body and strenuous physical effort.
- ```mycus``` acts as `THRESH_MYCUS`, makes all monsters with FUNGUS species friendly, fungicidal gas & antifungal pills cause worse effects.  Mutate when eating mycus fruit, or when sleeping.


### Mutation Categories

These branches are the valid `dreams` from [dreams.json](../data/json/dreams.json).

- ```MUTCAT_ALPHA``` "You feel... better.  Somehow."
- ```MUTCAT_BEAST``` "Your heart races and you see blood for a moment."
- ```MUTCAT_BIRD``` "Your body lightens and you long for the sky."
- ```MUTCAT_CATTLE``` "Your mind and body slow down.  You feel peaceful."
- ```MUTCAT_CEPHALOPOD``` "Your mind is overcome by images of eldritch horrors... and then they pass."
- ```MUTCAT_CHIMERA``` "You need to roar, bask, bite, and flap.  NOW."
- ```MUTCAT_ELFA``` "Nature is becoming one with you..."
- ```MUTCAT_FISH``` "You are overcome by an overwhelming longing for the ocean."
- ```MUTCAT_INSECT``` "You hear buzzing, and feel your body harden."
- ```MUTCAT_LIZARD``` "For a heartbeat, your body cools down."
- ```MUTCAT_MEDICAL``` "You can feel the blood rushing through your veins and a strange, medicated feeling washes over your senses."
- ```MUTCAT_PLANT``` "You feel much closer to nature."
- ```MUTCAT_RAPTOR``` "Mmm...sweet bloody flavor... tastes like victory."
- ```MUTCAT_RAT``` "You feel a momentary nausea."
- ```MUTCAT_SLIME``` "Your body loses all rigidity for a moment."
- ```MUTCAT_SPIDER``` "You feel insidious."
- ```MUTCAT_TROGLOBITE``` "You yearn for a cool, dark place to hide."


## Comestibles

- ```ACID``` When consumed using the `BLECH` function, penalties are reduced if character has `ACIDPROOF` or `ACIDBLOOD` traits.
- ```CARNIVORE_OK``` Can be eaten by characters with the Carnivore mutation at 50% kcal reduction.
- ```CANT_HEAL_EVERYONE``` This med can't be used by everyone, it requires a special mutation.  See `can_heal_with` in mutation.
- ```CORROSIVE``` when consumed using the `BLECH` function, causes the same penalties as `ACID` but is not affected by `ACIDPROOF` or `ACIDBLOOD` traits.
- ```EATEN_COLD``` Morale bonus for eating cold.
- ```EATEN_HOT``` Morale bonus for eating hot.
- ```EDIBLE_FROZEN``` Being frozen doesn't prevent eating it.  No morale bonus.
- ```INEDIBLE``` Inedible by default, enabled to eat when in conjunction with (mutation threshold) flags: `BIRD`, `CATTLE`, `FELINE`, `LUPINE`, `MOUSE`, `RABBIT`, `RAT`.
- ```FERTILIZER``` Works as fertilizer for farming, of if this consumed with the `PLANTBLECH` function penalties will be reversed for plants.
- ```FREEZERBURN``` First thaw is `MUSHY`, second is rotten.
- ```FUNGAL_VECTOR``` Will give a fungal infection when consumed.
- ```HIDDEN_HALLU``` Food causes hallucinations, visible only with a certain survival skill level.
- ```HIDDEN_POISON``` Food displays as poisonous with a certain survival skill level.  Note that this doesn't make items poisonous on its own, consider adding `"use_action": [ "POISON" ]` as well, or using `FORAGE_POISON` instead.
- ```MELTS``` Provides half fun unless frozen.  Edible when frozen.
- ```MILLABLE``` Can be placed inside a mill, to turn into flour.
- ```MUTAGEN_CATALYST``` Injecting it will jumpstart mutation.
- ```MUTAGEN_PRIMER``` Injecting it will prime your body for mutation.
- ```MYCUS_OK``` Can be eaten by post-threshold Mycus characters.  Only applies to Mycus fruits by default.
- ```NEGATIVE_MONOTONY_OK``` Allows `negative_monotony` property to lower comestible fun to negative values.
- ```NO_AUTO_CONSUME``` Consumables with this flag would not get consumed in auto-eat / auto-drink zone.
- ```NO_INGEST``` Administered by some means other than oral intake.
- ```NUTRIENT_OVERRIDE``` When you craft an item, game checks if it's a comestible, and if it is, it stores the components the item was created from.  The `NUTRIENT_OVERRIDE` flag will skip this step.
- ```PKILL_1``` Minor painkiller.
- ```PKILL_2``` Moderate painkiller.
- ```PKILL_3``` Heavy painkiller.
- ```PKILL_L``` Slow-release painkiller.
- ```RAD_STERILIZED``` Irradiated food that is safe to eat, but is not edible forever (such as MREs).
- ```RAW``` Reduces kcal by 25%, until cooked (that is, used in a recipe that requires a heat source).  Should be added to *all* uncooked food, unless that food derives more than 50% of its calories from sugars (i.e. many fruits, some veggies) or fats (i.e. butchered fat, coconut).  TODO: Make a unit test for these criteria after fat/protein/carbs are added.
- ```SMOKABLE``` Accepted by smoking rack.
- ```SMOKED``` Not accepted by smoking rack (product of smoking).
- ```USE_EAT_VERB``` "You drink your %s." or "You eat your %s."
- ```USE_ON_NPC``` Can be used on NPCs (not necessarily by them).
- ```ZOOM``` Zoom items can increase your overmap sight range.


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

### `use_action`

These flags apply to the `use_action` field, instead of the `flags` field.

- ```ALCOHOL_STRONG``` Greatly increases drunkenness.  Adds disease `drunk`.
- ```ALCOHOL_WEAK``` Slightly increases drunkenness.  Adds disease `drunk`.
- ```ALCOHOL``` Increases drunkenness.  Adds disease `drunk`.
- ```ANTIBIOTIC``` Helps fight infections.  Removes disease `infected` and adds disease `recover`.
- ```BANDAGE``` Stop bleeding.
- ```BIRDFOOD``` Makes a small bird friendly.
- ```BLECH``` Causes vomiting, adds disease `poison`, adds pain and hurts torso.
- ```BLECH_BECAUSE_UNCLEAN``` Causes warning.
- ```CATFOOD``` Makes a cat friendly.
- ```CATTLEFODDER``` Makes a large herbivore friendly.
- ```CHEW``` Displays message "You chew your %s.", but otherwise does nothing.
- ```CIG``` Alleviates nicotine cravings.  Adds disease `cig`.
- ```COKE``` Decreases hunger.  Adds disease `high`.
- ```CRACK``` Decreases hunger.  Adds disease `high`.
- ```DISINFECTANT``` Prevents infections.
- ```DOGFOOD``` Makes a dog friendly.
- ```FIRSTAID``` Heals.
- ```FLUMED``` Adds disease `took_flumed`.
- ```FLUSLEEP``` Adds disease `took_flumed` and increases sleepiness.
- ```FUNGICIDE``` Kills fungus and spores. Removes diseases `fungus` and `spores`.
- ```HALLU``` Adds disease `hallu`.
- ```HONEYCOMB``` Spawns wax.
- ```INHALER``` Removes disease `asthma`.
- ```IODINE``` Adds disease `iodine`.
- ```MARLOSS``` "As you eat the berry, you have a near-religious experience, feeling at one with your surroundings..."
- ```MYCUS``` if has trait `THRESH_MARLOSS`, neutral effect removes radiation, add 30 painkiller & heals all bodyparts by 4.  if good effect, add 1000 morale, sleep for 5 hours, add `THRESH_MYCUS`, also removes marloss addictions `addiction_marloss_r`,`addiction_marloss_b`, `addiction_marloss_y` .  With mycus threshold, adds 5 painkiller and stimulant.  With trait `M_DEPENDENT`, removes 87 kcal, add 10 thirst, adds 5 sleepiness, and add morale to negate mutation pains. not having previously mentioned traits causes you to vomit, mutate, randomly gain 2 pain, reduce daily health by 8-50, removes 87 kcal, add 10 thirst, and add 5 sleepiness.  Only applies to mycus fruit by default.
- ```METH``` Adds disease `meth`.
- ```NONE``` "You can't do anything of interest with your [x]."
- ```PKILL``` Reduces pain.  Adds disease `pkill[n]` where `[n]` is the level of flag `PKILL_[n]` used on this comestible.
- ```PLANTBLECH``` Activates `BLECH` iuse action if player does not have plant mutations.
- ```POISON``` Adds diseases `poison` and `foodpoison`.
- ```PROZAC``` Adds disease `took_prozac` if not currently present, otherwise acts as a minor stimulant.  Rarely has the `took_prozac_bad` adverse effect.
- ```PURIFIER``` Removes random number of negative mutations.
- ```SEWAGE``` Causes vomiting.
- ```SLEEP``` Greatly increases sleepiness.
- ```THORAZINE``` Removes diseases `hallu`, `visuals`, `high`.  Additionally removes disease `formication` if disease `dermatik` isn't also present.  Has a chance of a negative reaction which increases sleepiness.
- ```VITAMINS``` Increases healthiness (not to be confused with HP).
- ```WEED``` Makes you roll with Cheech & Chong.  Adds disease `weed_high`.
- ```XANAX``` Alleviates anxiety.  Adds disease `took_xanax`.


## Effects

These are checked by hardcode for monsters (introducing new flags will require C++ changes), but for characters are considered "character flags", meaning new ones can be implemented in JSON alone.  See also [Character flags](#character)

- ```DISABLE_FLIGHT``` Monsters affected by an effect with this flag will never count as flying (even if they have the `FLIES` flag).
- ```EFFECT_IMPEDING``` Character affected by an effect with this flag can't move until they break free from the effect.  Breaking free requires a strength check: `x_in_y( STR * limb lifting score * limb grip score, 6 * get_effect_int( eff_id )`.
- ```EFFECT_LIMB_DISABLE_CONDITIONAL_FLAGS``` Effect disables any conditional flags the limb has.
- ```EFFECT_LIMB_SCORE_MOD``` Effect with a limb score component to be used in Character::get_limb_score.  See [EFFECTS_JSON.md](EFFECTS_JSON.md) for the exact function of limb score modifiers and [JSON_INFO.md](JSON_INFO.md#limb-scores) for the effects of the scores.
- ```EFFECT_LIMB_SCORE_MOD_LOCAL``` Same as `EFFECT_LIMB_SCORE_MOD`, but limb score is modified only if effect is applied to body part, that has said score; effect, that apply -50% vision debuff, won't have effect if applied to leg with this flag.
- ```GRAB``` This effect is a grab, creatures will attempt to break it as such (see `character_escape.cpp`).
- ```GRAB_FILTER``` This effect is a grab filter effect, assigning grabs to their grabbing monster.  Handles targeted grab removal on grab break, as well as potentially acting as a filter for monster attack logic.  Bodypart `grabbing_effects` should have it defined.


## Furniture and Terrain

List of known flags, used in both `furniture` and `terrain`.  Some work for both, others are limited to either.

- ```ALARMED``` Sets off an alarm if smashed.
- ```ALLOW_FIELD_EFFECT``` Apply field effects to items inside `SEALED` terrain/furniture.
- ```AUTO_WALL_SYMBOL``` (only for terrain) The symbol of this terrain will be one of the line drawings (corner, T-intersection, straight line etc.) depending on the adjacent terrains.

    Example: `-` and `|` are both terrain with the `CONNECT_WITH_WALL` flag.  `O` does not have the flag, while `X` and `Y` have the `AUTO_WALL_SYMBOL` flag.

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
- ```CAN_SIT``` Furniture the player can sit on.  Player sitting near furniture with the `FLAT_SURF` tag will get mood bonus for eating.
- ```CHIP``` Used in construction menu to determine if wall can have paint chipped off.
- ```CHOCOLATE``` Made of delicious chocolate.  Used by the My Sweet Cataclysm mod.
- ```CLIMBABLE``` You can climb on this obstacle.
- ```CLIMB_SIMPLE``` You never fail climbing on this obstacle.
- ```COLLAPSES``` Has a roof that can collapse.
- ```CONNECT_WITH_WALL``` (only for terrain) This flag has been superseded by the JSON entries `connect_group` and `connects_to`, but is retained for backward compatibility.
- ```CONSOLE``` Used as a computer.
- ```CONTAINER``` Items on this square are hidden until looted by the player.
- ```CURRENT``` This water is flowing.
- ```DEEP_WATER``` This is water that can submerge the player.
- ```DESTROY_ITEM``` Items that land here are destroyed.  See also `NOITEM`.
- ```DIFFICULT_Z``` Most zombies will not be able to follow you up this terrain (i.e a ladder).
- ```DIGGABLE_CAN_DEEPEN``` Diggable location can be dug again to make deeper (e.g. shallow pit to deep pit).
- ```DIGGABLE``` Digging monsters, seeding monster, digging with shovel, etc.
- ```DONT_REMOVE_ROTTEN``` Plants contain a seed item which must not be removed under any circumstances.
- ```DOOR``` Can be opened (used for NPC path-finding).
- ```EASY_DECONSTRUCT``` Player can deconstruct this without tools.
- ```ELEVATOR``` Terrain with this flag will move player, NPCs, monsters, and items up and down when player activates nearby `elevator controls`.
- ```EXAMINE_FROM_ABOVE``` Furniture can be <kbd>e</kbd> examined from a ledge above.  If deployed furniture is taken down it will be placed on the ledge.
- ```FIRE_CONTAINER``` Stops fire from spreading (brazier, wood stove, etc).
- ```FISHABLE``` You can try to catch fish here.
- ```FLAMMABLE_ASH``` Burns to ash rather than rubble.
- ```FLAMMABLE_HARD``` Harder to light on fire, but still possible.
- ```FLAMMABLE``` Can be lit on fire.
- ```FLAT_SURF``` Furniture or terrain with a flat hard surface (e.g. table but not chair; tree stump, etc.).
- ```FLAT``` Player can build and move furniture on.
- ```FORAGE_HALLU``` This item can be found with the `HIDDEN_HALLU` flag when found through foraging.
- ```FORAGE_POISION``` This item can be found with the `HIDDEN_POISON` flag when found through foraging.
- ```FRESH_WATER``` Source of fresh water.  Will spawn fresh water (once) on terrains with `SPAWN_WITH_LIQUID` flag.
- ```GOES_DOWN``` Can use <kbd>></kbd> to go down a level.
- ```GOES_UP``` Can use <kbd><</kbd> to go up a level.
- ```GROWTH_HARVEST``` This plant is ready for harvest.
- ```GROWTH_MATURE``` This plant is in a mature stage of a growth.
- ```GROWTH_SEEDLING``` This plant is in its seedling stage of growth.
- ```HARVESTED``` Marks the harvested version of a terrain type (e.g. harvesting an apple tree turns it into a harvested tree, which later becomes an apple tree again).
- ```HIDE_PLACE``` Creatures on this tile can't be seen by creatures not standing on adjacent tiles.
- ```INDOORS``` Has a roof over it; blocks rain, sunlight, etc.
- ```LADDER``` This piece of furniture that makes climbing easy.
- ```LIQUIDCONT``` Furniture that contains liquid, allows for contents to be accessed in some checks even if `SEALED`.
- ```LIQUID``` Terrain is liquid (e.g. water, lava, etc.), blocking movement without being a wall.
- ```LOCKED``` Is locked, requiring either external control or lockpicking to open.
- ```MINEABLE``` Can be mined with a pickaxe/jackhammer.
- ```MOUNTABLE``` Suitable for guns with the `MOUNTED_GUN` flag.
- ```MURKY``` Liquid taken from tiles with this flag is badly poisoned (almost on par with sewage).
- ```NANOFAB_TABLE``` This is a nanofabricator, and it can generate items out of specific blueprints.  Hardcoded
- ```NOCOLLIDE``` Feature that simply doesn't collide with vehicles at all.
- ```NOITEM``` Items cannot be added here but may overflow to adjacent tiles.  See also `DESTROY_ITEM`.
- ```NO_FLOOR``` Things should fall when placed on this tile.
- ```NO_FLOOR_WATER``` This tile has no floor, but there is water so it doesn't free fall.
- ```NO_PICKUP_ON_EXAMINE``` Examining this tile (<kbd>e</kbd> by default) won't open Pick Up menu even if there are items here.
- ```NO_SCENT``` This tile cannot have scent values, which prevents scent diffusion through this tile.
- ```NO_SELF_CONNECT``` This terrain won't use multitile texture, and will always looks like a separate unit.
- ```NO_SHOOT``` Terrain with this flag cannot be damaged by ranged attacks, and ranged attacks will not pass through it.
- ```NO_SIGHT``` Creature on this tile have their sight reduced to one tile.
- ```NO_SPOIL``` Items placed in this tile do not spoil.
- ```OPENCLOSE_INSIDE``` If it's a door (with an 'open' or 'close' field), it can only be opened or closed if you're inside.
- ```PAINFUL``` May cause a small amount of pain.
- ```PERMEABLE``` Permeable for gases.
- ```PICKABLE``` This terrain/furniture could be picked with lockpicks.
- ```PIT_FILLABLE``` This terrain can be filled with dirt like a shallow pit.
- ```PLACE_ITEM``` Valid terrain for `place_item()` to put items on.
- ```PLANTABLE``` This terrain or furniture can have seeds planted in it.
- ```PLANT``` A 'furniture' that grows and fruits.
- ```PLOWABLE``` Terrain can be plowed.
- ```RAIL``` This is a railroad, railroad vehicles can use it to move.
- ```RAMP_DOWN``` The end of a ramp that leads down, walking into this moves you one z-level down.  Overrides `WALL`, while still displaying the tile as Impassable.
- ```RAMP_END``` Technical flag for proper work of ramps mechanics.
- ```RAMP_UP``` The end of a ramp that leads up, walking into this moves you one z-level up.  Overrides `WALL`, while still displaying the tile as Impassable.
- ```RAMP``` Can be used to move up a z-level.
- ```REDUCE_SCENT``` Reduces scent diffusion (not total amount of scent in area); only works if also bashable.
- ```ROAD``` Flat and hard enough to drive or skate (with rollerblades) on.
- ```ROUGH``` May hurt the player's feet.
- ```RUBBLE``` Furniture behaves like rubble: it can be cleared by the `CLEAR_RUBBLE` item action.  Can be applied to terrain, but it "clears up the nothing".
- ```RUG``` Enables the `Remove Carpet` Construction entry.
- ```SALT_WATER``` Source of salt water (works for terrains with examine action "water_source").
- ```SEALED``` Can't use <kbd>e</kbd> to retrieve items; must smash them open first.
- ```SEEN_FROM_ABOVE``` Visible from a higher level (provided the tile above has no floor).
- ```SHALLOW_WATER``` This is water that is not deep enough to submerge the player.
- ```SHARP``` May do minor damage to players/monsters passing through it.
- ```SHORT``` Feature too short to collide with vehicle protrusions (e.g. mirrors, blades).
- ```SIGN_ALWAYS``` Shows a message to indicate nothing is written here and lets you add a message if examined without a signage/snippet present.
- ```SIGN``` Show written message on examine.
- ```SMALL_HIDE``` Small creatures such as cockroaches and rats can hide under or inside of this furniture.  Should not be applied to anything bigger than a housecat unless it is particularly flexible, IE a snake.
- ```SMALL_PASSAGE``` This terrain or furniture is too small for large or huge creatures to pass through.
- ```SPAWN_WITH_LIQUID``` This terrain will place liquid (once) on its own spawn.  Type of liquid is defined by other flags.  For example, it spawns fresh water via `FRESH_WATER` flag.
- ```SUN_ROOF_ABOVE``` This furniture (terrain is not supported currently) has a "fake roof" above, that blocks sunlight.  Special hack for #44421, to be removed later.
- ```SUPPORTS_ROOF``` Used as a boundary for roof construction.
- ```SUPPRESS_SMOKE``` Prevents smoke from fires; used by ventilated wood stoves, etc.
- ```SWIMMABLE``` Player and monsters can swim through it.
- ```THIN_OBSTACLE``` ```SPEAR``` attacks can go through this to hit something on the other side.
- ```TINY``` Feature too short to collide with vehicle undercarriage.  Vehicles drive over them with no damage, unless a wheel hits them.
- ```TOILET_WATER``` Liquid taken from tiles with this flag is rather dirty and may poison you.
- ```TRANSLOCATOR``` Tile is a translocator gate, for purposes of the `translocator` examine action.
- ```TRANSPARENT_FLOOR``` This terrain allows light to the z-level below.
- ```TRANSPARENT``` Players and monsters can see through/past it.  Also sets ter_t.transparent.
- ```UNSTABLE``` Walking here cause the bouldering effect on the character.
- ```USABLE_FIRE``` This terrain or furniture counts as a nearby fire for crafting.
- ```WALL``` This terrain is an upright obstacle.  Used for fungal conversion, and also implies `CONNECT_WITH_WALL`.
- ```WATER_CUBE``` This tile is water, used to check can you go up or down using additional flags.
- ```WINDOW``` This terrain is a window, though it may be closed, broken, or covered up.  Used by the tiles code to align furniture sprites away from the window.
- ```WIRED_WALL``` This terrain is a wall with electric wires inside.  Allows the `Reveal wall wirings` construction.
- ```WORKOUT_ARMS``` This furniture is for training your arms.  Needed for checks like `is_limb_broken()`.
- ```WORKOUT_LEGS``` This furniture is for training your legs.  Needed for checks like `is_limb_broken()`.
- ```Z_TRANSPARENT``` Allows the lower floor to be rendered.


### Fungal Conversions Only

- ```FLOWER``` This furniture is a flower.
- ```FUNGUS``` Fungal covered.
- ```ORGANIC``` This furniture is partly organic.
- ```SHRUB``` This terrain is a shrub.
- ```TREE``` This terrain is a tree.
- ```YOUNG``` This terrain is a young tree.


### Furniture Only

- ```ACTIVE_GENERATOR``` This furniture is considered to be an active power source for the purpose of certain monster special attacks (e.g. milspec searchlight's `SEARCHLIGHT`).
- ```ALIGN_WORKBENCH``` (only for furniture) A hint to the tiles display that the sprite for this furniture should face toward any adjacent tile with a workbench quality.
- ```ALLOW_ON_OPEN_AIR``` Don't warn when this furniture is placed on `t_open_air` or similar 'open air' terrains which lack a floor.
- ```AMMOTYPE_RELOAD``` Furniture reloads by ammotype so player can choose from more than one fuel type.
- ```AUTODOC``` This furniture can be an Autodoc console, it also needs the `autodoc` examine action.
- ```AUTODOC_COUCH``` This furniture can be a couch for a furniture with the `autodoc` examine action.
- ```BLOCKSDOOR``` This will boost map terrain's resistance to bashing if `str_*_blocked` is set (see `map_bash_info`).
- ```BRIDGE``` If this furniture is placed over water tiles, it prevents player from becoming wet.
- ```FLOATS_IN_AIR``` If this furniture is placed over open air it won't fall.


## Generic

These flags can be applied via JSON item definition to most items.  Not to be confused with the set of flags listed under Tools > Flags that apply to items, which cannot be assigned via JSON.

- ```ACT_IN_FIRE``` This item would be activated if dropped on a tile with fire.
- ```ALLERGEN_MILK``` This item contain milk, which make it inedible for person with lactose intolerance.
- ```ANIMAL_PRODUCT``` This item can't be worn or eaten by vegan, despite it's materials is not blacklisted or it has no another flags, that restrict it.
- ```BAD_TASTE``` This comestible gives -5 to taste, that can't be covered through cooking.
- ```BANK_NOTE_SHAPED``` This item fits into the folded sleeve of wallets, like a bank note.
- ```BANK_NOTE_STRAP_SHAPED``` This item fits into pockets intended for money straps (like a cash register).
- ```BATTERY_HEAVY``` This item is a heavy battery, and can be put in pockets that have heavy battery restriction.
- ```BATTERY_LIGHT``` This item is a light battery, and can be put in pockets that have light battery restriction.
- ```BATTERY_MEDIUM``` This item is a medium battery, and can be put in pockets that have medium battery restriction.
- ```BATTERY_ULTRA_LIGHT``` This item is an ultra light battery, and can be put in pockets that have ultra light battery restriction.
- ```BIONIC_ARMOR_INTERFACE``` This bionic can provide power to powered armor.
- ```BIONIC_FUEL_SOURCE``` Contents of this item is used for fueling bionics.
- ```BIONIC_NPC_USABLE``` Safe CBMs that NPCs can use without extensive NPC rewrites to utilize toggle CBMs.
- ```BIONIC_POWER_SOURCE``` This bionic is a source of bionic power.
- ```BIONIC_SLEEP_FRIENDLY``` This bionic won't provide a warning if the player tries to sleep while it's active.
- ```BIONIC_TOGGLED``` This bionic only has a function when activated, instead of causing its effect every turn.
- ```BIONIC_WEAPON_MELEE``` This weapon is bionic melee, used for different checks in EOCs.
- ```BIRD``` Food that only player with `BIRD` threshold mutation can eat.  See also `INEDIBLE`.
- ```BURNOUT``` You can visually inspect how much it is burned out (candle, torch).
- ```CALORIES_INTAKE``` This item allows you to see detailed info about your calories intake for today and tomorrow in consuming menu.  Can be used with `CALORIES_INTAKE_TRACKER` `use_action`, that shows the same info.
- ```CAMERA_PRO``` This item is professional camera, and increase the quality of made photos.
- ```CATTLE``` Food that only player with `CATTLE` threshold mutation can eat.  See also `INEDIBLE`.
- ```CBM``` This item is CBM, and works respectively.
- ```COIN_SHAPED``` This item is shaped like a coin and fits into the coin purse of a wallet.
- ```COLLAPSE_CONTENTS``` This item has its content hidden by default, and you need to manually reveal it using `> show/hide content` button.
- ```CONDUCTIVE``` Item is considered as conducting electricity, even if material it's made of is non-conductive.  Opposite of `NONCONDUCTIVE`.
- ```COOP_CARD``` Gives you access to the artisans workshop.
- ```CORPSE``` Flag used to spawn various human corpses during the mapgen.
- ```CREDIT_CARD_SHAPED``` This item is shaped like a credit card and fits into the card slots of a wallet and similar pockets.
- ```CRUTCHES``` Item with this flag helps characters not to fall down if their legs are broken.
- ```CUSTOM_EXPLOSION``` Flag, automatically applied to items that has defined `explosion` data in definition.  See `JSON_INFO.md`.
- ```CUT_HARVEST``` You need a grass-cutting tool like sickle to harvest this plant.
- ```DANGEROUS``` NPCs will not accept this item.  Explosion iuse actor implies this flag.  Implies `NPC_THROW_NOW`.
- ```DETERGENT``` This item can be used as a detergent in a washing machine.
- ```DISCOUNT_VALUE_1``` This item gives a small discount for fuel, bought in automated gas console.
- ```DISCOUNT_VALUE_2``` This item gives an average discount for fuel, bought in automated gas console.
- ```DISCOUNT_VALUE_3``` This item gives a big discount for fuel, bought in automated gas console.
- ```DROP_ACTION_ONLY_IF_LIQUID``` Cause `drop_action` only if item in liquid phase.
- ```DURABLE_MELEE``` Item is made to hit stuff and it does it well, so it's considered to be a lot tougher than other weapons made of the same materials.
- ```ELECTRONIC``` This item contain sensitive electronics which can be fried by nearby EMP blast.
- ```FAKE_MILL``` Item is a fake item, to denote a partially milled product by @ref Item::process_fake_mill, where conditions for its removal are set.
- ```FAKE_SMOKE``` Item is a fake item generating smoke, recognizable by @ref item::process_fake_smoke, where conditions for its removal are set.
- ```FELINE``` Food that only player with `FELINE` threshold mutation can eat.  See also `INEDIBLE`.
- ```FIREWOOD``` This item can serve as a firewood.  Items with this flag are sorted out to "Loot: Wood" zone.
- ```FLAMING``` This item is on fire, you deal additional fire damage using it.
- ```FRAGILE_MELEE``` Fragile items that fall apart easily when used as a weapon due to poor construction quality and will break into components when broken.
- ```FRESH_GRAIN``` This item is fresh-cut grain, and can be dried in a stook.
- ```GASFILTER_MED``` This is a medium size gas filter cartridge, that is used as magazine for various gasmasks.
- ```GASFILTER_SM``` This is a small size gas filter cartridge, that is used as magazine for various gasmasks.
- ```GAS_DISCOUNT``` Discount cards for the automated gas stations.
- ```GAS_TANK``` This item can store gases.
- ```GEMSTONE``` This is a gemstone, and you can put it in some jewelry.
- ```HARD``` Override item checks to be hard, rigid and uncomfortable without padding.  Opposite of `SOFT`.
- ```HELMET_HEAD_ATTACHMENT``` This item can be attached to hard hat; Currently used only for flashlights.
- ```HURT_WHEN_WIELDED``` Weapon deal damage to your right arm (or to both if weapon is two-handed), equal it's damage.
- ```INDUSTRIAL_CARD``` Used in industrial ID cards to open industrial card reader `t_card_industrial`.
- ```IRREPLACEABLE_CONSUMABLE``` This item will grow in price the longer cataclysm goes.  Currently not used.
- ```IS_PET_ARMOR``` Is armor for a pet monster, not armor for a person.
- ```ITEM_BROKEN``` Item was broken and won't activate anymore.
- ```JAVELIN``` This item is javelin, and can be put into javelin bag.
- ```LEAK_ALWAYS``` Leaks (may be combined with `RADIOACTIVE`).
- ```LEAK_DAM``` Leaks when damaged (may be combined with `RADIOACTIVE`).
- ```LUPINE``` Food that only player with `LUPINE` threshold mutation can eat (like dog food).  See also `INEDIBLE`.
- ```MC_MOBILE```, ```MC_HAS_DATA``` Memory card related flags, see einktabletpc and camera related functions.
- ```METHANOL_TANK``` This item is methanol tank, and is used as magazine for various methanol-powered tools.
- ```MILITARY_CARD``` Used in military ID cards to open military card reader `t_card_military`.
- ```MISSION_ITEM``` This item's chance to spawn isn't affected by world item spawn scaling factor.
- ```MOP``` This item could be used to mop up spilled liquids like blood or water.
- ```MOUSE``` Food that only player with `MOUSE` threshold mutation can eat.  See also `INEDIBLE`.
- ```MUNDANE``` This item uses magic-related features, but is not magic itself.  Enchantments turn the item magenta, by applying this flag the item's color won't be changed.  Also, for spells the item description would be changed from "This item casts *spell_name* at level *spell_level*" to "This item when activated: *spell_name*".  `use_action` of `"type": "cast_spell"` can use this feature separately, using boolean `"mundane": true`.
- ```MUTAGEN_SAMPLE``` This item is mutagen sample, and show `Used in the creation of mutagenic drugs` message in the item description.
- ```NANOFAB_REPAIR``` This item can be repaired using nanofabricator.
- ```NANOFAB_TEMPLATE``` This item is nanofabricator template, and can use related syntax.
- ```NEEDS_UNFOLD``` Has an additional time penalty upon wielding.  For melee weapons and guns this is offset by the relevant skill.  Stacks with `SLOW_WIELD`.
- ```NO_CLEAN``` this item is impossible to clean.
- ```NO_PACKED``` This item is not protected against contamination and won't stay sterile.  Only applies to CBMs.
- ```NO_REPAIR``` Prevents repairing of this item even if otherwise suitable tools exist.
- ```NO_SALVAGE``` Item cannot be broken down through a salvage process.  Best used when something should not be able to be broken down (i.e. base components like leather patches).
- ```NO_STERILE``` This item is not sterile.  Only applies to CBMs.
- ```NPC_ACTIVATE``` NPCs can activate this item as an alternative attack.  Currently done by throwing it right after activation.  `BOMB` implies this.
- ```NPC_ALT_ATTACK``` Shouldn't be set directly.  Implied by `NPC_ACTIVATE` and `NPC_THROWN`.
- ```NPC_SAFE``` NPC will consume this item if you give them, no matter of it's trust about you.
- ```NPC_THROWN``` NPCs will throw this item (without activating it first) as an alternative attack.
- ```NPC_THROW_NOW``` NPCs will try to throw this item away, preferably at enemies.  Implies `TRADER_AVOID` and `NPC_THROWN`.
- ```OLD_CURRENCY``` Paper bills and coins that used to be legal tender before the Cataclysm and may still be accepted by some automated systems.
- ```PALS_LARGE``` This item can be attached to MOLLE straps, and it will consume 3 slots.
- ```PALS_MEDIUM``` This item can be attached to MOLLE straps, and it will consume 2 slots.
- ```PALS_SMALL``` This item can be attached to MOLLE straps, and it will consume 1 slot.
- ```PAPER_SHAPED``` This item is shaped in form of thin paper sheet, and can be stored in leather journal.
- ```PERFECT_LOCKPICK``` Item is a perfect lockpick.  Takes only 5 seconds to pick a lock and never fails, but using it grants only a small amount of lock picking xp.  The item should have `LOCKPICK` quality of at least 1.
- ```PLANTABLE_SEED``` This item is a seed, and you can plant it.
- ```PRESERVE_SPAWN_OMT``` This item will store the OMT that it spawns in, in the `spawn_location_omt` item var.
- ```PROVIDES_TECHNIQUES``` This item will provide martial arts techniques when worn/in the character's inventory, in addition to those provided by the weapon and martial art.
- ```PSEUDO``` Used internally to mark items that are referred to in the crafting inventory but are not actually items.  They can be used as tools, but not as components.  Implies `TRADER_AVOID`.
- ```RABBIT``` Food that only player with `RABBIT` threshold mutation can eat.  See also `INEDIBLE`.
- ```RADIOACTIVE``` Is radioactive (can be used with `LEAK_*`).
- ```RADIO_INVOKE_PROC``` This item can receive a signal, that will make it detonate.
- ```RAD_DETECT``` This item is a radiation badge, and can print it's change in color depending on radiation level around the player.  Hardcoded.
- ```RAIN_PROTECT``` Protects from sunlight and from rain when wielded.
- ```RAT``` Food that only player with `RAT` threshold mutation can eat.  See also `INEDIBLE`.
- ```REBREATHER_CART``` This is a rebreather cartridge, and is used as magazine for various rebreather masks.
- ```REBREATHER``` If you wear this item, your oxygen won't fall lower than 12 (default is ~50).
- ```REDUCED_BASHING``` Gunmod flag; reduces the item's bashing damage by 50%.
- ```REDUCED_WEIGHT``` Gunmod flag; reduces the item's base weight by 25%.
- ```REQUIRES_TINDER``` Requires tinder to be present on the tile this item tries to start a fire on.
- ```ROBOFAC_ROBOT_MEDIUM``` This item is a medium-size Hub 01 drone, and you can store it in specific slot in drone-tech harness.
- ```ROBOFAC_ROBOT_SMALL``` This item is a small-size Hub 01 drone, and you can store it in specific slot in drone-tech harness.
- ```SCIENCE_CARD_MAINTENANCE_BLUE```
- ```SCIENCE_CARD_MAINTENANCE_BLUE```
- ```SCIENCE_CARD_MAINTENANCE_GREEN```
- ```SCIENCE_CARD_MAINTENANCE_YELLOW```
- ```SCIENCE_CARD_MEDICAL_RED```
- ```SCIENCE_CARD_MUTAGEN_CYAN```
- ```SCIENCE_CARD_MUTAGEN_GREEN```
- ```SCIENCE_CARD_MUTAGEN_PINK```
- ```SCIENCE_CARD_MU_UNIVERSAL```
- ```SCIENCE_CARD_SECURITY_BLACK```
- ```SCIENCE_CARD_SECURITY_MAGENTA```
- ```SCIENCE_CARD_SECURITY_YELLOW```
- ```SCIENCE_CARD_TRANSPORT_1```
- ```SCIENCE_CARD_VISITOR``` This and above are used to open related doors in TCL.
- ```SHEATH_BOW``` This item can fit into bow sling.
- ```SHEATH_SPEAR``` This item can be attached to spear strap.
- ```SINGLE_USE``` This item is deleted after being used.  Items that count by charge do not need this as they are deleted when charges run out.
- ```SLEEP_AID_CONTAINER``` This item allows sleep aids inside of it to help in sleeping (e.g. a pillowcase).
- ```SLEEP_AID``` This item helps in sleeping.
- ```SLEEP_IGNORE``` This item is not shown as before-sleep warning.
- ```SLOW_WIELD``` Has an additional time penalty upon wielding.  For melee weapons and guns this is offset by the relevant skill.  Stacks with `NEEDS_UNFOLD`.
- ```SOFT``` Override item checks to be soft, not rigid and comfortable.  Opposite of `HARD`.
- ```SOLARPACK_ON``` This item is turned on solar backpack, and can charge different stuff if under the sun.
- ```SPAWN_ACTIVE``` This item always spawn active, no need to activate it manually.
- ```SPLINT``` This item is splint, when worn on broken body part, it slowly mend it.
- ```STRICT_HUMANITARIANISM``` Flag, automatically applied to food, if it was cooked from demihuman meat, and allow a different food interactions in names.
- ```TACK``` Item can be used as tack for a mount.
- ```TANGLE``` When this item is thrown, and hits a target, it has a chance to tangle them up and immobilize them.
- ```TARDIS``` Container item with this flag bypasses internal checks for pocket data, so inside it could be bigger than on the outside, and could hold items that otherwise won't fit its dimensions.
- ```TIE_UP``` Item can be used to tie up a creature.
- ```TINDER``` This item can be used as tinder for lighting a fire with a `REQUIRES_TINDER` flagged firestarter.
- ```TOBACCO``` This item is a lit cigar or cigarette, and gives smoking effect when you wear it.
- ```TOURNIQUET``` This item is tourniquet, it temporarily reduces bleed intensity and increases your effective compression limit.
- ```TOW_CABLE``` This item is a tow cable, and allow towing the vehicle.
- ```TRADER_AVOID``` NPCs will not start with this item.  Use this for active items (e.g. flashlight (on)), dangerous items (e.g. active bomb), fake items or unusual items (e.g. unique quest item).
- ```TRADER_KEEP_EQUIPPED``` NPCs will only trade this item if they aren't currently wearing or wielding it.
- ```TRADER_KEEP``` NPCs will not trade this item away under any circumstances.
- ```TWO_WAY_RADIO``` this items is two-way radio, and work accordingly.
- ```UNBREAKABLE_MELEE``` Never gets damaged when used as melee weapon.
- ```UNBREAKABLE``` This item can not be damaged, be that directly, while worn as armor, or when used as a melee weapon.
- ```UNRECOVERABLE``` Cannot be recovered from a disassembly.
- ```USE_POWER_WHEN_HIT``` This armor consume energy when you got hit, equal to damage that was dealt (energy consuming happen before the armor mitigation).
- ```WATER_BREAK_ACTIVE``` Item can get wet and is broken in water if active.
- ```WATER_BREAK``` Item is broken in water.
- ```WATER_DISSOLVE``` Item is dissolved in water.
- ```ZERO_WEIGHT``` Normally items with zero weight will generate an error.  Use this flag to indicate that zero weight is intentional and suppress that error.


## Guns

These can be applied to guns or gunmods, adding different effects to the guns.

- ```BACKBLAST``` Causes a small explosion behind the person firing the weapon.  Currently not implemented?
- ```BIPOD``` Handling bonus only applies on `MOUNTABLE` map/vehicle tiles.  Does not include wield time penalty (see `SLOW_WIELD`).
- ```BRASS_CATCHER``` This gunmod is brass catcher, and can store all casing you shoot.
- ```CHOKE``` This gunmod is a choke, and it prevent you from shooting slugs.
- ```COLLAPSED_STOCK``` Decrease the length of the gun for 20 cm.  Same as `FOLDED_STOCK`.  Currently not working.
- ```COLLAPSIBLE_STOCK``` Reduces weapon volume proportional to the base size of the gun (excluding any mods).  Does not include wield time penalt.  See also `NEEDS_UNFOLD`.
- ```CONSUMABLE``` Makes a gunpart have a chance to get damaged depending on ammo fired, and definable fields 'consume_chance' and 'consume_divisor'.
- ```DISABLE_SIGHTS``` Prevents use of the base weapon sights.
- ```EASY_CLEAN``` This weapon is relatively simple, and you spend half as time to clean and lube it.
- ```FIRE_TWOHAND``` Gun can only be fired if player has two free hands.
- ```FOLDED_STOCK``` Decrease the length of the gun for 20 cm.  Same as `COLLAPSED_STOCK`.
- ```INSTALL_DIFFICULT``` This gunmod is difficult to install, and potentially you can damage your gun if you fail.
- ```IRREMOVABLE``` Makes so that the gunmod cannot be removed.
- ```IS_ARMOR``` This gunmod can use armor syntax and can be worn (same as weapon you install this mod).
- ```LASER_SIGHT``` This gunmod is a laser sight, and provide a sight bonus if specific conditions are met (target is close, and it's not too bright?).
- ```MECH_BAT``` This is an exotic battery designed to power military mechs.
- ```MOUNTED_GUN``` Gun can only be used on terrain / furniture with the `MOUNTABLE` flag.
- ```NO_TURRET``` Prevents generation of a vehicle turret prototype for this gun.
- ```NO_UNLOAD``` Cannot be unloaded.
- ```PRIMITIVE_RANGED_WEAPON``` Allows using non-gunsmith tools to repair (but not reinforce) it.
- ```PUMP_ACTION``` Gun has rails on its pump action, allowing to install only mods with `PUMP_RAIL_COMPATIBLE` flag on underbarrel slot.
- ```PUMP_RAIL_COMPATIBLE``` Mod can be installed on underbarrel slot of guns with rails on their pump action.
- ```RELOAD_AND_SHOOT``` Firing automatically reloads and then shoots.
- ```RELOAD_EJECT``` Ejects shell from gun on reload instead of when fired.
- ```RELOAD_ONE``` Only reloads one round at a time.
- ```REMOVED_STOCK``` Decrease the length of the gun for 26 cm.  Applied when you saw off the stock.
- ```STR_DRAW``` Range with this weapon is reduced unless character has at least twice the required minimum strength.
- ```STR_RELOAD``` Reload speed is affected by strength.
- ```UNDERWATER_GUN``` Gun is optimized for usage underwater, performs badly outside of water.
- ```WATERPROOF_GUN``` Gun does not rust and can be used underwater.
- ```WONT_TRAIN_MARKSMANSHIP``` Shooting this gun won't train your marksmanship.


### Firing modes

- ```MELEE``` Melee attack using properties of the gun or auxiliary gunmod.
- ```NPC_AVOID``` NPCs will not attempt to use this mode.


### Gun faults

- ```BAD_CYCLING``` 1/16 chance that the gun fails to cycle when fired resulting in `fault_gun_chamber_spent` fault.
- ```BLACKPOWDER_FOULING_DAMAGE``` Causes the gun to take random acid damage over time.
- ```NEEDS_NO_LUBE``` Gun doesn't need lube to work properly.  Unaffected by the `UNLUBRICATED` fault.
- ```NEVER_JAMS``` Never malfunctions.  Unaffected by the `JAMMED_GUN` fault.
- ```NO_DIRTYING``` Prevents the gun from receiving `fault_gun_dirt` fault.
- ```NON_FOULING``` Gun does not become dirty or blackpowder fouled.
- ```JAMMED_GUN``` Stops burst fire.  Adds delay on next shot.
- ```UNLUBRICATED``` Randomly causes screeching noise when firing and applies damage when that happens.


## Magazines

- ```MAG_BULKY``` Can be stashed in an appropriate oversize ammo pouch (intended for bulky or awkwardly shaped magazines).
- ```MAG_COMPACT``` Can be stashed in an appropriate ammo pouch (intended for compact magazines).
- ```MAG_DESTROY``` Magazine is destroyed when the last round is consumed (intended for ammo belts).  Has precedence over `MAG_EJECT`.
- ```MAG_EJECT``` Magazine is ejected from the gun/tool when the last round is consumed.
- ```SPEEDLOADER``` Acts like a magazine, except it transfers rounds to the emptied target gun or magazine instead of being inserted into it.
- ```SPEEDLOADER_CLIP``` Acts like a `SPEEDLOADER`, except the target gun or magazine don't have to be emptied to oocur the transferments.


## Magic

See [Spell flags](MAGIC.md#spell-flags).


## Mapgen

See [Mapgen flags](MAPGEN.md#mapgen-flags).


## Map Specials

- ```mx_bandits_block```  Road block made by bandits from tree logs, caltrops, or nailboards.
- ```mx_burned_ground``` Fire has ravaged this place.
- ```mx_point_burned_ground``` Fire has ravaged this place (partial application).
- ```mx_casings``` Several types of spent casings (solitary, groups, entire overmap tile).
- ```mx_city_trap``` A spinning blade trap with a loudspeaker to attract zombies.
- ```mx_clay_deposit``` A small surface clay deposit.
- ```mx_clearcut``` All trees become stumps.
- ```mx_collegekids``` Corpses and items.
- ```mx_corpses``` Up to 5 corpses with everyday loot.
- ```mx_crater``` Crater formed using a bomb.
- ```mx_drugdeal``` Corpses and some drugs.
- ```mx_dead_vegetation``` Kills all plants (e.g. aftermath of acid rain).
- ```mx_point_dead_vegetation``` Kills all plants (e.g. aftermath of acid rain) (partial application).
- ```mx_exocrash_1``` Area of glassed sand created by a crashed pod of space travelers. Populated by zomborgs.
- ```mx_exocrash_2``` Area of glassed sand created by a crashed pod of space travelers. Populated by zomborgs.
- ```mx_fallen_shed``` A collapsed shed.
- ```mx_grove``` All trees and shrubs become a single species of tree.
- ```mx_grass``` A meadow with tall grass.
- ```mx_grass_2``` A meadow with tall grass.
- ```mx_grave``` A grave in the open field, with a corpse and some everyday loot.
- ```mx_helicopter``` Metal wreckage and some items.
- ```mx_house_spider``` A house with wasps, dermatiks, and walls converted to paper.
- ```mx_house_wasp``` A house with spiders, webs, eggs and some rare loot.
- ```mx_jabberwock``` A *chance* of a jabberwock.
- ```mx_looters``` Up to 5 bandits spawn in the building.
- ```mx_marloss_pilgrimage``` A sect of people worshiping fungaloids.
- ```mx_mass_grave``` Mass grave with zombies and everyday loot.
- ```mx_mayhem``` Several types of road mayhem (firefights, crashed cars etc).
- ```mx_military``` Corpses and some military items.
- ```mx_minefield``` A military roadblock at the entry of the bridges with landmines scattered in the front of it.
- ```mx_nest_dermatik``` Dermatik nest.
- ```mx_nest_wasp``` Wasp nest.
- ```mx_null``` No special at all.
- ```mx_pond``` A small pond.
- ```mx_pond_forest``` A small basin.
- ```mx_pond_forest_2``` A small basin.
- ```mx_pond_swamp``` A small bog.
- ```mx_pond_swamp_2``` A small bog.
- ```mx_portal_in``` Another portal to neither space.
- ```mx_portal``` Portal to neither space, with several types of surrounding environment.
- ```mx_prison_bus``` Prison bus with zombie cops and zombie prisoners.
- ```mx_prison_van``` Traces of a violent escape near a prison van.
- ```mx_reed``` Extra water vegetation.
- ```mx_roadblock``` Roadblock furniture with turrets and some cars.
- ```mx_roadworks``` Partially closed damaged road with chance of work equipment and utility vehicles.
- ```mx_science``` Corpses and some scientist items.
- ```mx_shrubbery``` All trees and shrubs become a single species of shrub.
- ```mx_spider``` A big spider web, complete with spiders and eggs.
- ```mx_supplydrop``` Crates with some military items in it.
- ```mx_Trapdoor_spider_den``` A spider spawning out of nowhere.
- ```mx_trees``` A small chunk of forest with puddles with fresh water.
- ```mx_trees_2``` A small chunk of forest with puddles with fresh water.
- ```mx_shia``` A *chance* of Shia, if the Crazy Cataclysm mod is enabled.
- ```mx_shia``` A *chance* of Jackson, if the Crazy Cataclysm mod is enabled.


## Material Phases

- ```GAS```
- ```LIQUID```
- ```NULL```
- ```PLASMA```
- ```SOLID```


## Melee

Melee flags are fully compatible with [tool flags](#tools), and vice versa.

- ```ALLOWS_BODY_BLOCK``` Allows body blocks (arms and legs blocks) to trigger even while wielding the item with the flag.  Used with small items like knives and pistols that do not interfere with the ability to block with your body.  Only works if your current martial art allows body blocks too.
- ```ALWAYS_TWOHAND``` Item is always wielded with two hands.  Without this, the items volume and weight are used to calculate this.
- ```BIONIC_WEAPON``` Cannot wield this item normally.  It has to be attached to a bionic and equipped through activation of the bionic.
- ```DIAMOND``` Diamond coating adds 30% bonus to cutting and piercing damage.
- ```MESSY``` Creates more mess when pulping.
- ```NO_CVD``` Item can never be used with a CVD machine.
- ```NO_RELOAD``` Item can never be reloaded (even if has a valid ammo type).
- ```NO_UNWIELD``` Cannot unwield this item.  Fake weapons and tools wielded from bionics will automatically have this flag added.
- ```NONCONDUCTIVE``` Item doesn't conduct electricity thanks to some feature (nonconductive material of handle or entire item) and thus can be safely used against electricity-themed monsters without the risk of zapback.  Opposite of `CONDUCTIVE`.
- ```POLEARM``` Item is clumsy up close and does 70% of normal damage against adjacent targets.  Should be paired with `REACH_ATTACK`.  Simple reach piercing weapons like spears should not get this flag.
- ```REACH_ATTACK``` Allows performing a melee attack on 2-tile distance.
- ```REACH3``` Allows performing a melee attack on 3-tile distance.
- ```SHEATH_AXE``` Item can be sheathed in an axe sheath.
- ```SHEATH_GOLF``` Item can be sheathed in a golf bag.
- ```SHEATH_KNIFE``` Item can be sheathed in a knife sheath, it's applicable to small/medium knives (with volume not bigger than 2).
- ```SHEATH_SWORD``` Item can be sheathed in a sword scabbard.
- ```SPEAR``` When making reach attacks intervening `THIN_OBSTACLE` terrain is not an obstacle.  Should be paired with `REACH_ATTACK`.
- ```UNARMED_WEAPON``` Fighting while wielding this item still counts as unarmed combat.
- ```WHIP``` Has a chance of disarming the opponent.


## Monsters

Used to describe monster characteristics and set their properties and abilities.

- ```ACIDPROOF``` Immune to acid.
- ```ACIDTRAIL``` Leaves a trail of acid.
- ```ACID_BLOOD``` Makes monster bleed acid.  Does not automatically dissolve in a pool of acid on death.
- ```ALL_SEEING``` Can see every creature within its vision (highest of day/night vision counts) on the same z-level.
- ```ALWAYS_SEES_YOU``` This monster always knows where the avatar is.
- ```ALWAYS_VISIBLE``` This monster can always be seen regardless of line of sight or light level.
- ```ANIMAL``` Is an *animal* for purposes of the `Animal Empathy` trait.
- ```AQUATIC``` Confined to water.
- ```ARTHROPOD_BLOOD``` Forces monster to bleed hemolymph.
- ```ATTACKMON``` Attacks other monsters regardless of faction relations when pathing through their space.
- ```ATTACK_LOWER``` Even though this monster is large in size it can't attack upper limbs.
- ```ATTACK_UPPER``` Even though this monster is small in size it can attack upper limbs.
- ```BADVENOM``` Attack may **severely** poison the player.
- ```BASHES``` Bashes down doors.
- ```BILE_BLOOD``` Makes monster bleed bile.
- ```BIOLOGICALPROOF``` Immune to biological damage.
- ```BORES``` Tunnels through just about anything (15x bash multiplier e.g. dark wyrms' bash skill 12 -> 180).
- ```CAMOUFLAGE``` Stays invisible up to (current Perception, + base Perception if the character has the Spotting proficiency) tiles away, even in broad daylight.  Monsters see it from the lower of `vision_day` and `vision_night` ranges.
- ```CANPLAY``` This creature can be played with if it's a pet.
- ```CAN_BE_CULLED``` This animal can be culled if it's a pet.
- ```CAN_DIG``` Will dig on any diggable terrain the same way `DIGS` does, however, will walk normally over non-diggable terrain.
- ```CAN_OPEN_DOORS``` Can open doors on its path.
- ```CLIMBS``` Can climb over fences or similar obstacles quickly.
- ```COLDPROOF``` Immune to cold damage.
- ```COMBAT_MOUNT```  This mount has better chance to ignore hostile monster fear.
- ```CONSOLE_DESPAWN``` Despawns when a nearby console is properly hacked.
- ```CONVERSATION``` This monster can engage in conversation.  Will need to have chat_topics as well.
- ```CORNERED_FIGHTER``` This creature will stop fleeing and fight back if enemies pursue it into melee range.
- ```DORMANT``` This monster will be revived by dormant corpse traps.
- ```DEADLY_VIRUS``` This monster can inflict the `zombie_virus` effect.  Used by the Dark Days of the Dead and Deadly Zombie Virus mods.
- ```DESTROYS``` Bashes down walls and more (2.5x bash multiplier, where base is the critter's max melee bashing).
- ```DIGS``` Digs through the ground.  Will not travel through non-diggable terrain such as roads.
- ```DOGFOOD``` Can be ordered to attack with a dog whistle.
- ```DRIPS_GASOLINE``` Occasionally drips gasoline on move.
- ```DRIPS_NAPALM``` Occasionally drips napalm on move.
- ```DROPS_AMMO``` This monster drops ammo.  Should not be set for monsters that use pseudo ammo.
- ```EATS``` This creature has a stomach_size (defined in its monster json) which gets filled up when it eats, and digests food over time.
- ```ELECTRIC_FIELD``` This monster is surrounded by an electrical field that ignites flammable liquids near it.  It also deals damage to other monsters with this flag, with "The %s's disabled electrical field reverses polarity!" message.
- ```ELECTRIC``` Shocks unarmed attackers.
- ```ELECTRONIC``` Affected by EMP blasts and similar stuff (e.g. a robot).
- ```FILTHY``` Any clothing it drops will be filthy.  The squeamish trait prevents wearing clothing with this flag, one can't craft anything from filthy components, and wearing filthy clothes may result in infection if hit in melee.
- ```FIREPROOF``` Immune to fire.
- ```FIREY``` Burns stuff and is immune to fire.
- ```FISHABLE``` This monster can be fished.
- ```FLAMMABLE``` Monster catches fire, burns, and spreads fire to nearby objects.
- ```FLIES``` Can fly over open air without dropping z-level, over water, etc.
- ```GOODHEARING``` Pursues sounds more than most monsters.
- ```GRABS``` Its attacks may grab you!
- ```GROUP_BASH``` Gets help from monsters around it when bashing, adding their strength together.
- ```GROUP_MORALE``` More courageous when near friends.
- ```GUILT_ANIMAL``` Killing this monster(i.e. a hatchling or a kitten) causes guilt to the player and is counted for the kill thresholds of animals where player experiences progressively less morale penalty. WARNING: Do not use without 'death_guilt' death function or together with other guilt flags.
- ```GUILT_CHILD``` Killing this monster(i.e. a zombie child or mutant child) causes guilt to the player and is counted for the kill thresholds of children where player experiences progressively less morale penalty. WARNING: Do not use without 'death_guilt' death function or together with other guilt flags.
- ```GUILT_HUMAN``` Killing this monster(i.e. a panicked person or futile fighter) causes guilt to the player and is counted for the kill thresholds of non-NPC humans where player experiences progressively less morale penalty. WARNING: Do not use without 'death_guilt' death function or together with other guilt flags.
- ```GUILT_OTHERS``` Killing this monster(i.e. a blood sacrifice) causes guilt to the player and is counted for the kill thresholds of monsters that do not fit other categories where player experiences progressively less morale penalty. WARNING: Do not use without 'death_guilt' death function or together with other guilt flags.
- ```HARDTOSHOOT``` It's one size smaller for ranged attacks, no less than the `TINY` flag.
- ```HAS_MIND``` Is sapient and capable of reason (mi-go, triffids, cyborgs, etc.).  `HUMAN` assumes `HAS_MIND`.
- ```HEARS``` It can hear you.
- ```HIT_AND_RUN``` Flee for several turns after a melee attack.
- ```HUMAN``` It's a live human, as long as it's alive.
- ```ID_CARD_DESPAWN``` Despawns when a science ID card is used on a nearby console.
- ```IMMOBILE``` Doesn't move (e.g. turrets).
- ```INSECTICIDEPROOF``` It's immune to insecticide even though it's made of bug flesh ("iflesh").
- ```INTERIOR_AMMO``` Monster contains ammo inside itself, no need to load on launch.  Prevents ammo from being dropped on disable.
- ```KEENNOSE``` Keen sense of smell.
- ```KEEP_DISTANCE``` Monster will try to keep `tracking_distance` number of tiles between it and its current target.
- ```LOUDMOVES``` Makes move noises as if ~2 sizes louder, even if flying.
- ```MECH_DEFENSIVE``` This mech can protect you thoroughly when piloted.
- ```MECH_RECON_VISION``` This mech grants you night-vision and enhanced overmap sight radius when piloted.
- ```MILITARY_MECH``` Is a military-grade mech.
- ```MILKABLE``` Produces milk when milked.
- ```NEMESIS``` Tags Nemesis enemies for the `HAS_NEMESIS` mutation.
- ```NEVER_WANDER``` This monster will never join wandering hordes.
- ```NIGHT_INVISIBILITY``` Monster becomes invisible if it's more than one tile away and the lighting on its tile is LL_LOW or less.  Visibility is not affected by night vision.
- ```NOGIB``` Does not leave gibs / meat chunks when killed with huge damage.
- ```NOHEAD``` Headshots not allowed!
- ```NOT_HALLUCINATION``` This monster does not appear while the player is hallucinating.
- ```NO_BREATHE``` Creature can't drown and is unharmed by gas, smoke or poison.
- ```NO_BREED``` Creature doesn't reproduce even though it has reproduction data.  Useful when using `copy-from` to make child versions of adult creatures.
- ```NO_FUNG_DMG``` This monster can't be damaged by fungal spores and can't be fungalized either.
- ```NO_NECRO``` This monster can't be revived by necros.  It will still rise on its own.
- ```NULL``` Source use only.
- ```PACIFIST``` Monster will never do melee attacks.  Useful for having them use grab without attacking the player.
- ```PARALYZEVENOM``` This monster can apply `paralyzepoison` effect for 10 minutes.
- ```PARALYZE``` Attack may paralyze the player with venom.
- ```PATH_AVOID_DANGER_1``` This monster will path around some dangers instead of through them.
- ```PATH_AVOID_DANGER_2``` This monster will path around most dangers instead of through them.
- ```PATH_AVOID_FALL``` This monster will path around cliffs instead of off of them.
- ```PATH_AVOID_FIRE``` This monster will path around heat-related dangers instead of through them.
- ```PAY_BOT``` Creature can be turned into a pet for a limited time in exchange of e-money.
- ```PET_HARNESSABLE``` Creature can be attached to a harness.
- ```PET_MOUNTABLE``` Creature can be ridden or attached to a harness.
- ```PET_WONT_FOLLOW``` This monster won't follow the player automatically when tamed.
- ```PHOTOPHOBIC``` Severely weakened if in light level >= 30 (within about 7 tiles of a full-strength flashlight) by applying photophobia effect.
- ```PLASTIC``` Absorbs physical damage to a great degree.
- ```POISON``` Poisonous to eat.
- ```PRIORITIZE_TARGETS``` This monster will prioritize targets depending on their danger levels.
- ```PUSH_MON``` Can push creatures out of its way.
- ```PUSH_VEH``` Can push vehicles out of its way.
- ```QUEEN``` When it dies, local populations start to die off too.
- ```QUIETMOVES``` Makes move noises as if ~2 sizes smaller.
- ```RANGED_ATTACKER``` Monster has any sort of ranged attack.
- ```REVIVES_HEALTHY``` When revived, this monster has full hitpoints and speed.
- ```REVIVES``` Monster corpse will revive after a short period of time.
- ```RIDEABLE_MECH``` This monster is a mech suit that can be piloted.
- ```SEES``` It can see you (and will run/follow).
- ```SHEARABLE``` This monster can be sheared for wool.
- ```SHORTACIDTRAIL``` Leaves an intermittent trail of acid.  See also `ACIDTRAIL`.
- ```SILENT_DISAPPEAR``` If this monster dissapear (left no corpse), the `The %s disappears.` message won't be printed.
- ```SILENTMOVES``` Makes no noise at all when moving.
- ```SLUDGEPROOF``` Ignores the effect of sludge trails.
- ```SLUDGETRAIL``` Causes the monster to leave a sludge trap trail when moving.
- ```SMALLSLUDGETRAIL``` Causes the monster to occasionally leave a 1-tile sludge trail when moving.
- ```SMALL_HIDER``` This small monster can hide under or behind furniture such as beds, refrigerators, and underbrush.
- ```SMELLS``` It can smell you.
- ```STUMBLES``` Stumbles in its movement.
- ```STUN_IMMUNE``` This monster is immune to stun.
- ```SUNDEATH``` Dies in full sunlight.
- ```SWARMS``` Groups together and forms loose packs.
- ```SWIMS``` Treats water as 50 movement point terrain.
- ```VAMP_VIRUS``` This monster can inflict the `vampire_virus` effect.  Used by Xedra Evolved mod.
- ```VENOM``` Attack may poison the player.
- ```VERMIN``` Obsolete flag for inconsequential monsters, now prevents loading.
- ```WARM``` Warm blooded.
- ```WATER_CAMOUFLAGE``` If in water, stays invisible up to (current Perception, + base Perception if the character has the Spotting proficiency) tiles away, even in broad daylight.  Monsters see it from the lower of `vision_day` and `vision_night` ranges.  Can also make it harder to see in deep water or across z-levels if it is underwater and the viewer is not.
- ```WEBWALK``` Doesn't destroy webs and won't get caught in them.


### Anger, Fear and Placation Triggers

For more information about monster aggro, see [MONSTERS.md](MONSTERS.md#aggrocharacter).

- ```FIRE``` Triggers if there's a fire within 3 tiles, the strength of the effect equals 5 * the field intensity of the fire.
- ```FRIEND_ATTACKED``` Triggers if the monster sees another monster of a friendly faction being attacked; strength = 15.  **Requires** an instance of the trigger on the attacked monster as well (the trigger type need not match, just the trigger itself).  Always triggers character aggro.
- ```FRIEND_DIED``` Triggers if the monster sees another monster of a friendly faction dying; strength = 15.  **Requires** an instance of the trigger on the attacked monster as well(the trigger type need not match, just the trigger itself)!  Always triggers character aggro.
- ```HOSTILE_SEEN``` Increases aggression/ decreases morale by a random amount between 0-2 for every potential enemy it can see, up to 20 aggression.  Anger/fear trigger only.  Triggers character aggro `<anger/2>%` of the time.
- ```HURT``` Triggers when the monster is hurt, strength equals 1 + (damage / 3 ).  Always triggers character aggro.
- ```NULL``` Source use only?
- ```PLAYER_CLOSE``` Triggers when a potential enemy is within 5 tiles range.  Anger/fear trigger only.  Triggers character aggro `<anger>%` of the time.
- ```PLAYER_WEAK``` Strength = `10 - (percent of hp remaining / 10)` if a non-friendly critter has less than 70% hp remaining .  Triggers character aggro `<anger>%` of the time.
- ```PLAYER_NEAR_BABY``` Increases monster aggression by 8 and morale by 4 if **the player** comes within 3 tiles of its offspring (defined by the baby_monster field in its reproduction data).  Anger trigger only.  Always triggers character aggro.
- ```SOUND``` Not an actual trigger, monsters above 10 aggression and 0 morale will wander towards, monsters below 0 morale will wander away from the source of the sound for 1 turn (6, if they have the `GOODHEARING` flag).
- ```STALK``` Raises monster aggression by 1, triggers 20% of the time each turn if aggression > 5.  Anger trigger only!
- ```MATING_SEASON``` Increases aggression by 3 if a potential enemy is within 5 tiles range and the season is the same as the monster's mating season (defined by the baby_flags field in its reproduction data).  Anger trigger only.  Triggers character aggro `<anger>%` of the time.


### Categories

- ```CLASSIC``` Only monsters we expect in a classic zombie movie.
- ```NULL``` No category.
- ```WILDLIFE``` Natural animals.


### Death Functions

Previously hardcoded death functions have been mostly replaced by the `death_function` field.

- ```BOOMER``` Explodes in vomit.
- ```BROKEN``` Spawns a broken robot item.  The monster's `id` has the prefix `mon_` replaced by `broken_`: `mon_eyebot` -> `broken_eyebot`.
- ```DISAPPEAR``` Hallucination disappears.
- ```FUNGUS``` Explodes in spores.


### Monster Groups

The condition flags limit when monsters can spawn.

#### Seasons

Multiple season conditions will be combined together so that any of those conditions become valid time of year spawn times.

- ```AUTUMN```
- ```SPRING```
- ```SUMMER```
- ```WINTER```


#### Time of day

Multiple time of day conditions will be combined together so that any of those conditions become valid time of day spawn times.

- ```DAWN```
- ```DAY```
- ```DUSK```
- ```NIGHT```


### Special attacks

Special attacks have been moved to [MONSTER_SPECIAL_ATTACKS.md](MONSTER_SPECIAL_ATTACKS.md) as they have all been migrated away from flags.


## Mutations

See [Character](#character)


## Overmap

### Overmap connections

- ```ORTHOGONAL``` The connection generally prefers straight lines, avoids turning wherever possible.

### Overmap specials

- ```BEE``` Location is related to bees.  Used to classify location.
- ```BLOB``` Location should "blob" outward from the defined location with a chance to be placed in adjacent locations.
- ```CLASSIC``` Location is allowed when classic zombies are enabled.
- ```DERMATIK``` Location is related to a dermatik infestation.  Used to classify location.
- ```FARM```
- ```FUNGAL``` Location is related to fungi.  Used to classify location.
- ```GLOBALLY_UNIQUE``` Location will only occur once per world.  `occurrences` is overridden to define a percent chance (e.g. `"occurrences" : [75, 100]` is 75%).
- ```UNIQUE``` Location is unique and will only occur once per overmap.  `occurrences` is overridden to define a percent chance (e.g. `"occurrences" : [75, 100]` is 75%).
- ```LAKE``` Location is placed on a lake and will be ignored for placement if the overmap doesn't contain any lake terrain.
- ```MAN_MADE``` For location, created by human.  Used by the Innawood mod.
- ```MI-GO``` Location is related to mi-go.
- ```SAFE_AT_WORLDGEN``` Location will not spawn overmap monster groups during worldgen (does not affect monsters spawned by mapgen).
- ```TRIFFID``` Location is related to triffids.  Used to classify location.
- ```URBAN```
- ```WILDERNESS``` Locations that have no road connection.

### Overmap terrains

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
- ```RISK_HIGH``` For NPC AI, this location has a high risk associated with it (e.g. labs, superstores, etc.).
- ```RISK_LOW``` For NPC AI, this location is secluded and remote, and appears to be safe.
- ```GENERIC_LOOT``` This is a place that may contain any of the above, but at a lower frequency, usually a house.


## Recipes

- ```ALLOW_ROTTEN``` Explicitly allow rotten components when crafting non-perishables.
- ```BLIND_EASY``` Easy to craft with little to no light.
- ```BLIND_HARD``` Possible to craft with little to no light, but difficult.
- ```FULL_MAGAZINE``` Crafted or deconstructed items from this recipe will have fully-charged magazines.
- ```NEED_FULL_MAGAZINE``` If this recipe requires magazines, it needs one that is full.
- ```NO_RESIZE``` This clothes you crafted spawn unfitted 
- ```SECRET``` Not automatically learned at character creation time based on high skill levels.


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


### Camp building recipes

These flags apply only to camp building recipes (hubs and expansions), i.e. those that have category `CC_BUILDING`.

- ```NO_FOOD_REQ``` Food requirements are waived for this camp building recipe.


#### Blueprint reorientation flags

The purpose of these flags is to allow reuse of blueprints to create the "same" facility oriented differently.  Mirroring takes place before rotation, and it is an error to try to apply mirroring multiple times with the same orientation, as well as to try to apply multiple rotations.  It is permitted to apply different versions of the flags if they apply to different directions (and it is indeed the primary intended usage).

- ```MAP_MIRROR_HORIZONTAL``` Causes the building recipe to mirror both the location and contents of the blueprint(s) used by the recipe.
- ```MAP_MIRROR_VERTICAL``` Causes the building recipe to mirror both the location and contents of the blueprint(s) used by the recipe.
- ```MAP_ROTATE_[X]``` X has to be one of 90, 180, or 270 and requests the blueprint to be rotated by the given number of degrees before being applied.
- ```MAP_MIRROR_HORIZONTAL_IF_[Y]``` Similar to MAP_MIRROR_HORIZONTAL, but is applied only if the tile the expansion is on is Y.  The legal values for Y are "NW", "N", "NE", "E", "SE", "S", SW", and "W".
- ```MAP_MIRROR_VERTICAL_IF_[Y]``` The vertical version of the previous flag.
- ```MAP_ROTATE_[X]_IF_[Y]``` The expansion location dependent version of "MAP_ROTATE_X", with Y having the same legal values as the two sets of flags above.


## Scenarios

- ```BORDERED``` Initial start location is bordered by an enormous wall of solid rock.
- ```CHALLENGE``` Game won't choose this scenario in random game types.
- ```CITY_START``` Scenario is available only when city size value in world options is more than 0.
- ```FIRE_START``` Player starts the game with fire nearby.
- ```HELI_CRASH``` Player starts the game with various limbs wounds.
- ```LONE_START``` This scenario won't spawn a fellow NPC on game start.
- ```NO_BONUS_ITEMS``` This scenario prevent bonus items (such as inhalers with the `ASTHMA` trait) from being given to this profession
- ```SUR_START``` Write `Zombies nearby` in the scenario info, doesn't spawn monsters by itself (put close to `LONE_START`)


### Profession

- ```SCEN_ONLY``` Profession can be chosen only as part of the appropriate scenario.


### Starting Location

- ```ALLOW_OUTSIDE``` Allows placing player outside of building, useful for outdoor start.
- ```BOARDED``` Start in boarded building (windows and doors are boarded, movable furniture is moved to windows and doors).


## Skills

### Tags

- ```combat_skill``` The skill is considered a combat skill.  It's affected by `PACIFIST`, `PRED1`, `PRED2`, `PRED3`, and `PRED4` traits.
- ```contextual_skill``` The skill is abstract, it depends on context (an indirect item to which it's applied).  Neither player nor NPCs can possess it.


## Technical flags

These are added programatically when the game is running, not by JSON.  These are set to specific items (a single thingamabob, not *all* thingamabob) by the engine, depending on in-game action or environmental context.

- ```COLD``` Item is cold.  See also `EATEN_COLD`.
- ```DIRTY``` Item (liquid) was dropped on the ground and is now irreparably dirty.
- ```FIELD_DRESS_FAILED``` Corpse was damaged by unskillful field dressing.  Affects butcher results.
- ```FIELD_DRESS``` Corpse was field dressed.  Affects butcher results.
- ```FIT``` Reduces encumbrance by one.
- ```FROZEN``` Item is frozen solid (used by freezer).
- ```HIDDEN_ITEM``` This item cannot be seen in AIM.
- ```HOT``` Item is hot.  See also `EATEN_HOT`.
- ```IRRADIATED``` Item has been irradiated and will spoil at a much reduced rate.
- ```LITCIG``` Marks a lit smoking item (cigarette, joint etc.).
- ```MUSHY``` `FREEZERBURN` item was frozen and is now mushy and tasteless and will go bad after freezing again.
- ```NO_PARASITES``` Invalidates parasites count set in food -> type -> comestible -> parasites
- ```QUARTERED``` Corpse was quartered into parts.  Affects butcher results, weight, volume.
- ```REVIVE_SPECIAL``` Corpses revives when the player is nearby.
- ```WARM``` A hidden flag used to track an item's journey to/from hot, buffers between `HOT` and `COLD`.
- ```WET``` Item is wet and will slowly dry off (e.g. towel).


## Techniques

Techniques may be used by tools, armors, weapons and anything else that can be wielded.

- See contents of [techniques.json](../data/json/techniques.json).
- Techniques are also used with martial arts styles, see [martialarts.json](../data/json/martialarts.json).


## Tools

[Melee flags](#melee) are fully compatible with tool flags, and vice versa.

- ```ACT_ON_RANGED_HIT```  The item should activate when thrown or fired, then immediately get processed if it spawns on the ground.
- ```ALLOWS_REMOTE_USE``` This item can be activated or reloaded from adjacent tile without picking it up.
- ```BELT_CLIP``` The item can be clipped or hooked on to a belt loop of the appropriate size (belt loops are limited by their max_volume and max_weight properties).
- ```BOMB``` It can be a remote controlled bomb.
- ```CABLE_SPOOL``` This item is a spool of cable and must be processed as such.  It should usually have a "link_up" iuse_action, which it has special behavior for.
- ```CANNIBALISM``` The item is a food that contains human flesh, and applies all applicable effects when consumed.
- ```CHARGEDIM``` If illuminated, light intensity fades with charge, starting at 20% charge left.
- ```DIG_TOOL``` If wielded, digs thorough terrain like rock and walls, as player walks into them.  If item also has `POWERED` flag, then it digs faster, but uses up the item's ammo as if activating it.
- ```FIRESTARTER``` Item will start fire with some difficulty.
- ```FIRE``` Item will start a fire immediately.
- ```HAS_RECIPE``` Used by the E-Ink tablet to indicate it's currently showing a recipe.
- ```IS_UPS``` Item is Unified Power Supply.  Used in active item processing.
- ```LIGHT_[X]``` Illuminates the area with light intensity `[X]` where `[X]` is an intensity value (e.g. `LIGHT_4` or `LIGHT_100`).  Note: this flags sets `itype::light_emission` field and then is removed (can't be found using `has_flag`).
- ```MAGICAL``` Causes magical effects or functions based on arcane principles. Currently used by `iuse::robotcontrol` to determine if the hacking device is a computer (and thus has a screen that must be read etc).
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
- ```USES_BIONIC_POWER``` Allows item to use energy from player bionic power to satisfy its `energy_drain`.  Tools can also consume bionic power instead of battery ammo.
- ```USE_PLAYER_ENERGY``` Item with `use_action` that `cast_spell` consumes the specified `base_energy_cost`.
- ```USE_UPS``` Allows item to use energy from UPS to satisfy its `energy_drain`.  Tools can also consume UPS instead of battery ammo.
- ```WATER_EXTINGUISH``` Is extinguishable in water or under precipitation.  Converts items (requires `reverts_to` or `use_action` `transform` to be set).
- ```WET``` Item is wet and will slowly dry off (e.g. towel).
- ```WIND_EXTINGUISH``` This item will be extinguished by the wind.
- ```WRITE_MESSAGE``` This item could be used to write messages on signs.


### `use_action`

These flags apply to the `use_action` field, instead of the `flags` field.

- ```ACIDBOMB_ACT``` Get rid of it, or you'll end up like that guy in Robocop.
- ```ACIDBOMB``` Pull the pin on an acid bomb.
- ```AUTOCLAVE``` Sterilize one CBM by autoclaving it.
- ```BELL``` Ring the bell.
- ```BOLTCUTTERS``` Use your town key to gain access anywhere.
- ```BREAK_STICK``` Breaks long branch into two.
- ```C4``` Arm the C4.
- ```CAN_GOO``` Release a little blob buddy.
- ```CAPTURE_MONSTER_ACT``` Capture and encapsulate a monster. The associated action is also used for releasing it.
- ```CROWBAR``` Pry open doors, windows, man-hole covers and many other things that need prying.
- ```DIG``` Clear rubble.
- ```DIRECTIONAL_ANTENNA``` Find the source of a signal with your radio.
- ```DIVE_TANK``` Use compressed air tank to breathe.
- ```DOG_WHISTLE``` Dogs hate this thing; your dog seems pretty cool with it though.
- ```DOLLCHAT``` That creepy doll just keeps on talking.
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
- ```MININUKE``` Set the timer and run.  Or hit with a hammer (not really).
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
- ```PICK_LOCK``` Pick a lock on a door.  Speed and success chance are determined by skill, `LOCKPICK` item quality and `PERFECT_LOCKPICK` item flag.
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
- ```SEED``` Asks if you are sure that you want to eat the seed.  As it is better to plant seeds.
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
- ```TURRET``` Activate a turret.
- ```WASH_ALL_ITEMS``` Wash items with `FILTHY` flag.
- ```WASH_HARD_ITEMS``` Wash hard items with `FILTHY` flag.
- ```WASH_SOFT_ITEMS``` Wash soft items with `FILTHY` flag.
- ```WATER_PURIFIER``` Purify water.


## Traps

- ```AVATAR_ONLY``` Only the player character will trigger this trap.
- ```CONVECTS_TEMPERATURE``` This trap convects temperature, like lava.
- ```PIT``` This trap is a version of the pit terrain.
- ```SONAR_DETECTABLE``` This trap can be identified with ground-penetrating `SONAR`.
- ```UNCONSUMED``` If this trap is a spell type it will not be removed after activation.
- ```UNDODGEABLE``` This trap can't be dodged.


## Vehicles

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


### Parts

Note: Vehicle parts requiring other parts is defined by setting a `requires_flag` field with the flag requirement (i.e. the `ON_ROOF` flag contains the field `"requires_flag": "ROOF"`).

- ```ADVANCED_PLANTER``` This planter doesn't spill seeds and avoids damaging itself on non-diggable surfaces.
- ```AIRCRAFT_REPAIRABLE_NOPROF``` Allows the player to safely remove part from an aircraft without any proficiency.
- ```AISLE_LIGHT``` This part lightens up surroundings.
- ```AISLE``` Player can move over this part with less speed penalty than normal.
- ```ALTERNATOR``` Recharges batteries installed on the vehicle.  Can only be installed on a part with `E_ALTERNATOR` flag.
- ```ANCHOR_POINT``` Allows secure seatbelt attachment.
- ```ANIMAL_CTRL``` Can harness an animal, need `HARNESS_bodytype` flag to specify bodytype of animal.
- ```APPLIANCE``` This vehicle part is an appliance, and treated accordingly
- ```ARCADE``` Allow player to play games when vehicle part is active
- ```ARMOR``` Protects the other vehicle parts it's installed over during collisions.
- ```ATOMIC_LIGHT``` This part lightens up surroundings.
- ```AUTOPILOT``` This part will enable a vehicle to have a simple autopilot.
- ```BATTERY_MOUNT``` This part allows mounting batteries for quick change.
- ```BED``` A bed where the player can sleep.
- ```BEEPER``` Generates noise when the vehicle moves backward.
- ```BELTABLE``` Seatbelt can be attached to this part.
- ```BIKE_RACK_VEH``` Can be used to merge an adjacent single tile wide vehicle, or split a single tile wide vehicle off into its own vehicle.
- ```BOARDABLE``` The player can safely move over or stand on this part while the vehicle is moving.
- ```CAMERA_CONTROL```This part allows for using the camera system installed on a vehicle.
- ```CAMERA``` Vehicle part which allows looking through the installed camera system.
- ```CAPTURE_MOSNTER_VEH``` Can be used to capture monsters when mounted on a vehicle.
- ```CARGO_LOCKING``` This cargo area is inaccessible to NPCs.  Can only be installed on a part with `LOCKABLE_CARGO` flag.
- ```CARGO``` Cargo holding area.
- ```CHIMES``` Generates continuous noise when used.
- ```CIRCLE_LIGHT``` Projects a circular radius of light when turned on.
- ```CONE_LIGHT``` Projects a cone of light when turned on.
- ```CONTROLS``` Can be used to control the vehicle.
- ```CONTROL_ANIMAL``` These controls can only be used to control a vehicle pulled by an animal (e.g., reins and other tack).
- ```COOLER``` There is a separate command to toggle this part.
- ```COVERED``` Prevents items in cargo parts from emitting any light.
- ```CTRL_ELECTRONIC``` Controls electrical and electronic systems of the vehicle.
- ```CURTAIN``` Can be installed over a part flagged with `WINDOW`, and functions the same as blinds found on windows in buildings.
- ```DISHWASHER``` Can be used to wash filthy non-soft items en masse.
- ```DOME_LIGHT``` This part lightens up surroundings.
- ```DOOR_LOCKING``` This is a lock that can be installed on a door. Can only be installed on a part with `LOCKABLE_DOOR` flag.
- ```DOOR_MOTOR``` Can only be installed on a part with `OPENABLE` flag.
- ```ENABLED_DRAINS_EPOWER``` Make vehicle part to require some energy to start it's work.  Requires `epower` field.
- ```ENGINE``` Is an engine and contributes towards vehicle mechanical power.
- ```EVENTURN``` Only on during even turns.
- ```EXTRA_DRAG``` Tells the vehicle that the part exerts engine power reduction.
- ```E_ALTERNATOR``` Is an engine that can power an alternator.
- ```E_COLD_START``` Is an engine that starts much slower in cold weather.
- ```E_COMBUSTION``` Is an engine that burns its fuel and can backfire or explode when damaged.
- ```E_DIESEL_FUEL``` This vehicle part can burn diesel or JP8 (also biodiesel or kerosene, albeit less effective) from tank.
- ```E_HEATER``` Is an engine and has a heater to warm internal vehicle items when on.
- ```E_HIGHER_SKILL``` Is an engine that is more difficult to install as more engines are installed.
- ```E_STARTS_INSTANTLY``` Is an engine that starts instantly, like food pedals.
- ```FLAT_SURF``` Part with a flat hard surface (e.g. table).
- ```FLUIDTANK``` Allow to store liquid in this part.  Amount of liquid should be defined in item for this vehicle part.
- ```FREEZER``` Can freeze items in below zero degrees Celsius temperature.
- ```FRIDGE``` Can refrigerate items.
- ```FUNNEL``` If installed over a vehicle tank, can collect rainwater during rains.
- ```HALF_CIRCLE_LIGHT``` Projects a directed half-circular radius of light when turned on.
- ```HANDHELD_BATTERY_MOUNT``` Same as `BATTERY_MOUNT`, but for handheld battery mount.
- ```HARNESS_bodytype``` Replace bodytype with `any` to accept any type, or with the targeted type.
- ```HORN``` Generates noise when used.
- ```INITIAL_PART``` When starting a new vehicle via the construction menu, this vehicle part will be the initial part of the vehicle (if the used item matches the item required for this part).  The items of parts with this flag are automatically added as component to the vehicle start construction.
- ```INTERNAL``` Can only be installed on a part with `CARGO` flag.
- ```LOCKABLE_CARGO``` Cargo containers that are able to have a lock installed.
- ```LOCKABLE_DOOR``` Doors that are able to have a lock installed. (See `DOOR_LOCKING`)
- ```MUFFLER``` Muffles the noise a vehicle makes while running.
- ```MULTISQUARE``` Causes this part and any adjacent parts with the same ID to act as a singular part.
- ```MUSCLE_ARMS``` Power of the engine with such flag depends on player's strength (it's less effective than `MUSCLE_LEGS`).
- ```MUSCLE_LEGS``` Power of the engine with such flag depends on player's strength.
- ```NAILABLE``` Attached with nails.
- ```NEEDS_BATTERY_MOUNT``` Part with this flag needs to be installed over part with `BATTERY_MOUNT` flag.
- ```NEEDS_HANDHELD_BATTERY_MOUNT``` Same as `NEEDS_BATTERY_MOUNT`, but for handheld battery mount.
- ```NEEDS_WHEEL_MOUNT_HEAVY``` Can only be installed on a part with `WHEEL_MOUNT_HEAVY` flag.
- ```NEEDS_WHEEL_MOUNT_LIGHT``` Can only be installed on a part with `WHEEL_MOUNT_LIGHT` flag.
- ```NEEDS_WHEEL_MOUNT_MEDIUM``` Can only be installed on a part with `WHEEL_MOUNT_MEDIUM` flag.
- ```NEEDS_WINDOW``` Can only be installed on a part with `WINDOW` flag.
- ```NO_INSTALL_HIDDEN``` Part can't be installed by player and hidden in install menu (e.g. power cords, inflatable boat parts, summoned vehicle parts).
- ```NO_INSTALL_PLAYER``` Part can't be installed by player but visible in install menu (e.g. helicopter rotors).
- ```NO_LEAK``` Causes a boat hull to float even when damaged.
- ```NO_MODIFY_VEHICLE``` Installing a part with this flag on a vehicle will mean that it can no longer be modified.  Parts with this flag should not be installable by players.
- ```NO_REPAIR``` Cannot be repaired.
- ```NO_UNINSTALL``` Cannot be uninstalled.
- ```OBSTACLE``` Cannot walk through part, unless the part is also `OPENABLE`.
- ```ODDTURN``` Only on during odd turns.
- ```ON_CONTROLS``` Can only be installed on a part with `CONTROLS` flag.
- ```ON_ROOF``` Parts with this flag could only be installed on a roof (parts with `ROOF` flag).
- ```OPAQUE``` Cannot be seen through.
- ```OPENABLE``` Can be opened or closed.
- ```OPENCLOSE_INSIDE```  Can be opened or closed, but only from inside the vehicle.
- ```OVER``` Can be mounted over other parts.
- ```PERPETUAL``` If paired with REACTOR, part produces electrical power without consuming fuel.
- ```PLANTER``` Plants seeds into tilled dirt, spilling them when the terrain underneath is unsuitable.  It is damaged by running it over non-`DIGGABLE` surfaces.
- ```PLOW``` Tills the soil underneath the part while active.  Takes damage from unsuitable terrain at a level proportional to the speed of the vehicle.
- ```POWER_TRANSFER``` Transmits power to and from an attached thingy (probably a vehicle).
- ```PROTRUSION``` Part sticks out so no other parts can be installed over it.
- ```REACTOR``` When enabled, part consumes fuel to generate epower.
- ```REAPER``` Cuts down mature crops, depositing them on the square.
- ```RECHARGE``` Recharge items with the same flag (currently only the rechargeable battery mod).
- ```REMOTE_CONTROLS``` Once installed, allows using vehicle through remote controls.
- ```REVERSIBLE``` Removal has identical requirements to installation but is twice as quick.
- ```ROOF``` Covers a section of the vehicle.  Areas of the vehicle that have a roof and roofs on surrounding sections, are considered inside.  Otherwise they're outside.
- ```SCOOP``` Pulls items from underneath the vehicle to the cargo space of the part.  Also mops up liquids.
- ```SEATBELT``` Helps prevent the player from being ejected from the vehicle during an accident.  Can only be installed on a part with `BELTABLE` flag.
- ```SEAT``` A seat where the player can sit or sleep.
- ```SECURITY``` If installed, will emit a loud noise when the vehicle is smashed.
- ```SHARP``` Striking a monster with this part does cutting damage instead of bashing damage, and prevents stunning the monster.
- ```SHOCK_ABSORBER``` This part protects non-frame parts on the same tile from shock damage from collisions.  It doesn't provide protect against direct impacts or other attacks.
- ```SIMPLE_PART``` This part can be installed or removed from that otherwise prevent modification.
- ```SMASH_REMOVE``` When you remove this part, instead of getting the item back, you will get the bash results.
- ```SOLAR_PANEL``` Recharges vehicle batteries when exposed to sunlight.  Has a 1/4 chance of being broken on car generation.
- ```SPACE_HEATER``` There is separate command to toggle this part.
- ```STABLE``` Similar to `WHEEL`, but if the vehicle is only a 1x1 section, this single wheel counts as enough wheels.
- ```STEERABLE``` This wheel is steerable.
- ```STEREO``` Allows playing music for increasing the morale.
- ```TRACKED``` Contributes to steering effectiveness but doesn't count as a steering axle for install difficulty and still contributes to drag for the center of steering calculation.
- ```TRACK``` Allows the vehicle installed on to be marked and tracked on map.
- ```TRANSFORM_TERRAIN``` Transform terrain (using rules defined in `transform_terrain`).
- ```TURRET_CONTROLS``` If part with this flag is installed over the turret, it allows to set said turret's targeting mode to full auto.  Can only be installed on a part with `TURRET` flag.
- ```TURRET_MOUNT``` Parts with this flag are suitable for installing turrets.
- ```TURRET``` Is a weapon turret. Can only be installed on a part with `TURRET_MOUNT` flag.
- ```UNMOUNT_ON_DAMAGE``` Part breaks off the vehicle when destroyed by damage.  Item is new and typically undamaged.
- ```UNMOUNT_ON_MOVE``` Dismount this part when the vehicle moves.  Doesn't drop the part, unless you give it special handling.
- ```UNSTABLE_WHEEL``` This will not provide for the wheeling needs of your vehicle if installed alone.  Opposite of `STABLE`.
- ```VARIABLE_SIZE``` Has 'bigness' for power, wheel radius, etc.
- ```VISION``` Gives vision of otherwise unseen directions (e.g. mirrors).
- ```WALL_MOUNTED``` This vehicle part is mounted on wall, and can't be moved by itself.
- ```WASHING_MACHINE``` Can be used to wash filthy clothes en masse.
- ```WATER_WHEEL``` Recharges vehicle batteries when submerged in moving water.
- ```WHEEL``` Counts as a wheel in wheel calculations.
- ```WIDE_CONE_LIGHT``` Projects a wide cone of light when turned on.
- ```WINDOW``` Can see through this part and can install curtains over it.
- ```WIND_POWERED``` This engine is powered by wind (e.g. sails).
- ```WIND_TURBINE``` Recharges vehicle batteries when exposed to wind.
- ```WIRING``` TBD, seems related to `check_no_wiring`.
- ```WORKBENCH``` Can craft at this part, must be paired with a workbench JSON entry.


### Vehicle faults

- ```BAD_COLD_START``` The engine starts as if the temperature was 20 F colder.  Does not stack with multiples of itself.
- ```BAD_FUEL_PUMP``` Prevents engine from starting and makes it stutter.
- ```BAD_STARTER``` Prevents engine from starting and makes click noise.
- ```DOUBLE_FUEL_CONSUMPTION``` Doubles fuel consumption of the engine.  Does not stack with multiples of itself.
- ```ENG_BACKFIRE``` Causes the engine to backfire as if it had zero hp.
- ```EXTRA_EXHAUST``` Makes the engine emit more exhaust smoke.  Does not stack with multiples of itself.
- ```IMMOBILIZER``` Prevents engine from starting and makes it beep.
- ```NO_ALTERNATOR_CHARGE``` The alternator connected to this engine does not work.
- ```REDUCE_ENG_POWER``` Multiplies engine power by 0.6.  Does not stack with multiples of itself.

