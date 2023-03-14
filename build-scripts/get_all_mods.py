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


def order_modlist(modlist):
    deps_by_mod = {}
    final_modlist = []
    mods_left = set()
    # As a base, find all mods with no dependencies
    for mod in modlist:
        deps = all_mod_dependencies[mod]
        for dep in all_mod_load_afters[mod]:
            if dep in modlist:
                deps.append(dep)
        deps_by_mod[mod] = deps
        if len(deps) == 0:
            final_modlist.append(mod)
        else:
            mods_left.add(mod)

    last_len = 0
    while (len(mods_left) != 0 and last_len != len(mods_left)):
        last_len = len(mods_left)
        to_remove = []
        for mod in mods_left:
            if set(deps_by_mod[mod]).issubset(set(final_modlist)):
                to_remove.append(mod)
                final_modlist.append(mod)

        for mod in to_remove:
            mods_left.remove(mod)

    if (len(mods_left) != 0):
        for mod in mods_left:
            raise RuntimeError("Mod " + mod + " could not be added!" +
                               " Dependencies: " + ",".join(deps_by_mod[mod]))

    return final_modlist


def print_modlist(modlist, master_list):
    print(','.join(order_modlist(modlist)))
    master_list -= set(modlist)
    modlist.clear()


all_mod_dependencies = {}
all_mod_load_afters = {}
total_conversions = set()

for info in glob.glob('data/mods/*/modinfo.json'):
    mod_info = json.load(open(info, encoding='utf-8'))
    for e in mod_info:
        if (e["type"] == "MOD_INFO" and
                ("obsolete" not in e or not e["obsolete"])):
            ident = e["id"]
            all_mod_dependencies[ident] = e.get("dependencies", [])
            all_mod_load_afters[ident] = e.get("load_after", [])
            if e["category"] == "total_conversion":
                total_conversions.add(ident)

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
