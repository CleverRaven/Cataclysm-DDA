from ..helper import get_singular_name
from ..write_text import write_text


def parse_vehicle_part_location(json, origin):
    name = ""
    if "name" in json:
        name = get_singular_name(json["name"])
        write_text(json["name"], origin,
                   comment="Name of vehicle part location")
    if "desc" in json:
        write_text(json["desc"], origin,
                   comment="Description of vehicle part location"
                   " \"{}\"".format(name))
