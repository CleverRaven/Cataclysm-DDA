#!/usr/bin/env python3

import argparse
import json
import os

args = argparse.ArgumentParser()
args.add_argument("dir", action="store", help="specify data directory")
args_dict = vars(args.parse_args())

types = [
    "AMMO",
    "ARMOR",
    "BATTERY",
    "BIONIC_ITEM",
    "BOOK",
    "COMESTIBLE",
    "CONTAINER",
    "ENGINE",
    "GENERIC",
    "GUN",
    "GUNMOD",
    "MAGAZINE",
    "PET_ARMOR",
    "TOOL",
    "TOOLMOD",
    "TOOL_ARMOR",
]


def adjust_pockets(jo):
    if "pocket_data" not in jo:
        return None

    changed = False
    for pocket in jo["pocket_data"]:
        if "flag_restriction" in pocket:
            if ("BELT_CLIP" in pocket["flag_restriction"] or
                    "HELMET_HEAD_ATTACHMENT" in pocket["flag_restriction"]):
                pocket["transparent"] = True
                changed = True
                continue
        ripoff = 0
        encumb = 0
        if "ripoff" in pocket:
            ripoff = pocket["ripoff"]
        if "extra_encumbrance" in pocket:
            encumb = pocket["extra_encumbrance"]

        if ripoff > 0 and encumb > 0:
            pocket["transparent"] = True
            changed = True

    return jo if changed else None


def gen_new(path):
    change = False
    new_datas = list()
    # Having troubles with this script halting?
    # Uncomment below to find the file it's halting on
    #print(path)
    with open(path, "r", encoding="utf-8") as json_file:
        try:
            json_data = json.load(json_file)
        except json.JSONDecodeError:
            print(f"Json Decode Error at {path}")
            print("Ensure that the file is a JSON file consisting of"
                  " an array of objects!")
            return None

        for jo in json_data:
            if type(jo) is not dict:
                print(f"Incorrectly formatted JSON data at {path}")
                print("Ensure that the file is a JSON file consisting"
                      " of an array of objects!")
                break

            if "type" not in jo:
                continue

            if jo["type"] not in types:
                continue

            new_data = adjust_pockets(jo)

            if new_data is not None:
                jo = new_data
                change = True

    if len(new_datas) > 0:
        json_data.extend(new_datas)
    return json_data if change else None


def format_json(path):
    file_path = os.path.dirname(__file__)
    format_path_linux = os.path.join(file_path, "../format/json_formatter.cgi")
    path_win = "../format/JsonFormatter-vcpkg-static-Release-x64.exe"
    format_path_win = os.path.join(file_path, path_win)
    if os.path.exists(format_path_linux):
        os.system(f"{format_path_linux} {path}")
    elif os.path.exists(format_path_win):
        os.system(f"{format_path_win} {path}")
    else:
        print("No json formatter found")


# adjust items
json_path = os.path.join(args_dict["dir"], "json")
for root, directories, filenames in os.walk(json_path):
    for filename in filenames:
        if not filename.endswith(".json"):
            continue

        path = os.path.join(root, filename)
        new = gen_new(path)
        if new is not None:
            with open(path, "w", encoding="utf-8") as jf:
                json.dump(new, jf, ensure_ascii=False)
            format_json(path)

for f in os.scandir(os.path.join(args_dict["dir"], "mods")):
    if f.is_dir():
        for root, directories, filenames in os.walk(f.path):
            for filename in filenames:
                if not filename.endswith(".json"):
                    continue

                path = os.path.join(root, filename)
                new = gen_new(path)
                if new is not None:
                    with open(path, "w", encoding="utf-8") as jf:
                        json.dump(new, jf, ensure_ascii=False)
                    format_json(path)
