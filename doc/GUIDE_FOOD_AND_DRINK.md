# Guide to add Food and Drink

- [Making Comestibles](#making-comestibles)
  - [Notes](#notes)
  - [Getting nutrient data](#getting-nutrient-data)
  - [Determining values](#determining-values)
    - [Weight, volume and portions](#weight-volume-and-portions)
    - [Calories and vitamins](#calories-and-vitamins)
    - [Quench](#quench)
    - [Health](#health)
  - [Example](#example)


## Notes

This is a guide for adding normal food and drink items. It does not cover drugs, 

## Getting nutrient data

For consistency and because the game mostly have products made in the USA, use the FDA website:

* https://fdc.nal.usda.gov/

## Determining values

### Weight, volume and portion size.

You generally get weight from the package or the FDA website linked above. 

Packaged comestibles should have the volume of the package, and portioned comestibles should have the volume of the portion. For food and drinks that come in bulk you can use a unit of 250ml. 

If you have nutrient data per 100g, you should first determine approximate density of the food.

Density table for common food components:

component | density (g/ml)
---|--:
water | 1
alcohol (ethanol) | 0.789
animal/vegetable oil | 0.9
protein | 1.35
carbohydrates | 1.55 (the "Carbohydrate, by difference" on FDA website)

Anything with air bubbles in it (e.g. baked with yeast or baking powder) has lower density. 

If you can get several portions from the same unit, you use `charges`.

To get a conversion factor from units per 100g to units of 250ml, divide weight by density and multiply by 2.5. 

To get a conversion factor from units per 100g to portions, divide the number above by `charges`.

For a food item with portions. Volume remains the same, but weight, calories, quench, vitamins and minerals should be scaled to volume/charges.

### Calories and vitamins

You should get calories from the nutrition data. If not you can use the following table:

* Fat 9 calories/g
* Carbohydrates 4 calories/g
* Protein 4 calories/g
* Ethanol 7 calories/g

If you have nutrition data per 100g, multiply it by the conversion factor we calculated in weight and volume.

We're interested in 12 values, calories, water, saturated fat, sugar, fiber, sodium, Vitamin A, Vitamin B12, Vitamin C, Calcium, Iron and alcohol.

You can copy the table from FDA website for pasting in a spreadsheet by selecting the text or holding ctrl for select table cells.

Vitamins and minerals are measured in units percentage of Reference Daily Intake (DRI).

Reference:

Nutrient | FDA DRI | Scaled to 2500 kcal | unit
--- | --: | --: | ---
Vitamin A | 900 | 1125 | µg/day
Vitamin B12 | 2.4 | 3 | µg/day
Vitamin C | 90 | 112.5 | mg/day
Calcium | 1300 | 1625 | mg/day
Iron | 18 | 22.5 | mg/day
Dietary Fiber | 28 | 35 | g/day
Sodium | 1500 | 1875 | mg/day
Sugar | 62.5 | 78.125 | g/day
Saturated fat | 20 | 25 | g/day

### Quench

The base value is 5 ml of water gives 1 quench, then we remove quench for calories, alcohol and salt.

Quench is calculated as: water(g)/5 - 0.005 * calories(g) - 0.004 * sodium(mg) - 1.281 * alcohol(g)

### Health

* First we calculate the average DRI of Vitamin A, Vitamin B12, Vitamin C, Calcium, Iron and dietary fiber.
* Then we subtract the average DRI of sugar, saturated fats and salt.
* Then we subtract every 1g of alcohol.
* Then we take it to the 3/2th root, or equivalently to the power of 2/3, to make the values more manageable. If health is negative, we take the root by its absolute value and add the sign afterwards.

### Fun

Fun is a matter of taste and somewhat arbitrary, compare with the food already in game.

* Generally high-end luxury foodstuffs, alcohol, desserts, snacks have high fun (15-30)
* Sweet berries and fruit, healthy snacks, junk food, breakfast food, complex cooked meals have medium fun (5-15).
* Basic everyday food, canned food, non-sweet fruits and berries has low fun (0-5).
* Stuff that you wouldn't eat on its own has negative fun (<0).

Some base values:

* Cocktails: 20
* Strong spirits: 12-18
* Wine: 15-16
* Beer: 10-18
* Non-alcoholic beverages: 2-10.
* Sweet berries: 5
* Canned food: -2-2

## Example

This is better done with automated tools provided, but just to illustrate the principle say we want to make a Bloody Mary drink.

#### Step 1: Get basic guidelines for comestibles from `JSON_INFO.md`. 

Our .json now looks like this:

````
{
    "type": "COMESTIBLE",
    "comestible_type": "DRINK",
    "id": "bloody_mary",
    "name": { "str": "Bloody Mary", "str_pl": "Bloody Marys" },
    "description": "A cocktail containing vodka, tomato juice, and other spices and flavorings."
}
````

#### Step 2: We get nutrients per 100g for a Bloody Mary from the FDA website:

* https://fdc.nal.usda.gov/fdc-app.html#/food-details/789607/nutrients

#### Step 3: We calculate weight and volume.

We see it has 87.31g water, 3.16g carbohydrates, 7.8g ethanol and negligible amount of protein and fat. 

Total weight is 87.31g + 3.16g + 7.8g =  99.14 g.

Total volume is:

87.31g / 1g/ml = 87.31ml
7.8g / 0.789g/ml = 9.89ml
3.16g / 1.55g/ml = 4.00ml
Total = 101.2 ml

Density = 99.14g / 101.2ml = 1.02g/ml

Then we have the conversion factor from units per 100g to units of 250ml = 0.998g/ml * 2.5 = 2.507

As you would consume the drink in one go you don't need portion sizes.

#### Step 4: We determine Calories and vitamins

We copy the table from the FDA website, paste it in a spreadsheet and multiply by 2.495g to get unit per 250ml.

Nutrient | Per 100g | Unit | Value per 250ml | DRI | Percent of DRI
---|--:|---|--:|--:|--:
Water  | 87.31 | g  | 218.92 |  | 
Energy  | 69 | kcal  | 173.01 |  | 
Fiber, total dietary  | 0.4 | g  | 1.00 | 35 | 2.87
Sugars, total including NLEA  | 2.22 | g  | 5.57 | 78.125 | 7.13
Calcium, Ca  | 10 | mg  | 25.07 | 1625 | 1.54
Iron, Fe  | 0.4 | mg  | 1.00 | 22.5 | 4.46
Sodium, Na  | 207 | mg  | 519.03 | 1875 | 27.68
Vitamin C, total ascorbic acid  | 51.1 | mg  | 128.13 | 112.5 | 113.89
Vitamin B-12  | 0 | µg  | 0 | 3 | 0
Vitamin A, RAE  | 17 | µg  | 42.63 | 1125 | 3.79
Fatty acids, total saturated  | 0.017 | g  | 0.04216 | 25 | 0.17

We divide the vitamins and minerals by their DRI to get DRI per portion.

Our .json now looks like this:

````
{
    "type": "COMESTIBLE",
    "comestible_type": "DRINK",
    "id": "bloody_mary",
    "name": { "str": "Bloody Mary", "str_pl": "Bloody Marys" },
    "weight": "248 g",
    "volume": "250 ml",
    "calories": 173,
    "vitamins": [ [ "vitA", 4 ], [ "vitC", 114 ], [ "calcium", 2 ], [ "iron", 4 ] ],
}
````

#### Step 5: We determine Quench

We plug in the values in the formula:

Nutrient | Calculation | result
---|--|---
water | 217.84 / 5 | 43.784
calories | 172.16 * -0.005 | -0.865
sodium | 516.47 * -0.004 | -2.076
alcohol | 19.56 * -1.281 | -25.053
total quench | | 15.79

Our .json now looks like this:

````
{
    "type": "COMESTIBLE",
    "comestible_type": "DRINK",
    "id": "bloody_mary",
    "name": { "str": "Bloody Mary", "str_pl": "Bloody Marys" },
    "weight": "248 g",
    "volume": "250 ml",
    "calories": 172,
    "vitamins": [ [ "vitA", 4 ], [ "vitC", 114 ], [ "calcium", 2 ], [ "iron", 4 ] ],
    "quench": 16,
}
````

#### Step 5: We determine Health

First we average the DRI of the good stuff:

Vitamin A 3.79%
Vitamin B12 0.00%
Vitamin C 113.89%
Calcium 1.54%
Iron 4.46%
Dietary fiber 2.87%
Average	21.09%

Then we subtract the average DRI of the bad stuff:

Sodium 27.68%
Sugar 7.13%
Saturated fat 0.17%
Average 11.66%


good stuff - bad stuff = 21.09% - 11.66% = 0.0943

Then we subtract the alcohol penalty 19.56*0.2 and we get 0.0943-3,912 = -3.8177. This to the power of 2/3 gives us the health value -2.444 which is rounded to -2.

Our .json now looks like this:

````
{
    "type": "COMESTIBLE",
    "comestible_type": "DRINK",
    "id": "bloody_mary",
    "name": { "str": "Bloody Mary", "str_pl": "Bloody Marys" },
    "description": "A cocktail containing vodka, tomato juice, and other spices and flavorings.",
    "container": "bottle_glass",
    "material": [ "alcohol", "fruit" ],
    "primary_material": "alcohol",
    "weight": "249 g",
    "volume": "250 ml",
    "calories": 172,
    "vitamins": [ [ "vitA", 4 ], [ "vitC", 114 ], [ "calcium", 2 ], [ "iron", 4 ] ],
    "quench": 16,
    "health": -2
}
````