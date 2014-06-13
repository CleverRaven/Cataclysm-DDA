#!/usr/bin/env python

# Run from the commandline with
#
#     python json_audit.py
#
# Should work with vanilla Python 2.7+.
# Although unfortunately won't work as is with Python 3+.

import sys
import os
import json
from fnmatch import fnmatch
from collections import Counter

SCRIPT_DIR = os.path.abspath(os.path.dirname(__file__))
JSON_DIR = os.path.normpath(os.path.join(SCRIPT_DIR, "../data/json"))
FILE_MATCH = "*.json"
SEARCH_KEY = "material"

# Single list of all JSON blobs found in each file.
JSON_DATA = []

def get_json_files(fmatch):
    """Use a UNIX like file match expression to weed out the JSON files.
    """
    data = []
    for d_descriptor in os.walk(JSON_DIR):
        d = d_descriptor[0]
        for f in d_descriptor[2]:
            if fnmatch(f, fmatch):
                json_file = os.path.join(d, f)
                with open(json_file, "r") as f:
                    try:
                        candidates = json.load(f)
                    except Exception as err:
                        print "Problem reading file %s, reason: %s" % (json_file, err)
                    if type(candidates) != list:
                        print "Problem reading file %s, reason: expected a list." % json_file
                    else:
                        data += candidates
    return data

#print "Script running in", SCRIPT_DIR
print "I'm a Cataclysm DDA JSON parser."
print "I read a clump of JSON files, count occurrences, and print them out."
print "\nAssuming the Cataclysm DDA JSON directory is:", JSON_DIR
print "If the above directory is wrong, ctrl-c out and fix the script."

userin = raw_input("Which files should be read? [default: %s]:\n" % FILE_MATCH)
FILE_MATCH = userin.strip() or FILE_MATCH

print "Finding elligible JSON files."
JSON_DATA = get_json_files(FILE_MATCH)
if not JSON_DATA:
    print "We could not find any JSON data in"
    print "\t", JSON_DIR
    print "that matched:"
    print "\t", FILE_MATCH
    print "good bye."
    sys.exit()

print "Found %s blobs of JSON data." % len(JSON_DATA)

userin = raw_input("Which JSON key should we aggregate and count?\n[default: '%s', or 'list-keys' to list keys on blobs]\n" % SEARCH_KEY)
SEARCH_KEY = userin.strip() or SEARCH_KEY



if SEARCH_KEY == "list-keys":
    # special case, handle and exit
    all_keys = set()
    for b in JSON_DATA:
        all_keys.update(b.keys())
    print "%s keys found in all the blobs:\n" % len(all_keys)
    key_field_len = len(max(all_keys, key=len))+1
    # Make small screen friendly.
    cols = 80/key_field_len
    iters = 0
    all_keys = sorted(all_keys)
    while all_keys:
        key = all_keys.pop(0)
        print key.ljust(key_field_len),
        iters += 1
        if iters % cols == 0:
            print ""
    sys.exit()



def value_counter(search_key, data):
    """Takes a search_key {str}, and for values found in data {list of dicts}
    with those keys, counts the values.
    
    Returns a tuple of data.
    """
    stats = Counter()
    # Which blobs had our search key?
    blobs_matched = 0
    for item in data:
        if search_key in item:
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


title, stats, matches = value_counter(SEARCH_KEY, JSON_DATA)
if not stats:
    print "Sorry, didn't find any stats for '%s' in the JSON." % SEARCH_KEY
    sys.exit()



# Display....
if hasattr(stats, "most_common"):
    # we're a counter
    key_vals = stats.most_common()
else:
    # we're an unsorted dict
    key_vals = stats.items()

print "\n\n%s" % title
print "(Data from %s out of %s blobs)" % (matches, len(JSON_DATA))
print "-" * len(title)
# Values in left column, counts in right, left column as wide as longest string length.
key_field_len = len(max(stats.keys(), key=len))+1
output_template = "%%-%ds: %%s" % key_field_len
for k_v in key_vals:
    print output_template % k_v

