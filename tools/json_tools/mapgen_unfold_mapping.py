#!/usr/bin/env python3

"""

Example usage:
py monstergroup_name_to_id.py ../../data/mods/my_mod

See https://github.com/CleverRaven/Cataclysm-DDA/pull/79177
Should be removed after 0.I

"""
import argparse
import json
import os

args = argparse.ArgumentParser()
args.add_argument("dir", action="store", help="specify json directory")
args_dict = vars(args.parse_args())

failures = set()


def format_json(path):
    file_path = os.path.dirname(__file__)
    # Might need changing to match your setup
    format_path_linux = os.path.join(file_path, "../format/json_formatter.cgi")
    path_win = "../../json_formatter.exe"
    alt_path_win = "../format/json_formatter.exe"
    format_path_win = os.path.join(file_path, path_win)
    format_alt_path_win = os.path.join(file_path, alt_path_win)
    if os.path.exists(format_path_linux):
        os.system(f"{format_path_linux} {path}")
    elif os.path.exists(format_path_win):
        os.system(f"{format_path_win} {path}")
    elif os.path.exists(format_alt_path_win):
        os.system(f"{format_alt_path_win} {path}")
    else:
        print("No json formatter found")


def readWriteJson(path, jo_function):
    change = False
    format_error = "Json Decode Error at:\n{0}\nEnsure that the file is a "\
        "JSON file consisting of an object or array of objects!".format(path)
    with open(path, "r", encoding="utf-8") as json_file:
        try:
            json_data = json.load(json_file)
        except json.JSONDecodeError:
            failures.add(format_error)
            return
        if type(json_data) is dict:  # File with a single json object
            json_data = jo_function(json_data)
            change = json_data is not None
        elif type(json_data) is list:
            for jo in json_data:
                if type(jo) is not dict:
                    failures.add(format_error)
                    return
                jo_new = jo_function(jo)
                changed = jo_new is not None
                if changed:
                    change = True
                    pos = json_data.index(jo)
                    json_data.insert(pos, jo_new)
                    json_data.remove(jo)
        else:
            failures.add(format_error)
        if change:
            with open(path, "w", encoding="utf-8") as jf:
                json.dump(json_data, jf, ensure_ascii=False)
            format_json(path)


def migrateMappingMember(mapgen_object):
    assert type(mapgen_object["mapping"]) is dict
    mapping_object = mapgen_object["mapping"]
    symbols = list(mapping_object.keys())
    functions = dict()
    for symbol in symbols:
        symbol_data = mapping_object[symbol]
        symbol_functions = list(symbol_data.keys())
        for symbol_function in symbol_functions:
            if not symbol_function in functions:
                functions[symbol_function] = dict()
            functions[symbol_function][symbol] = symbol_data[symbol_function]
    
    mapgen_object.pop("mapping")

    for function_name in functions.keys():
        if not function_name in mapgen_object:
            mapgen_object[function_name] = dict()
        mapgen_function = mapgen_object[function_name]
        for symbol in functions[function_name].keys():
            if not symbol in mapgen_function:
                # No such symbol - add symbol and 1 entry as object
                mapgen_function[symbol] = functions[function_name][symbol]
                continue
            elif not type(mapgen_function[symbol]) is list:
                # Only 1 entry for this symbol - convert symbol entry object into list
                mapgen_function[symbol] = [ mapgen_function[symbol] ]
            # Symbol has list of data - append to the list
            mapgen_function[symbol].append(functions[function_name][symbol])


def migrateMapgenMappingMember(jo):
    assert "type" in jo
    if jo["type"] == "mapgen" and "object" in jo:
        assert type(jo["object"]) is dict
        mapgen_object = jo["object"]
        if "mapping" in mapgen_object:
            migrateMappingMember(mapgen_object)
            return jo
    if jo["type"] == "palette" and "mapping" in jo:
        migrateMappingMember(jo)
        return jo
    return None


for root, directories, filenames in os.walk(args_dict["dir"]):
    for filename in filenames:
        path = os.path.join(root, filename)
        if path.endswith(".json"):
            readWriteJson(path, migrateMapgenMappingMember)

for statement in failures:
    print(statement)
