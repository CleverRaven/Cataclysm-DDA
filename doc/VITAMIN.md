# Vitamins

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
  "weight_per_unit": "1 mg",
  "rate": "15 m",
  "flags": [ "FOO" ],
  "disease": [ [ -4800, -5600 ], [ -5601, -6400 ], [ -6401, -12000 ] ],
  "disease_excess": [ [ 10, 19 ], [ 20, 29 ], [ 30, 40 ] ],
  "decays_into": [ [ "calcium", 2 ], [ "blood", 6 ] ]
},
```
### `id`
Mandatory. The id of the vitamin.

### `type`
Mandatory. Must be `vitamin`.

### `vit_type`
Mandatory. The type of the vitamin. Valid values are:

#### `vitamin`
When simplified nutrition is enabled, this vitamin will not be added to any items and any time the game attempts to retrieve it from the player it will give 0.
Only nutritional vitamins should have this type. **Vitamins are inputted in JSON as RDA everything else is a constant value**

#### `toxin`
This is some toxic chemical or component. This currently has no effect.

#### `counter`
This is a counter for something, that is neither a toxin, vitamin, or drug.

#### `drug`
This is a drug. This currently acts as a 'counter', except that it takes 30 minutes (1 stomach cycle) to effect the player character, in order to simulate slow digestion and metabolism of drugs. In order to properly make use of this delay effect, make sure to add it to the base drug item itself, and not it's use effect(s). Look at ibuprofen at med.json for an example.

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

### `weight_per_unit`
Allows specifying this vitamin in foods in terms of the amount of that vitamin in that food by weight.
The weight specified for the food will be divided by this quantity to determine the units of this vitamin the food has.

### `rate`
How long it takes to lose one unit of this vitamin.

### `flags`
An array of string flags; see the flags section below for valid ones.

### `disease`
What the thresholds of deficiency of this vitamin are.
Each pair in the list determines the start and end points of that tier of deficiency.
Each tier of deficiency corresponds to the intensity level of the effect defined in `deficiency`.

### `disease_excess`
What the thresholds of excess of this vitamin are.
Each pair in the list determines the start and end points of that tier of excess.
Each tier of excess corresponds to the intensity level of the effect defined in `excess`.


### `decays_into`
Whenever this vitamin is naturally metabolized, it can adjust the levels of other vitamins in the body.
Each pair in the list contains a vitamin ID and a number, with the number representing how much that vitamin will be adjusted by when this vitamin is metabolized by one unit.
In the provided example, each unit of iron will decay into two units of calcium; four units of iron decays into eight units of calcium, and so on.
Negative values can be used to purge existing vitamins from the body, instead of adding them.
This only happens with natural decay, as determined by `rate`; other sources of vitamin removal won't cause this to trigger.

## flags

- ```NO_DISPLAY``` - This vitamin will not be shown when examining a food.
- ```OBSOLETE``` - This vitamin will not be displayed when tracking vitamins in the consume menu.


#### opiate MME conversion chart
Opioid Oral Morphine Milligram Equivalent (MME) Conversion Factorsi,ii
Type of Opioid (strength units) MME Conversion Factor
Buprenorphine film/tabletiii (mg) -----------------?
Buprenorphine patchiii (mcg/hr) -------------------?
Buprenorphine film iii (mcg)-----------------------?
Butorphanol (mg) ----------------------------------7
Codeine (mg) --------------------------------------0.15
Dihydrocodeine (mg) -------------------------------0.25
Fentanyl buccal or SL tablets, or lozenge/trocheiv
(mcg) ---------------------------------------------0.13
Fentanyl film or oral sprayv (mcg) ----------------0.18
Fentanyl nasal sprayvi (mcg) ----------------------0.16
Fentanyl patchvii (mcg) ---------------------------7.2
Hydrocodone (mg) ----------------------------------1
Hydromorphone (mg) --------------------------------4
Levorphanol tartrate (mg) -------------------------11
Meperidine hydrochloride (mg) ---------------------0.1
Methadoneviii (mg) --------------------------------3
>0, <= 20 -----------------------------------------4
>20, <=40 -----------------------------------------8
>40, <=60 -----------------------------------------10
>60 -----------------------------------------------12
Morphine (mg) -------------------------------------1
Opium (mg) ----------------------------------------1
Mutated Poppy (mg) in game only -------------------1
Oxycodone (mg) ------------------------------------1.5
Oxymorphone (mg) ----------------------------------3
Pentazocine (mg) ----------------------------------0.37
Tapentadolix (mg) ---------------------------------0.4
Tramadol (mg) -------------------------------------0.1