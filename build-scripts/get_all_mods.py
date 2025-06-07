#!/usr/bin/env python3

# The goal of this script is to print out sets of mods for testing.  Each line
# of output is a comma-separated list of mods.  Together the lines should cover
# all in-repo mods, in as few lines as possible.  Each line must contain only
# mods which are mutually compatible.

import glob
import json
import os

mods_this_time = []

exclusions = [
    # Tuple of (mod_id, mod_id) - these two mods will be incompatible
    # Note that mod_id is case sensitive
]


def compatible_with(mod, existing_mods):
    if mod in total_conversions and total_conversions & set(existing_mods):
        return False
    for entry in existing_mods:
        if mod in all_mod_conflicts[entry] or entry in all_mod_conflicts[mod]:
            return False
    for entry in exclusions:
        if entry[0] == mod and entry[1] in existing_mods:
            return False
        if entry[1] == mod and entry[0] in existing_mods:
            return False
    return True


def add_mods(mods):
    for mod in mods:
        if mod not in all_mod_dependencies:
            # Either an invalid mod id, or blacklisted.
            return False
    for mod in mods:
        if mod in mods_this_time:
            continue
        if not compatible_with(mod, mods_this_time):
            return False
        if add_mods(all_mod_dependencies[mod]):
            mods_this_time.append(mod)
        else:
            return False
    return True


def print_modlist(modlist, master_list):
    print(','.join(modlist))
    master_list -= set(modlist)
    modlist.clear()


all_mod_dependencies = {}
all_mod_conflicts = {}
total_conversions = set()

for info in glob.glob('data/mods/*/modinfo.json'):
    mod_info = json.load(open(info, encoding='utf-8'))
    for e in mod_info:
        if (e["type"] == "MOD_INFO" and
                ("obsolete" not in e or not e["obsolete"])):
            ident = e["id"]
            all_mod_dependencies[ident] = e.get("dependencies", [])
            all_mod_conflicts[ident] = e.get("conflicts", [])
            if e["category"] == "total_conversion":
                total_conversions.add(ident)

for r, d, f in os.walk('data/mods'):
    if 'mod_interactions' not in d:
        continue
    if 'modinfo.json' not in f:
        continue
    ident = ""
    info_path = os.path.join(r, 'modinfo.json')
    mod_info = json.load(open(info_path, encoding='utf-8'))
    for e in mod_info:
        if (e["type"] == "MOD_INFO" and
                ("obsolete" not in e or not e["obsolete"])):
            ident = e["id"]
    if ident == "":
        continue
    add_mods([ident])
    for mod in os.scandir(os.path.join(r, 'mod_interactions')):
        mods_this_time.append(os.path.basename(mod.path))
    print(','.join(mods_this_time))
    mods_this_time.clear()

mods_remaining = set(all_mod_dependencies)

# Make sure aftershock can load by itself.
add_mods(["aftershock"])
print_modlist(mods_this_time, mods_remaining)

while mods_remaining:
    for mod in mods_remaining:
        if mod not in mods_this_time:
            add_mods([mod])
    if not mods_remaining & set(mods_this_time):
        raise RuntimeError(
            'mods remain ({}) but none could be added'.format(mods_remaining))
    print_modlist(mods_this_time, mods_remaining)
