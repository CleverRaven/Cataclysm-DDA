#!/usr/bin/env python3

import json
import argparse
import os
import copy
import math
import string
from operator import itemgetter, attrgetter


STRIDE_X = 3
STRIDE_Y = 2
MIN_X = 10000
MIN_Y = 10000
OM_SPEC_TYPES = ["overmap_special", "city_building"]
KEYED_TERMS = [
    "terrain", "furniture", "fields", "npcs", "signs", "vendingmachines", "toilets", "gaspumps",
    "items", "monsters", "vehicles", "item", "traps", "monster", "rubble", "liquids",
    "sealed_item", "graffiti", "mapping"
]
PLACE_TERMS = ["set", "place_groups"]
MAP_ROTATE = [
    {"dir": "_south", "x": 1, "y": 1},
    {"dir": "_north", "x": 0, "y": 0}
]


def x_y_bucket(x, y):
    return "{}__{}".format(math.floor((x - MIN_X) / STRIDE_X), math.floor((y - MIN_Y) / STRIDE_Y))


def x_y_sub(x, y, is_north):
    if is_north:
        return "{}__{}".format((x - MIN_X) % STRIDE_X, (y - MIN_Y) % STRIDE_Y)
    else:
        return "{}__{}".format((x - MIN_X - 1) % STRIDE_X,
                               (y - MIN_Y - 1) % STRIDE_Y)



def x_y_simple(x, y):
    return "{}__{}".format(x, y)


def get_data(argsDict, resource_name):
   resource = []
   resource_sources = argsDict.get(resource_name, [])
   if not isinstance(resource_sources, list):
       resource_sources = [resource_sources]
   for resource_filename in resource_sources:
       if resource_filename.endswith(".json"):
          try:
              with open(resource_filename) as resource_file:
                  resource += json.load(resource_file)
          except FileNotFoundError:
              exit("Failed: could not find {}".format(resource_filename))
       else:
           print("Invalid filename {}".format(resource_filename))
   return resource


def adjacent_to_set(x, y, coord_set):
    for coords in coord_set:
        if y == coords["y"] and abs(x - coords["x"]) == 1:
            return True
        if x == coords["x"] and abs(y - coords["y"]) == 1:
            return True


def validate_keyed(key_term, old_obj, entry):
    new_keyset = entry["object"].get(term, {})
    old_keyset = old_obj.get(key_term, {})
    if new_keyset:
        for old_key, old_val in old_keyset.items():
            new_keyset.setdefault(old_key, old_val)
            if new_keyset[old_key] != old_val:
                return False
    else:
        new_keyset = old_keyset
    return new_keyset


# make sure that all keyed entries have the same key values and the maps have the same weight
# and fill_ter.  Don't try to resolve them.
def validate_old_map(old_map, entry):
    old_obj = old_map.get("object", {})

    if entry["weight"] and old_map.get("weight") and entry["weight"] != old_map.get("weight"):
        return False
    if entry["object"].get("fill_ter") and old_obj.get("fill_ter") and \
        entry["object"]["fill_ter"] != old_obj.get("fill_ter"):
        return False

    new_palettes = entry.get("palettes", {})
    old_palettes = old_obj.get("palettes")
    if new_palettes:
        for palette in old_palettes:
            if palette not in new_palettes:
                return False
    else:
        new_palettes = old_palettes

    keysets = {}
    for key_term in KEYED_TERMS:
        new_keyset = validate_keyed(key_term, old_obj, entry)
        if new_keyset:
            keysets[key_term] = new_keyset
        elif new_keyset != {}:
            return False
             
    if not entry["weight"]:
        entry["weight"] = old_map.get("weight", 0)
    if not entry["object"].get("fill_ter"):
        entry["object"]["fill_ter"] = old_obj.get("fill_ter", "")
    for key_term, new_keyset in keysets.items():
        entry["object"][key_term] = new_keyset
    if new_palettes:
        entry["object"]["palettes"] = new_palettes

    return True


# adjust the X, Y co-ords of a place_ entry to match the new map
def adjust_place(term, old_obj, offset_x, offset_y):
    def adjust_coord(x_or_y, new_entry, old_entry, offset):
        val = old_entry.get(x_or_y, "False")
        if val == "False":
             return False
        if isinstance(val, list):
             val[0] += offset
             val[1] += offset
        else:
            val += offset
        new_entry[x_or_y] = val

    results = []
    for old_entry in old_obj.get(term, []):
        new_entry = copy.deepcopy(old_entry)
        if offset_x:
            adjust_coord("x", new_entry, old_entry, offset_x)
            adjust_coord("x2", new_entry, old_entry, offset_x)
        if offset_y:
            adjust_coord("y", new_entry, old_entry, offset_y)
            adjust_coord("y2", new_entry, old_entry, offset_y)
        results.append(new_entry)
    return results


args = argparse.ArgumentParser(description="Merge individual OMT maps into blocks of maps.")
args.add_argument("mapgen_sources", action="store", nargs="+",
                  help="specify jsons file to convert to blocks.")
args.add_argument("specials_sources", action="store", nargs="+",
                  help="specify json file with overmap special data.")
args.add_argument("--x", dest="stride_x", action="store",
                  help="number of horizontal maps in each block.  Defaults to {}.".format(STRIDE_X))
args.add_argument("--y", dest="stride_y", action="store",
                  help="number of vertictal maps in each block.  Defaults to {}.".format(STRIDE_Y))
args.add_argument("--output", dest="output_name", action="store",
                  help="Name of output file.  Defaults to the command line.")
argsDict = vars(args.parse_args())

mapgen = get_data(argsDict, "mapgen_sources")
specials = get_data(argsDict, "specials_sources")
string_x = argsDict.get("stride_x")
if string_x and int(string_x):
    STRIDE_X = int(string_x)
string_y = argsDict.get("stride_y")
if string_y and int(string_y):
    STRIDE_Y = int(string_y)

output_name = argsDict.get("output_name", "")
if output_name and not output_name.endswith(".json"):
    output_name += ".json"

# very first pass, sort the overmaps and find the minimum X and Y values
for special in specials:
    if special.get("type") in OM_SPEC_TYPES:
        overmaps = special.get("overmaps")
        if not overmaps:
            continue
        overmaps.sort(key=lambda om_data: om_data.get("point", [1000, 0])[0])
        MIN_X = overmaps[0].get("point", [1000, 0])[0]
        overmaps.sort(key=lambda om_data: om_data.get("point", [0, 1000])[1])
        MIN_Y = overmaps[0].get("point", [0, 1000])[1]

# create the merge sets of maps
merge_sets = {}
for special in specials:
    if special.get("type") in OM_SPEC_TYPES:
        overmaps = special.get("overmaps")
        for om_data in overmaps:
            om_map = om_data.get("overmap")
            for map_dir in MAP_ROTATE:
                if om_map.endswith(map_dir["dir"]):
                    om_map = om_map.split(map_dir["dir"])[0]
                    break
            om_point = om_data.get("point", [])
            if len(om_point) == 3:
                is_north = map_dir["dir"] == "_north"
                x = om_point[0]
                y = om_point[1]
                z = om_point[2]
                merge_sets.setdefault(z, {})
                merge_sets[z].setdefault(x_y_bucket(x, y), {})
                merge_sets[z][x_y_bucket(x, y)][x_y_sub(x, y, is_north)] = om_map

# convert the mapgen list into a dictionary for easier access
map_dict = {}
new_mapgen = []
for om_map in mapgen:
    if om_map.get("type") != "mapgen":
        new_mapgen.append(om_map)
        continue
    om_id = om_map["om_terrain"]
    if isinstance(om_id, list):
        if len(om_id) == 1:
            om_id = om_id[0]
        else:
            continue
    map_dict[om_id] = om_map

# dynamically expand the list of "place_" terms
for term in KEYED_TERMS:
    PLACE_TERMS.append("place_" + term)

basic_entry = {
    "method": "json",
    "object": {
        "fill_ter": "",
        "rows": [],
        "palettes": [],
    },
    "om_terrain": [],
    "type": "mapgen",
    "weight": 0
}

# debug: make sure the merge sets look correct
#print("mergesets: {}".format(json.dumps(merge_sets, indent=2)))

# finally start merging maps
for z, zlevel in merge_sets.items():
    for x_y, mapset in zlevel.items():
        # first, split the mergeset into chunks with common KEYED_TERMS using a weird floodfill
        chunks = []
        chunks = [{ "maps": [], "entry": copy.deepcopy(basic_entry)}]
        for y in range(0, STRIDE_Y):
            for x in range(0, STRIDE_X):
                om_id = mapset.get(x_y_simple(x, y), "")
                if not om_id:
                    continue
                old_map = map_dict.get(om_id, {})
                if not old_map:
                    continue
                validated = False
                for chunk_data in chunks:
                    # try to filter out maps that are no longer adjacent to the rest of the
                    # merge set due to other maps not being valid
                    if chunk_data["maps"] and not adjacent_to_set(x, y, chunk_data["maps"]):
                        continue
                    # check that this map's keyed terms match the other keyed terms in this chunk
                    if validate_old_map(old_map, chunk_data["entry"]):
                        chunk_data["maps"].append({"x": x, "y": y})
                        validated = True                            
                if not validated:
                    new_entry = copy.deepcopy(basic_entry)
                    chunks.append({ "maps": [{"x": x, "y": y}], "entry": new_entry })

        # then split up any irregular shapes that made it past the screen
        # T and L shapes are possible because every map is adjacent, for instance
        final_chunks = []
        for chunk_data in chunks:
            maps = chunk_data["maps"]
            if len(maps) < 3:
                final_chunks.append(chunk_data)
                continue
            maps.sort(key=itemgetter("x"))
            max_x = maps[-1]["x"]
            min_x = maps[0]["x"]
            maps.sort(key=itemgetter("y"))
            max_y = maps[-1]["y"]
            min_y = maps[0]["y"] 
            # if this is a line, square, or rectangle, it's continguous
            if len(maps) == ((max_x - min_x + 1) * (max_y - min_y + 1)):
                final_chunks.append(chunk_data)
                continue
            # if not, just break it into individual maps
            for coords in maps:
                final_chunks.append({
                    "maps": [{"x": coords["x"], "y": coords["y"]}],
                    "entry": copy.deepcopy(chunk_data["entry"])
                })

        if not final_chunks:
            continue

        # debug: do the final chunks look sane?
        #print("chunks: {}".format(json.dumps(chunks, indent=2)))
        # go through the final chunks and merge them
        for chunk_data in final_chunks:
            new_rows = []
            entry = chunk_data["entry"]
            maps = chunk_data["maps"]
            if not maps:
                continue
            first_x = maps[0]["x"]
            first_y = maps[0]["y"] 
            for coords in maps:
                x = coords["x"]
                y = coords["y"]
                row_offset = 24 * (y - first_y)
                col_offset = 24 * (x - first_x)
                om_id = mapset.get(x_y_simple(x, y), "")
                old_map = map_dict.get(om_id, {})
                old_obj = old_map.get("object", {})
                old_rows = old_obj.get("rows", {})
                # merge the rows, obviously
                for i in range(0, 24):
                    if not col_offset:
                        new_rows.append(old_rows[i])
                    else:
                        new_rows[i + row_offset] = "".join([new_rows[i + row_offset], old_rows[i]])
                if len(maps) == 1:
                    entry["om_terrain"] = om_id
                else:
                    if len(entry["om_terrain"]) < (y - first_y + 1):
                        entry["om_terrain"].append([om_id])
                    else:
                        entry["om_terrain"][y - first_y].append(om_id)
                # adjust the offsets for place_ entries before potentially converting set entries
                for place_term in PLACE_TERMS:
                    new_terms = adjust_place(place_term, old_obj, col_offset, row_offset)
                    if new_terms:
                        entry["object"].setdefault(place_term, [])
                        for term_entry in new_terms:
                            entry["object"][place_term].append(term_entry)
            # finally done with the chunk, so add it to the list
            entry["object"]["rows"] = new_rows
            new_mapgen.append(entry)
            # debug: make sure sure the final chunk is correct
            #print("{}".format(json.dumps(entry, indent=2)))

if output_name:
    with open(output_name, 'w') as output_file:
        output_file.write(json.dumps(new_mapgen))
    exit()

print("{}".format(json.dumps(new_mapgen, indent=2)))
