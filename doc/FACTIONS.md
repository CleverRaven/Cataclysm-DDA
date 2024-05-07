# NPC factions

An NPC faction looks like this:

```json
  {
    "type": "faction",
    "id": "free_merchants",
    "name": "The Free Merchants",
    "likes_u": 30,
    "respects_u": 30,
    "trusts_u": 30,
    "known_by_u": false,
    "size": 100,
    "power": 100,
    "fac_food_supply": { "calories": 115200, "vitamins": { "iron": 800, "calcium": 800, "vitC": 600 } },
    "lone_wolf_faction": true,
    "wealth": 75000000,
    "currency": "FMCNote",
    "price_rules": [
      { "item": "money_strap_FMCNote", "fixed_adj": 0 },
      { "category": "tools", "markup": 1.5 },
      {
        "group": "test_item_group",
        "markup": 2.0,
        "fixed_adj": 0.1,
        "condition": { "npc_has_var": "bool_allnighter_thirsty", "value": "yes" }
      }
    ],
    "relations": {
      "free_merchants": {
        "kill on sight": false,
        "watch your back": true,
        "share my stuff": true,
        "guard your stuff": true,
        "lets you in": true,
        "defends your space": true,
        "knows your voice": true
      },
      "old_guard": {
        "kill on sight": false,
        "watch your back": true,
        "share my stuff": false,
        "guard your stuff": true,
        "lets you in": true,
        "defends your space": true,
        "knows your voice": true
      },
      "hells_raiders": {
        "kill on sight": true
      }
    },
	"epilogues": [ { "power_min": 0, "power_max": 149, "id": "epilogue_faction_your_followers_0" } ],
    "description": "A conglomeration of entrepreneurs and businessmen that stand together to hammer-out an existence through trade and industry."
  },
```

Field                 | Meaning
--------------------- | --
`"type"`              | string, must be `"faction"`
`"id"`                | string, unique faction id
`"name"`              | string, the faction's common name
`"likes_u"`           | integer, the faction's starting opinion of the player.  `"likes_u"` can be increased or decreased in play.  If it goes below -10, members of the faction will be hostile.
`"respects_u"`        | integer, the faction's starting opinion of the player.  Has no meaningful effect in game and may be removed in the future.
`"trusts_u"`          | integer, the faction's starting trust of the player.  Determines which item groups are available for trade by NPCs in the faction.  Scales similarly to `"likes_u"` and `"respects_u"`.
`"known_by_u"`        | boolean, whether the player has met members of the faction.  Can be changed in play.  Unknown factions will not be displayed in the faction menu.
`"size"`              | integer, an approximate count of the members of the faction.  Has no effect in play currently.
`"power"`             | integer, an approximation of the faction's power.  Has no effect in play currently.
`"fac_food_supply"`   | integer, the number of calories (not kilocalories!) available to the faction. Has no effect in play currently.
`"vitamins"`          | array, *units* of vitamins available to this faction. This is not the same as RDA, see [the vitamins doc](VITAMIN.md) for more details. Has no effect in play currently.
`"wealth"`            | integer, number of post-apocalyptic currency in cents that that faction has to purchase stuff. Serves as an upper limit on the amount of items restocked by a NPC of this faction with a defined shopkeeper_item_group (see NPCs.md)
`"currency"`          | string, the item `"id"` of the faction's preferred currency.  Faction shopkeeps will trade faction current at 100% value, for both selling and buying.
`"price_rules"`       | array, allows defining `premium`, `markup`, `price` and/or `fixed_adj` for an `item`/`category`/`group`.<br/><br/>`premium` is a price multiplier that applies to both sides.<br/> `markup` is only used when an NPC is selling to the avatar and defaults to `1`.<br/>`price` replaces the item's `price_postapoc`.<br/>`fixed_adj` is used instead of adjustment based on social skill and intelligence stat and can be used to define secondary currencies.<br/><br/>Lower entries override higher ones. For conditionals, the avatar is used as alpha and the evaluating npc as beta
`"relations"`         | dictionary, a description of how the faction sees other factions.  See below
`"mon_faction"`       | string, optional.  The monster faction `"name"` of the monster faction that this faction counts as.  Defaults to "human" if unspecified.
`"lone_wolf_faction"` | bool, optional. This is a proto/micro faction template that is used to generate 1-person factions for dynamically spawned NPCs, defaults to "false" if unspecified.
`"description"`       | string, optional. The player's description of this faction as seen in the faction menu.
`"epilogues"`         | array of objects, optional. Requires these objects: `power_min` - minimal faction power for epilogue to appear, `power_max` - maximum faction power for epilogue to appear, `id` - id of text snippet containing text of epilogue.

## Scale of faction values
Interacting with factions has certain effects on how the faction sees the player. These are reflected in values like `likes_u`, `respects_u` and `trusts_u`. Here's a (non-comprehensive) list to provide some context on how much these values are worth:

| Type of interaction                         | Effect on `likes_u` | Effect on `respects_u` | Effect on `trusts_u` |
| ------------------------------------------- | ------------------- | ---------------------- | -------------------- |
| Neutral state                               |                   0 |                      0 |                    0 |
| Player is warned by faction                 |                 - 1 |                    - 1 |                  - 1 |
| Player delivers food supply to faction camp | + food nutritional value / 1250 | + food nutritional value / 625 | + food nutritional value / 625 |
| Player triggers a mutiny                    |  `likes_u` / 2 + 10 |                    - 5 |                  - 5 |
| Player angers an NPC                        |                 - 5 |                    - 5 |                  - 5 |
| Player completes a mission                  |                + 10 |                   + 10 |                 + 10 |


## Faction relations
Factions can have relations with each other that apply to each member of the faction.  Faction relationships are not reciprocal: members of the Free Merchants will defend members of the Lobby Beggars, but members of the Lobby Beggars will not defend members of the Free Merchants.

Faction relationships are stored in a dictionary, with the dictionary keys being the name of the faction and the values being a second dictionary of boolean flags.  All flags are optional and default to false.  The faction with the dictionary is the acting faction and the other faction is the object faction.

The flags have the following meanings:

Flag                   | Meaning
---------------------- | --
`"kill on sight"`      | Members of the acting faction are always hostile to members of the object faction.
`"watch your back"`    | Members of the acting faction will treat attacks on members of the object faction as attacks on themselves.
`"share my stuff"`     | Members of the acting faction will not object if members of the object faction take items owned by the acting faction.
`"guard your stuff"`   | Members of the acting faction will object if someone takes items owned by the object faction.
`"lets you in"`        | Members of the acting faction will not object if a member of the object faction enters territory controlled by the acting faction.
`"defends your space"` | Members of the acting faction will become hostile if someone enters territory controlled by the object faction.
`"knows your voice"`   | Members of the acting faction will not comment on speech by members of the object faction.

So far, only `"kill on sight"`, `"knows your voice"`, and `"watch your back"` have been implemented.
