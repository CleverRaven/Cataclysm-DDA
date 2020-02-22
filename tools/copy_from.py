#!/usr/bin/env python3

import json
import argparse
import os
import copy
import math
import string
from operator import itemgetter, attrgetter


IGNORE_MISMATCH = ["id", "abstract"]

def get_data(argsDict, resource_name):
   resource = []
   resource_sources = argsDict.get(resource_name, [])
   if not isinstance(resource_sources, list):
       resource_sources = [resource_sources]
   for resource_filename in resource_sources:
       if resource_filename.endswith(".json"):
          try:
              with open(resource_filename) as resource_file:
                  resource += json.load(resource_file)
          except FileNotFoundError:
              exit("Failed: could not find {}".format(resource_filename))
       else:
           print("Invalid filename {}".format(resource_filename))
   if not resource:
        exit("Failed: {} was empty".format(resource_filename))
   return resource


args = argparse.ArgumentParser(description="Make items use copy-from.")
args.add_argument("item_source", action="store",
                  help="specify json file to rewrite using copy-from.")
args.add_argument("--output", dest="output_name", action="store",
                  help="Name of output file.  Defaults to the command line.")
argsDict = vars(args.parse_args())

items = get_data(argsDict, "item_source")

base_item = items[0]
base_id = base_item.get("id") or base_item.get("abstract")
if not base_id:
    exit("Failure: first item malformed")
for item in items:
    if item == base_item:
        continue
    if item.get("copy-from"):
        continue
    del_keys = []
    for key in item:
        if key == "type":
           continue
        if item[key] == base_item.get(key):
           del_keys.append(key)
    can_copy = True
    for key in base_item:
        if key in del_keys:
            continue
        if key in IGNORE_MISMATCH:
            continue
        if key not in item:
            can_copy = False
            break

    if not can_copy:
        continue

    for key in del_keys:
        del item[key]
    item["copy-from"] = base_id

output_name = argsDict.get("output_name", "")
if output_name and not output_name.endswith(".json"):
    output_name += ".json"

print(json.dumps(items, indent=2))
