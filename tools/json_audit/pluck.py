#!/usr/bin/env python
"""Usage info:

    pluck.py -h

"""

from __future__ import print_function

import sys
import os
import json
import argparse
from util import import_data, matches_all_wheres, CDDAJSONWriter, WhereAction

parser = argparse.ArgumentParser(description="""Search for matches within the json data.

Example usages:

    %(prog)s --all type=dream strength=2

    %(prog)s material=plastic material=steel
""", formatter_class=argparse.RawDescriptionHelpFormatter)
parser.add_argument("--all",
        action="store_true",
        help="if set, includes all matches. if not set, includes first match in the stream.")
parser.add_argument("where",
        action=WhereAction, nargs='+', type=str,
        help="where exclusions of the form 'where_key=where_val', no quotes.")



if __name__ == "__main__":
    args = parser.parse_args()

    json_data, _ = import_data()

    # Wasteful iteration, but less code to maintain on a tool that will likely
    # change again.
    plucked = [item for item in json_data if matches_all_wheres(item, args.where)]

    if not plucked:
        sys.exit(1)
    elif plucked and not args.all:
        print(CDDAJSONWriter(plucked[0]).dumps())
    else:
        # ugh, didn't realize JSON writer only wanted single objects when I
        # wrote it.
        # TODO: get rid of ugh
        print("[")
        for i, p in enumerate(plucked):
            eol = ",\n" if i < len(plucked)-1 else "\n"
            print(CDDAJSONWriter(p, 1).dumps(), end=eol)
        print("]")
