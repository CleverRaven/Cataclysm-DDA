import json

from .parser import parsers


def parse_json_object(json, origin):
    """
    Extract strings from the JSON object.
    Silently ignores JSON objects without "type" key.
    Raises exception if the JSON object contains unrecognized "type".
    """
    if "type" in json and type(json["type"]) is str:
        json_type = json["type"].lower()
        if json_type in parsers:
            try:
                parsers[json_type](json, origin)
            except Exception as E:
                print("Exception when parsing JSON data type \"{}\""
                      .format(json_type))
                raise E
        else:
            raise Exception("Unrecognized JSON data type \"{}\""
                            .format(json_type))


def parse_json_file(file_path):
    """Extract strings from the specified JSON file."""
    with open(file_path, encoding="utf-8") as fp:
        json_data = json.load(fp)

    try:
        json_objects = json_data if type(json_data) is list else [json_data]
        for json_object in json_objects:
            parse_json_object(json_object, file_path)
    except Exception:
        print("Error in JSON object\n'{0}'\nfrom file: '{1}'".format(
            json.dumps(json_object, indent=2), file_path))
        raise
