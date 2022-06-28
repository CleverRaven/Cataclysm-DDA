#!/usr/bin/env python3

"""itemgroups reporter and checker"""

import argparse
import json
import os
import pprint
import sys
from pathlib import Path


TYPE_WHITELIST = [
    "AMMO", "ammobelt", "ARMOR", "BATTERY", "BIONIC_ITEM", "BOOK",
    "COMESTIBLE", "ENGINE", "GENERIC", "GUN", "GUNMOD", "holster",
    "MAGAZINE", "material", "musical_instrument", "mutagen", "mutagen_iv",
    "PET_ARMOR", "TOOL", "TOOL_ARMOR", "TOOLMOD", "WHEEL"
]
DEFAULT_CATEGORIES = [
    "AMMO", "ARMOR", "BIONIC_ITEM", "BOOK", "ENGINE", "GUN", "GUNMOD",
    "MAGAZINE", "PET_ARMOR", "TOOL", "TOOLMOD", "TOOL_ARMOR"
]
DATA_DIRECTORY = Path(os.path.dirname(__file__), "../..",
                      "data/json").resolve()
pp = pprint.PrettyPrinter(indent=2)


def _parse_arguments():
    parser = argparse.ArgumentParser(
        description="This is an itemgroup reporter and linter.  "
                    "This object menaces with subtle bugs and "
                    "recursive apocalypses.",
        epilog="Do not blindly trust the output of this program")
    parser.add_argument("-o", "--orphans", action="store_true",
                        help="Display orphaned items")
    parser.add_argument("-m", "--map", action="store_true",
                        help="Display item to item_group mapping")
    cathelp = "Limit output to these categories (default: '%s').  " \
              "'CATEGORIES' is a comma-separated list, like " \
              "'GUN, MAGAZINE'. Do not forget to quote the argument " \
              "if necessary." % (", ".join(DEFAULT_CATEGORIES))
    parser.add_argument("-c", "--categories", help=cathelp)
    return parser.parse_args()


def load_json_data(data_directory):
    """Load the JSON data.
       Returns:
        (itemgroups, items)"""
    entries = []
    ignore_filenames = ["obsolete.json", "migration.json"]
    for filename in data_directory.glob("**/*.json"):
        if filename.name in ignore_filenames:
            continue
        try:
            with open(filename, encoding="utf-8") as json_fp:
                json_data = json.load(json_fp)
            for data in json_data:
                if isinstance(data, str):
                    continue
                data["original_filename"] = \
                    filename.relative_to(data_directory)
                entries.append(data)
        except AttributeError as err:
            print("ERROR: Failed to read data from '%s': %s" %
                  (filename, err), file=sys.stderr)
            print(data)
            sys.exit(1)
    return entries


def _add_to_group(item_id, group, group_id):
    """Helper function to add an item id to the group map"""
    if item_id not in group:
        group[item_id] = {group_id: 1}
    elif group_id not in group[item_id]:
        group[item_id][group_id] = 1
    else:
        group[item_id][group_id] += 1


def _recurse_through_igroups(igroup, item_group, item_to_group, group_id=None):
    """Recurses through the item_group hierarchy and tries
       to resolve the various item groups, and fill out
       the item_to_group map as it goes"""
    if isinstance(igroup, dict):
        group_id = igroup.get("id")
        entries = igroup.get("entries", [])
        if not entries:
            entries = igroup.get("items", [])
        if not entries:
            groups = igroup.get("groups", [])
            if groups:
                for group in groups:
                    if isinstance(group, list):
                        groupname = group[0]
                    else:
                        groupname = group
                    if groupname in item_group:
                        _recurse_through_igroups(item_group[groupname],
                                                 item_group,
                                                 item_to_group,
                                                 group_id=group_id)
                    else:
                        print("ERROR: '%s' does not have 'items', or "
                              "'entries', or usable 'groups'" %
                              group_id, file=sys.stderr)
                        pp.pprint(igroup)
                        sys.exit(1)
    else:
        entries = igroup

    for entry in entries:
        if isinstance(entry, list):
            itemname = entry[0]
            if isinstance(itemname, str):
                _add_to_group(itemname, item_to_group, group_id)
            continue
        if "item" in entry:
            _add_to_group(entry["item"], item_to_group, group_id)
            continue
        if "group" in entry:
            if entry["group"] in item_group:
                _recurse_through_igroups(item_group[entry["group"]],
                                         item_group, item_to_group)
            continue
        if "distribution" in entry:
            _recurse_through_igroups(entry["distribution"], item_group,
                                     item_to_group, group_id)
            continue
        if "collection" in entry:
            _recurse_through_igroups(entry["collection"],
                                     item_group, item_to_group, group_id)
            continue


def get_item_data(entries, categories=None, ignore=None):
    """Scans the raw data structure from JSON and constructs
       an item to group map, a dict of orphans, a dict of items,
       an a list of potential problems"""
    ignore_items = ["battery_test"]
    TYPE_WHITELIST.append("item_group")
    if ignore:
        ignore_items = ignore
    item_categories = DEFAULT_CATEGORIES
    if categories:
        item_categories = categories
    item_group = {}
    item_entry = {}
    item_orphan = {}
    item_to_group = {}
    problems = []
    for entry in entries:
        copy_from = entry.get("copy-from", "")
        if copy_from == "fake_item":
            continue
        entry_type = entry.get("type")
        path = entry.pop("original_filename")
        if not entry_type or entry_type not in TYPE_WHITELIST:
            continue
        if entry.get("id") in ignore_items:
            continue
        if entry_type == "item_group":
            igroup_id = entry.get("id")
            if not igroup_id:
                problems.append({"type": "missing id", "path": path,
                                 "entry": entry})
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
        item_categories = [c.strip(' ') for c in
                           args.categories.split(",")]
    (itemgroup, orphan, items, problems) = \
        get_item_data(data_entries, item_categories)
    if problems:
        for problem in problems:
            print(problem, file=sys.stderr)
    if args.orphans:
        print("Orphans (items not found in any item groups):")
        print("=============================================")
        for oid in orphan:
            name = orphan[oid].get("name")
            if not name:
                name = "*no name entry*"
            if isinstance(name, dict):
                if "str" in name:
                    name = name["str"]
                elif "str_sp" in name:
                    name = name["str_sp"]
                elif "str_pl" in name:
                    name = name["str_pl"]
            else:
                print("WARNING: the 'name' property of '%s' is not a dict" %
                      (oid), file=sys.stderr)
                print(orphan[oid], file=sys.stderr)
            print("%s ('%s')" % (oid, name))
    if args.map:
        print("item to itemgroup mapping:")
        print("==========================")
        for item in itemgroup:
            if item not in items:
                continue
            itemtype = items[item].get("type", "")
            if itemtype not in item_categories:
                continue
            groups = list(itemgroup[item].keys())
            print("%s: %s" % (item, ", ".join(groups)))
    if not args.orphans and not args.map:
        print("%d items in %d item groups in categories:\n%s\n"
              "Default categories:\n%s" %
              (len(items), len(itemgroup),
               ", ".join(TYPE_WHITELIST),
               ", ".join(DEFAULT_CATEGORIES)))
        print("Use -h or --help for more comprehensive information")
