# Expanding Bombastic Perks With Mods


## Adding the perk
To add a perk in another mod you need to add the following JSON

*you don't need to worry about Bombastic Perks being in a mod list to include the following code. Without it included the perks just will not be available*


This adds the option to the Perk Menu. Add one object to responses per perk. Replace [YOUR_PERK_ID] with your perks id, and  TALK_PERK_MENU_[YOUR_MENU_ENTRY] with the name of the menu entry for your confirm screen.
``` json
[
  {
    "type": "talk_topic",
    "id": "TALK_PERK_MENU_MAIN",
    "responses": [
      {
        "condition": { "not": { "u_has_trait": "[YOUR_PERK_ID]" } },
        "text": "Gain [<trait_name:[YOUR_PERK_ID]>]",
        "topic": "TALK_PERK_MENU_[YOUR_MENU_ENTRY]"
      }
      {
        ...
      }
    ]
  }
]
```

Each menu entry needs a confirm screen with the following code:
``` json
{
  "type": "talk_topic",
  "id": "TALK_PERK_MENU_[YOUR_MENU_ENTRY]",
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

Then you just need to implement your perks.