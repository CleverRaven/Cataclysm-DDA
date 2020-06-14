#!/usr/bin/env python2
"""Run this script with -h for usage info and docs.
"""

from __future__ import print_function

import argparse
import sys
import json
from util import import_data, value_counter, ui_counts_to_columns,\
        WhereAction

parser = argparse.ArgumentParser(description="""Count the number of times a specific values occurs
for a specific key.

Example usages:

    # What values are present in the material key?
    %(prog)s --human -k material

    # What cost values are in bionics that are active?
    %(prog)s --key=cost type=bionic active=true
""", formatter_class=argparse.RawDescriptionHelpFormatter)
parser.add_argument("--fnmatch",
        default="*.json",
        help="override with glob expression to select a smaller fileset.")
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

    json_data, load_errors = import_data(json_fmatch=args.fnmatch)
    if load_errors:
        # If we start getting unexpected JSON or other things, might need to
        # revisit quitting on load_errors
        print("Error loading JSON data.")
        for e in load_errors:
            print(e)
        sys.exit(1)
    elif not json_data:
        print("No data loaded.")
        sys.exit(1)

    stats, num_matches = value_counter(json_data, search_key, args.where)

    if not stats:
        print("Nothing found.")
        sys.exit(1)

    if args.human:
        title = "Count of values from field '%s'" % search_key
        print("\n\n%s" % title)
        print("(Data from %s out of %s blobs)" % (num_matches, len(json_data)))
        print("-" * len(title))
        ui_counts_to_columns(stats)
    else:
        print(json.dumps(stats))
