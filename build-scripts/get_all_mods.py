#!/usr/bin/env python3

# The goal of this script is to print out sets of mods for testing.  Each line
# of output is a comma-separated list of mods.  Together the lines should cover
# all in-repo mods, in as few lines as possible.  Each line must contain only
# mods which are mutually compatible.

import glob
import json

mods_this_time = []


def compatible_with(mod, existing_mods):
    if mod in total_conversions and total_conversions & set(existing_mods):
        return False
    return True


def add_mods(mods):
    for mod in mods:
        if mod not in all_mod_dependencies:
            # Either an invalid mod id, or blacklisted.
            return False
    for mod in mods:
        if mod not in mods_this_time and compatible_with(mod, mods_this_time):
            if add_mods(all_mod_dependencies[mod]):
                mods_this_time.append(mod)
            else:
                return False
    return True


all_mod_dependencies = {}
total_conversions = set()

for info in glob.glob('data/mods/*/modinfo.json'):
    mod_info = json.load(open(info))
    for e in mod_info:
        if e["type"] == "MOD_INFO":
            ident = e["id"]
            all_mod_dependencies[ident] = e.get("dependencies", [])
            if e["category"] == "total_conversion":
                total_conversions.add(ident)

mods_remaining = set(all_mod_dependencies)

while mods_remaining:
    for mod in mods_remaining:
        if mod not in mods_this_time:
            add_mods([mod])
    if not mods_remaining & set(mods_this_time):
        raise RuntimeError(
            'mods remain ({}) but none could be added'.format(mods_remaining))

    print(','.join(mods_this_time))
    mods_remaining = mods_remaining - set(mods_this_time)
    mods_this_time = []
