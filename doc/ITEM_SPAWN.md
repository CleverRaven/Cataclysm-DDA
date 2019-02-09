# Item spawn system

## Collection or Distribution

In a Collection each entry is chosen independently from the other entries. Therefore the probability associated with each entry is absolute, in the range of 0...1. In the json files it is implemented as percentage with values from 0 to 100.

A probability of 0 (or negative) means the entry is never chosen, a probability of 100% means its always chosen. The default is 100, because it's the most useful value. Default 0 would mean the entry can be removed anyway.

A Distribution is a weighted list like the current system. Exactly one entry is chosen from it. The probability of each entry is relative to the probability of the other entries. A probability of 0 (or negative) means it is never chosen.

An example: Suppose item A has a probability of 30 and item B has a probability of 20. Then the probabilities of the 4 combinations of A and B are:


| Combination     | Collection | Distribution |
| ----------------|------------|------------- |
| Neither A nor B | 56%        | 0%           |
| Only A          | 24%        | 60%          |
| Only B          | 14%        | 40%          |
| Both A and B    | 6%         | 0%           |

## Format

The format is this:

```json
{
    "type": "item_group",
    "subtype": "<subtype>",
    "id": "<some name>",
    "ammo": <some number>,
    "magazine": <some number>,
    "entries": [ ... ]
}
```

`subtype` is optional. It can be `collection` or `distribution`. If unspecified, it defaults to `old`, which denotes that this item group uses the old format (which is technically a distribution).

There are [some caveats](#ammo-and-magazines) to watch out for when using `ammo` or `magazine`.

### Entries array

The `entries` list contains entries, each of which can be one of the following:

1. Item
    ```json
    { "item": "<item-id>", ... }
    ```

2. Group
    ``` json
    { "group": "<group-id>", ... }
    ```

3. Distribution
    ```json
    { "distribution": [
        "An array of entries, each of which can match any of these 4 formats"
    ] }
    ```

4. Collection
    ```json
    { "collection": [
        "An array of entries, each of which can match any of these 4 formats"
    ] }
    ```

The game decides based on the existence of either the `item` or the `group` value if the entry denotes a item or a reference to another item group.

Each entry can have more values (shown above as `...`). They allow further properties of the item(s):

```json
"damage": <number>|<array>,
"damage-min": <number>,
"damage-max": <number>,
"count": <number>|<array>,
"count-min": <number>,
"count-max": <number>,
"charges": <number>|<array>,
"charges-min": <number>,
"charges-max": <number>,
"contents-item": "<item-id>" (can be a string or an array of strings),
"contents-group": "<group-id>" (can be a string or an array of strings),
"ammo-item": "<ammo-item-id>",
"ammo-group": "<group-id>",
"container-item": "<container-item-id>",
"container-group": "<group-id>",
```

`contents` is added as contents of the created item. It is not checked if they can be put into the item. This allows water, that contains a book, that contains a steel frame, that contains a corpse.

`count` makes the item spawn repeat to create more items, each time creating a new item.

```json
"damage-min": 0,
"damage-max": 3,
"count": 4
"charges": [10, 100]
```

This will create 4 items, they can have different damage levels as the damage value is rolled separately for each of these items. Each item has charges (AKA ammo) in the range of 10 to 100 (inclusive); if the item needs a magazine before it can have charges, that will be taken care of for you. Using an array (which must have 2 entries) for charges/count/damage is equivalent to writing explicit min and max values. In other words `"count": [a,b]` is the same as `"count-min": a, "count-max": b`.

The container is checked and the item is put inside the container, and the charges of the item are capped/increased to match the size of the container.

### Ammo and Magazines

Here are some ways to make items spawn with/without ammo/magazines (note that `ammo-item` can
be specified for guns and magazines in the entries array to use a non-default ammo type):

* Specify an ammo/magazine chance (introduced in Section 2) for the entire item group.
  `ammo` specifies the percent chance that the entries will spawn fully loaded (if it needs a magazine, it will be added for you).
  `magazine` specifies the percent chance that the entries will spawn with a magazine.
  Both of these default to 0 if unspecified.

  Note that `ammo` and `magazine` only apply to tools, guns, and magazines. Furthermore, they don't apply to tools whose entry explicitly specifies how much ammo (charges) to spawn with, or to tools whose JSON item definition specifies a random amount or a fixed, nonzero amount of initial charges.

  If any item groups are referenced from your item group, then their ammo/magazine chances are ignored, and yours are used instead.

*  Use `charges`, `charges-min`, or `charges-max` in the entries array. A default magazine will be added for you if needed.

## Shortcuts

This:

```json
"items": [ "<id-1>", [ "<id-2>", 10 ] ]
```

means the same as:

```json
"entries": [ { "item": "<id-1>" }, { "item": "<id-2>", "prob": 10 } ]
```

In other words: a single string denotes an item id, an array (which must contain a string and a number) denotes an item id and a probability.

This is true for groups as well:

```json
"groups": [ "<id-1>", [ "<id-2>", 10 ] ]
```

This format does not support further properties of the created items - the probability is only optional for entries of collections!

The content of "entries", "items" and "groups" are all added if those members exist. This will have the item `<id-1>` appear twice in the item group:

```json
{
    "items": [ "<id-1>" ],
    "entries": [ { "item": "<id-1>" } ]
}
```

Another example. The group "milk" spawns a container (taken from milk_containers) that contains milk (maximal amount that fits into the container, because there is no specific charges value defined).

```json
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

## Inlined item groups

In some places one can define an item group directly instead of giving the id of a group. One can not refer to that group as it has no visible id (it has an unspecific/random id internally). This is most useful when the group is very specific to the place it is used and wont ever appear anywhere else.

As an example: monster death drops (`death_drops` entry in the `MONSTER` object, see [JSON_INFO.md](https://github.com/CleverRaven/Cataclysm-DDA/blob/master/doc/JSON_INFO.md)) can do this. If the monster is very specific (e.g. a special robot, a unique endgame monster), the item spawned upon its death won't (in that form) appear in any other group.

Therefore, this snippet:

```json
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

```json
{
    "death_drops": {
        "subtype": "distribution",
        "items": [ "a", "b" ]
    }
}
```

The inline group is read like any other group and one can use all the properties mentioned above. Its `type` and its `id` members are always ignored.

Instead of a full JSON object, one can also write a JSON array. The default subtype is used and the array is read like the "entries" array (see above). Each entry of that array must be a JSON object. Example:

```json
{
    "death_drops": [
        { "item": "rag", "damage": 2 }, { "item": "bowling_ball" }
    ]
}
```

----

### Notes

You can test your item groups in the game:

1. Load a game and call the debug menu
    > *TIP* (If a key isn't bound to the debug menu or you forgot it, use <kbd>ESC</kbd> > <kbd>1</kbd>)

2. Choose "Test Item Group".

3. Select the item group you want to debug.

    The game will then spawn items in that group 100 times and count the spawned items. They'll then be displayed, sorted by their frequency.

    > *TIP*: You can filter anything in the debug menu using <kbd>/</kbd>

You should not add items to the item group `EMPTY_GROUP`. This group can be used when the game requires a group id, but you don't want to spawn any items there. The group will never spawn items.
