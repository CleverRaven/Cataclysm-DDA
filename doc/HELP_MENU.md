# Help Menu

The help menu consists of categorised scrollable help pages that would ideally explain everything a new survivor needs to know, as well as any information the game can't convey clearly in an immersive way.

## Categories

Each category is made up of a json object which for mods can be placed anywhere but for vanilla should be place in `/data/core`.

|    Field     |                                                                      Description                                                                                |
| ------------ | --------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `"type"`     | (required) must be `"help"`                                                                                                                                     |
| `"order"`    | (required) integer, place this category should appear relative to others from the same source, with 0 being first. Must be unique per source                    |
| `"name"`     | (required) string, name of the category                                                                                                                         |
| `"messages"` | (required) array of strings, each string respresents a paragraph containing information for this category, can use [color tags](COLOR.md#Color tags) and [keybind tags](#Keybind tags) |

### Keybind tags

Keybind tags take the form <press_keybind> where keybind corresponds to a keybind id, most of which are found in [keybindings.json](./data/raw/keybindings.json)

### Special tags

`<DRAW_NOTE_COLORS>` and `<HELP_DRAW_DIRECTIONS>` are special hardcoded tags that can be found in [help.cpp](./src/help.cpp)

### Example

```json
  {
    "type": "help",
    "order": 0,
    "name": "First Category Name",
    "messages": [ "Paragraph 1.", "Paragraph 2.", "Paragraph 3." ]
  },
```
