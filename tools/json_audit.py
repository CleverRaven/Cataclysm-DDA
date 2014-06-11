#!/usr/bin/env python

import os
import json
from fnmatch import fnmatch
from collections import Counter

SCRIPT_DIR = os.path.abspath(os.path.dirname(__file__))
JSON_DIR = os.path.normpath(os.path.join(SCRIPT_DIR, "../data/json"))
FILE_MATCH = "*.json"


print "Script running in", SCRIPT_DIR
print "Assuming JSON directory is", JSON_DIR
print "Grabbing files that match glob", FILE_MATCH



# utility functions
def total_num_objects(data):
    return "Total number of objects:", { "# objects": len(data) }

def material_counts(data):
    materials = Counter()
    for item in data:
        if "material" in item:
            m = item["material"]
            if type(m) == list:
                materials.update(m)
            else:
                # assume string
                materials[m] += 1
    return "Count of materials used in item definitions", materials

def ammo_counts(data):
    ammo = Counter()
    for item in data:
        if "ammo" in item:
            a = item["ammo"]
            if type(a) == list:
                ammo.update(a)
            else:
                # assume string
                ammo[a] += 1
    return "Count of ammo types used in item definitions", ammo

def flag_counts(data):
    flags = Counter()
    for item in data:
        if "flags" in item:
            f = item["flags"]
            if type(f) == list:
                flags.update(f)
            else:
                # assume string
                flags[f] += 1
    return "Counts of flags used in item definitions", flags

def effect_counts(data):
    effects = Counter()
    for item in data:
        if "effects" in item:
            e = item["effects"]
            if type(e) == list:
                effects.update(e)
            else:
                # assume string
                effects[e] += 1
    return "Count of effects used in item definitions", effects

def covers_counts(data):
    c = Counter()
    for item in data:
        if "covers" in item:
            val = item["covers"]
            if type(val) == list:
                c.update(val)
            else:
                # assume string
                c[str(val)] += 1
    return "Count of covers used in item definitions", c

def storage_counts(data):
    c = Counter()
    for item in data:
        if "storage" in item:
            val = item["storage"]
            if type(val) == list:
                c.update(val)
            else:
                # assume string
                c[str(val)] += 1
    return "Count of storage used in item definitions", c

def type_counts(data):
    types = Counter()
    for item in data:
        if "type" in item:
            types[item["type"]] += 1
    return "Count of types used in item definitions", types

def symbol_counts(data):
    symbols = Counter()
    for item in data:
        if "symbol" in item:
            symbols[item["symbol"]] += 1
    return "Count of symbols used in item definitions", symbols

def color_counts(data):
    colors = Counter()
    for item in data:
        if "color" in item:
            colors[item["color"]] += 1
    return "Count of colors used in item definitions", colors

def category_counts(data):
    categories = Counter()
    for item in data:
        if "category" in item:
            c = item["category"]
            if type(c) == list:
                categories.update(c)
            else:
                # assume string
                categories[c] += 1
    return "Count of categories used in item definitions", categories


# Single list of all JSON blobs found in each file.
JSON_DATA = []

# main....
print "Finding elligible JSON files."
for d_descriptor in os.walk(JSON_DIR):
    d = d_descriptor[0]
    for f in d_descriptor[2]:
        if fnmatch(f, FILE_MATCH):
            json_file = os.path.join(d, f)
            with open(json_file, "r") as f:
                try:
                    candidates = json.load(f)
                except Exception as err:
                    print "Problem reading file %s, reason: %s" % (json_file, err)
                if type(candidates) != list:
                    print "Problem reading file %s, reason: expectd a list." % json_file
                else:
                    JSON_DATA += candidates


print "Running stat functions on data..."
processes = [
    total_num_objects,
    category_counts,
    type_counts,
    material_counts,
    ammo_counts,
    flag_counts,
    effect_counts,
    covers_counts,
    storage_counts,
    symbol_counts,
    color_counts,
]

for fn in processes:
    title, stats = fn(JSON_DATA)
    # Key field length
    key_field_len = len(max(stats.keys(), key=len))+1
    # check if we're a Counter
    if hasattr(stats, "most_common"):
        key_vals = stats.most_common()
    else:
        key_vals = stats.items()
    
    output_template = "%%-%ds: %%s" % key_field_len
    print "\n\n%s" % title
    print "-" * len(title)
    for k_v in key_vals:
        print output_template % k_v

