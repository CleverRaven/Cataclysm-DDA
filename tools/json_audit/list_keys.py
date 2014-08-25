#!/usr/bin/env python
"""List distinct keys used on JSON objects.

Usage:

    python list_keys.py [--json]

"""

from __future__ import print_function

import sys
import os
import json
from util import import_data, ui_import_data, ui_values_to_columns


def distinct_keys(data):
    """Return a sorted-ascending list of keys scraped from the list of data
    assumed to be dictionaries.
    """
    all_keys = set()
    for d in data:
        all_keys.update(list(d.keys()))
    return sorted(all_keys)


if __name__ == "__main__":
    if len(sys.argv) > 1 and sys.argv[1] == "--json":
        # Assume raw data might be piped, no frills.
        json_data, _ = import_data()
        if json_data:
            print(json.dumps(list(distinct_keys(json_data))))
        else:
            sys.exit(1)
    else:
        # Default behavior, assuming human consumption.
        print("Printing distinct list of keys.")
        json_data = ui_import_data()
        all_keys = distinct_keys(json_data)
        print("%s keys found in all the blobs:\n" % len(all_keys))
        ui_values_to_columns(all_keys)
