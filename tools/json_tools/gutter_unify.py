#!/usr/bin/env python3

"""

Unifies "2", "3", "|" old gutters into "-" on maps using the roof_palette which don't define their own terrain definition for said characters.
Prints any maps using other palettes with the roof_palette for manual correcting.

"""
import argparse
import json
import os
import re

args = argparse.ArgumentParser()
args.add_argument("dir", action="store", help="specify json directory")
args_dict = vars(args.parse_args())

manuallychange = set()

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
            if jo["type"] == "mapgen" and "palettes" in jo["object"] and "rows" in jo["object"] and "roof_palette" in jo["object"]["palettes"]:
                if len(jo["object"]["palettes"]) == 1:
                    terms = ["2", "3", "|"]
                    potentialterms = terms
                    if "terrain" in jo["object"]:
                        for potentialterm in potentialterms:
                            if potentialterm in jo["object"]["terrain"]:
                                terms.remove(potentialterm)
                    if len(terms) != 0:
                        for term in terms:
                            if term == "|":
                                term = "/|"
                        pattern = r"'|'.join(terms)"
                        rowsn = len(jo["object"]["rows"])
                        i = 0
                        while i < rowsn:
                            if re.search(pattern, jo["object"]["rows"][i]):
                                jo["object"]["rows"][i] = re.sub(pattern, '-',jo["object"]["rows"][i])
                                change = True
                            i += 1
                else:
                    if isinstance(jo["om_terrain"], str):
                        mapname = jo["om_terrain"]
                    else:
                        mapnames = []
                        omrowsn = len(jo["om_terrain"])
                        j = 0
                        while j < omrowsn:
                            mapnames.append(','.join(jo["om_terrain"][j]))
                            j += 1
                        mapname = ','.join(mapnames)
                    manuallychange.add("Can't automatically correct map(s):\n" + mapname + "\nin file " + path + "\n due to multiple palettes being present.")
    return json_data if change else None

for root, directories, filenames in os.walk(args_dict["dir"]):
    for filename in filenames:
        path = os.path.join(root, filename)
        if path.endswith(".json"):
            new = gen_new(path)
            if new is not None:
                with open(path, "w", encoding="utf-8") as jf:
                    json.dump(new, jf, ensure_ascii=False)
                os.system(f"tools\\format\\json_formatter.exe {path}")

for statement in manuallychange:
    print(statement)
