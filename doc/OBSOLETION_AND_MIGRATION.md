
If you want to remove some item, it is rarely as straightforward as "remove the item json and call it a day". Everything that is stored in save file (like furniture, monsters or items, but not item or monstergroups) may cause harmless, but annoying errors when thing is removed. This document describe how to properly obsolete most `type`s of json we have in the game.

Migration is used, when we want to remove one item by replacing it with another item, that do exist in the game, or to maintain a consistent list of item type ids. After migration, the item can be safely removed - remember to remove all mentions of entity elsewhere

Migration and obsoletion should happen in `\data\json\obsoletion_and_migration_<current_stable_version>` folder

# Item migration

Any of item types (AMMO, GUN, ARMOR, PET_ARMOR, TOOL, TOOLMOD, TOOL_ARMOR, BOOK, COMESTIBLE, ENGINE, WHEEL, GUNMOD, MAGAZINE, GENERIC, BIONIC_ITEM) should be migrated to another item, then it can be removed with no issues
AMMO and COMESTIBLE types require additional handling, explained in [Charge and temperature removal](#Charge_and_temperature_removal)

```json
{
  "type": "MIGRATION", // type, mandatory
  "id": "arrowhead",  // id of item, that you want to migrate, mandatory
  "from_variant": "scar_l", // if used, only specific variant of this item would be removed instead of an item
  "replace": "steel_chunk", // item, that replace the removed item, mandatory
  "variant": "m1014", // variant of an item, that would be used to replace the item. only for items, that do use variants
  "flags": [ "DIAMOND" ], // additional flag, that item would have when replaced
  "charges": 1930, // amount of charges, that replaced item would have
  "contents": [ { "id": "dogfood", "count": 1 } ], // if `replace` is container, describes what would be inside of it
  "sealed": false, // if `replace` is container, will it be sealed or not
  "reset_item_vars": true // if original item has variables stored inside, and this field is true, this variables won't be migrated to the next item
}
```

# Charge and temperature removal

If an item that used to have charges (e.g. `AMMO` or `COMESTIBLE` types) is changed to another type that does not use charges, migration is needed to ensure correct behavior when loading from existing save files, and prevent spurious error messages from being shown to the player.  Migration lists for this are found in `charge_removal.json`.

Such items may be added to one of the following:

`blacklist_charge_migration.json` a `charge_migration_blacklist` list:
Items in existing save files with `n` charges will be converted to `n` items with no charges.  This will preserve item count.

`blacklist_charge_removal.json` a `charge_removal_blacklist` list
* `charge_removal_blacklist`: items will simply have charges removed.

Additionally, `COMESTIBLE` items have temperature and rot processing, and are thus set as always activated.  When an item is changed from `COMESTIBLE` to a different type, migration is needed to check and unset this if applicable:

`blacklist_temperature_removal.json` a `temperature_removal_blacklist` list:

* In most cases, the item has no other features that require it to remain activated, in which case it can be simply added to `temperature_removal_blacklist`.  Items in this list will be deactivated and have temperature-related data cleared *without any further checks performed*.
* In case of an item that may be active for additional reasons other than temperature/rot tracking, an instance of the item loaded from existing save file cannot be blindly deactivated -- additional checks are required to see if it should remain active.  Instead of adding to the above list, a separate special case should be added in `src/savegame_json.cpp` to implement the necessary item-specific deactivation logic.

# Vehicle part migration
Migrating vehicle parts is done using `vehicle_part_migration` type, in the example below - when loading the vehicle any part with id `from` will have it's id switched to `to`.
For `VEH_TOOLS` parts only - `add_veh_tools` is a list of itype_ids to add to the vehicle tools after migrating the part.
Vehicle part can be removed safely afterwards

```json
  {
    "type": "vehicle_part_migration",
    "//": "migration to VEH_TOOLS, remove after 0.H release",
    "from": "afs_metal_rig",
    "to": "veh_tools_workshop",
    "add_veh_tools": [ "welder", "soldering_iron", "forge", "kiln" ]
  }
```

# Bionic migration
For bionics, you should use `bionic_migration` type. The migration happens when character is loaded; if `to` is `null` or is not defined, the bionic is removed, if `to` is not null the id will be changed to the provided value.

```json
  {
    "type": "bionic_migration",
    "from": "bio_tools_extend",
    "to": "bio_pitch_perfect"
  },
  {
    "type": "bionic_migration",
    "from": "bio_tools_extend"
  }
```

# Trait migration

A mutation migration can be used to migrate a mutation that formerly existed gracefully into a proficiency, another mutation (potentially a specific variant), or to simply remove it without any fuss.

```json
  {
    "type": "TRAIT_MIGRATION",      // Mandatory. String. Must be "TRAIT_MIGRATION"
    "id": "hair_red_mohawk",        // Mandatory. String. Id of the trait that has been removed.
    "trait": "hair_mohawk",         // Optional*. String. Id of the trait this trait is being migrated to.
    "variant": "red",               // Optional. String. Can only be specified if trait is specified. Id of a variant of trait that this mutation will be set to.
    "proficiency": "prof_aaa",      // Optional*. String. Id of proficiency that will replace this trait.
    "remove": true                  // Optional*. Boolean. If neither trait or variant are specified, this must be true. 
                                    // *One of these three must be specified.
  },
  {
    "type": "TRAIT_MIGRATION",
    "id": "dead_trait1",
    "trait": "new_trait",
    "variant": "correct_variant"
  },
  {
    "type": "TRAIT_MIGRATION",
    "id": "trait_now_prof",
    "proficiency": "prof_old_trait"
  },
  {
    "type": "TRAIT_MIGRATION",
    "id": "deleted_trait",
    "remove": true
  }
```

# Monster migration

Monster can be removed without migration, the game replace all critters without matched monster id with mon_breather when loaded

# Recipe migration

Recipes can be removed without migration, the game will simply delete the recipe when loaded

# Terrain and furniture migration

Terrain and furniture migration replace the provided id as submaps are loaded. You can use `f_null` with `to_furn` to remove furniture entirely without creating errors, however `from_ter` must specify a non null `to_ter`.

```json
{
    "type": "ter_furn_migration",   // Mandatory. String. Must be "ter_furn_migration"
    "from_ter": "t_old_ter",        // Optional*. String. Id of the terrain to replace.
    "from_furn": "f_old_furn",      // Optional*. String. Id of the furniture to replace.
    "to_ter": "t_new_ter",          // Mandatory if from_ter specified, optional otherwise. String. Id of new terrain to place. Overwrites existing ter when used with from_furn.
    "to_furn": "f_new_furn",        // Mandatory if from_furn specified, optional otherwise. String. Id of new furniture to place.
                                    // *Exactly one of these two must be specified.
},
```

## Examples

Replace a fence that bashes into t_dirt into a furniture that can be used seamlessly over all terrains

```json
  {
    "type": "ter_furn_migration",
    "from_ter": "t_fence_dirt",
    "to_ter": "t_dirt",
    "to_furn": "f_fence"
  }
```

Move multiple ids that don't need to be unique any more to a single id

```json
  {
    "type": "ter_furn_migration",
    "from_furn": "f_underbrush_harvested_spring",
    "to_furn": "f_underbrush_harvested"
  }
```
...
```json
  {
    "type": "ter_furn_migration",
    "from_furn": "f_underbrush_harvested_winter",
    "to_furn": "f_underbrush_harvested"
  }
```

# Overmap terrain migration

Overmap terrain migration replaces the location, if it's not generated, and replaces the entry shown on your map even if it's already generated. If you need the map to be removed without alternative, use `omt_obsolete`

```json
  {
    "type": "oter_id_migration",
    "oter_ids": {
      "old_overmap_terrain_id": "new_overmap_terrain_id",
      "hive": "omt_obsolete",
      "hive2": "omt_obsolete"
    }
  }
```

# Overmap specials migration

Overmap special can be either migrated to another overmap special, or just removed, using overmap_special_migration

```json
  {
    "type": "overmap_special_migration",
    "id": "Farm with silo",
  },
  {
    "type": "overmap_special_migration",
    "id": "FakeSpecial_cs_open_sewer",
    "new_id": "cs_open_sewer",
  },
```

# Dialogue / EoC variable migration
For EOC/dialogue variables you can use `var_migration`. This currently only migrates **Character**, and **Global** vars. If "to" isn't provided the variable will be migrated to a key of "NULL_VALUE".

```json
{
    "type": "var_migration",
    "from": "temp_var",
    "to": "new_temp_var"
},
{
    "type": "var_migration",
    "from": "temp_var"
}
```

# Ammo types

Ammo types don't need an infrastructure to be obsoleted, but it is required to remove all items that use this ammo type

# Spells

Spells have no infrastructure to migrate them, just copy the entire json of obsoleted spell to the obsoletion folder. Additionally one can make EoC, that makes the level of the spell to be -1, and make the spell run this EoC, removing it in such way

# city_building

city_building have no infrastructure to migrate them, just copy the entire json of obsoleted spell to the obsoletion folder (not sure one is needed?)

# Item groups

Don't need any migration

# Monster groups

Don't need any migration

# body_part migration

body_part have no infrastructure to migrate them, just copy the entire json of body_part to the obsoletion folder

# Mod obsoletion

For mods, you need to add an `"obsolete": true,` boolean into MOD_INFO, which prevent the mod from showing into the mod list.

```json
  {
    "type": "MOD_INFO",
    "id": "military_professions",
    "name": "Military Profession Pack",
    "description": "Numerous military themed professions",
    "category": "rebalance",
    "dependencies": [ "dda" ],
    "obsolete": true
  }
```
