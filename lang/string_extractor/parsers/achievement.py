from ..helper import get_singular_name
from ..write_text import write_text


def parse_achievement(json, origin):
    name = ""
    if "name" in json:
        name = get_singular_name(json["name"])
        write_text(json["name"], origin, comment="Name of achievement")
    if "description" in json:
        write_text(json["description"], origin,
                   comment="Description of achievement \"{}\"".format(name))
    if "requirements" in json:
        for req in json["requirements"]:
            if "description" in req:
                comment = "Description of requirement " \
                          "of achievement \"{}\"".format(name)
                write_text(req["description"], origin, comment=comment)
