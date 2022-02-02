#!/usr/bin/env python3

import argparse
import json
import os

args = argparse.ArgumentParser()
args.add_argument("dir", action="store", help="specify json directory")
args_dict = vars(args.parse_args())


def portionize(jo):
    dat = dict()

    for member in ["encumbrance", "max_encumbrance",
                   "coverage", "covers"]:
        if member in jo:
            dat[member] = jo.pop(member)

    if not dat:
        return None

    if "max_encumbrance" in dat:
        if "encumbrance" not in dat:
            dat["encumbrance"] = 0
        dat["encumbrance"] = [dat["encumbrance"], dat.pop("max_encumbrance")]

    if "armor" not in jo:
        jo["armor"] = [dat]
    else:
        dat["//0"] = "Autogenned for item with non-portion and portion data"
        dat["//1"] = "Ensure that this works correctly!"
        jo["armor"].insert(0, dat)

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

            if ("type" not in jo or
                jo["type"] != "ARMOR" and
                jo["type"] != "TOOL_ARMOR" and
               "armor_data" not in jo):
                continue

            if "armor_data" in jo:
                new_data = portionize(jo["armor_data"])
                if new_data is not None:
                    jo["armor_data"] = new_data
                    change = True
                continue

            # I don't know python - assiging to jo is not necessary here
            # but is necessary for "armor_data" above
            new_jo = portionize(jo)
            if new_jo is not None:
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
