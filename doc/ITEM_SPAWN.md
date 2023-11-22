# Item spawn system

## Collection or Distribution

In a Collection each entry is chosen independently from the other entries.  Therefore, the probability associated with each entry is absolute, in the range of 0...1.  In the json files it is implemented as a percentage (with values from 0 to 100).

A probability of 0 (or negative) means the entry is never chosen; a probability of 100% means it's always chosen.  The default is 100, because it's the most useful value.  A default of 0 would mean the entry could be removed anyway.

A Distribution is a weighted list.  Exactly one entry is chosen from it.  The probability of each entry is relative to the probability of the other entries.  A probability of 0 (or negative) means it is never chosen.

An example: Suppose item A has a probability of 30 and item B has a probability of 20. Then the probabilities of the 4 combinations of A and B are:


| Combination     | Collection | Distribution |
| --------------- | ---------- | ------------ |
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
    "container-item": "<container-item-id>",
    "on_overflow": "<discard|spill>",
    "entries": [ ... ]
  }
```

`subtype` is optional. It can be `collection` or `distribution`.  If unspecified, it defaults to `old`, which denotes that this item group uses the old format (essentially a distribution).

`container-item` causes all the items of the group to spawn in a container,
rather than as separate top-level items.  If the items might not all fit in the
container, you must specify how to deal with the overflow by setting
`on_overflow` to either `discard` to discard items at random until they fit, or
`spill` to have the excess items be spawned alongside the container.

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

Each entry can have more values (shown above as `...`).  They allow further properties of the item(s):

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
"container-group": "<group-id>",
"entry-wrapper": "<item-id>",
"sealed": <boolean>
"variant": <string>
"artifact": <object>
"event": <string>
"snippets": <string>
```

`contents` is added as contents of the created item.  It is not checked if they can be put into the item.  This allows water, that contains a book, that contains a steel frame, that contains a corpse.

`count` makes the item spawn repeat, each time creating a new item.

`charges`: Setting only min and not max will make the game calculate the max charges based on container or ammo/magazine capacity. Setting max too high will decrease it to the maximum capacity. Not setting min will set it to 0 when max is set.

`sealed`: If true, a container will be sealed when the item spawns.  Default is `true`.

`variant`: A valid itype variant id for this item.

`event`: A reference to a holiday in the `holiday` enum. If specified, the entry only spawns during the specified real holiday. This works the same way as the seasonal title screens, where the holiday is checked against the current system's time. If the holiday matches, the item's spawn probability is taken from the `prob` field. Otherwise, the spawn probability becomes 0.

`snippets`: If item uses `snippet_category` instead of description, and snippets contain ids, allow to pick a specific description of an item to spawn; see [JSON_INFO.md#snippets](JSON_INFO.md#snippets)

Current possible values are:
- "none" (Not event-based. Same as omitting the "event" field.)
- "new_year"
- "easter"
- "independence_day"
- "halloween"
- "thanksgiving"
- "christmas"

`artifact`: This object determines that the item or group that is spawned by this entry will become an artifact. Here is an example:
```json
"artifact": { "procgen_id": "cult", "rules": { "power_level": 1000, "max_attributes": 5, "max_negative_power": -2000 } }
```
The procgen_id relates directly to a `relic_procgen_data` object's id. The `rules` object has three parts.  The first is `power_level`, which is the target power level of the spawned artifact; an artifact's power level is the sum of the power levels of all the parts.  The second, `max_negative_power`, is the sum of only negative power levels of the parts.  The third, `max_attributes`, is the number of parts.

```json
"damage-min": 0,
"damage-max": 3,
"count": 4
"charges": [ 10, 100 ]
```

This will create 4 items; they can have different damage levels as the damage value is rolled separately for each of these items.  Each item has charges (AKA ammo) in the range of 10 to 100 (inclusive); if the item needs a magazine before it can have charges, that will be taken care of for you.  Using an array (which must have 2 entries) for charges/count/damage is equivalent to writing explicit min and max values.  In other words, `"count": [a,b]` is the same as `"count-min": a, "count-max": b`.

The container is checked and the item is put inside the container, and the charges of the item are capped/increased to match the size of the container.

`entry-wrapper`: Used for spawning lots of non-stackable items inside a container.  Instead of creating a dedicated itemgroup for that, you can use this field to define that inside an entry.  Note that you may want to set `container-item` to null to override the item's default container.

### Ammo and Magazines

Here are some ways to make items spawn with/without ammo/magazines (note that `ammo-item` can
be specified for guns and magazines in the entries array to use a non-default ammo type):

* Specify an ammo/magazine chance (introduced in Section 2) for the entire item group.
  `ammo` specifies the percent chance that the entries will spawn fully loaded (if it needs a magazine, it will be added for you).
  `magazine` specifies the percent chance that the entries will spawn with a magazine.
  Both of these default to 0 if unspecified.

  Note that `ammo` and `magazine` only apply to tools, guns, and magazines.  Furthermore, they don't apply to tools whose entry explicitly specifies how much ammo (charges) to spawn with, or to tools whose JSON item definition specifies a random amount or a fixed, nonzero amount of initial charges.

  If any item groups are referenced from your item group, then their ammo/magazine chances are ignored, and yours are used instead.

*  Use `charges`, `charges-min`, or `charges-max` in the entries array.  A default magazine will be added for you if needed.

## Shortcuts

This:

```json
"items": [ "<id-1>", [ "<id-2>", 10 ] ]
```

means the same as:

```json
"entries": [ { "item": "<id-1>" }, { "item": "<id-2>", "prob": 10 } ]
```

In other words: a single string denotes an item id; an array (which must contain a string and a number) denotes an item id and a probability.

This is true for groups as well:

```json
"groups": [ "<id-1>", [ "<id-2>", 10 ] ]
```

This format does not support further properties of the created items - the probability is only optional for entries of collections!

The content of "entries", "items" and "groups" are all added if those members exist.  This will have the item `<id-1>` appear twice in the item group:

```json
  {
    "items": [ "<id-1>" ],
    "entries": [ { "item": "<id-1>" } ]
  }
```

Another example: The group "milk" spawns a container (taken from milk_containers) that contains milk (the maximal amount that fits into the container, because there is no defined charges value).

```json
  {
    "type" : "item_group",
    "id": "milk_containers",
    "subtype": "distribution",
    "items": [
      "bottle_plastic", "bottle_glass", "flask_glass",
      "jar_glass_sealed", "jar_3l_glass_sealed", "flask_hip", "55gal_drum"
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

## Adding to item groups

Mods can add entries to item groups by specifying a group with the same id that copies-from the previous group (`"copy-from": group_id`), and encompassing the added items within an `extend` block, like so:

```json
  {
    "type" : "item_group",
    "id": "milk_containers",
    "copy-from": "milk_containers",
    "subtype": "distribution",
    "extend": {
      "items": [
        "bottle_plastic", "bottle_glass", "flask_glass",
        "jar_glass_sealed", "jar_3l_glass_sealed", "flask_hip", "55gal_drum"
      ]
    }
  },
  {
    "type" : "item_group",
    "id": "milk",
    "copy-from": "milk",
    "subtype": "distribution",
    "extend": {
      "entries": [
        { "item": "milk", "container-group": "milk_containers" }
      ]
    }
  },
```

## Inlined item groups

In some places one can define an item group directly instead of giving the id of a group.  One cannot refer to that group elsewhere - it has no visible id (it has an unspecific/random id internally).  This is most useful when the group is very specific to the place it is used and won't ever appear anywhere else.

As an example: monster death drops (`death_drops` entry in the `MONSTER` object, see [JSON_INFO.md](https://github.com/CleverRaven/Cataclysm-DDA/blob/master/doc/JSON_INFO.md)) can do this.  If the monster is very specific (e.g. a special robot, a unique endgame monster), the item spawned upon its death won't (in that form) appear in any other group.

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

The inline group is read like any other group and one can use all the properties mentioned above.  Its `type` and `id` members are always ignored.

Instead of a full JSON object, one can also write a JSON array.  The default subtype is used and the array is read like the "entries" array (see above).  Each entry of that array must be a JSON object. Example:

```json
  {
    "death_drops": [
      { "item": "rag", "damage": 2 }, { "item": "bowling_ball" }
    ]
  }
```

----

### Notes

#### Testing

You can test your item groups in the game:

1. Load a game and call the debug menu
    > *TIP*: If a key isn't bound to the debug menu or you forgot it, use <kbd>ESC</kbd> > <kbd>1</kbd>.

2. Choose "Test Item Group".

3. Select the item group you want to debug.

    The game will then spawn items in that group 100 times and count the spawned items.  They'll then be displayed, sorted by their frequency.

    > *TIP*: You can filter anything in the debug menu using <kbd>/</kbd>.

You should not add items to the item group `EMPTY_GROUP`.  This group can be used when the game requires a group id, but you don't want to spawn any items there; it will never spawn items.

#### SUS

When adding items to item groups, attempt to locate or create **SUS item groups.**  SUS item groups are collections that contain a reasonable realistic distribution of items that might spawn in a given piece of storage furniture.  SUS stands for "specific use storage."  One of the aims of organizing item groups into SUS groups is to promote reusable tables that can be maintained and extended.

You can find existing SUS item groups at [/data/json/itemgroups/SUS](https://github.com/CleverRaven/Cataclysm-DDA/blob/master/data/json/itemgroups/SUS/).
