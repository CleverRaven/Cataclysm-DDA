# Soundpacks

A soundpack can be installed in the `data/sound` directory. It has to be a subdirectory that contains at least a file named `soundpack.txt`. It can include any number of json files which add any number of sound_effect or playlist.

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
        "id" : "menu_move",
        "volume" : 100,
        "files" : [
            "nenadsimic_menu_selection_click.wav"
        ]
    },
    {
        "type": "sound_effect",
        "id" : "fire_gun",
        "volume" : 90,
        "variant" : "bio_laser_gun",
        "files" : [
            "guns/energy_generic/weapon_fire_laser.ogg"
        ]
    }
]
```
Adding variety: If for a certain `id`'s `variant` multiple `files` are defined, they will be chosen at random when `variant` is played.

The volume key may range from 0-100.

Cataclysm has its own set of user-controllable volumes that will additionally affect the sound.  These range from 0-128, and the default is 100.  This means that at default volume, any sound that Cataclysm plays will default to playing at about 78% of the maximum; if you are working on sounds in an external audio editor, expect Cataclysm at default volume settings to play that sound file back more-quietly than your editor does.

### Preloading SFX

Sound effects can be included for preloading with a format like this:

```javascript
[
    {
        "type": "sound_effect_preload",
        "preload": [
            { "id": "environment", "variant": "daytime" },
            { "id": "environment" }
        ]
    }
]
```

### Playlist

A playlist can be included with a format like this:

```javascript
[
    {
        "type": "playlist",
        "playlists":
        [
            {
                "id" : "title",
                "shuffle" : false,
                "files" : [
                    {
                        "file": "Dark_Days_Ahead_demo_2.wav",
                        "volume": 100
                    },
                    {
                        "file": "cataclysmthemeREV6.wav",
                        "volume": 90
                    }
                ]
            }
        ]
    }
]
```

Each sound effect is identified by an id and a variant. If a sound effect is played with a variant that does not exist in the json files, but a variant "default" exists, then the "default" variant is played instead. The file name of the sound effect is relative to the soundpack directory, so if the file name is set to "sfx.wav" and your soundpack is in `data/sound/mypack`, the file must be placed at `data/sound/mypack/sfx.wav`.

## JSON Format Sound Effects List

A full list of sound effect id's and variants is given in the following. Each line in the list has the following format:

`id variant1|variant2`

Where id describes the id of the sound effect, and a list of variants separated by | follows. When the variants are omitted, the variant "default" is assumed. Where the variants do not represent literal strings, but variables, they will be enclosed in `<` `>`. For instance, `<furniture>` is a placeholder for any valid furniture ID (as in the furniture definition JSON).

    # open/close doors
* `open_door default|<furniture>|<terrain>`
* `close_door default|<furniture>|<terrain>`

    # smashing attempts and results, few special ones and furniture/terrain specific
* `bash default`
* `smash wall|door|door_boarded|glass|swing|web|paper_torn|metal`
* `smash_success hit_vehicle|smash_glass_contents|smash_cloth|<furniture>|<terrain>`
* `smash_fail default|<furniture>|<terrain>`

    # melee sounds
* `melee_swing default|small_bash|small_cutting|small_stabbing|big_bash|big_cutting|big_stabbing`
* `melee_hit_flesh default|small_bash|small_cutting|small_stabbing|big_bash|big_cutting|big_stabbing|<weapon>`
* `melee_hit_metal default|small_bash|small_cutting|small_stabbing|big_bash|big_cutting|big_stabbing!<weapon>`
* `melee_hit <weapon>` # note: use weapon id "null" for unarmed attacks

    # firearm/ranged weapon sounds
* `fire_gun <weapon>|brass_eject|empty`
* `fire_gun_distant <weapon>`
* `reload <weapon>`
* `bullet_hit hit_flesh|hit_wall|hit_metal|hit_glass|hit_water`

    # enviromental sfx, here divided by sections for clarity
* `environment thunder_near|thunder_far`
* `environment daytime|nighttime`
* `environment indoors|indoors_rain|underground`
* `environment <weather_type>` # examples: `WEATHER_DRIZZLE|WEATHER_RAINY|WEATHER_THUNDER|WEATHER_FLURRIES|WEATHER_SNOW|WEATHER_SNOWSTORM`
* `environment alarm|church_bells|police_siren`
* `environment deafness_shock|deafness_tone_start|deafness_tone_light|deafness_tone_medium|deafness_tone_heavy`

    # misc environmental sounds
* `footstep default|light|clumsy|bionic`
* `explosion default|small|huge`

    # ambient danger theme for seeing large numbers of zombies
* `danger_low`
* `danger_medium`
* `danger_high`
* `danger_extreme`

    # chainsaw pack
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

    # monster death and bite attacks
* `mon_death zombie_death|zombie_gibbed`
* `mon_bite bite_miss|bite_hit`

* `melee_attack monster_melee_hit`

* `player_laugh laugh_f|laugh_m`

    # player movement sfx
    important: `plmove <terrain>` has priority over default `plmove|walk_<what>` (excluding `|barefoot`)
    example: if `plmove|t_grass_long` is defined it will be played before default `plmove|walk_grass` default for all grassy terrains

* `plmove <terrain>|<vehicle_part>`
* `plmove walk_grass|walk_dirt|walk_metal|walk_water|walk_tarmac|walk_barefoot|clear_obstacle`

    # fatigue
* `plmove fatigue_m_low|fatigue_m_med|fatigue_m_high|fatigue_f_low|fatigue_f_med|fatigue_f_high`

    # player hurt sounds
* `deal_damage hurt_f|hurt_m`

    # player death and end-game sounds
* `clean_up_at_end game_over|death_m|death_f`

    # variuos bionic sounds
* `bionic elec_discharge|elec_crackle_low|elec_crackle_med|elec_crackle_high|elec_blast|elec_blast_muffled|acid_discharge|pixelated`
* `bionic bio_resonator|bio_hydraulics|`

    # various tools/traps being used (including some associated terrain/furniture)
* `tool alarm_clock|jackhammer|pickaxe|oxytorch|hacksaw|axe|shovel|crowbar|boltcutters|compactor|gaspump|noise_emitter|repair_kit|camera_shutter|handcuffs`
* `tool geiger_low|geiger_medium|geiger_high`
* `trap bubble_wrap|bear_trap|snare|teleport|dissector|glass_caltrop|glass`

    # various activities
* `activity burrow`

    # musical instruments, `_bad` is used when you fail to play it well
* `musical_instrument <instrument>`
* `musical_instrument_bad <instrument>`

    # various shouts and screams
* `shout default|scream|scream_tortured|roar|squeak|shriek|wail|howl`

    # speach, it is currently linked with either item or monster id, or is special `NPC` or `NPC_loud`
    # TODO: full vocalization of speech.json
* `speech <item_id>` # examples: talking_doll, creepy_doll, Granade, 
* `speech <monster_id>` # examples: eyebot, minitank, mi-go, many robots
* `speech NPC_m|NPC_f|NPC_m_loud|NPC_f_loud` # special for NPCs
* `speech robot` # special for robotic voice from a machine etc.

    # radio chatter
* `radio static|inaudible_chatter`

    # humming sounds of various origin
* `humming electric|machinery`

    # sounds related to (burning) fire
* `fire ignition`

    # vehicle sounds - engine and other parts in action
    # note: defaults are executed when specific option is not defined
* `engine_start <vehicle_part>` # note: specific engine start (id of any engine/motor/steam_engine/paddle/oar/sail/etc. )
* `engine_start combustion|electric|muscle|wind` # default engine starts groups
* `engine_stop <vehicle_part>` # note: specific engine stop (id of any engine/motor/steam_engine/paddle/oar/sail/etc. )
* `engine_stop combustion|electric|muscle|wind` # default engine stop groups

    # note: internal engine sound is dynamically pitch shifted depending on vehicle speed
    # it is an ambient looped sound with dedicated channel
* `engine_working_internal <vehicle_part>` # note: sound of engine working heard inside vehicle
* `engine_working_internal combustion|electric|muscle|wind` # default engine working (inside) groups

    # note: external engine sound volume and pan is dynamically shifted depending on distance and angle to vehicle
    # volume heard at given distance is linked to engine's `noise_factor` and stress to the engine (see `vehicle::noise_and_smoke()` )
    # it is an ambient looped sound with dedicated channel
    # this is a single-channel solution (TODO: multi-channel for every heard vehicle); it picks loudest heard vehicle
    # there is no pitch shift here (may be introduced when need for it emerges)
* `engine_working_external <vehicle_part>` # note: sound of engine working heard outside vehicle
* `engine_working_external combustion|electric|muscle|wind` # default engine working (outside) groups

    # note: gear_up/gear_down is done automatically by pitch manipulation
    # gear shift is dependant on max safe speed, and works in assumption, that there are
    # 6 forward gears, gear 0 = neutral, and gear -1 = reverse
* `vehicle gear_shift`


* `vehicle engine_backfire|engine_bangs_start|fault_immobiliser_beep|engine_single_click_fail|engine_multi_click_fail|engine_stutter_fail|engine_clanking_fail`
* `vehicle horn_loud|horn_medium|horn_low|rear_beeper|chimes|car_alarm`
* `vehicle reaper|scoop|scoop_thump`

* `vehicle_open <vehicle_part>` # note: id of: doors, trunks, hatches, etc.
* `vehicle_close <vehicle part>`

    # miscellaneous sounds
* `misc flashbang|flash|shockwave|earthquake|stairs_movement|stones_grinding|bomb_ticking|lit_fuse|cow_bell|bell|timber`
* `misc rc_car_hits_obstacle|rc_car_drives`
* `misc default|whistle|airhorn|horn_bicycle|servomotor`
* `misc beep|ding|`
* `misc rattling|spitting|coughing|heartbeat|puff|inhale|exhale|insect_wings|snake_hiss` # mostly organic noises
