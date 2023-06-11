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
        try:
            json_data = json.load(json_file)
        except json.JSONDecodeError:
            print(f"Json Decode Error at {path}")
            print("Ensure that the file is a JSON file consisting of"
                  " an array of objects!")
            return None
        for jo in json_data:
            if jo["type"] == "mapgen" and "palettes" in jo["object"] and "rows" in jo["object"] and len(jo["object"]["palettes"]) == 1 and jo["object"]["palettes"][0] == "roof_palette":
                terms = ["2", "3", "|"]
                potentialterms = terms
                if "terrain" in jo["object"]:
                    for potentialterm in potentialterms:
                        if potentialterm in jo["object"]["terrain"]:
                            terms.remove(potentialterm)
                if len(terms) != 0:
                    rowsn = len(jo["object"]["rows"])
                    i = 0
                    while i < rowsn:
                        for term in terms:
                            jo["object"]["rows"][i] = jo["object"]["rows"][i].replace(term, "-")
                            change = True
                        i += 1

    return json_data if change else None

all_filepaths = set()
for root, directories, filenames in os.walk(args_dict["dir"]):
    for filename in filenames:
        if not filename.endswith(".json"):
            continue

        path = os.path.join(root, filename)
        all_filepaths.add(path)

for path in all_filepaths:
    new = gen_new(path)
    if new is not None:
        with open(path, "w", encoding="utf-8") as jf:
            json.dump(new, jf, ensure_ascii=False)
        os.system(f"./tools/format/json_formatter.cgi {path}")
