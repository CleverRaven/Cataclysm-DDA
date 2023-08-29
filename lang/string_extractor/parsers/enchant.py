from ..helper import get_singular_name
from ..write_text import write_text


def parse_enchant(json, origin):
    if "name" in json:
        name = get_singular_name(json["name"])
        write_text(json["name"], origin,
                   comment="Name of a enchantment", plural=True)

    if "description" in json:
        write_text(json["description"], origin,
                   comment="Description of enchantment \"{}\"".format(name))
