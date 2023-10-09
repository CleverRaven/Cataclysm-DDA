#!/usr/bin/env python3

import argparse
import json
import os

args = argparse.ArgumentParser()
args.add_argument("dir", action="store", help="specify json directory")
args_dict = vars(args.parse_args())


def to_resist_vals(jo):
    res = dict()
    if "bash_resist" in jo:
        res["bash"] = jo.pop("bash_resist")
    if "cut_resist" in jo:
        res["cut"] = jo.pop("cut_resist")
    if "acid_resist" in jo:
        res["acid"] = jo.pop("acid_resist")
    if "fire_resist" in jo:
        res["heat"] = jo.pop("fire_resist")
    if "bullet_resist" in jo:
        res["bullet"] = jo.pop("bullet_resist")
    if "elec_resist" in jo:
        res["electric"] = jo.pop("elec_resist")
    if "biologic_resist" in jo:
        res["biological"] = jo.pop("biologic_resist")
    if "cold_resist" in jo:
        res["cold"] = jo.pop("cold_resist")

    if res:
        jo["resist"] = res

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

            if ("type" not in jo or jo["type"] != "material"):
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
