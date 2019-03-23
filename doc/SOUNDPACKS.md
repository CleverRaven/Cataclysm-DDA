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
                        "volume": 128
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

A full list of sound effect id's and variants is given in the following. Each line in the list has the following format:

`id variant1|variant2`

Where id describes the id of the sound effect, and a list of variants separated by | follows. When the variants are omitted, the variant "default" is assumed. Where the variants do not represent literal strings, but variables, they will be enclosed in `<` `>`. For instance, `<furniture>` is a placeholder for any valid furniture ID (as in the furniture definition JSON).

* `footstep default|light|clumsy|bionic`
* `explosion default|small|huge`
* `open_door <furniture>|<terrain>`
* `close_door <furniture>|<terrain>`
* `bash default`
* `smash wall|door|door_boarded|glass`
* `melee_hit <weapon> # note: use weapon id "null" for unarmed attacks`
* `fire_gun <weapon>`
* `reload <weapon>`
* `environment thunder_near|thunder_far|daytime|nighttime|indoors|indoors_rain|underground|WEATHER_DRIZZLE|WEATHER_RAINY|WEATHER_THUNDER|WEATHER_FLURRIES|WEATHER_SNOW|WEATHER_SNOWSTORM`
