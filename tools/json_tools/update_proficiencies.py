#!/usr/bin/env python3

import argparse
import json
import os

args = argparse.ArgumentParser()
args.add_argument("dir", action="store", help="specify json directory")
args_dict = vars(args.parse_args())


def gen_new(path):
    change = False
    with open(path, "r", encoding="utf-8") as json_file:
        json_data = json.load(json_file)

        for jo in json_data:
            if type(jo) is not dict or "type" not in jo:
                continue

            if "proficiencies" in jo:
                for prof in jo["proficiencies"]:
                    if "fail_multiplier" in prof:
                        prof["skill_penalty"] = prof["fail_multiplier"] - 1
                        del prof["fail_multiplier"]
                        change = True

            elif (jo["type"] == "proficiency" and
                  "default_fail_multiplier" in jo):
                jo["default_skill_penalty"] = jo["default_fail_multiplier"] - 1
                del jo["default_fail_multiplier"]
                change = True

    return json_data if change else None


for root, directories, filenames in os.walk(args_dict["dir"]):
    for filename in filenames:
        path = os.path.join(root, filename)
        if path.endswith(".json"):
            new = gen_new(path)
            if new is not None:
                with open(path, "w", encoding="utf-8") as jf:
                    json.dump(new, jf, ensure_ascii=False)
                os.system(f"./tools/format/json_formatter.cgi {path}")
