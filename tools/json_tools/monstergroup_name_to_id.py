#!/usr/bin/env python3

"""

Example usage:
py monstergroup_name_to_id.py ../../data/mods/my_mod

See https://github.com/CleverRaven/Cataclysm-DDA/pull/79177 for context and motivation
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


def migrateMonsterGroupNameToId(jo):
    assert "type" in jo
    if jo["type"] == "monstergroup" and "name" in jo:
        assert type(jo["name"]) is str
        # While dicts are ordered they have no way to directly edit keys
        # or access by index
        pos = list(jo.keys()).index("name")
        items = list(jo.items())
        items.insert(pos, ("id", jo["name"]))
        new_jo = dict(items)
        new_jo.pop("name")
        return new_jo
    return None


for root, directories, filenames in os.walk(args_dict["dir"]):
    for filename in filenames:
        path = os.path.join(root, filename)
        if path.endswith(".json"):
            readWriteJson(path, migrateMonsterGroupNameToId)

for statement in failures:
    print(statement)
