# Item Categories

The following table lists base game item categories in their default sort order.  When adding new items, please consult this table and make sure the item gets included in the most appropriate category so it will be listed/sorted how players will expect.

Some items will match the descriptions for multiple categories.  Notes indicating which category should take prioriy have been added for commonly expected overlaps otherwise, just use your common sense.

NAME | ID | Description
---- | -- |---------
GUNS | guns |Any handheld ranged weapon.
BOWS | bows |Ranged weapons that launch projectiles by the sudden righting of a flexed material.  ( Takes priority over GUNS )
MAGAZINES | magazines | Ammunition storage and feeding device for a ranged weapon. ( Takes priority over CONTAINERS )
THROWING | throwing | Throwing weapons / things meant to be thrown.
WEAPONS | weapons | Items designed to be or whose main use is to be a melee weapon.
<!--- TRAPS | traps | Items or kits that place 'traps' when 'a'ctivated. (Takes priority over DEPLOYABLES) --->
DEPLOYABLES | deployables | Items that get placed when 'a'ctivated.
TOOLS | tools | Items readily recognizable as a tool or are otherwise meant to be used.
CRAFTING TOOLS | tools_craft | Tools designed or used specifically for crafting.  (Takes priority over TOOLS)
EQUIPMENT | equipment | Wearable items with functions or uses beyond that of being clothing or armor.
ARMOR | armor | Wearable items ment to protect the wearer from harm.  (Takes priority over CLOTHING)
CLOTHING | clothing | Wearable items.
MEDICAL STUFF | medical | Medical items and drugs.
<!--- FOOD-PERISH | food_perish | Food with a relatively short shelf life. ( < 10 days ) (Priority subset of FOOD) --->
<!--- DRINKS-PERISH | drinks_perish | Drinks with a relatively short shelf life. ( < 10 days ) (Priority subset of DRINKS) --->
FOOD | food | Anything a survivor can (reasonably) eat and gain nutrients  from. 
DRINKS | drinks | Anything a survivor can (reasonibly) drink and gain nutrients  or hydration from.
SNACKS | snacks | Food that doesn't have much of a nutrition value .  You know… munchies. ( <100 Kcal ) (Takes priority over FOOD)
BOOZE | booze | Achoholic beverages. (Takes priority over DRINKS)
DRUGS | drugs | Drugs or substances that are not intended for medical use. (Takes priority over CHEMICAL STUFF)
AMMO | ammo | Anything meant to be fired, dropped, scattered or detonated from any weapon. (Takes priority over CHARGES)
FUEL | fuel | Liquid or gas fuels. (Takes priority over CHARGES)
CHARGES | charges | Stuff consumed by other things.
CONTAINERS | containers | Stuff designed to hold other stuff.
MUTAGENS | mutagen | Anything that causes mutations. (mutations from radiation don't count)
BIONICS | bionics | Compact Bionic Modules. (CBMs)
MODS | mods | Things a survivor can attach to other items.
TRANSITIONAL | transitional | Things that are either timed (tanning, fermenting etc.) or awaiting a survivor action.
SPARE PARTS | spare_parts | Parts or pieces that can be 'assembled' or 'placed' when building or crafting. (Takes priority over MATERIALS)
MATERIALS | materials | Raw materials for either building or crafting.
SEEDS | seeds | Things that a survivor can (in theory) make grow something other than mold. (Takes priority over PLANTS)
PLANTS | plants | Stuff harvested from plants that isn't readily or traditionally recognizable as food.
CHEMICAL STUFF | chems | Chemicals and other substances that don't fall into other categories.
PACKED | packed | Items that are either folded up or packed into a bundle to reduce space.
<!--- MANUALS | manuals | Things that a survivor can read and learn from.  (Takes priority over BOOKS) --->
BOOKS | books | Things a survivor can read.
ENTERTAINMENT | entertain | Fun stuff that is designed only to be fun and not useful or dangerous like flamethrowers.
VEHICLE PARTS | vehicle_parts | Spare parts for a vehicle. (Takes priority over SPARE PARTS)
<!--- VEHICLE ELECTRONICS | v_elec | Electronic vehicle parts or those directly related to a vehicle's electronics.  (Takes priority over VEHICLE PARTS) --->
FRAMES | frames | Things that can be used as a vehicle's base.  (Takes priority over VEHICLE PARTS)
<!--- MOTORS | motors | Motors, engines or anything else that can make a vehicle move.  (Takes priority over VEHICLE PARTS) --->
<!--- PLATING  | plating | Armor plating.  (Takes priority over VEHICLE PARTS) --->
<!--- VEHICLE RIGS | rigs | Parts meant/designed to be installed on/in a vehicle to provide extra functionality.  (Takes priority over VEHICLE PARTS) --->
VEHICLE WEAPONS | v_weapons | Weapons meant/designed to be mounted on a vehicle or is too big for a survivor to reasonbly use as a handheld weapon. (Takes priority over VEHICLE PARTS)
<!--- VEHICLE WHEELS | wheels | Items that can be attached to a vehicle and used as wheels.  (Takes priority over VEHICLE PARTS) --->
ARTIFACTS | artifacts | SPOILER! If you don't already know what belongs here you have no buisness knowing.
SALVAGE | salvage | Things that to a survivor exist solely to be scrapped, rebuilt or disassembled for parts.
OTHER | other | The fallback category for anything that can't be classified.
<!--- TRASH | trash | Anything that is completely useless to a survivor. --->

`TODO: Add another column to the table with examples for each category.`

## Default Classifications

Item category is an optional field in the json files.  Below is an explanation  of how the default categorisation logic works.

#### TODO (once logic gets expanded for new categories)

copy paste from shortcut taking Pyhton sctipt:
{"GENERIC": "OTHER",
                "ENGINE": "VEHICLE PARTS",
                "WHEEL": "VEHICLE PARTS",
                "TOOLMOD": "",
                "GUN": "GUN",
                "AMMO": "AMMO",
                "MAGAZINE": "MAGAZINE",
                "CONTAINER": "OTHER",
                "ARMOR": "CLOTHING",
                "TOOL": "TOOL",
                "BIONIC_ITEM": "BIONICS",
                "COMESTIBLE": "DRUGS/FOOD",
                "TOOL_ARMOR": "CLOTHING",
                "GUNMOD": "MODS",
                "BOOK": "BOOKS"}
