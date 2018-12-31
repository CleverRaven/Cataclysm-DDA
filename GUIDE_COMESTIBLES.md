# Guide to add Comestibles

There are a large number of files in `json/items/comestibles`, and this guide will help you decide where to put your new comestible!

## List of special comestibles:
`med.json` -- comestible that has an addiction effect besides alcohol and caffeine, plus bandages and disinfectant type items.
`mre.json` -- items and comestibles related to MREs
`mutagen.json` -- comestible that has any mutagen effect
`carnivore.json` -- item that would normally be butchered from an animal or monster, and their cooked versions. examples: chunks of meat, tainted bones, boiled stomach
`protein.json` -- comestibles based on protein powder
`spice.json` -- comestibles that normally do not have nutrients, but are used to flavor a dish. examples: salt, thyme, black pepper
`frozen.json` -- comestibles best eaten frozen
`brewing.json` -- items used in the brewing process

## List of normal comestibles, in order of priority:
When you have a comestible you want to add to the files, just go down this list, and the first filename you see in the list that matches the criteria is your choice!

`alcohol.json` -- "Drink" comestible with alcohol addiction
`soup.json` -- "Drink" comestible which is a soup. This is more of a "food" than a drink - you primarily want the calories from this
`drink.json` -- "Drink" comestible. This is your drink of choice when you're thirsty! examples: tea, juice, water
`drink_other.json` -- "Drink" comestible that does not fit any other criteria. example: vinegar, mustard

`junkfood.json` -- comestible with the "junk" material. examples: cake, sugary cereal, nachos
`sandwich.json` -- a "sandwich" is generally two slices of "bread" with something in between. examples: BLT, PB&J sandwich, fish sandwich
`offal_dishes.json` -- comestible made of various offals. the offal types are: liver, brain, kidney, sweetbread, stomach. This must be the primary part of the dish.
`seed.json` -- seeds
`meat_dishes_human.json` -- comestible made of human flesh. examples: hobo helper, tio taco
`meat_dishes.json` -- comestible made with meat.
`raw_veggy.json` -- comestible that is a vegetable in its raw form.
`irradiated_veggy.json` -- comestible that is an irradiated vegetable. has an indefinite shelf life.
`raw_fruit.json` -- comestible that is a fruit in its raw form.
`irradiated_fruit.json` -- comestible that is an irradiated fruit. has an indefinite shelf life.
`veggy_dishes.json` -- comestible made of vegetables.
`bread.json` -- comestible that is either a bread starter, or a cooked bread.
`wheat.json` -- either raw wheat, or made of wheat
`egg.json` -- either an egg, or made of egg
`dairy.json` -- made of milk
`mushroom.json` -- a mushroom or made of mushrooms
`nuts.json` -- a nut or made of nuts
`other.json` -- if you made it here, your comestible doesn't fit any other category!