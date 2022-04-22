#!/usr/bin/env python3

# Usage 'extend_itemgroups.py dir file'
# Will convert all itemgroups within dir that have an id listed in 'file'
# to "copy-from" an itemgroup with the same id and use "extend" for all entries

import argparse
import json
import os


args = argparse.ArgumentParser()
args.add_argument("dir", action="store", help="specify json directory")
args.add_argument("converted", action="store",
                  help="specify file containing itemgroup ids."
                  "Only itemgroups with an id contained within"
                  "this file will be converted")
args_dict = vars(args.parse_args())

# The ids of itemgroups we convert
converted_ids = set()

# The keys we are moving to extend
transferred_keys = [
    "items",
    "entries",
    "groups"
]


def gen_new(path):
    change = False
    with open(path, "r", encoding="utf-8") as json_file:
        try:
            json_data = json.load(json_file)
        except json.JSONDecodeError:
            print(f"Json Decode Error at {path}")
            print("Ensure that the file is a JSON file consisting of"
                  " an array of objects!")
            return None

        for jo in json_data:
            if ("type" not in jo or jo["type"] != "item_group"):
                continue
            if jo["id"] not in converted_ids:
                continue

            change = True
            jo["copy-from"] = jo["id"]

            jo["extend"] = dict()
            for key in transferred_keys:
                if key not in jo:
                    continue
                jo["extend"][key] = jo[key]
                del(jo[key])

        return json_data if change else None


all_filepaths = set()
for root, directories, filenames in os.walk(args_dict["dir"]):
    for filename in filenames:
        if not filename.endswith(".json"):
            continue

        path = os.path.join(root, filename)
        all_filepaths.add(path)

with open(args_dict["converted"]) as file:
    for line in file:
        # Remove the newline
        line = line[:-1]
        converted_ids.add(line)

for path in all_filepaths:
    new = gen_new(path)
    if new is not None:
        with open(path, "w", encoding="utf-8") as jf:
            json.dump(new, jf, ensure_ascii=False)
        os.system(f"./tools/format/json_formatter.cgi {path}")
