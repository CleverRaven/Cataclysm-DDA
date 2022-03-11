#!/usr/bin/env python3

"""List items potentally needing a `price_postapoc` key

Script prints `id` and `name` of each item in following lines separating
them with an empty line.

It ignores `id`s found in blacklist.json.
"""

import argparse
import json
import sys
from pathlib import Path

from util import import_data

basepath = Path(__file__).parent
relpath = (basepath if not basepath.is_relative_to(Path().resolve()) else
           basepath.relative_to(Path().resolve()))

default_blacklist = relpath.joinpath('reprice/blacklist.json')
parser = argparse.ArgumentParser(description=__doc__)

# TODO: add option to json output
parser.add_argument(
    '--json-dir', type=Path,
    help='directory where JSON files are',
    metavar='<dir>',
    dest='json_dir'
)
parser.add_argument(
    '--files-pattern', type=str,
    help="filename glob pattern for selecting a subset of JSON files",
    metavar='<pattern>',
    dest='fn_pattern'
)
parser.add_argument(
    '--noblacklist', action='store_false',
    help='ignores blacklist',
    dest='use_blacklist'
)
parser.add_argument(
    '--blacklist', default=default_blacklist, type=Path,
    help="loads a different blacklist (default is {blacklist})".format(
        blacklist=default_blacklist),
    metavar='<filename>',
    dest='blacklist_file'
)


def main(json_dir, fn_pattern, blacklist_file, use_blacklist):
    if use_blacklist:
        with open(blacklist_file, 'r') as fp_bl:
            blacklist = json.load(fp_bl)
    else:
        blacklist = []

    data, errors = import_data(
        **{key: value for key, value in zip(('json_dir, json_fmatch'),
                                            (json_dir, fn_pattern))
            if value is not None})
    reprice = [
        item for item in data
        if 'price' in item and 'price_postapoc' not in item and
        'id' in item and item['id'] not in blacklist
    ]

    def get_item_name(item):
        if 'name' not in item:
            return ""
        if not isinstance(item['name'], dict):
            return item['name']
        return (item['name'].get('str') or item['name'].get('str_sp') or
                item['name'].get('str_pl'))

    print('\n\n'.join("\"id\": \"{id:s}\"\n\"name\": \"{name:s}\"".format(
        id=item['id'], name=get_item_name(r)) for item in reprice)
    )

    if errors:
        print("ERROR: following occured loading JSON data:", file=sys.stderr)
        print(errors, file=sys.stderr)


if __name__ == '__main__':
    args = parser.parse_args()

    main(**vars(args))
