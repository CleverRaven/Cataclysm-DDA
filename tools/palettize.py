#!/usr/bin/env python3

import json
import argparse
import os
import copy

def hash_key(key):
    if isinstance(key, list):
        return "list_" + "".join(key)
    else:
        return key


def parse_furnter(om_objs, palette, conflicts):
    for map_key, map_val in om_objs.items():
        if isinstance(map_val, list) and len(map_val) == 1:
            map_val = map_val[0]
        pal_val = palette.get(map_key, "")
        if not pal_val:
            palette[map_key] = map_val
        elif pal_val != map_val:
            map_hash = hash_key(map_val)
            pal_hash = hash_key(pal_val)
            conflicts.setdefault(map_key, {})
            conflicts[map_key].setdefault(map_hash, { "val": map_val, "count": 0 })
            conflicts[map_key].setdefault(pal_hash, { "val": pal_val, "count": 0 })
            conflicts[map_key][map_hash]["count"] += 1
            conflicts[map_key][pal_hash]["count"] += 1


def decide_conflicts(palette, conflicts):
    for con_key, conflict in conflicts.items():
        most_val = ""
        most_count = -1
        for _, data in conflict.items():
            if data["count"] > most_count:
                most_count = data["count"]
                most_val = data["val"]
        palette[con_key] = most_val


def resolve_conflicts(om_objs, palette, conflicts):
    temp_objs = copy.deepcopy(om_objs)
    for map_key in om_objs:
        map_val = om_objs[map_key]
        if isinstance(map_val, list) and len(map_val) == 1:
            map_val = map_val[0]
        pal_val = palette.get(map_key, "")
        if pal_val == map_val:
            del temp_objs[map_key]
    return temp_objs


args = argparse.ArgumentParser(description="Read all the terrain and furniture definitions from a "
                                           "mapgen .json file and move them into a palette file.  "
                                           "Symbols with multiple definitions put the most common "
                                           "definition in the the palette and leave the others in "
                                           "the mapgen file.")
args.add_argument("mapgen_source", action="store", help="specify json file to convert to palette.")
args.add_argument("palette_name", action="store", help="specify the name of the resulting palette.")
args.add_argument("palette_path", action="store", help="specify the folder for the palette.")
argsDict = vars(args.parse_args())

mapgen = []
mapgen_source = argsDict.get("mapgen_source", "")
palette_name = argsDict.get("palette_name", "")
palette_source = argsDict.get("palette_path", "") + "/" + palette_name + ".json"
if mapgen_source.endswith(".json"):
   try:
       with open(mapgen_source) as mapgen_file:
           mapgen += json.load(mapgen_file)
   except FileNotFoundError:
       exit("Failed: could not find {}".format(mapgen_source))
else:
   exit("Failed: invalid mapgen file name {}".format(mapgen_source))

furn_pal = {}
furn_conflicts = {}
ter_pal = {}
ter_conflicts = {}

# first pass, go through and find all the existing terrains and furnitures and any conflicts
for om_tile in mapgen:
    om_object = om_tile.get("object", {})
    om_furn = om_object.get("furniture", {})
    parse_furnter(om_furn, furn_pal, furn_conflicts)
    om_ter = om_object.get("terrain", {})
    parse_furnter(om_ter, ter_pal, ter_conflicts)

# if there are conflicts, pick the most common version for the palette
if furn_conflicts:
    decide_conflicts(furn_pal, furn_conflicts)
if ter_conflicts:
    decide_conflicts(ter_pal, ter_conflicts)

# second pass, remove entries in the palette
for om_tile in mapgen:
    om_object = om_tile.get("object", {})
    om_object.setdefault("palettes", [])
    om_object["palettes"].append(palette_name)
    if om_object.get("furniture"):
        if furn_conflicts:
            om_furn = om_object.get("furniture", {})
            om_furn_final = resolve_conflicts(om_furn, furn_pal, furn_conflicts)
            if om_furn_final:
                om_object["furniture"] = om_furn_final
            else:
                del om_object["furniture"]
        else:
            del om_object["furniture"]

    if om_object.get("terrain"):
        if ter_conflicts:
            om_ter = om_object.get("terrain", {})
            om_ter_final = resolve_conflicts(om_ter, ter_pal, ter_conflicts)
            if om_ter_final:
                om_object["terrain"] = om_ter_final
            else:
                 del om_object["terrain"]
        else:
            del om_object["terrain"]

with open(mapgen_source, 'w') as mapgen_file:
   mapgen_file.write(json.dumps(mapgen, indent=2))

palette_json = [
    {
        "type": "palette",
        "id": palette_name,
        "furniture": furn_pal,
        "terrain": ter_pal
    }
]

with open(palette_source, 'w') as palette_file:
   palette_file.write(json.dumps(palette_json, indent=2))

#print("furniture palette {}".format(json.dumps(furn_pal, indent=2)))
#print("terrain palette {}".format(json.dumps(ter_pal, indent=2)))


