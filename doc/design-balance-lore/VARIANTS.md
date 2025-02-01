# Variants

## What are variants?

Variants, added in [#51613](https://github.com/CleverRaven/Cataclysm-DDA/pull/51613) and [#46814](https://github.com/CleverRaven/Cataclysm-DDA/pull/46814) are CDDAs solution to allowing the same item have different names. It's used in guns to have generic or specific names changeable with a setting, or in items such as clothing to have lots of different colors without having a bunch of essentially duplicate entries for an item. They can be weighted, meaning different variants have a different chance of showing up when the base item is spawned. They're a great way to add in extra variety to the game at a low cost to both effort, and file sizes. Do note that for some items, such as newspapers where the description changes and not the name, [snippets](../JSON/JSON_INFO.md#snippets) may be better.

## What aren't variants?

Variants cannot change any stat of an item, besides the name, description, color, sprite, and symbol. They're nothing more than an aesthetic change and have no impact on gameplay. 

# Inclusion Criteria 

## Issues

As cool as variants are, sometimes issues can arise if there's too much variants of an item with nothing to reign them in. As an example, condoms had a few joke flavors that didn't make sense, and snowballed into other items having variants that were nothing more than jokes, such as chewing gum having fart, cat hairball, and zombie flavors. This doc aims to provide contributors with a good idea of what is, and isn't acceptable as variants. Another factor is that more variants complicate translating, making a difficult job more difficult.

## Criteria 

### Goals

Variants should add meaningful variety to CDDA. They should be distinct from other variants and also be reasonably attainable in the real world. There isn't any hard number on the amount of variants that are acceptable, as it can vary greatly from item to item. 

Don't be overzealous with joke variants, one joke variant alone is not likely to be accepted while multiple surely aren't.

While this entry is good for a general grasp of what is acceptable, below are some more specific examples of variants you may want to add.

### Foods

Variants for foods generally cover just flavors, as those tend to have little to no calorie difference. You should try to cover a base of flavors without being too specific. Strawberry, banana, raspberry flavours are acceptable as chewing gum flavors, while Crazy Carl's Supreme Strawberry Surprise flavor would not. You also want to avoid overlap, strawberry banana raspberry twist chewing gum doesn't add more variety than the individual variants already do. 

### Clothing

Clothing can benefit greatly from variants, as there's a lot of different clothes that have different colors. However, it shouldn't be too in the weeds for details. Aim for covering basic colors and patterns, while avoiding overly specific ones.  A pink hoodie is fine, but a shrimp pink hoodie or a cream and blue Rorschach polka dot hoodie is not.

### Guns 

Guns can have variants as well, such as the Ruger Mk IV having different versions, or the AK having different manufacturers. Gun variants should follow [GUN_NAMING_AND_INCLUSION](../GUN_NAMING_AND_INCLUSION.md) doc for inclusion. However, being overly specific such as specifying a Glock 19 with red stippled grips and a dark gray slide is unneeded. You're aiming to cover different guns, not overly specific details. 

### Religious items 

Religous item variants should focus mainly on major religions. Stuff like The Church of the Flying Spaghetti Monster, Heaven's Gate, Norse Mythology should be excluded or at the very least, very rare and not spawn in place of commonplace religons. 

### Books and manuals

Books, and especially manual variants should aim to be relatively low in count, as a high variant count for what is the same item can confuse people and cause them to either pick up a copy of a book they already have, or have to put in extra effort to examine the book to make sure. 
