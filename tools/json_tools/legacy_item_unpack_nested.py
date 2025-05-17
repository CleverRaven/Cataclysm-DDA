#!/usr/bin/env python3

#Takes an inline object (e.g. `book_data`) and unpacks it into the parent object, adding a new subtype if needed

import argparse
import json
import os
import sys

args = argparse.ArgumentParser()
args.add_argument("dir", action="store", help="specify json directory")
args.add_argument("object_key", action="store", help="provide inline object key to unpack")
args.add_argument("new_subtype", action="store", help="provide new subtype to add")
args_dict = vars(args.parse_args())


def object_key_check(jo, object_key, new_subtype):
    if object_key not in jo:
        return None
        
    printid = "unidentified object"
    if "id" in jo:
        printid = jo["id"]
    elif "abstract" in jo:
        printid = jo["abstract"]
    
    for key in jo[object_key]:
        if key in jo:
            print(key+" already exists in "+printid)
            return None
        jo[key] = jo[object_key][key]
    del jo[object_key]
    
    if "subtypes" not in jo:
        pos = list(jo.keys()).index("type")
        items = list(jo.items())
        items.insert(pos + 1, ("subtypes", []))
        jo = dict(items)
    if new_subtype not in jo["subtypes"]:
        jo["subtypes"].append(new_subtype)
    if len(jo["subtypes"]) == 0:
        del jo["subtypes"]
    
    return jo

def gen_new(path, object_key, new_subtype):
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

            new_data = object_key_check(jo, object_key, new_subtype)
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
            new = gen_new(path, args_dict["object_key"], args_dict["new_subtype"])
            if new is not None:
                with open(path, "w", encoding="utf-8") as jf:
                    json.dump(new, jf, ensure_ascii=False)
