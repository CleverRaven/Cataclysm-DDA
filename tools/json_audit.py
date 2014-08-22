#!/usr/bin/env python

# Run from the commandline with
#
#     python json_audit.py
#
# Updated by mafagafogigante @ 29/07/2014
# I did NOT thoroughly tested this script. Seems to be A-okay, though.
# PEP-8 compliant: 100%.
# Apart from the 2 to 3 'migration' and PEP-8 fixes, some (really) small modifications were made.

from __future__ import print_function

import sys
import os
import json
from fnmatch import fnmatch
from collections import Counter

SCRIPT_DIR = os.path.abspath(os.path.dirname(__file__))
JSON_DIR = os.path.normpath(os.path.join(SCRIPT_DIR, "../data/json"))
FILE_MATCH = "*.json"
SEARCH_KEY = "material"
WHERE_KEY = None
WHERE_VALUE = None

# What can I say? I am sorry, I just do not like the idea of having 2 different scripts. Again, I am sorry.
try:
    input = raw_input
except NameError:
    pass
# Sorry for making you read those 4 lines. It will surely pass.


def get_json_files(fmatch):
    """Use a UNIX like file match expression to weed out the JSON files.
    """
    data = []
    for d_descriptor in os.walk(JSON_DIR):
        d = d_descriptor[0]
        for f in d_descriptor[2]:
            if fnmatch(f, fmatch):
                json_file = os.path.join(d, f)
                with open(json_file, "r") as file:
                    try:
                        candidates = json.load(file)
                    except Exception as err:
                        print("Problem reading file %s, reason: %s" % (json_file, err))
                    if type(candidates) != list:
                        print("Problem reading file %s, reason: expected a list." % json_file)
                    else:
                        data += candidates
    return data

#print "Script running in", SCRIPT_DIR
print("I'm a Cataclysm DDA JSON parser.")
print("I read a clump of JSON files, count occurrences, and print them out.")
print("\nAssuming the Cataclysm DDA JSON directory is:", JSON_DIR)
print("If the above directory is wrong, ctrl-c out and fix the script.")

userin = input("Which files should be read? [default: %s]:\n" % FILE_MATCH)
FILE_MATCH = userin.strip() or FILE_MATCH

print("Finding eligible JSON files.")
# Single list of all JSON blobs found in each file.
JSON_DATA = get_json_files(FILE_MATCH)
if not JSON_DATA:
    print("We could not find any JSON data in")
    print("\t", JSON_DIR)
    print("that matched:")
    print("\t", FILE_MATCH)
    print("good bye.")
    sys.exit()

print("Found %s blobs of JSON data." % len(JSON_DATA))

print("Which JSON key should we aggregate and count?")
userin = input("[default: '%s', or 'list-keys' to list keys on blobs]\n" % SEARCH_KEY)
SEARCH_KEY = userin.strip() or SEARCH_KEY

# Offer a where clause for non-describe searches 
if SEARCH_KEY != "list-keys":
    print("Add a where clause, or just hit return for select * equivalent.")
    userin = input("[default: %s or list a key to compare]\n" % WHERE_KEY)
    WHERE_KEY = userin.strip() or WHERE_KEY
    if WHERE_KEY:
        print("Which value should '%s' be equal to?" % WHERE_KEY)
        userin = input("[default: %s]\n" % WHERE_VALUE) 
        WHERE_VALUE = userin or WHERE_VALUE

if SEARCH_KEY == "list-keys":
    # special case, handle and exit
    all_keys = set()
    for b in JSON_DATA:
        all_keys.update(list(b.keys()))
    print("%s keys found in all the blobs:\n" % len(all_keys))
    key_field_len = len(max(all_keys, key=len))+1
    # Make small screen friendly.
    cols = 80/key_field_len
    iters = 0
    all_keys = sorted(all_keys)
    while all_keys:
        key = all_keys.pop(0)
        print(key.ljust(key_field_len), end=' ')
        iters += 1
        if iters % cols == 0:
            print("")
    print("")
    sys.exit()

def matches_where(item, where_key, where_value):
    """True if:
    where_key exists AND
    where_value somehow matches (either directly or guesstimated).

    False if:
    no where_key passed in
    where_key not in item
    where_key in item but where_value does not match
    """
    if not where_key:
        return True
    if where_key not in item:
        return False
    # So we have some value.
    item_value = item[where_key]
    # Matching interpolation for keyboard constrained input.
    if type(item_value) == str or type(item_value) == unicode:
        # Direct match
        return where_value == item_value
    elif type(item_value) == int or type(item_value) == float:
        # via a string match
        return where_value == str(item_value)
    elif type(item_value) == list:
        # 1 level deep search at the moment
        return where_value in item_value
    elif type(item_value) == dict:
        # Perhaps not the correct logic...
        return where_value in item_value
    else:
        return False

    

def value_counter(search_key, data, where_key, where_value):
    """Takes a search_key {str}, and for values found in data {list of dicts}
    with those keys, counts the values.
    
    Returns a tuple of data.
    """
    stats = Counter()
    # Which blobs had our search key?
    blobs_matched = 0
    for item in data:
        if search_key in item and matches_where(item, where_key, where_value):
            v = item[search_key]
            blobs_matched += 1
            if type(v) == list:
                stats.update(v)
            elif type(v) == int or type(v) == float:
                # Cast to string.
                stats[str(v)] += 1
            else:
                # assume string
                stats[v] += 1
    # Proposed title of table, counts of values, matched blobs out of total blobs that had key
    return "Count of %s values in item definitions" % search_key, stats, blobs_matched


title, stats, matches = value_counter(SEARCH_KEY, JSON_DATA, WHERE_KEY, WHERE_VALUE)
if not stats:
    print("Sorry, didn't find any stats for '%s' in the JSON." % SEARCH_KEY)
    sys.exit()

# Display....
if hasattr(stats, "most_common"):
    # we're a counter
    key_vals = stats.most_common()
else:
    # we're an unsorted dict
    key_vals = list(stats.items())

print("\n\n%s" % title)
print("(Data from %s out of %s blobs)" % (matches, len(JSON_DATA)))
print("-" * len(title))
# Values in left column, counts in right, left column as wide as longest string length.
key_field_len = len(max(list(stats.keys()), key=len))+1
output_template = "%%-%ds: %%s" % key_field_len
for k_v in key_vals:
    print(output_template % k_v)
