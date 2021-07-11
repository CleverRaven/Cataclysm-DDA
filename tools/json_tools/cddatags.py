#!/usr/bin/env python3
"""Update a tags file with locations of the definitions of CDDA json entities.

If you already have a tags file with some data in, this will only
replace tags in json files, not e.g. cpp files, so it should be safe
to use after running e.g. ctags.
"""

import argparse
import json
import os
import sys

SCRIPT_DIR = os.path.abspath(os.path.dirname(__file__))
TOP_DIR = os.path.normpath(os.path.join(SCRIPT_DIR, "..", ".."))
JSON_DIR = os.path.join(TOP_DIR, "data")
TAGS_FILE = os.path.join(TOP_DIR, "tags")


def make_tags_line(id_key, id, filename):
    pattern = '/"{id_key}": "{id}"/'.format(id_key=id_key, id=id)
    return '\t'.join((id, filename, pattern)).encode('utf-8')


def is_json_tag_line(line):
    return b'.json\t' in line


def main(args):
    parser = argparse.ArgumentParser(description=__doc__)

    parser.parse_args(args)

    definitions = []

    # JSON keys to generate tags for
    id_keys = (
        'id', 'abstract', 'ident',
        'nested_mapgen_id', 'update_mapgen_id', 'result')

    for dirpath, dirnames, filenames in os.walk(JSON_DIR):
        for filename in filenames:
            if filename.endswith('.json'):
                full_path = os.path.join(dirpath, filename)
                assert full_path.startswith(TOP_DIR)
                relative_path = full_path[len(TOP_DIR):].lstrip(os.path.sep)
                with open(full_path, encoding='utf-8') as file:
                    try:
                        json_data = json.load(file)
                    except Exception as err:
                        sys.stderr.write(
                            "Problem reading file %s, reason: %s" %
                            (filename, err))
                        continue
                    if type(json_data) == dict:
                        json_data = [json_data]
                    elif type(json_data) != list:
                        sys.stderr.write(
                            "Problem parsing data from file %s, reason: "
                            "expected a list." % filename)
                        continue

                    # Check each object in json_data for taggable keys
                    for obj in json_data:
                        for id_key in id_keys:
                            if id_key in obj:
                                id = obj[id_key]
                                if type(id) == str and id:
                                    definitions.append(
                                        (id_key, id, relative_path))

    json_tags_lines = [make_tags_line(*d) for d in definitions]
    existing_tags_lines = []
    try:
        with open(TAGS_FILE, 'rb', encoding="utf-8") as tags_file:
            existing_tags_lines = tags_file.readlines()
    except FileNotFoundError:
        pass

    existing_tags_lines = [
        l.rstrip(b'\n') for l in existing_tags_lines if
        not is_json_tag_line(l)]

    all_tags_lines = sorted(json_tags_lines + existing_tags_lines)

    with open(TAGS_FILE, 'wb', encoding="utf-8") as tags_file:
        tags_file.write(b'\n'.join(all_tags_lines))


if __name__ == '__main__':
    main(sys.argv[1:])
