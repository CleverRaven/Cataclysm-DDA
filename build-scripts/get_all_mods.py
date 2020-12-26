#!/usr/bin/env python3

import glob
import json


mods_to_keep = []


def add_mods(mods):
    for mod in mods:
        if mod not in all_mod_dependencies:
            # Either an invalid mod id, or blacklisted.
            return False
    for mod in mods:
        if mod not in mods_to_keep:
            mods_to_keep.append(mod)
    return True


all_mod_dependencies = {}

for info in glob.glob('data/mods/*/modinfo.json'):
    mod_info = json.load(open(info))
    for e in mod_info:
        if e["type"] == "MOD_INFO":
            ident = e["id"]
            all_mod_dependencies[ident] = e.get("dependencies", [])

for mod in all_mod_dependencies:
    if mod not in mods_to_keep:
        if add_mods(all_mod_dependencies[mod]):
            mods_to_keep.append(mod)

print(','.join(mods_to_keep))
