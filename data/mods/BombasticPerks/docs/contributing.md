# Contributing to Bombastic Perks

## Rules for all content
* All contributions to the mod should revolve around perks, and core game items. Perks can give items, but a perk can't require a custom item that isn't provided by the perk.
* Standard perks must be just kinda okay.
* Any standard perk that's more than just kinda okay needs downsides or to be complex / niche.
* Playstyle perks can be much more powerful since players can potentially disable them.
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
You need to fill in the:
`PERK_ID`: The requirements text and the perk condition.
`REQUIREMENTS TEXT`: A description of the requirements to select the perk. If you don't have requirements put: "No Requirements".
`CONDITION_GOES_HERE`: The EOC conditional that is required to select the perk. If you don't have a condition put: "math": [ "0", "==", "0" ].
`ANY ADDITIONAL INFO TEXT`: Any additional info you want to include. If you don't have any additional details put "".
``` json 
{
  "condition": { "not": { "u_has_trait": "PERK_ID" } },
  "text": "Gain [<trait_name:PERK_ID>]",
  "effect": [
    { "set_string_var": "<trait_name:PERK_ID>", "target_var": { "context_val": "trait_name" } },
    { "set_string_var": "<trait_description:PERK_ID>", "target_var": { "context_val": "trait_description" } },
    { "set_string_var": "PERK_ID", "target_var": { "context_val": "trait_id" } },
    { "set_string_var": "REQUIREMENTS TEXT", "target_var": { "context_val": "trait_requirement_description" } },
    { "set_condition": "perk_condition", "condition": { CONDITION_GOES_HERE } },
    {
      "set_string_var": "ANY ADDITIONAL INFO TEXT",
      "target_var": { "context_val": "trait_additional_details" }
    }
  ],
  "topic": "TALK_PERK_MENU_SELECT"
}
```

## Reasoning For The Above
* If everyone stays organized the mod should be easy to read, and contribute to.
* Players should be able to see all the perks that they could possibly get, and read their effects (even if they can't select them)
* The menu should have as little clutter as possible
