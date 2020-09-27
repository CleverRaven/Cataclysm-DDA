# ter_furn_transform

A ter_furn_transform is a type of json object that allows you to specify a transformation of a tile from one terrain to another terrain, and from one furniture to another furniture.

```json
[
  {
    "type": "ter_furn_transform",
    "id": "example",
    "terrain": [
      {
        "result": "t_dirt",
        "valid_terrain": [ "t_sand" ],
        "message": "sandy!",
        "message_good": true
      }
    ]
  }
]
```

The example above turns "sand" into "dirt". It does so by comparing the direct terrain ids. In addition, we can add a fail message to the transform. 
If, however, we wanted to turn sand into "dirt or grass" we can do:

```json
"terrain": [
  {
    "fail_message": "no sand!",
    "result": [ "t_dirt", "t_grass" ],
    "valid_terrain": [ "t_sand" ],
    "message": "sandy!"
  }
]
```

message_good is optional and defaults to true. 
This example chooses either dirt or grass at a 1:1 ratio. But, if you want a 4:1 ratio:

```json
"terrain": [
  {
    "result": [ [ "t_dirt", 4 ], "t_grass" ],
    "valid_terrain": [ "t_sand" ],
    "message": "sandy!"
  }
]
```

As you can see, you can mix and match arrays with weights with single strings. Each single string has a weight of 1.

All of the above applies to furniture as well.

```json
"furniture": [
  {
    "result": [ [ "f_null", 4 ], "f_chair" ],
    "valid_furniture": [ "f_hay", "f_woodchips" ],
   "message": "I need a chair"
  }
]
```

You can also use flags instead of specific IDs for both furniture and terrain.

```json
"terrain": [
  {
    "result": "t_dirt",
    "valid_flags": [ "DIGGABLE" ],
    "message": "digdug"
  }
]
```

A ter_furn_transform can have both terrain and furniture fields. It treats them separately, so no "if dirt, add chair."
