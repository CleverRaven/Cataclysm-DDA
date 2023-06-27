#!/usr/bin/env python3

"""

Unifies "2", "3", "|" old gutters into "-" on maps using the roof_palette
which don't define their own terrain definition for said characters.

Prints the names of maps that use other palettes
alongside roof_palette to be corrected manually.

Can be run in /json and /mods, to run on specific maps
just temporarily move them to their own folder.

"""
import argparse
import json
import os
import re

args = argparse.ArgumentParser()
args.add_argument("dir", action="store", help="specify json directory")
args_dict = vars(args.parse_args())

failures = set()


def gen_new(path):
    change = False
    with open(path, "r", encoding="utf-8") as json_file:
        try:
            json_data = json.load(json_file)
        except json.JSONDecodeError:
            failures.add(
                "Json Decode Error at:\n" +
                path +
                "\nEnsure that the file is a JSON"
                "file consisting of an array of objects!"
            )
            return None
        for jo in json_data:
            if isinstance(jo, dict):
                if (
                    jo["type"] == "mapgen" and
                    "palettes" in jo["object"] and
                    "rows" in jo["object"] and
                    "roof_palette" in jo["object"]["palettes"]
                ):
                    if len(jo["object"]["palettes"]) == 1:
                        terms = ["|", "2", "3"]
                        if "terrain" in jo["object"]:
                            terms[:] = [
                                term
                                for term in terms
                                if term not in jo["object"]["terrain"]
                            ]
                        if len(terms) != 0:
                            pattern = r"[" + "".join(terms) + r"]"
                            rowsn = len(jo["object"]["rows"])
                            i = 0
                            while i < rowsn:
                                if re.search(pattern, jo["object"]["rows"][i]):
                                    jo["object"]["rows"][i] = re.sub(
                                        pattern, "-", jo["object"]["rows"][i]
                                    )
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
                                mapnames.append(",".join(jo["om_terrain"][j]))
                                j += 1
                            mapname = ",".join(mapnames)
                        failures.add(
                            "Can't automatically correct map(s):\n" +
                            mapname +
                            "\nin file:\n" +
                            path +
                            "\ndue to multiple palettes being present."
                        )
    return json_data if change else None


def format_json(path):
    file_path = os.path.dirname(__file__)
    format_path_linux = os.path.join(file_path, "../format/json_formatter.cgi")
    path_win = "../format/JsonFormatter-vcpkg-static-Release-x64.exe"
    format_path_win = os.path.join(file_path, path_win)
    if os.path.exists(format_path_linux):
        os.system(f"{format_path_linux} {path}")
    elif os.path.exists(format_path_win):
        os.system(f"{format_path_win} {path}")
    else:
        print("No json formatter found")


for root, directories, filenames in os.walk(args_dict["dir"]):
    for filename in filenames:
        path = os.path.join(root, filename)
        if path.endswith(".json"):
            new = gen_new(path)
            if new is not None:
                with open(path, "w", encoding="utf-8") as jf:
                    json.dump(new, jf, ensure_ascii=False)
                format_json(path)

for statement in failures:
    print(statement)
