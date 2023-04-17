# How to give items a sensible postapoc_price value

### Pricing philosophy
Prices were based on the philosophy that there is a decent portion of humanity unwilling to regularly
brave the risks of city scavenging, that are always on the lookout for supplies to stay alive.
This mean food and medicine are relatively quite valuable compared to the goods you would use to acquire them.

**The standard unit of currency**, the free merchants note, is priced to be the equivalent to
**one piece of meat jerky (250 cents)**.

One piece of meat jerky will stay fresh for 24 days, a decent time, and contains 347 calories.

A survivor that bring back 2000 cents worth of goods back to the merchants every day is
doing well enough to eke out a subsistence living.

A survivor that brings back 4000 cents worth of goods every day is doing quite well for himself.

#### Items are priced on a combination of their availability and their utility.

Raw materials tend to either be worthless, or only somewhat worthwhile if you can get them in bulk ~(0-25)/kg.

Common items with no utility are worthless.

Common items with some theoretical utility are worth very little (~10).

Common items that are useful tend to be in the ~(250-2000) price range.

Rare items with no utility have some small value ~(10-50).

Rare items with some theoretical utility are worth ~(100-500).

Rare items with considerable use tend to have the highest variety in prices ~(1000-15000).

**No item should be worth more than 15000.** They might be worth more if the economy was larger but that's
pretty much the most anyone will be willing to spend on any one thing, no matter how nice it is.

#### Currently Implemented faction currencies

```json
  {
    "id": "FMCNote",
    "description": "The Free Merchant Certified Note, also known by names such as a 'c-note' or 'merch', is a currency based on old American bills.  Fifty dollar bills and larger are printed with a promissory note signed by the treasurer of the Free Merchants, along with a complex design.  The note explains that this can be exchanged for food, water, and other services through the Free Merchants in the Refugee Center.",
    "name": { "str": "merch" }, 
    "price_postapoc": 250
  },
  {
    "id": "RobofacCoin",
    "name": { "str": "Hub 01 Gold Coin" },
    "description": "This is a small but surprisingly heavy gold coin.  One side is etched with circuitry and the other side reads 'Hub 01 exchange currency'.",
    "price_postapoc": 5000
  },
	{
    "id": "FlatCoin",
    "name": { "str": "FlatCoin" },
    "description": "This is a coin that has been flattened in a novelty coin flattening machine.  The machine has been somewhat crudely altered so that the design - which appears to once have been Mickey Mouse - is overlaid with a handwritten emblem of a book.  There is some text that faintly reads 'Campus Exchange Token'.",
    "price_postapoc": 250
  },
  {
    "id": "signed_chit",
    "name": { "str": "chit" },
    "description": "This is a slip of paper signed by the issuer.",
    "price_postapoc": 250
  },
  {
    "id": "icon",
    "name": { "str": "icon" },
    "description": "This is a small picture, about the same size as an ID card, symbolizing a religious figure.  On the back, there is a text that faintly reads 'New England Church Community'.",
    "price_postapoc": 250
  }
```
	
##### Some benchmark prices

```json
  {
    "id": "antibiotics",
    "name": { "str_sp": "antibiotics" },
    "description": "A strong antibacterial medication designed to prevent or stop the spread of infection.  It's the safest way to cure any infections you might have.  One dose lasts twelve hours.",
    "price_postapoc": 40000,
    "charges": 15,
    "stack_size": 200,
    "flags": [ "NPC_SAFE", "IRREPLACEABLE_CONSUMABLE" ]
  },
  {
    "id": "boltcutters",
    "type": "TOOL",
    "name": { "str": "pair of bolt cutters", "str_pl": "pairs of bolt cutters" },
    "description": "This is a large pair of bolt cutters.  You could use them to cut padlocks or heavy gauge wire.",
    "price_postapoc": 250,
  },
  {
    "id": "armor_lightplate",
    "name": { "str": "plate armor" },
    "description": "A suit of Gothic plate armor.",
    "price_postapoc": 12000,
  },
  {
    "id": "bat_metal",
    "name": { "str": "aluminum bat" },
    "description": "An aluminum baseball bat, lighter than a wooden bat and a little easier to swing as a result.",
    "price_postapoc": 1250,
  },
```

#### Food pricing
Food is priced mostly for a combination of calories, how preservable they are, and how nutritious they are.

Meat jerky is 250.

Food that has more calories, is canned, and is very nutritious might be 500.

Food that has less calories, goes bad quickly, and is junk food, might be 50.

#### Stack size
For items that have a stack size, you need to **divide** the price by stack size find the price per unit of the item.

#### IRREPLACEABLE_CONSUMABLE Flag
Pre cataclysm consumables that can not be replaced can be given the flag:
```json
    "flags": [ "IRREPLACEABLE_CONSUMABLE" ],
```

This is used for things such as unreloaded ammo, medicine, and luxury consumables such as coffee and tea.

In the future we can implement a feature that will allow this items to increase in price over time.
