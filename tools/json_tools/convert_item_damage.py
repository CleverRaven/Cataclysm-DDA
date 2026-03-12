#!/usr/bin/env python3

import argparse
import json
import os

args = argparse.ArgumentParser()
args.add_argument("dir", action="store", help="specify json directory")
args_dict = vars(args.parse_args())


def to_damage_vals(jo):
    convert_stab = False
    if "flags" in jo and ("STAB" in jo["flags"] or "SPEAR" in jo["flags"]):
        convert_stab = True
        if "STAB" in jo["flags"]:
            jo["flags"].remove("STAB")

    res = dict()
    if "bashing" in jo:
        res["bash"] = jo.pop("bashing")
    if "cutting" in jo:
        cut_str = "cut"
        if convert_stab:
            cut_str = "stab"
        res[cut_str] = jo.pop("cutting")

    if res:
        jo["melee_damage"] = res

    if "proportional" in jo:
        res = dict()
        if "bashing" in jo["proportional"]:
            res["bash"] = jo["proportional"].pop("bashing")
        if "cutting" in jo["proportional"]:
            cut_str = "cut"
            if convert_stab:
                cut_str = "stab"
            res[cut_str] = jo["proportional"].pop("cutting")
        if res:
            jo["proportional"]["melee_damage"] = res

    if "relative" in jo:
        res = dict()
        if "bashing" in jo["relative"]:
            res["bash"] = jo["relative"].pop("bashing")
        if "cutting" in jo["relative"]:
            cut_str = "cut"
            if convert_stab:
                cut_str = "stab"
            res[cut_str] = jo["relative"].pop("cutting")
        if res:
            jo["relative"]["melee_damage"] = res

    return jo


def gen_new(path):
    change = False
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

            item_types = {
                "GENERIC",
                "AMMO",
                "GUN",
                "ARMOR",
                "PET_ARMOR",
                "TOOL",
                "TOOLMOD",
                "TOOL_ARMOR",
                "BOOK",
                "COMESTIBLE",
                "ENGINE",
                "WHEEL",
                "GUNMOD",
                "MAGAZINE",
                "BATTERY",
                "BIONIC_ITEM"
            }

            if ("type" not in jo or jo["type"] not in item_types):
                continue

            new_data = to_damage_vals(jo)
            if new_data is not None:
                jo = new_data
                change = True

    return json_data if change else None


for root, directories, filenames in os.walk(args_dict["dir"]):
    for filename in filenames:
        if not filename.endswith(".json"):
            continue

        path = os.path.join(root, filename)
        new = gen_new(path)
        if new is not None:
            with open(path, "w", encoding="utf-8") as jf:
                json.dump(new, jf, ensure_ascii=False)
            os.system(f"./tools/format/json_formatter.cgi {path}")
