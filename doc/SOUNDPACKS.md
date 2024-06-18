# Soundpacks

A soundpack can be installed in the `data/sound` directory. It has to be a subdirectory that contains at least a file named `soundpack.txt`. It can include any number of json files which add any number of `sound_effect` or playlists.

## soundpack.txt format

The `soundpack.txt` file format needs 2 values, a NAME and a VIEW. The NAME value should be unique. The VIEW value will be shown in the options menu. Every line starting with a `#` is a comment line.

```
#Basic provided soundpack
#Name of the soundpack
NAME: basic
#Viewing name of the soundpack
VIEW: Basic
```

## JSON format

### Sound effects

Sound effects can be included with a format like this:

```javascript
[
  {
    "type": "sound_effect",
    "id": "menu_move",
    "volume": 100,
    "files": [ "nenadsimic_menu_selection_click.wav" ]
  },
  {
    "type": "sound_effect",
    "id": "fire_gun",
    "volume": 90,
    "variant": "bio_laser_gun",
    "season": "summer",
    "files": [ "guns/energy_generic/weapon_fire_laser.ogg" ]
  },
  {
    "type": "sound_effect",
    "id": "open_door",
    "volume": 75,
    "variant": "t_door_c",
    "season": "summer",
    "is_indoors": true,
    "is_night": false,
    "files": [ "indoors/open_door_daytime.ogg" ]
  }
]
```

Each sound effect is identified by an id, a variant, and a season. If a sound effect is played with a variant that does not exist in the json files, but a variant "default" exists, then the "default" variant is played instead. The file name of the sound effect is relative to the soundpack directory, so if the file name is set to "sfx.wav" and your soundpack is in `data/sound/mypack`, the file must be placed at `data/sound/mypack/sfx.wav`.

##### Adding variety

Several optional fields can be defined to target specific situations:

| Field        | Purpose
|---           |---
| `variant`    | Defines a specific subset of the id's sound group. (ex: `"id": "environment", "variant": "WEATHER_DRIZZLE"`)
| `season`     | If defined, the sound will only play on the specified season. (possible values are `spring`, `summer`, `autumn`, and `winter`).
| `is_indoors` | If defined, the sound will only play if the player is indoors/outdoors when true/false.
| `is_night`   | If defined, the sound will only play if the current time is night/day when true/false.

If multiple `files` are defined for the sound effect, one will be chosen at random when the sound is played.

The volume key may range from 0-100.

Cataclysm has its own set of user-controllable volumes that will additionally affect the sound. These range from 0-128, and the default is 100. This means that at default volume, any sound that Cataclysm plays will default to playing at about 78% of the maximum; if you are working on sounds in an external audio editor, expect Cataclysm at default volume settings to play that sound file back quieter than your editor does.

### Preloading SFX

Sound effects can be included for preloading with a format like this:

```javascript
[
  {
    "type": "sound_effect_preload",
    "preload": [
      { "id": "environment", "variant": "thunder_near", "season": "spring", "is_night": false },
      { "id": "environment" }
    ]
  }
]
```

### Playlists

A playlist can be included with a format like this:

```javascript
[
  {
    "type": "playlist",
    "playlists": [
      {
        "id": "title",
        "shuffle": false,
        "files": [ { "file": "Dark_Days_Ahead_demo_2.wav", "volume": 100 }, { "file": "cataclysmthemeREV6.wav", "volume": 90 } ]
      }
    ]
  },
  {
    "type": "playlist",
    "playlists": [
      {
        "id": "mp3",
        "shuffle": false,
        "files": [ { "file": "Dark_Days_Ahead_demo_2.wav", "volume": 100 }, { "file": "cataclysmthemeREV6.wav", "volume": 90 } ]
      }
    ]
  }
]
```

Playlists are similar to sound effects in that they are each identified by an id. However, their similarities end there. Playlists cannot overlap each other unlike sound effects, they do not have optional fields such as variant and season, and the exact formatting for the files field is slightly different.

Playlists are governed mainly by their id, which dictate in which situation a given playlist will be activated. Different situations trigger different ids, and if more than one ids are active at the same time, then the conflict is resolved by a priority model. Basically, each music id are given a hard-coded priority, where ids with higher priority takes precedent.

## Playlists List

A full list and priority of playlist ids is given in the following.

* `mp3`
This id is assigned with the highest priority value. Playlists that has this id will be activated when the player hears music with no sound, usually via a mp3 player or its variant.

* `instrument`
This id is assigned with the second highest priority value. Playlists that has this id will be activated when the player plays any musical instrument.

* `sound`
This id is assigned with the third highest priority value. Playlists that has this id will be activated when the player hears music with some sound, usually via a stereo or an i-pad.

* `title`
This id is assigned with the lowest priority value. It will be played only when all other playlists are inactive.

## JSON Format Sound Effects List

A full list of sound effect IDs and variants is given in the following. Each line in the list has the following format:

`id variant1|variant2`

Where `id` describes the ID of the sound effect, and a list of variants separated by | follows. When the variants are omitted, the variant "default" is assumed. Where the variants do not represent literal strings, but variables, they will be enclosed in `<` `>`. For instance, `<furniture>` is a placeholder for any valid furniture ID (as in the furniture definition JSON).

### Open/close doors

* `open_door default|<furniture>|<terrain>`
* `close_door default|<furniture>|<terrain>`

### Smashing attempts and results

Includes special ones that are furniture/terrain specific.

* `bash default`
* `smash wall|door|door_boarded|glass|swing|web|paper_torn|metal`
* `smash_success hit_vehicle|smash_glass_contents|smash_cloth|<furniture>|<terrain>`
* `smash_fail default|<furniture>|<terrain>`

### Melee

* `melee_swing default|small_bash|small_cutting|small_stabbing|big_bash|big_cutting|big_stabbing`
* `melee_hit_flesh default|small_bash|small_cutting|small_stabbing|big_bash|big_cutting|big_stabbing|<weapon>`
* `melee_hit_metal default|small_bash|small_cutting|small_stabbing|big_bash|big_cutting|big_stabbing!<weapon>`
* `melee_hit <weapon>` # note: use weapon id "null" for unarmed attacks

### Firearm/ranged weapon

* `fire_gun <weapon>|brass_eject|empty`
* `fire_gun_distant <weapon>`
* `reload <weapon>`
* `bullet_hit hit_flesh|hit_wall|hit_metal|hit_glass|hit_water`

### Environmental SFX

Divided by sections for clarity.

* `environment thunder_near|thunder_far`
* `environment daytime|nighttime`
* `environment indoors|indoors_rain|underground`
* `environment <weather_type>` # examples: `WEATHER_DRIZZLE|WEATHER_RAINY|WEATHER_RAINSTORM|WEATHER_THUNDER|WEATHER_FLURRIES|WEATHER_SNOW|WEATHER_SNOWSTORM|WEATHER_CLEAR|WEATHER_SUNNY|WEATHER_CLOUDY|WEATHER_PORTAL_STORM`
* `environment alarm|church_bells|police_siren`
* `environment deafness_shock|deafness_tone_start|deafness_tone_light|deafness_tone_medium|deafness_tone_heavy`

### Misc environmental sounds

* `footstep default|light|clumsy|bionic`
* `explosion default|small|huge`

### Ambient danger theme

Triggered by seeing large numbers of zombies.

* `danger_low` when seeing 5-9 enemies.
* `danger_medium` when seeing 10-14 enemies.
* `danger_high` when seeing 15-19 enemies.
* `danger_extreme` when seeing ≥ 20 enemies.

### Chainsaw pack

* `chainsaw_cord     chainsaw_on`
* `chainsaw_start    chainsaw_on`
* `chainsaw_start    chainsaw_on`
* `chainsaw_stop     chainsaw_on`
* `chainsaw_idle     chainsaw_on`
* `melee_swing_start chainsaw_on`
* `melee_swing_end   chainsaw_on`
* `melee_swing       chainsaw_on`
* `melee_hit_flesh   chainsaw_on`
* `melee_hit_metal   chainsaw_on`
* `weapon_theme      chainsaw`

### Monster death and bite attacks

* `mon_death zombie_death|zombie_gibbed`
* `mon_bite bite_miss|bite_hit`
* `melee_attack monster_melee_hit`

### Player movement sfx

Important: `plmove <terrain>` has priority over default `plmove|walk_<what>` (excluding `|barefoot`).

Example: if `plmove|t_grass_long` is defined it will be played before default `plmove|walk_grass` default for all grassy terrains.

* `plmove <terrain>|<vehicle_part>`
* `plmove walk_grass|walk_dirt|walk_metal|walk_water|walk_tarmac|walk_barefoot|clear_obstacle`

### Fatigue

* `plmove sleepiness_m_low|sleepiness_m_med|sleepiness_m_high|sleepiness_f_low|sleepiness_f_med|sleepiness_f_high`

### Player hurt

* `deal_damage hurt_f|hurt_m`

### Player death and end-game

* `clean_up_at_end death_m|death_f`

### Various bionics

* `bionic elec_discharge|elec_crackle_low|elec_crackle_med|elec_crackle_high|elec_blast|elec_blast_muffled|acid_discharge|pixelated`
* `bionic bio_resonator|bio_hydraulics|`

### Various tools/traps being used

Includes some associated terrain/furniture.

* `tool alarm_clock|jackhammer|pickaxe|oxytorch|hacksaw|axe|shovel|crowbar|boltcutters|compactor|gaspump|noise_emitter|repair_kit|camera_shutter|handcuffs`
* `tool geiger_low|geiger_medium|geiger_high`
* `trap bubble_wrap|bear_trap|snare|teleport|dissector|glass_caltrop|glass`

### Various activities

* `activity burrow`

### Musical instruments

* `musical_instrument <instrument>`
* `musical_instrument_bad <instrument>`: used when you fail to play well

### Various shouts and screams

* `shout default|scream|scream_tortured|roar|squeak|shriek|wail|howl`

### Speech

This is currently linked with either item or monster id, with the exception of NPCs and robots.

TODO: full vocalization of speech.json

* `speech <item_id>` # examples: `talking_doll`, `creepy_doll`, `granade` (sic),
* `speech <monster_id>` # examples: eyebot, minitank, mi-go, many robots
* `speech NPC_m|NPC_f|NPC_m_loud|NPC_f_loud` # special for NPCs
* `speech robot` # special for robotic voice from a machine etc.

### Radio chatter

* `radio static|inaudible_chatter`

### Humming sounds of various origins

* `humming electric|machinery`

### Fire

* `fire ignition`

### Vehicle

Engines and other parts in action. The defaults are executed when a specific option is not defined.

* `engine_start <vehicle_part>`: specific engine start (ID of any `engine`/`motor`/`steam_engine`/`paddle`/`oar`/`sail`/etc.)
* `engine_start combustion|electric|muscle|wind`: default engine starts groups
* `engine_stop <vehicle_part>`: specific engine stop (ID of any `engine`/`motor`/`steam_engine`/`paddle`/`oar`/`sail`/etc.)
* `engine_stop combustion|electric|muscle|wind`: default engine stop groups

  Internal engine sound is dynamically pitch shifted depending on vehicle speed. It is an ambient looped sound with a dedicated channel.
* `engine_working_internal <vehicle_part>`: sound of engine working heard inside vehicle
* `engine_working_internal combustion|electric|muscle|wind`: default engine working (inside) groups

  External engine sound volume and pan is dynamically shifted depending on distance and angle to vehicle. Volume heard at given distance is linked to engine's `noise_factor` and stress to the engine (see `vehicle::noise_and_smoke()`). It is an ambient looped sound with its own dedicated channel. This is a single-channel solution that picks loudest heard vehicle (TODO: multi-channel for every heard vehicle). There is no pitch shift here (may be introduced when need for it emerges).
* `engine_working_external <vehicle_part>`: sound of engine working heard outside vehicle
* `engine_working_external combustion|electric|muscle|wind`: default engine working (outside) groups

  `gear_up`/`gear_down` is done automatically by pitch manipulation. Gear shift is dependent on max safe speed, and works on the assumption that there are 6 forward gears, gear 0 is neutral, and gear -1 is reverse.
* `vehicle gear_shift`
* `vehicle engine_backfire|engine_bangs_start|fault_immobiliser_beep|engine_single_click_fail|engine_multi_click_fail|engine_stutter_fail|engine_clanking_fail`
* `vehicle horn_loud|horn_medium|horn_low|rear_beeper|chimes|car_alarm`
* `vehicle reaper|scoop|scoop_thump`
* `vehicle_open <vehicle_part>` # id of: doors, trunks, hatches, etc.
* `vehicle_close <vehicle part>`

### Miscellaneous

* `misc flashbang|flash|shockwave|earthquake|stairs_movement|stones_grinding|bomb_ticking|lit_fuse|cow_bell|bell|timber`
* `misc rc_car_hits_obstacle|rc_car_drives`
* `misc default|whistle|airhorn|horn_bicycle|servomotor`
* `misc beep|ding|`
* `misc rattling|spitting|coughing|heartbeat|puff|inhale|exhale|insect_wings|snake_hiss` # mostly organic noises
