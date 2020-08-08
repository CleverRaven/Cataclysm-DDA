#!/usr/bin/env python3

"""itemgroups reporter and checker"""

import argparse
import json
import os
import pprint
import sys
from pathlib import Path


# by default, we are not interested in any of these types, since they
# are not items
IGNORE_TYPES = [ "GENERIC", "EXTERNAL_OPTION", "ITEM_CATEGORY", "LOOT_ZONE", "MONSTER_BLACKLIST", "MONSTER_FACTION", "SPECIES", "SPELL", "achievement", "activity_type", "anatomy", "ammo_effect", "ascii_art", "bionic", "body_part", "clothing_mod", "conduct", "construction", "construction_category", "disease_type", "dream", "effect_type", "emit", "enchantment", "field_type", "json_flag", "gate", "harvest", "hit_range", "item_action", "martial_art", "material", "monster_attack", "movement_mode", "morale_type", "mutation", "obsolete_terrain", "scent_type", "score", "speech", "technique", "trap", "vehicle_placement", "vehicle_spawn", "vitamin", "rotatable_symbol", "profession_item_substitutions", "profession", "region_settings", "scenario", "skill_display_type", "skill", "start_location", "event_statistic", "event_transformation", "overmap_terrain", "terrain", "furniture", "tool_quality", "weather_type", "vehicle_group", "butchery_requirement", "fault", "ammunition_type", "MIGRATION", "mapgen", "palette", "monstergroup", "snippet", "ter_furn_transform", "vehicle", "MONSTER", "behavior", "overlay_order", "mutation_type", "mutation_category", "talk_topic", "trait_group", "overmap_location", "npc_class", "npc", "mission_definition", "faction", "overmap_connection", "overmap_land_use_code", "map_extra", "city_building", "overmap_special", "recipe", "recipe_category", "uncraft", "recipe_group", "requirement", "charge_removal_blacklist", "relic_procgen_data", "proficiency" ]
DEFAULT_CATEGORIES = [ "ENGINE", "WHEEL", "BOOK", "PET_ARMOR", "TOOL_ARMOR", "ARMOR", "TOOLMOD", "COMESTIBLE", "AMMO", "GUN", "MAGAZINE", "TOOL", "GUNMOD", "BIONIC_ITEM", "vehicle_part" ]
DATA_DIRECTORY = Path(os.path.dirname(__file__), "../..", "data/json").resolve()


pp = pprint.PrettyPrinter(indent=2)


def _parse_arguments():
    parser = argparse.ArgumentParser(
            description="This is an itemgroup reporter and linter.  This object menaces with subtle bugs and recursive apocalypses.",
            epilog="Do not blindly trust the output of this program")
    parser.add_argument("-o", "--orphans", action="store_true", help="Display orphaned items")
    parser.add_argument("-m", "--map", action="store_true", help="Display item to item_group mapping")
    cathelp = "Limit output to these categories (default: '%s').  'CATEGORIES' is a comma-separated list, like 'GUN, MAGAZINE'. Do not forget to quote the argument if necessary." % (", ".join(DEFAULT_CATEGORIES))
    parser.add_argument("-c", "--categories", help=cathelp)
    args = parser.parse_args()
    return args


def load_json_data(data_directory):
    """Load the JSON data.
       Returns:
        (itemgroups, items)"""
    entries = []
    ignore_filenames = "obsolete.json"
    for filename in data_directory.glob("**/*.json"):
        if filename.name in ignore_filenames:
            continue
        try:
            with open(filename) as json_fp:
                json_data = json.load(json_fp)
            for data in json_data:
                if type(data) is str:
                    continue
                data["original_filename"] = filename.relative_to(data_directory)
                entries.append(data)
        except AttributeError as err:
            print("ERROR: Failed to read data from '%s': %s" % (filename, err), file=sys.stderr)
            print(data)
            sys.exit(1)
    return entries


def _add_to_group(item, group, group_id):
    """Helper function to add an item id to the group map"""
    if item not in group:
        group[item] = { group_id: 1 }
    elif group_id not in group[item]:
        group[item][group_id] = 1
    else:
        group[item][group_id] += 1


def _recurse_through_igroups(igroup, item_group, item_to_group, group_id=None):
    """Recurses through the item_group hierarchy and tries
       to resolve the various item groups, and fill out
       the item_to_group map as it goes"""
    if type(igroup) is dict:
        subtype = igroup.get("subtype")
        group_id = igroup.get("id")
        entries = igroup.get("entries", [])
        if not entries:
            entries = igroup.get("items", [])
        if not entries:
            groups = igroup.get("groups", [])
            if groups:
                for group in groups:
                    if type(group) is list:
                        groupname = group[0]
                    else:
                        groupname = group
                    if groupname in item_group:
                        _recurse_through_igroups(item_group[groupname], item_group, item_to_group, group_id=group_id)
                    else:
                        print("ERROR: '%s' does not have 'items', or 'entries', or usable 'groups'" % group_id, file=sys.stderr)
                        pp.pprint(igroup)
                        sys.exit(1)
    else:
        entries = igroup

    for entry in entries:
        if type(entry) is list:
            itemname = entry[0]
            if type(itemname) is str:
                _add_to_group(itemname, item_to_group, group_id)
            continue
        if "item" in entry:
            _add_to_group(entry["item"], item_to_group, group_id)
            continue
        if "group" in entry:
            if entry["group"] in item_group:
                _recurse_through_igroups(item_group[entry["group"]], item_group, item_to_group)
            continue
        if "distribution" in entry:
            _recurse_through_igroups(entry["distribution"], item_group, item_to_group, group_id)
            continue
        if "collection" in entry:
            _recurse_through_igroups(entry["collection"], item_group, item_to_group, group_id)
            continue


def get_item_data(entries, categories=None, ignore=None):
    ignore_items = [ "battery_test" ]
    item_categories = DEFAULT_CATEGORIES
    if categories:
        item_categories = categories
    item_group = {}
    item_entry = {}
    item_orphan = {}
    item_to_group = {}
    problems = []
    for entry in entries:
        entry_type = entry.get("type")
        path = entry.pop("original_filename")
        if not entry_type:
            continue
        if entry_type in IGNORE_TYPES:
            continue
        if entry.get("id") in ignore_items:
            continue
        if entry_type == "item_group":
            igroup_id = entry.get("id")
            if not igroup_id:
                problems.append({"type": "missing id", "path": path, "entry": entry})
            item_group[igroup_id] = entry
            continue

        # if it's not an item_group, it's probably an item
        item_id = entry.get("id")
        item_entry[item_id] = entry
    for igroup in item_group:
        _recurse_through_igroups(item_group[igroup], item_group, item_to_group)
    for item in item_entry:
        itemtype = item_entry[item].get("type")
        if itemtype not in item_categories:
            continue
        if item not in item_to_group:
            item_orphan[item] = item_entry[item]
    return (item_to_group, item_orphan, item_entry, problems)


if __name__ == "__main__":
    args = _parse_arguments()
    data_entries = load_json_data(DATA_DIRECTORY)
    item_categories = DEFAULT_CATEGORIES
    if args.categories:
        item_categories = args.categories.split(",")
    (itemgroup, orphan, items, problems) = get_item_data(data_entries, item_categories)
    if problems:
        for problem in problems:
            print(problem, file=sys.stderr)
    if args.orphans:
        print("Orphans (items not found in any itemgroups):")
        for oid in orphan:
            name = orphan[oid].get("name")
            if not name:
                name = "*no name entry*"
            if "str" in name:
                name = name["str"]
            elif "str_sp" in name:
                name = name["str_sp"]
            elif "str_pl" in name:
                name = name["str_pl"]
            print("%s: ('%s')" % (name, oid))
    if args.map:
        print("item to itemgroup map:")
        for item in itemgroup:
            groups = list(itemgroup[item].keys())
            print("%s: %s" % (item, ", ".join(groups)))
    if not args.orphans and not args.map:
        print("%d items in %d categories (%s)" % (len(items), len(itemgroup), ", ".join(DEFAULT_CATEGORIES)))
        print("Use -h or --help for more comprehensive information")
