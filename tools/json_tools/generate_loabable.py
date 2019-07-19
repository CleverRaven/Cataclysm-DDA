#!/usr/bin/env python3

import argparse
import json
import os

args = argparse.ArgumentParser()
args.add_argument("dir", action="store", help="specify json directory")
args_dict = vars(args.parse_args())

changed = False


def gen_new(path):
    with open(path, "r") as json_file:
        json_data = json.load(json_file)
        global changed
        changed = False
        for jo in json_data:
            loadables = {
                "integrals": [{
                    "type": "MAGAZINE",
                    "symbol": "#",
                }]
            }
            if "ammo" in jo and "max_charges" in jo:
                loadables["integrals"][0][
                    "id"] = f"{jo['id']}_integral_magazine"
                loadables["integrals"][0][
                    "name"] = f"{jo['name']} integral magazine"
                loadables["integrals"][0][
                    "description"] = f"This item defines the integral magazine of a {jo['id']} item. It should not be visible to the player."
                loadables["integrals"][0]["ammo_type"] = jo["ammo"]

                loadables["integrals"][0]["capacity"] = jo["max_charges"]
                del jo["max_charges"]

                try:
                    loadables["integrals"][0]["count"] = jo["initial_charges"]
                    del jo["initial_charges"]
                except KeyError:
                    pass

                jo["loadables"] = loadables

                changed = True

    return json_data


for root, directories, filenames in os.walk(args_dict["dir"]):
    for filename in filenames:
        path = os.path.join(root, filename)
        if path.endswith(".json"):
            new = gen_new(path)
            if changed:
                with open(path, "w") as jf:
                    json.dump(new, jf)
                os.system(f"../format/json_formatter.cgi {path}")
