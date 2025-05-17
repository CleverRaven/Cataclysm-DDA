#!/usr/bin/env python3

#Renames legacy item "type" field to "ITEM"

import argparse
import json
import os

args = argparse.ArgumentParser()
args.add_argument("dir", action="store", help="specify json directory")
args.add_argument("modified_type", action="store", help="provide JSON type field to modify")
args_dict = vars(args.parse_args())

def object_type_check(jo, modified_type):
    if jo["type"] != modified_type:
        return None
    jo["type"] = "ITEM"
    
    if "subtypes" not in jo:
        pos = list(jo.keys()).index("type")
        items = list(jo.items())
        items.insert(pos + 1, ("subtypes", []))
        jo = dict(items)
    if modified_type not in jo["subtypes"]:
        if modified_type != "GENERIC":
            if modified_type == "TOOL_ARMOR":
                if "TOOL" not in jo["subtypes"]:
                    jo["subtypes"].append("TOOL")
                if "ARMOR" not in jo["subtypes"]:
                    jo["subtypes"].append("ARMOR")
            else:
                jo["subtypes"].append(modified_type)

    if len(jo["subtypes"]) == 0:
        del jo["subtypes"]

    return jo


def gen_new(path, modified_type):
    change = False
    # Having troubles with this script halting?
    # Uncomment below to find the file it's halting on
    # print(path)
    with open(path, "r", encoding="utf-8") as json_file:
        try:
            json_data = json.load(json_file)
        except json.JSONDecodeError:
            print(f"Json Decode Error at {path}")
            print("Ensure that the file is a JSON file consisting of"
                  " an array of objects!")
            return None
        
        new_json_data = []
        for jo in json_data:
            if type(jo) is not dict:
                print(f"Incorrectly formatted JSON data at {path}")
                print("Ensure that the file is a JSON file consisting"
                      " of an array of objects!")
                break

            new_data = object_type_check(jo, modified_type)
            if new_data is not None:
                jo = new_data
                change = True
            new_json_data.append(jo)

    return new_json_data if change else None

if __name__ == "__main__":
    for root, directories, filenames in os.walk(args_dict["dir"]):
        for filename in filenames:
            if not filename.endswith(".json"):
                continue

            path = os.path.join(root, filename)
            new = gen_new(path, args_dict["modified_type"])
            if new is not None:
                with open(path, "w", encoding="utf-8") as jf:
                    json.dump(new, jf, ensure_ascii=False)
