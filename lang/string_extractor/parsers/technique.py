from ..helper import get_singular_name
from ..write_text import write_text


def parse_technique(json, origin):
    name = ""
    if "name" in json:
        name = get_singular_name(json["name"])
        write_text(json["name"], origin, comment="Martial technique name")

    if "description" in json:
        write_text(json["description"], origin, c_format=False,
                   comment="Description of martial technique \"{}\""
                   .format(name))

    for msg in json.get("messages", []):
        write_text(msg, origin,
                   comment="Message of martial technique \"{}\"".format(name))

    if "condition_desc" in json:
        write_text(json["condition_desc"], origin, c_format=False,
                   comment="Condition description of martial technique \"{}\""
                   .format(name))
