from ..helper import get_singular_name
from ..write_text import write_text


def parse_help(json, origin):
    name = ""
    if "name" in json:
        name = get_singular_name(json["name"])
        write_text(json["name"], origin, comment="Help menu")

    for msg in json.get("messages", []):
        write_text(msg, origin,
                   comment="Help message in menu \"{}\"".format(name))
