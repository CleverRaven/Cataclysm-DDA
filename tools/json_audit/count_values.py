#!/usr/bin/env python
"""Usage info:

    count_values.py -h

"""

from __future__ import print_function

import argparse
import sys
import os
import json
from util import import_data, value_counter_all_wheres, ui_counts_to_columns,\
        matches_all_wheres, CDDAJSONWriter, WhereAction

parser = argparse.ArgumentParser(description="""Count the number of times a specific values occurs
for a specific key.

Example usages:

    # What values are present in the material key?
    %(prog)s --human -k material

    # What cost values are in bionics that are active?
    %(prog)s --key=cost type=bionic active=true
""", formatter_class=argparse.RawDescriptionHelpFormatter)
parser.add_argument("--human",
        action="store_true",
        help="if set, makes output human readable. default is to return output in JSON dictionary.")
parser.add_argument("-k", "--key",
        required=True, type=str,
        help="key on JSON objects from which to count values")
parser.add_argument("where",
        action=WhereAction, nargs='*', type=str,
        help="where exclusions of the form 'where_key=where_val', no quotes.")


if __name__ == "__main__":
    args = parser.parse_args()
    search_key = args.key
    json_data, _ = import_data()
    stats, num_matches = value_counter_all_wheres(json_data, search_key, args.where)

    if args.human:
        if not stats:
            print("Sorry, didn't find any stats for '%s' in the JSON." % search_key)
            sys.exit(1)

        title = "Count of values from field '%s'" % search_key
        print("\n\n%s" % title)
        print("(Data from %s out of %s blobs)" % (num_matches, len(json_data)))
        print("-" * len(title))
        ui_counts_to_columns(stats)
    else:
        if not stats:
            # Still JSON parser friendly, indicator of fail with emptiness.
            print(json.dumps([]))
            sys.exit(1)
        else:
            print(json.dumps(stats))
