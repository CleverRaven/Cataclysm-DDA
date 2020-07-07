#!/usr/bin/env python3

import argparse
import json
import os

args = argparse.ArgumentParser()
args.add_argument("dir", action="store", help="specify json directory")
args_dict = vars(args.parse_args())

inheriting_item_ids = []

def gen_new(path):
    change = False
    with open(path, "r") as json_file:
        json_data = json.load(json_file)
        for jo in json_data:
            if "capacity" in jo:
                if "ammo_type" in jo:
                    pocket_data = [{
                        "pocket_type": "MAGAZINE",
                        "ammo_restriction": {k: jo["capacity"] for k in jo["ammo_type"]}
                    }]
                    jo["pocket_data"] = pocket_data
                    change = True
                else:
                    inheriting_item_ids.append(jo["id"])

    if change:
        return json_data
    else:
        return None


for root, directories, filenames in os.walk(args_dict["dir"]):
    for filename in filenames:
        path = os.path.join(root, filename)
        if path.endswith(".json"):
            new = gen_new(path)
            if new:
                with open(path, "w") as jf:
                    json.dump(new, jf, ensure_ascii=False)
                os.system(f"./tools/format/json_formatter.cgi {path}")

print(*inheriting_item_ids, sep="\n")
