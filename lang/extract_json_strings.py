#!/usr/bin/env python
"Extract translatable strings from the .json files in data/raw."

from __future__ import print_function

import json
import os

# allow running from main directory, or from script subdirectory
if os.path.exists("data/raw"):
    raw_folder = "data/raw"
    to_folder = "lang/json"
elif os.path.exists("../data/raw"):
    raw_folder = "../data/raw"
    to_folder = "../lang/json"
else:
    print("Error: Couldn't find the 'data/raw' subdirectory.")
    exit(1)

def gettextify(string):
    "Put the string in a fake gettext call, and add a newline."
    return "_(%r)\n" % string

# create the output directory, if it does not already exist
if not os.path.exists(to_folder):
    os.mkdir(to_folder)

# extract "name" and "description" fields from each file in data/raw/items
with open(os.path.join(to_folder,"json_items.py"), 'w') as items_jtl:
    for filename in os.listdir(os.path.join(raw_folder,"items")):
        jsonfile = os.path.join(raw_folder, "items", filename)
        jsondata = json.loads(open(jsonfile).read())
        names = [item["name"] for item in jsondata]
        descriptions = [item["description"] for item in jsondata]
        for name in names:
            items_jtl.write(gettextify(name))
        for description in descriptions:
            items_jtl.write(gettextify(description))

