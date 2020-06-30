# `movement_mode`

## definition

```JSON
{
  "type": "movement_mode",
  "id": "fly",
  "character": "f",
  "panel_char": "F",
  "name": "Fly",
  "panel_color": "light_green",
  "symbol_color": "light_green",
  "exertion_level": "EXTRA_EXERCISE",
  "change_good_none": "You throw yourself at the ground, and miss.",
  "change_good_animal": "You steer your steed into a... fly?",
  "change_good_mech": "You enable your mech's jetpack unit.",
  "change_bad_none": "You throw yourself at the ground, and hit.",
  "change_bad_animal": "Your steed doesn't know how to fly.",
  "change_bad_mech": "Your mech cannot fly.",
  "move_type": "running",
  "stamina_multiplier": 12.0,
  "sound_multiplier": 4.0,
  "move_speed_multiplier": 2.5,
  "mech_power_use": 5,
  "swim_speed_mod": -80,
  "stop_hauling": true
},
```

### `type`
Mandatory. Must be `"movement_mode"`.

### `id`
Mandatory. The id of the movement mode.

### `character`
Mandatory. The character shown in the move mode menu to select this mode.

### `panel_char`
Mandatory. The character shown on the panels when this mode is selected.

### `name`
Mandatory. The name of the move mode displayed whenever it is displayed.

### `panel_color`
Mandatory. The color shown on the panels when this mode is selected.

### `symbol_color`
Mandatory. The color shown in curses when your character is in this move mode.

### `extertion_level`
Mandatory. What extertion level this move mode will put you into. The options are, from least to most exercise:
```
NO_EXERCISE
LIGHT_EXERCISE
MODERATE_EXERCISE
ACTIVE_EXERCISE
EXTRA_EXERCISE
```

### `change_good_x`
Mandatory. The message given when the character switches to this move mode.
There are three values for x: `none`, `animal`, and `mech`.
- `none` is the message given when the character is not mounted.
- `animal` is the message given when riding an animal.
- `mech` is the message given when in a mech.

### `change_bad_x`
Optional. The message given when the character fails to switch to this move mode.
Right now, the only move mode that you can fail to switch are `running` type modes.
There are three values for x: `none`, `animal`, and `mech`.
- `none` is the message given when the character is not mounted.
- `animal` is the message given when riding an animal.
- `mech` is the message given when in a mech.

Defaults to `You feel bugs crawl over your skin.`

### `move_type`
Mandatory. The type of move mode - used to determine if the character is in stealth, walking normally, or fleeing.
Valid values are:
- `crouching`
- `walking`
- `running`

### `stamina_multiplier`
Optional. Multiplier on the character's stamina use when in this mode.
All non-negative floating point values are valid.

Defaults to `1.0` (no multiplier).

### `sound_multiplier`
Optional. Multiplier on the sound made when moving in this mode.
All non-negative floating point values are valid.

Defaults to `1.0` (no multiplier).

### `move_speed_multiplier`
Optional. Multiplier on the move speed when moving in this mode.
All non-negative floating point values are valid.

Defaults to `1.0` (no multiplier).

### `mech_power_use`
Optional. Mech power used each step when in this mode.
All integer values are valid.

Defaults to `2`.

### `swim_speed_mod`
Optional. How many additional moves .
All integer values are valid.

Defaults to `0`.

### `stop_hauling`
Optional. Determines whether or not hauling is cancelled switching to this mode.
Valid values are `true` and `false`.

Defaults to false.

