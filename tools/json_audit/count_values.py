#!/usr/bin/env python
"""Grep data from cataclysm data and grab count of number of occurences of
values.

    python json_audit.py

Updated by mafagafogigante @ 29/07/2014
"""

from __future__ import print_function

import sys
import os
import json
from util import ui_import_data, value_counter, JSON_DIR, JSON_FNMATCH

# Normalize between Python 2.x and 3.x.
try:
    input = raw_input
except NameError:
    pass

search_key = "material"
where_key = None
where_value = None
raw_output = False

print("\nAssuming Cataclysm DDA JSON directory is:", JSON_DIR)
print("Assuming files match glob expression: ", JSON_FNMATCH)

json_data = ui_import_data()

print("Which JSON key should we aggregate and count?")
userin = input("[default: '%s']\n" % search_key)
search_key = userin.strip() or search_key

print("Include raw output?")
userin = input("[default: '%s', type something for yes, just hit return for no]\n" % raw_output)
raw_output = True if userin.strip() else False

print("Add a where clause to filter, or hit return to skip filtering.")
userin = input("[default: %s or list a key to compare]\n" % where_key)
where_key = userin.strip() or where_key
if where_key:
    print("Which value should '%s' be equal to?" % where_key)
    userin = input("[default: %s]\n" % where_value)
    where_value = userin or where_value

stats, num_matches = value_counter(search_key, json_data, where_key, where_value)
if not stats:
    print("Sorry, didn't find any stats for '%s' in the JSON." % search_key)
    sys.exit()

# Display, assuming a counter
key_vals = stats.most_common()

title = "Count of values from field '%s'" % search_key
print("\n\n%s" % title)
print("(Data from %s out of %s blobs)" % (num_matches, len(json_data)))
print("-" * len(title))
# Values in left column, counts in right, left column as wide as longest string length.
key_field_len = len(max(list(stats.keys()), key=len))+1
output_template = "%%-%ds: %%s" % key_field_len
for k_v in key_vals:
    print(output_template % k_v)

if raw_output:
    print("\n\nAnd here is your raw output:\n")
    print(json.dumps(key_vals))
