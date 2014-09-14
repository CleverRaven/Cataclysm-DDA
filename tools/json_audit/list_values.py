#!/usr/bin/env python
"""Grep data from cataclysm data and grab count of number of occurences of
values within a field. Friendly to some different fields types (numbers, strings,
arrays/lists).

Usage:

    python list_values.py <key> [where-key] [where-value] [--json]
"""

from __future__ import print_function

import sys
import os
import json
from util import import_data, ui_import_data, value_counter, ui_values_to_columns

if __name__ == "__main__":
    if len(sys.argv) == 2:
        # Count values associated with key, human friendly output.
        search_key = sys.argv[1]
        where_key = None
        where_value = None

        json_data = ui_import_data()

        stats, num_matches = value_counter(json_data, search_key, where_key, where_value)
        if not stats:
            print("Sorry, didn't find any stats for '%s' in the JSON." % search_key)
            sys.exit(1)

        title = "List of values from field '%s'" % search_key
        print("\n\n%s" % title)
        print("(Data from %s out of %s blobs)" % (num_matches, len(json_data)))
        print("-" * len(title))
        ui_values_to_columns(sorted(stats.keys()))
    elif len(sys.argv) == 3 and sys.argv[2] == "--json":
        # Count values associated with key, machine output.
        search_key = sys.argv[1]
        where_key = None
        where_value = None

        json_data = import_data()[0]
        stats, num_matches = value_counter(json_data, search_key, where_key, where_value)
        if not stats:
            # Still JSON parser friendly, indicator of fail with emptiness.
            print(json.dumps([]))
            sys.exit(1)
        else:
            print(json.dumps(sorted(stats.keys())))
    elif len(sys.argv) == 4:
        # Count values associated with key, filter, human friendly output.
        search_key = sys.argv[1]
        where_key = sys.argv[2]
        where_value = sys.argv[3]

        json_data = import_data()[0]
        stats, num_matches = value_counter(json_data, search_key, where_key, where_value)
        if not stats:
            print("Sorry, didn't find any stats for '%s' in the JSON." % search_key)
            sys.exit(1)

        title = "List of values from field '%s' filtered by [%s=%s]" % (search_key, where_key, where_value)
        print("\n\n%s" % title)
        print("(Data from %s out of %s blobs)" % (num_matches, len(json_data)))
        print("-" * len(title))
        ui_values_to_columns(sorted(stats.keys()))
    elif len(sys.argv) == 5 and sys.argv[4] == "--json":
        search_key = sys.argv[1]
        where_key = sys.argv[2]
        where_value = sys.argv[3]

        json_data = import_data()[0]
        stats, num_matches = value_counter(json_data, search_key, where_key, where_value)
        if not stats:
            # Still JSON parser friendly, indicator of fail with emptiness.
            print(json.dumps([]))
            sys.exit(1)
        else:
            print(json.dumps(sorted(stats.keys())))
    else:
        print("\n%s" % __doc__)
        sys.exit(1)
