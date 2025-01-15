#!/usr/bin/env python3

"""
py identifyMeaningfulWeights.py ../data/json ../Cataclysm-DDA/data/mods
"""
import argparse
import json
import os
import re

args = argparse.ArgumentParser()
args.add_argument("ddadir", action="store", help="specify json directory")
args.add_argument("modsdir", action="store", help="specify mods directory")
args_dict = vars(args.parse_args())

omtFoundIds = set()
omtMeaningfulWeightIds = set()
nestFoundIds = set()
nestMeaningfulWeightIds = set()
updateFoundIds = set()
updateMeaningfulWeightIds = set()
failures = set()


def determineMapsWithMeaningfulWeights(path):
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
        for jo in json_data:
            if isinstance(jo, dict):
                if jo["type"] == "mapgen":
                    # Regular map
                    if "om_terrain" in jo:
                        # String
                        if type(jo["om_terrain"]) is str:
                            check(jo["om_terrain"], omtFoundIds, omtMeaningfulWeightIds)
                        # Array
                        else:
                            for value in jo["om_terrain"]:
                                # Multiple om_terrains sharing a map or just pointlessly arrayed single value
                                if type(value) is str:
                                    check(value, omtFoundIds, omtMeaningfulWeightIds)
                                # Multi-omt map
                                else:
                                    for part in value:
                                        check(part, omtFoundIds, omtMeaningfulWeightIds)
                    elif "nested_mapgen_id" in jo:
                        assert type(jo["nested_mapgen_id"]) is str
                        check(jo["nested_mapgen_id"], nestFoundIds, nestMeaningfulWeightIds)
                    elif "update_mapgen_id" in jo:
                        assert type(jo["update_mapgen_id"]) is str
                        check(jo["update_mapgen_id"], updateFoundIds, updateMeaningfulWeightIds)


def check(id, setOfFoundIds, setOfMeaningfulWeightIds):
    if id in setOfFoundIds:
        if id not in setOfMeaningfulWeightIds:
            setOfMeaningfulWeightIds.add(id)
    else:
        setOfFoundIds.add(id)


for root, directories, filenames in os.walk(args_dict["ddadir"]):
    for filename in filenames:
        path = os.path.join(root, filename)
        if path.endswith(".json"):
            determineMapsWithMeaningfulWeights(path)

for root, directories, filenames in os.walk(args_dict["modsdir"]):
    for filename in filenames:
        path = os.path.join(root, filename)
        if path.endswith(".json"):
            determineMapsWithMeaningfulWeights(path)

def print_results(setOfMeaningfulWeightIds, name):
    if len(setOfMeaningfulWeightIds) != 0:
        print("\n")
        output = "";
        first = True;
        for statement in setOfMeaningfulWeightIds:
            if not first:
                output += ", "
            else:
                first = False
            output += '"'
            output += statement
            output += '"'
        print("Mapgen " + name + " with meaningful weights:\n[ " + output + " ]")


#print_results(omtMeaningfulWeightIds, "om_terrains")
#print_results(nestMeaningfulWeightIds, "nested_mapgen_ids")
#print_results(updateMeaningfulWeightIds, "update_mapgen_ids")

def deleteMeaninglessWeights(path):
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
        for jo in json_data:
            if isinstance(jo, dict):
                if jo["type"] == "mapgen" and "weight" in jo:
                    # Regular map
                    if "om_terrain" in jo:
                        # String
                        if type(jo["om_terrain"]) is str:
                            if not jo["om_terrain"] in omtMeaningfulWeightIds:
                                jo.pop("weight")
                                change = True
                        # Array
                        else:
                            omts = set()
                            for value in jo["om_terrain"]:
                                # Multiple om_terrains sharing a map or just pointlessly arrayed single value
                                if type(value) is str:
                                    if value != "null":
                                        omts.add(value)
                                # Multi-omt map
                                else:
                                    for part in value:
                                        if part != "null":
                                            omts.add(part)
                            if len(omts & omtMeaningfulWeightIds) == 0:
                                jo.pop("weight")
                                change = True
                    elif "nested_mapgen_id" in jo:
                        if not jo["nested_mapgen_id"] in nestMeaningfulWeightIds:
                            jo.pop("weight")
                            change = True
                    elif "update_mapgen_id" in jo:
                        if not jo["update_mapgen_id"] in updateMeaningfulWeightIds:
                            jo.pop("weight")
                            change = True
        return json_data if change else None


def format_json(path):
    file_path = os.path.dirname(__file__)
    format_path_linux = os.path.join(file_path, "/format/json_formatter.cgi")
    path_win = "../json_formatter.exe"
    format_path_win = os.path.join(file_path, path_win)
    if os.path.exists(format_path_linux):
        os.system(f"{format_path_linux} {path}")
    elif os.path.exists(format_path_win):
        os.system(f"{format_path_win} {path}")
    else:
        print("No json formatter found")


for root, directories, filenames in os.walk(args_dict["ddadir"]):
    for filename in filenames:
        path = os.path.join(root, filename)
        if path.endswith(".json"):
            new = deleteMeaninglessWeights(path)
            if new is not None:
                with open(path, "w", encoding="utf-8") as jf:
                    json.dump(new, jf, ensure_ascii=False)
                format_json(path)

for root, directories, filenames in os.walk(args_dict["modsdir"]):
    for filename in filenames:
        path = os.path.join(root, filename)
        if path.endswith(".json"):
            new = deleteMeaninglessWeights(path)
            if new is not None:
                with open(path, "w", encoding="utf-8") as jf:
                    json.dump(new, jf, ensure_ascii=False)
                format_json(path)

if len(failures) != 0:
    print("\nErrors:\n[")
    for statement in failures:
        print(statement)
    print("]")
