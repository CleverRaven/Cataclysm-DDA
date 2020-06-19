#!/usr/bin/env python2
"""Detect and print orphaned or unused JSON data.
"""
# For now just prints "nested_mapgen_id" values unused by any mapgen "chunks"

import util

# TODO:
# - Add command-line options to customiz output and do filtering
# - Be able to detect other kinds of unused or dead JSON data

def get_nested_object_chunks(json_item):
    """Given a JSON item like { "object": { "place_nested": [ ... ] } },
    pick out all the nested mapgen IDs for objects in the list with "chunks",
    and return them in a list.
    """

    # Get all "nested" and "place_nested" objects
    jo = json_item["object"]
    nested_objects = []
    if "nested" in jo:
        nested_objects.extend(jo["nested"].values())
    if "place_nested" in jo and type(jo["place_nested"]) == list:
        nested_objects.extend(jo["place_nested"])

    # Keep the "chunks" list from any objects that have it
    nested_chunk_lists = []
    for nest in nested_objects:
        if "chunks" in nest and type(nest["chunks"]) == list:
            nested_chunk_lists.append(nest["chunks"])
        if "else_chunks" in nest and type(nest["else_chunks"]) == list:
            nested_chunk_lists.append(nest["else_chunks"])

    # Parse the "chunks" fields to get mapgen IDs within
    all_chunks = []
    for chunks in nested_chunk_lists:
        # Simple list of strings
        if all(isinstance(c, basestring) for c in chunks):
            all_chunks.extend(chunks)

        # List of [ "nest_id", num ]
        elif all(isinstance(c, list) for c in chunks):
            # Get the first value from each sub-list, ensuring string
            ids = [c[0] for c in chunks if isinstance(c[0], basestring)]
            all_chunks.extend(ids)

        else:
            raise ValueError("Unexpected chunks: %s" % chunks)

    return all_chunks


def mapgen_ids_with_no_chunks(json_data):
    """Return a list of "nested_mapgen_id" values not used by any mapgen "chunks".
    """
    # Keep track of IDs and references to them
    nested_ids = []
    chunk_refs = []

    # Read "nested_mapgen_id" and "place_nested" chunks from "mapgen" types
    for item in json_data:
        if item.get("type") != "mapgen":
            continue

        if "nested_mapgen_id" in item:
            nested_ids.append(item["nested_mapgen_id"])

        if "object" in item and "nested" in item["object"]:
            chunk_refs.extend(get_nested_object_chunks(item))

        if "object" in item and "place_nested" in item["object"]:
            chunk_refs.extend(get_nested_object_chunks(item))

    # Remove duplicates
    nested_ids = list(set(nested_ids))
    chunk_refs = list(set(chunk_refs))

    # Get a sorted list of IDs that aren't in any chunks
    unused_ids = sorted(list(set(nested_ids) - set(chunk_refs)))
    return unused_ids


if __name__ == "__main__":
    json_data, load_errors = util.import_data()

    print("Nested mapgen IDs (nested_mapgen_id) not appearing in any chunks:")
    ids = mapgen_ids_with_no_chunks(json_data)
    print("\n".join(ids))

