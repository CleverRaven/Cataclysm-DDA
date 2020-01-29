# NPC factions

An NPC faction looks like this:

```json
  {
    "type": "faction",
    "id": "free_merchants",
    "name": "The Free Merchants",
    "likes_u": 30,
    "respects_u": 30,
    "known_by_u": false,
    "size": 100,
    "power": 100,
    "food_supply": 115200,
    "lone_wolf_faction": true,
    "wealth": 75000000,
    "currency": "FMCNote",
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
    "description": "A conglomeration of entrepreneurs and businessmen that stand together to hammer-out an existence through trade and industry."
  },
```

Field | Meaning
-- | --
`"type"` | string, must be `"faction"`
`"id"` | string, unique faction id
`"name"` | string, the faction's common name
`"likes_u"` | integer, the faction's starting opinion of the player.  `"likes_u"` can be increased or decreased in play.  If it goes below -10, members of the faction will be hostile.
`"respects_u"` | integer, the faction's starting opinionof the player.  Has no meaningful effect in  game and may be removed in the future.
`"known_by_u"` | boolean, whether the player has met members of the faction.  Can be changed in play.  Unknown factions will not be displayed in the faction menu.
`"size"` | integer, an approximate count of the members of the faction.  Has no effect in play currently.
`"power"` | integer, an approximation of the faction's power.  Has no effect in play currently.
`"food_supply"` | integer, the number of calories available to the faction.  Has no effect in play currently.
`"wealth"` | integer, number of post-apocalyptic currency in cents that that faction has to purchase stuff.
`"currency"` | string, the item `"id"` of the faction's preferred currency.  Faction shopkeeps will trade faction current at 100% value, for both selling and buying.
`"relations"` | dictionary, a description of how the faction sees other factions.  See below
`"mon_faction"` | string, optional.  The monster faction `"name"` of the monster faction that this faction counts as.  Defaults to "human" if unspecified.
`"lone_wolf_faction"` | bool, optional. This is a proto/micro faction template that is used to generate 1-person factions for dynamically spawned NPCs, defaults to "false" if unspecified.

## Faction relations
Factions can have relations with each other that apply to each member of the faction.  Faction relationships are not reciprocal: members of the Free Merchants will defend members of the Lobby Beggars, but members of the Lobby Beggars will not defend members of the Free Merchants.

Faction relationships are stored in a dictionary, with the dictionary keys being the name of the faction and the values being a second dictionary of boolean flags.  All flags are optional and default to false.  The faction with the dictionary is the acting faction and the other faction is the object faction.

The flags have the following meanings:

Flag | Meaning
-- | --
`"kill on sight"` | Members of the acting faction are always hostile to members of the object faction.
`"watch your back"` | Members of the acting faction will treat attacks on members of the object faction as attacks on themselves.
`"share my stuff"` | Members of the acting faction will not object if members of the object faction take items owned by the acting faction.
`"guard your stuff"` | Members of the acting faction will object if someone takes items owned by the object faction.
`"lets you in"` | Members of the acting faction will not object if a member of the object faction enters territory controlled by the acting faction.
`"defends your space"` | Members of the acting faction will become hostile if someone enters territory controlled by the object faction.
`"knows your voice"` | Members of the acting faction will not comment on speech by members of the object faction.

So far, only `"kill on sight"`, `"knows your voice"`, and `"watch your back"` have been implemented.
