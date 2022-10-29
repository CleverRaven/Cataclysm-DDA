from ..helper import get_singular_name
from ..write_text import write_text


def parse_bionic(json, origin):
    name = ""
    if "name" in json:
        name = get_singular_name(json["name"])
        write_text(json["name"], origin, comment="Name of a bionic")

    if "description" in json:
        write_text(json["description"], origin, c_format=False,
                   comment="Description of bionic \"{}\"".format(name))
