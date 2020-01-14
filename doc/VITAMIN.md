# vitamin

## definition

```JSON
{
  "id": "iron",
  "type": "vitamin",
  "vit_type": "vitamin",
  "name": "Iron",
  "excess": "hypervitaminosis",
  "deficiency": "anemia",
  "min": -12000,
  "max": 3600,
  "rate": "15 m",
  "flags": [ "FOO" ],
  "disease": [ [ -4800, -5600 ], [ -5601, -6400 ], [ -6401, -12000 ] ],
  "disease_excess": [ [ 10, 19 ], [ 20, 29 ], [ 30, 40 ] ]
},
```
### `id`
Mandatory. The id of the vitamin.

### `type`
Mandatory. Must be `vitamin`.

### `vit_type`
The type of the vitamin. Valid values are:

#### `vitamin`
When simplified nutrition is enabled, this vitamin will not be added to any items and any time the game attempts to retrieve it from the player it will give 0.
Only nutritional vitamins should have this type.

#### `toxin`
This is some toxic chemical or component. This currently has no effect.

#### `drug`
This is a drug. This currently has no effect.

#### `counter`
This is a counter for something, that is neither a toxin, vitamin, or drug. This currently has no effect.

### `name`
What the vitamin shows up as where vitamins are displayed, such as the vitamins display in the item menu.

### `deficiency`
The id of an effect that is triggered by a deficiency of this vitamin.

### `excess`
The id of an effect that is triggered by a excess of this vitamin.

### `min`
The smallest amount of this vitamin that the player can have.

### `max`
The highest amount of this vitamin that the avatar can have.

### `rate`
How long it takes to lose one unit of this vitamin.

### `flags`
An array of string flags, see the flags section below for valid ones

### `disease`
What the thresholds of deficiency of this vitamin are.
Each pair in the list determines the start and end points of that tier of deficiency.
Each tier of deficiency corresponds to the intensity level of the effect defined in `deficiency`.

### `disease_excess`
What the thresholds of excess of this vitamin are.
Each pair in the list determines the start and end points of that tier of excess.
Each tier of excess corresponds to the intensity level of the effect defined in `excess`.

## flags

- ```NO_DISPLAY``` - This vitamin will not be shown when examining a food
