from ..helper import get_singular_name
from ..write_text import write_text


def parse_skill(json, origin):
    name = ""
    if "name" in json:
        name = get_singular_name(json["name"])
        write_text(json["name"], origin, comment="Skill name")

    if "description" in json:
        write_text(json["description"], origin,
                   comment="Description of skill \"{}\"".format(name))

    for level in json.get("level_descriptions_theory", []):
        write_text(level["description"], origin,
                   comment="Level description of skill \"{}\"".format(name))

    for level in json.get("level_descriptions_practice", []):
        write_text(level["description"], origin,
                   comment="Level description of skill \"{}\"".format(name))
