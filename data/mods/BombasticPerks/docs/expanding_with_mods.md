# Expanding Bombastic Perks With Mods


## Adding the perk
To add a perk in another mod you need to add the following JSON

*you don't need to worry about Bombastic Perks being in a mod list to include the following code. Without it included the perks just will not be available*


This adds the option to the Perk Menu. Add one object to responses per perk. They should have the same format as laid out in `contributing.md`.
``` json
[
  {
    "type": "talk_topic",
    "id": "TALK_PERK_MENU_MAIN",
    "responses": [
      {
        ...
      }
      {
        ...
      }
    ]
  }
]
```

Then you just need to implement your perks.