#!/usr/bin/env python3
"""Make a distinct list of keys from JSON input.

Main purpose to take output from other scripts, which will be JSON
dictionaries or lists of JSON dictionaries and get a distinct set of
keys from all the items.

Can be used to answer questions like, "What types of material are on
the items?" when all you want to know is types, not counts.

Example usages:

    values.py -k material | %(prog)s

"""

import argparse
import os
import stat
import json
import sys
from util import ui_values_to_columns

parser = argparse.ArgumentParser(
    description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)

parser.add_argument(
    "--human",
    action="store_true",
    help="if set, makes output human readable. default is JSON output.")


if __name__ == "__main__":
    args = parser.parse_args()

    # Saw this trick here:
    # http://stackoverflow.com/questions/13442574/how-do-i-determine-if-
    #     sys-stdin-is-redirected-from-a-file-vs-piped-from-another
    mode = os.fstat(0).st_mode
    if not stat.S_ISFIFO(mode) and not stat.S_ISREG(mode):
        # input is terminal, not what we want
        print("Redirect data to stdin, please (e.g. with a pipe)."
              " Run with -h for usage.")
        sys.exit(1)

    try:
        json_data = json.loads(sys.stdin.read())
    except Exception:
        print("Could not read json data from stdin")
        sys.exit(1)

    # We'll either get a dict or a list...
    if type(json_data) == dict:
        # ... and just make it a list.
        json_data = [json_data]

    # All keys on JSON data should be strings.
    keys = set()
    for item in json_data:
        # We assume only dictionaries here
        keys.update(list(item.keys()))

    keys = sorted(list(keys))

    if args.human:
        ui_values_to_columns(keys)
    else:
        print(json.dumps(keys))
