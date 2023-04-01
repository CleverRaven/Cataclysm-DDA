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
* perk menu expects a `menu entry` and a `confirmation menu`. These are explained bellow.

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