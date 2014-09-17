#!/usr/bin/env python
"""Usage info:

    list_keys.py -h

"""

from __future__ import print_function

import sys
import os
import json
import argparse
from util import import_data, distinct_keys, ui_values_to_columns,\
        matches_all_wheres, CDDAJSONWriter, WhereAction

parser = argparse.ArgumentParser(description="""List distinct keys on JSON objects.

Example usages:

    # List all keys, human friendly.
    %(prog)s --human

    # List keys on JSON objects of type "bionic", output in JSON.
    %(prog)s type=bionic
""", formatter_class=argparse.RawDescriptionHelpFormatter)
parser.add_argument("--human",
        action="store_true",
        help="if set, makes output human readable. default is to return output in JSON.")
parser.add_argument("where",
        action=WhereAction, nargs='*', type=str,
        help="where exclusions of the form 'where_key=where_val', no quotes.")



if __name__ == "__main__":
    args = parser.parse_args()
    json_data, _ = import_data()

    plucked = [item for item in json_data if matches_all_wheres(item, args.where)]

    all_keys = list(distinct_keys(plucked))

    if args.human:
        print("Printing distinct list of keys.")
        print("%s keys found in %s blobs out of %s blobs:\n" % (len(all_keys), len(plucked), len(json_data)))
        ui_values_to_columns(all_keys)
    else:
        if json_data:
            print(json.dumps(all_keys))
        else:
            sys.exit(1)
