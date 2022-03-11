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

    parse_errors = []
    data, json_errors = import_data(
        **{key: value for key, value in zip(('json_dir, json_fmatch'),
                                            (json_dir, fn_pattern))
            if value is not None})
    reprice = [
        item for item in data
        if 'price' in item and 'price_postapoc' not in item and
        'id' in item and item['id'] not in blacklist
    ]

    def parse_item(item):
        if 'name' not in item:
            name = ""
        elif not isinstance(item['name'], dict):
            name = item['name']
        else:
            name = (item['name'].get('str') or item['name'].get('str_sp') or
                    item['name'].get('str_pl'))

        first_line = "\"id\": \"{:s}\"".format(item['id'])
        if name is None:
            parse_error.append(
                "conldn't parse item name with '{:s} id".format(item['id']))
            return first_line
        return '\n'.join([first_line, "\"name\": \"{:s}\"".format(name)])

    for item in reprice[:-1]:
        print(parse_item(item), '\n')
    if reprice:
        print(parse_item(reprice[-1]))

    if parse_errors:
        print("ERROR: following occured parsing JSON data:", file=sys.stderr)
        print('\n'.join(parse_errors, file=sys.stderr))
    if json_errors:
        print("ERROR: following occured loading JSON files:", file=sys.stderr)
        print('\n'.jon(json_errors), file=sys.stderr)


if __name__ == '__main__':
    args = parser.parse_args()

    main(**vars(args))
