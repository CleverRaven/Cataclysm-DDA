#!/usr/bin/env python3

#Takes a list of fields from an object and groups them inside a new inline object 

import argparse
import json
import os

args = argparse.ArgumentParser()
args.add_argument("dir", action="store", help="specify json directory")
args.add_argument("object_key", action="store", help="provide inline object key")
args.add_argument("old_object_field", action="store", help="provide inline object's original field name")
args.add_argument("new_object_field", action="store", help="provide inline object's new field name")
args_dict = vars(args.parse_args())

def object_key_check(jo, object_key, old_field, new_field):
    if object_key not in jo:
        return None
    if old_field in jo[object_key]:
        jo[object_key][new_field] = jo[object_key][old_field]
        del jo[object_key][old_field]
    return jo

def gen_new(path, object_key, old_field, new_field):
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

            new_data = object_key_check(jo, object_key, old_field, new_field)
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
            new = gen_new(path, args_dict["object_key"], args_dict["old_object_field"], args_dict["new_object_field"])
            if new is not None:
                with open(path, "w", encoding="utf-8") as jf:
                    json.dump(new, jf, ensure_ascii=False)
