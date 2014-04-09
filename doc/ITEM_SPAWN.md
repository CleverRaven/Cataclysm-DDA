Item spawn system:
=====

Collection or Distribution:
====

In a Collection each entry is chosen independently from the other entries. Therefor the probability associated with each entry is absolute, in the range of 0...1. In the json files it is implemented as percentage with values from 0 to 100.

A probability of 0 means the entry is never chosen, a probability of 100% means its always chosen. The default is 100, because it's the most useful value. Default 0 would mean the entry can be removed anyway.

A Distribution is a weighted list like the current system. Exactly one entry is chosen from it. The probability of each entry is relative to the probability of the other entries. A probability of 0 means it is never chosen.

The format is this:
```
{
    "type": "item_group",
    "subtype": "<subtype>",
    "id": "<some name>",
    "entries": [ ... ]
}
```
`subtype` defaults to `old`, which denotes that this item group uses the old format (which is technically a distribution).

`subtype` can be `collection` or `distribution`.

The `entries` list contains entries, each entry can be one of this:
```
{ "item": "<item-id>", ... }
```
or
```
{ "group": "<group-id>", ... }
```

The game decides based on the existence of either the `item` or the `group` value if the entry denotes a item or a reference to another item group.

Each entry can have more values (shown above as `...`). They allow further properties of the item(s):
```
"damage": <number>,
"damage-min": <number>,
"damage-max": <number>,
"count": <number>,
"count-min": <number>,
"count-max": <number>,
"charges": <number>,
"charges-min": <number>,
"charges-max": <number>,
"contents-item": "<item-id>",
"contents-group": "<group-id>",
"ammo-item": "<ammo-item-id>",
"ammo-group": "<group-id>",
"container-item": "<container-item-id>",
"container-group": "<group-id>",
```
`contents` is added as contents of the created item. It is not checked if they can be put into the item. This allows water, that contains a book, that contains a steel frame, that contains a corpse.

`count` makes the item spawn repeat to create more items, each time creating a new item.
```
"damage-min": 0,
"damage-max": 3,
"count": 4
``´
This will create 4 items, they can have different damage levels as the damage value is rolled separately for each of these items.

The ammo type is checked and applied only to weapon / gunmods.
The container is checked and the item is put inside the container, and the charges of the item are capped/increased to match the size of the container.

Some special shortcuts exist also:
This:
```
"items": [ "<id-1>", [ "<id-2>", 10 ] ]
```
means the same as
```
"entries": [ { "item": "<id-1>" }, { "item": "<id-2>", "prob": 10 } ]
```

Similar for groups:
```
"groups": [ "<id-1>", [ "<id-2>", 10 ] ]
```

This format does not support further properties of the created items, the probability is only optional for entries of collections!

