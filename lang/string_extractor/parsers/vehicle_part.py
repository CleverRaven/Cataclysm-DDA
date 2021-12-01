from ..helper import get_singular_name
from ..write_text import write_text


def parse_vehicle_part(json, origin):
    name = ""
    if "name" in json:
        name = get_singular_name(json["name"])
        write_text(json["name"], origin, comment="Vehicle part name")
    elif "id" in json:
        name = json["id"]

    if "description" in json:
        write_text(json["description"], origin,
                   comment="Description of vehicle part \"{}\"".format(name))
