#!/usr/bin/env python3

import argparse
import json
import os

args = argparse.ArgumentParser()
args.add_argument("dir", action="store", help="specify json directory")
args_dict = vars(args.parse_args())


def gen_new(path):
    change = False
    with open(path, "r") as json_file:
        json_data = json.load(json_file)
        for jo in json_data:
            if "ammo" in jo and type(jo["ammo"]) != list and (
                    jo["type"] == "TOOL" or jo["type"] == "GUN"):
                ammo_list = [jo["ammo"]]
                jo["ammo"] = ammo_list
                change = True

    return json_data if change else None


for root, directories, filenames in os.walk(args_dict["dir"]):
    for filename in filenames:
        path = os.path.join(root, filename)
        if path.endswith(".json"):
            new = gen_new(path)
            if new != None:
                with open(path, "w") as jf:
                    json.dump(new, jf, ensure_ascii=False)
                os.system(f"./tools/format/json_formatter.cgi {path}")
