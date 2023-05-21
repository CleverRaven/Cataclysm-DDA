# Contributing to Bombastic Perks

## Rules for all content
* All contributions to the mod should revolve around perks, and core game items. Perks can give items, but a perk can't require a custom item that isn't provided by the perk.
* All perks must be just kinda okay.
* Any perk that's more than just kinda okay needs downsides or to be complex / niche.
* Perks should try to be funny and lean into the absurdity.

## Naming Convention
* Perks should have an ID of: `perk_[name_of_perk]`
* Confirmation menu IDs should be in the format `TALK_PERK_MENU_[NAME_OF_PERK]`

## Adding
When adding stuff to the mod you should organize your files as follows:
* the perks (which are implemented as mutations) go in the `perks.json` file.
* any additional things for your perk: items, effects, spells, EOCs should be gathered in a single file called `[name_of_perk].json` in the `perkdata` folder.
* perk menu expects a `menu entry` and a `confirmation menu` which go in `perkmenu.json`. These are explained bellow:

### Menu Entry
Each perk needs to be selectable in the menu itself. To do this append a response to `TALK_PERK_MENU_MAIN` with the following format:
``` json 
{
    "condition": { "not": { "u_has_trait": "[YOUR_PERK_ID]" } },
    "text": "Gain [<trait_name:perk_STR_UP>]",
    "topic": "TALK_PERK_MENU_STRONGER"
}
```

### Confirmation Menu
Each perk needs a confirmation menu entry where it explains what the perk does and lets you confirm selection. That entry should be formatted as follows. Replace [YOUR_PERK_ID] with your perks id, and  [YOUR_PERK_NAME] with your perks name:
``` json
{
  "type": "talk_topic",
  "id": "TALK_PERK_MENU_[YOUR_PERK_NAME]",
  "dynamic_line": "<trait_name:[YOUR_PERK_ID]>: \"<trait_description:[YOUR_PERK_ID]>\"",
  "responses": [
    {
      "text": "Select Perk.",
      "topic": "TALK_PERK_MENU_MAIN",
      "condition": { "compare_num": [ { "u_val": "var", "var_name": "num_perks" }, ">", { "const": 0 } ] },
      "failure_explanation": "Requirements Not Met",
      "failure_topic": "TALK_PERK_MENU_FAIL",
      "effect": [
        { "u_add_trait": "[YOUR_PERK_ID]" },
        {
          "arithmetic": [ { "u_val": "var", "var_name": "num_perks" }, "=", { "u_val": "var", "var_name": "num_perks" }, "-", { "const": 1 } ]
        }
      ]
    },
    { "text": "Go Back.", "topic": "TALK_PERK_MENU_MAIN" },
    { "text": "Quit.", "topic": "TALK_DONE" }
  ]
}
```

### With Requirements
This is an example of a confirmation menu that also has requirements, as above but you also need to include a conditional for [YOUR CONDITIONAL GOES HERE] and you need to describe it in the dynamic line filling in [DESCRIPTION OF REQUIREMENTS]:
``` json
{
  "type": "talk_topic",
  "id": "TALK_PERK_MENU_[YOUR_PERK_NAME]",
  "dynamic_line": "<trait_name:[YOUR_PERK_ID]>: \"<trait_description:[YOUR_PERK_ID]>\"\nRequires [DESCRIPTION OF REQUIREMENTS]",
  "responses": [
    {
      "text": "Select Perk.",
      "topic": "TALK_PERK_MENU_MAIN",
      "condition": {
        "and": [
          {
            "or": [
              { "compare_num": [ { "u_val": "var", "var_name": "no_prerecs" }, ">", { "const": 0 } ] },
              {
                { [YOUR CONDITIONAL GOES HERE] }
              }
            ]
          },
          { "compare_num": [ { "u_val": "var", "var_name": "num_perks" }, ">", { "const": 0 } ] }
        ]
      },
      "failure_explanation": "Requirements Not Met",
      "failure_topic": "TALK_PERK_MENU_FAIL",
      "effect": [
        { "u_add_trait": "[YOUR_PERK_ID]" },
        {
          "arithmetic": [ { "u_val": "var", "var_name": "num_perks" }, "=", { "u_val": "var", "var_name": "num_perks" }, "-", { "const": 1 } ]
        }
      ]
    },
    { "text": "Go Back.", "topic": "TALK_PERK_MENU_MAIN" },
    { "text": "Quit.", "topic": "TALK_DONE" }
  ]
}
```

### With Variants
If a perk has multiple variants that are mutually exclusive you should have a single menu entry that leads to all options on display:
``` json
{
  "type": "talk_topic",
  "id": "TALK_PERK_MENU_[NAME_OF_VARIANTS]",
  "dynamic_line": "<trait_name:[YOUR_PERK_ID1]>: \"<trait_description:[YOUR_PERK_ID1]>\"\nRequires [DESCRIPTION OF REQUIREMENTS1]\n<trait_name:[YOUR_PERK_ID2]>: \"<trait_description:[YOUR_PERK_ID2]>\"\nRequires [DESCRIPTION OF REQUIREMENTS2]",
  "responses": [
    {
      "text": "Select Perk Variant <trait_name:[YOUR_PERK_ID1]>.",
      "topic": "TALK_PERK_MENU_MAIN",
      "condition": {
        "and": [
          {
            "or": [
              { "compare_num": [ { "u_val": "var", "var_name": "no_prerecs" }, ">", { "const": 0 } ] },
              {
                { [YOUR CONDITIONAL GOES HERE] }
              }
            ]
          },
          { "compare_num": [ { "u_val": "var", "var_name": "num_perks" }, ">", { "const": 0 } ] }
        ]
      },
      "failure_explanation": "Requirements Not Met",
      "failure_topic": "TALK_PERK_MENU_FAIL",
      "effect": [
        { "u_add_trait": "[YOUR_PERK_ID1]" },
        {
          "arithmetic": [ { "u_val": "var", "var_name": "num_perks" }, "=", { "u_val": "var", "var_name": "num_perks" }, "-", { "const": 1 } ]
        }
      ]
    },
    {
      "text": "Select Perk Variant <trait_name:[YOUR_PERK_ID2]>.",
      "topic": "TALK_PERK_MENU_MAIN",
      "condition": {
        "and": [
          {
            "or": [
              { "compare_num": [ { "u_val": "var", "var_name": "no_prerecs" }, ">", { "const": 0 } ] },
              {
                { [YOUR CONDITIONAL GOES HERE] }
              }
            ]
          },
          { "compare_num": [ { "u_val": "var", "var_name": "num_perks" }, ">", { "const": 0 } ] }
        ]
      },
      "failure_explanation": "Requirements Not Met",
      "failure_topic": "TALK_PERK_MENU_FAIL",
      "effect": [
        { "u_add_trait": "[YOUR_PERK_ID2]" },
        {
          "arithmetic": [ { "u_val": "var", "var_name": "num_perks" }, "=", { "u_val": "var", "var_name": "num_perks" }, "-", { "const": 1 } ]
        }
      ]
    },
    { "text": "Go Back.", "topic": "TALK_PERK_MENU_MAIN" },
    { "text": "Quit.", "topic": "TALK_DONE" }
  ]
}
```

## Reasoning For The Above
* If everyone stays organized the mod should be easy to read, and contribute to.
* Players should be able to see all the perks that they could possibly get, and read their effects (even if they can't select them)
* The menu should have as little clutter as possible