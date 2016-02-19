Item spawn system:
=====

Collection or Distribution:
====

In a Collection each entry is chosen independently from the other entries. Therefor the probability associated with each entry is absolute, in the range of 0...1. In the json files it is implemented as percentage with values from 0 to 100.

A probability of 0 (or negative) means the entry is never chosen, a probability of 100% means its always chosen. The default is 100, because it's the most useful value. Default 0 would mean the entry can be removed anyway.

A Distribution is a weighted list like the current system. Exactly one entry is chosen from it. The probability of each entry is relative to the probability of the other entries. A probability of 0 (or negative) means it is never chosen.

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
"damage": <number>|<array>,
"damage-min": <number>,
"damage-max": <number>,
"count": <number>|<array>,
"count-min": <number>,
"count-max": <number>,
"charges": <number>|<array>,
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
"charges": [10, 100]
```
This will create 4 items, they can have different damage levels as the damage value is rolled separately for each of these items. Each item has charges in the range of 10 to 100 (inclusive). Using an array (which must have 2 entries) for charges/count/damage is equivalent to writing explicit min and max values. In other words `"count": [a,b]` is the same as `"count-min": a, "count-max": b`.

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
In other words: a single string denotes an item id, an array (which must contain a string and a number) denotes an item id and a probability.

Similar for groups:
```
"groups": [ "<id-1>", [ "<id-2>", 10 ] ]
```

This format does not support further properties of the created items, the probability is only optional for entries of collections!

The content of "entries", "items" and "groups" are all added if those members exist. This will have the item "<id-1>" appear twice in the item group:
```
{
    "items": [ "<id-1>" ],
    "entries": [ { "item": "<id-1>" } ]
}
```

Another example. The group "milk" spawns a container (taken from milk_containers) that contains milk (maximal amount that fits into the container, because there is no specific charges value defined).
```
{
    "type" : "item_group",
    "id": "milk_containers",
    "subtype": "distribution",
    "items": [
    "bottle_plastic", "bottle_glass", "flask_glass",
    "jar_glass", "jar_3l_glass", "flask_hip", "55gal_drum"
    ]
},
{
    "type" : "item_group",
    "id": "milk",
    "subtype": "distribution",
    "entries": [
        { "item": "milk", "container-group": "milk_containers" }
    ]
},
```

Inlined item groups
====

At some places one can define an item group directly instead of giving the id of a group. One can not refer to that group as it has no visible id (it has an unspecific/random id internally). This is most useful when the group is very specific to the place it is used and wont ever appear anywhere else.

As an example: monster death drops ("death_drops" entry in the "MONSTER" object, see JSON_INFO.md) can do this. If the monster is very specific (e.g. a special robot, a unique endgame monster), the item spawned upon its death wont (in that form) appear in any other group.

Therefor this snippet:
```JSON
{
    "type": "item_group",
    "id": "specific_group_id",
    "subtype": "distribution",
    "items": [ "a", "b" ]
},
{
    "death_drops": "specific_group_id"
}
```
is equivalent to:

```JSON
{
    "death_drops": {
        "subtype": "distribution",
        "items": [ "a", "b" ]
    }
}
```

The inline group is read like any other group and one can use all the properties mentioned above. Its "type" and its "id" members are always ignored.

Instead of a full JSON object, one can also write a JSON array. The default subtype is used and the array is read like the "entries" array (see above). Each entry of that array must be a JSON object. Example:
```JSON
{
    "death_drops": [
        { "item": "rag", "damage": 2 }, { "item": "bowling_ball" }
    ]
}
```

----

You can test your item groups in the game:
- enable the debug menu (use '?' -> '1' to go to the keybindings and bind a key to "Debug menu"),
- load a game and call the debug menu,
- choose "item spawn debug".
- select the item group you want to debug. It will spawn items based on that 100 times and count the spawned items. They are displayed, sorted by their frequency.

----

You should not add items to the item group "EMPTY_GROUP". This group can be used when the game requires a group id, but you don't want to spawn any items there. The group will never spawn items.
