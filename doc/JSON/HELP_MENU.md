# Help Menu

The help menu consists of scrollable categorised help pages that would ideally explain everything a new survivor needs to know, as well as any information the game can't convey clearly in an immersive way.

## Categories

Each category is made up of a json object which for mods can be placed anywhere but for vanilla should be placed in `/data/core`.

|    Field     |         Type                               | Description                                                                                                                                                                                                                                                                               |
| ------------ | ---------------------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `"type"`     | (required) string                          | Must be `"help"`                                                                                                                                                                                                                                                                          |
| `"order"`    | (required) integer                         | Where this category should appear in the list relative to others from the same source, with 0 being first. Must be unique per source (Every mod can start from 0). Each mods categories are appended to the end of the list per their load order.                                         |
| `"name"`     | (required) string                          | Name of the category, can use [color tags](/doc/user-guides/COLOR.md#color-tags)                                                                                                                                                                                                          |
| `"messages"` | (required) array of strings and/or objects | See below for objects. Each string respresents a new line seperated paragraph containing information for this category, can use [color tags](/doc/user-guides/COLOR.md#color-tags) and [tags](/doc/JSON/NPC.md##special-custom-entries) notably including keybind tags. Can contain `\n`. |

### Message objects
You can use an object with one of these

|         Field        |            Type             | Description                                                                                                             |
| -------------------- | ------------------------- | ------------------------------------------------------------------------------------------------------------------- |
| `"subtitle"`         | (mutually exclusive) string | Formats the string in a box character drawing in a similar way to the title. Includes an imgui separator.               |
| `"force_monospaced"` | (mutually exclusive) string | Forces the string to be printed using a monospaced font for text drawings and diagrams (Currently still has \n padding) |
| `"separator"`        | (mutually exclusive) string | Prints an imgui separator. String must be the name of a valid [color](/doc/user-guides/COLOR.md#possible-colors)        |

### Example

```jsonc
  {
    "type": "help",
    "order": 0,
    "name": "First Category Name",
    "messages": [
      "A useful tip.",
      { "separator": "red" },
      "<color_red>A really important warning.</color>",
      { "separator": "red" },
      "A list of three keybinds.\n<keybind:pause> lets you pass one second.\n<keybind:wait> lets you wait for longer.\n<keybind:sleep> lets you sleep.",
      { "subtitle": "And Now For Something Completely Different" },
      {
        "force_monospaced": "   █▀▀▀▀▀▓         \n   ▌     ▓O        \n   ▌     ▓│⌐~~  ∩∩ \n  ▄▌     ▓▄\\  ~~│oD\n/┬\\═══════/┬\\  ─── \n├Θ┤       ├Θ┤ │   │\n\\┴/       \\┴/ ││ ││\n"
      }
    ]
  },
```
Which will be displayed as:

![help example](https://github.com/user-attachments/assets/47b035db-c801-4b96-bc53-d694ff4a79f7)

