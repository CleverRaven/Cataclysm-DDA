#!/usr/bin/env python
"""Search for first match that matches the where key and value.

Default finds first, --all flag will return an array of all objects that match.

Output is always JSON. Keys should respect the order in which they were
found in the data.

Usage:

    python pluck.py <where-key> <where-value> [--all]
"""

from __future__ import print_function

import sys
import os
import json
from util import import_data, matches_where

if __name__ == "__main__":
    if len(sys.argv) == 3:
        # pluck one
        where_key = sys.argv[1]
        where_value = sys.argv[2]

        # TODO: Put the errors back in, someday, maybe.
        json_data, _ = import_data()

        plucked = None
        for item in json_data:
            is_match = matches_where(item, where_key, where_value)
            if is_match:
                plucked = item
                break

        if not plucked:
            sys.exit(1)
        else:
            print(json.dumps(plucked, indent=4))
    elif len(sys.argv) == 4 and sys.argv[3] == "--all":
        # pluck all
        where_key = sys.argv[1]
        where_value = sys.argv[2]

        # TODO: Put the errors back in, someday, maybe.
        json_data, _ = import_data()

        plucked = [item for item in json_data if matches_where(item, where_key, where_value)]

        if not plucked:
            sys.exit(1)
        else:
            print(json.dumps(plucked, indent=4))
    else:
        print("\n%s" % __doc__)
        sys.exit(1)
