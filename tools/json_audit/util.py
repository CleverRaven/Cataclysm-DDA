"""Utility stuff for json tools.
"""

from __future__ import print_function

import sys
import os
import json
import re
from collections import Counter, OrderedDict
from fnmatch import fnmatch

SCRIPT_DIR = os.path.abspath(os.path.dirname(__file__))
JSON_DIR = os.path.normpath(os.path.join(SCRIPT_DIR, "..", "..", "data", "json"))
JSON_FNMATCH = "*.json"



def import_data(json_dir=JSON_DIR, json_fmatch=JSON_FNMATCH):
    """Use a UNIX like file match expression to weed out the JSON files.

    returns tuple, first element containing json read, second element containing
    list of any errors found. error list will be empty if no errors
    """
    data = []
    errors = []
    candidates = None
    for d_descriptor in os.walk(json_dir):
        d = d_descriptor[0]
        for f in d_descriptor[2]:
            if fnmatch(f, json_fmatch):
                json_file = os.path.join(d, f)
                with open(json_file, "r") as file:
                    try:
                        candidates = json.load(file, object_pairs_hook=OrderedDict)
                    except Exception as err:
                        errors.append("Problem reading file %s, reason: %s" % (json_file, err))
                    if type(candidates) != list:
                        errors.append("Problem parsing data from file %s, reason: expected a list." % json_file)
                    else:
                        data += candidates
    return (data, errors)



def match_primitive_values(item_value, where_value):
    """Perform any odd logic on item matching.
    """
    # Matching interpolation for keyboard constrained input.
    if type(item_value) == str or type(item_value) == unicode:
        # Direct match, and don't convert unicode in Python 2.
        return bool(re.search(re.escape(where_value), item_value))
    elif type(item_value) == int or type(item_value) == float:
        # match after string conversion
        return bool(re.search(re.escape(where_value), str(item_value)))
    else:
        return False



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
    if type(item_value) == list:
        # 1 level deep.
        for next_level in item_value:
            if match_primitive_values(next_level, where_value):
                return True
        # else...
        return False
    elif type(item_value) == dict:
        # Match against the keys of the dictionary... I question my logic.
        # 1 level deep.
        for next_level in item_value:
            if match_primitive_values(next_level, where_value):
                return True
        # else...
        return False
    else:
        return match_primitive_values(item_value, where_value)


def distinct_keys(data):
    """Return a sorted-ascending list of keys scraped from the list of data
    assumed to be dictionaries.
    """
    all_keys = set()
    for d in data:
        all_keys.update(list(d.keys()))
    return sorted(all_keys)



def value_counter(data, search_key, where_key=None, where_value=None):
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
    return stats, blobs_matched



def ui_import_data(json_dir=JSON_DIR, json_fmatch=JSON_FNMATCH):
    """Load data with import data and provide user friendly output.

    Will cause program to exit if bad errors occur, call at your own risk.

    If no errors, returns just the list of imported JSON dictionaries.
    """
    print("Finding eligible JSON files.")
    # Single list of all JSON blobs found in each file.
    json_data, import_errors = import_data(json_dir, json_fmatch)
    if import_errors:
        # Considered benign errors.
        print("There were import errors:")
        for e in import_errors:
            print(e)
        print("")
    elif not json_data:
        # Not considered benign.
        print("We could not find any JSON data in")
        print("\t", JSON_DIR)
        print("that matched:")
        print("\t", JSON_FNMATCH)
        print("good bye.")
        sys.exit(1)
    print("Found %s blobs of JSON data." % len(json_data))
    return json_data



def ui_values_to_columns(values, screen_width=80):
    """Take a list of strings and output in fixed width columns.
    """
    max_val_len = len(max(values, key=len))+1
    cols = screen_width/max_val_len
    iters = 0
    for v in values:
        print(v.ljust(max_val_len), end=' ')
        iters += 1
        if iters % cols == 0:
            print("")
    print("")



def ui_counts_to_columns(counts):
    """Take a Counter instance and display in single fixed width key:value
    column.
    """
    # Values in left column, counts in right, left column as wide as longest string length.
    key_vals = counts.most_common()
    key_field_len = len(max(list(counts.keys()), key=len))+1
    output_template = "%%-%ds: %%s" % key_field_len
    for k_v in key_vals:
        print(output_template % k_v)
