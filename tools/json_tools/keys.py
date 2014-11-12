#!/usr/bin/env python
"""Run this script with -h for usage info and docs.
"""

from __future__ import print_function

import sys
import os
import json
import argparse
from util import import_data, key_counter, ui_counts_to_columns,\
        matches_all_wheres, CDDAJSONWriter, WhereAction

parser = argparse.ArgumentParser(description="""Count the number of times a specific key occurs.

Example usages:

    # List all keys, human friendly.
    %(prog)s --human

    # List keys on JSON objects of type "bionic", output in JSON.
    %(prog)s type=bionic
""", formatter_class=argparse.RawDescriptionHelpFormatter)
parser.add_argument("--fnmatch",
        default="*.json",
        help="override with glob expression to select a smaller fileset.")
parser.add_argument("--human",
        action="store_true",
        help="if set, makes output human readable. default is to return output in JSON.")
parser.add_argument("where",
        action=WhereAction, nargs='*', type=str,
        help="where exclusions of the form 'where_key=where_val', no quotes.")



if __name__ == "__main__":
    args = parser.parse_args()

    json_data, load_errors = import_data(json_fmatch=args.fnmatch)
    if load_errors:
        # If we start getting unexpected JSON or other things, might need to
        # revisit quitting on load_errors
        print("Error loading JSON data.")
        for e in load_errrors:
            print(e)
        sys.exit(1)
    elif not json_data:
        print("No data loaded.")
        sys.exit(1)

    stats, num_matches = key_counter(json_data, args.where)

    if not stats:
        print("Nothing found.")
        sys.exit(1)

    if args.human:
        title = "Count of keys"
        print("\n\n%s" % title)
        print("(Data from %s out of %s blobs)" % (num_matches, len(json_data)))
        print("-" * len(title))
        ui_counts_to_columns(stats)
    else:
        print(json.dumps(stats))
