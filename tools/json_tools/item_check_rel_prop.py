#!/usr/bin/env python3

#Takes a list of fields from an object and groups them inside a new inline object 

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
    
    printid = "unidentified object"
    if "id" in jo:
        printid = jo["id"]
    elif "abstract" in jo:
        printid = jo["abstract"]
    
    if "relative" in jo:
        for key in jo["relative"]:
            if key in jo:
                print(printid + " has relative for already-defined field " + key)
    if "proportional" in jo:
        for key in jo["proportional"]:
            if key in jo:
                print(printid + " has proportional for already-defined field " + key)
    """
    if new_object_name in jo:
        if "relative" in jo:
            if new_object_name in jo["relative"]:
                print(printid + " has an extra relative field")
        if "proportional" in jo:
            if new_object_name in jo["proportional"]:
                print(printid + " has an extra proportional field")
    """
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

    return json_data if change else None

if __name__ == "__main__":
    for root, directories, filenames in os.walk(args_dict["dir"]):
        for filename in filenames:
            if not filename.endswith(".json"):
                continue

            path = os.path.join(root, filename)
            new = gen_new(path, args_dict["modified_type"])
