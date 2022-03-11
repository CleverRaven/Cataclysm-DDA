#!/usr/bin/env python3

"""List items potentally needing a `price_postapoc` key

Script prints `id` and `name` of each item in following lines separating
them with an empty line.
"""
# TODO: add example


import sys
from util import import_data


def parse_args():
    # TODO: add option to load a blacklist
    # TODO: add option to load different json dir
    # TODO: add option to match json files
    pass


if __name__ == '__main__':
    parse_args()

    (data, errors) = import_data()
    reprice = [item for item in data
               if 'price' in item and 'price_postapoc' not in item and
               'id' in item]
    print('\n\n'.join("\"id\": \"{id:s}\"\n\"name\": \"{name:s}\"".format(
        id=r['id'],
        name=r['name'].get('str') or r['name'].get('str_sp') or
        r['name'].get('str_pl') if 'name' in r else "") for r in reprice)
    )

    if errors:
        print("ERROR: following occured loading JSON data:", file=sys.stderr)
        print(errors, file=sys.stderr)
