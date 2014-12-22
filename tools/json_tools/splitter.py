#!/usr/bin/env python
"""Run this script with -h for usage info and docs.
"""

from __future__ import print_function

import sys
import os
import json
import argparse
from util import import_data, matches_all_wheres, CDDAJSONWriter, WhereAction

parser = argparse.ArgumentParser(description="""Taking a filtered list of JSON data,
look for all items that match the where clause and output to stdout. All items
that don't match are output to stderr.

Example usages:

    # Anything of type dream and strength 2 goes to stdout
    # everything else goes to stderr
    %(prog)s --fnmatch=dreams.json type=dream strength=2 1>matched.json 2>not_matched.json
""", formatter_class=argparse.RawDescriptionHelpFormatter)
parser.add_argument("--fnmatch",
        default="*.json",
        help="override with glob expression to select a smaller fileset.")
parser.add_argument("where",
        action=WhereAction, nargs='+', type=str,
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

    matched = []
    not_matched = []
    for item in json_data:
        if matches_all_wheres(item, args.where):
            matched.append(item)
        else:
            not_matched.append(item)

    # matched first
    print("[")
    for i, p in enumerate(matched):
        eol = ",\n" if i < len(matched)-1 else "\n"
        print(CDDAJSONWriter(p, 1).dumps(), end=eol)
    print("]")

    # not_matched second
    print("[", file=sys.stderr)
    for i, p in enumerate(not_matched):
        eol = ",\n" if i < len(not_matched)-1 else "\n"
        print(CDDAJSONWriter(p, 1).dumps(), end=eol, file=sys.stderr)
    print("]", file=sys.stderr)
