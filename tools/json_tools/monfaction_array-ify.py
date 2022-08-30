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
            if (type(jo) == dict and "type" in jo and
                    jo["type"] == "MONSTER_FACTION"):
                if "by_mood" in jo and type(jo["by_mood"]) == str:
                    jo["by_mood"] = [jo.pop("by_mood")]
                    change = True
                if "neutral" in jo and type(jo["neutral"]) == str:
                    jo["neutral"] = [jo.pop("neutral")]
                    change = True
                if "friendly" in jo and type(jo["friendly"]) == str:
                    jo["friendly"] = [jo.pop("friendly")]
                    change = True
                if "hate" in jo and type(jo["hate"]) == str:
                    jo["hate"] = [jo.pop("hate")]
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
