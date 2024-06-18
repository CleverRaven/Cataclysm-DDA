#!/usr/bin/env python3

import argparse
import json
import os

args = argparse.ArgumentParser()
args.add_argument("dir", action="store", help="specify json directory")
args_dict = vars(args.parse_args())


def to_resist_vals(jo):
    res = dict()
    if "armor_bash" in jo:
        res["bash"] = jo.pop("armor_bash")
    if "armor_cut" in jo:
        res["cut"] = jo.pop("armor_cut")
    if "armor_stab" in jo:
        res["stab"] = jo.pop("armor_stab")
    if "armor_acid" in jo:
        res["acid"] = jo.pop("armor_acid")
    if "armor_fire" in jo:
        res["heat"] = jo.pop("armor_fire")
    if "armor_bullet" in jo:
        res["bullet"] = jo.pop("armor_bullet")
    if "armor_elec" in jo:
        res["electric"] = jo.pop("armor_elec")
    if "armor_biological" in jo:
        res["biological"] = jo.pop("armor_biological")
    if "armor_cold" in jo:
        res["cold"] = jo.pop("armor_cold")
    if "armor_pure" in jo:
        res["pure"] = jo.pop("armor_pure")

    if res:
        jo["armor"] = res

    if "proportional" in jo:
        jo["proportional"] = to_resist_vals(jo["proportional"])

    if "relative" in jo:
        jo["relative"] = to_resist_vals(jo["relative"])

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

            if ("type" not in jo or jo["type"] != "MONSTER"):
                continue

            new_data = to_resist_vals(jo)
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
