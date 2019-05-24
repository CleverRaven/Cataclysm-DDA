#!/usr/bin/env python3

import argparse
import json
import os

args = argparse.ArgumentParser()
args.add_argument("dir", action="store", help="specify json directory")
args.add_argument("key", action="store", help="specify key")
args.add_argument("factor", type=int, action="store", help="specify factor")
args_dict = vars(args.parse_args())


def gen_new(path):
    with open(path, "r") as json_file:
        json_data = json.load(json_file)
        for jo in json_data:
            if args_dict["key"] in jo:
                val = jo[args_dict["key"]] // args_dict["factor"]
                jo[args_dict["key"]] = f"{val} m"
    return json_data


for root, directories, filenames in os.walk(args_dict["dir"]):
    for filename in filenames:
        path = os.path.join(root, filename)
        if path.endswith(".json"):
            new = gen_new(path)
            with open(path, "w") as jf:
                json.dump(new, jf)
            os.system(f"../format/json_formatter.cgi {path}")
