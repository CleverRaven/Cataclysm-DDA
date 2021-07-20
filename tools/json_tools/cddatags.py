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


def cdda_style_json(v):
    if type(v) == str:
        return json.dumps(v)
    elif type(v) == list:
        return f'[ {", ".join(cdda_style_json(e) for e in v)} ]'
    else:
        raise RuntimeError('Unexpected type')


def make_tags_line(id_key, id, full_id, filename):
    length_limit = 120
    pattern = f'/"{id_key}": {cdda_style_json(full_id)}/'
    if len(pattern) > length_limit - 5:
        pattern = f'/"{id}"/'
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
        'nested_mapgen_id', 'om_terrain', 'update_mapgen_id', 'result')

    def add_definition(id_key, id, full_id, relative_path):
        if not id:
            return
        if type(id) == str:
            definitions.append((id_key, id, full_id, relative_path))
        elif type(id) == list:
            for i in id:
                add_definition(id_key, i, full_id, relative_path)

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
                                add_definition(id_key, id, id, relative_path)

    json_tags_lines = [make_tags_line(*d) for d in definitions]
    existing_tags_lines = []
    try:
        with open(TAGS_FILE, 'rb') as tags_file:
            existing_tags_lines = tags_file.readlines()
    except FileNotFoundError:
        pass

    existing_tags_lines = [
        l.rstrip(b'\n') for l in existing_tags_lines if
        not is_json_tag_line(l)]

    all_tags_lines = sorted(json_tags_lines + existing_tags_lines)

    with open(TAGS_FILE, 'wb') as tags_file:
        tags_file.write(b'\n'.join(all_tags_lines))


if __name__ == '__main__':
    main(sys.argv[1:])
