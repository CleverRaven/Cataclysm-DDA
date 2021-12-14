from ..helper import get_singular_name
from ..write_text import write_text


def parse_vehicle_part_category(json, origin):
    name = get_singular_name(json["name"])
    write_text(json["name"], origin, comment="Name of a vehicle part category")
    write_text(json["short_name"], origin,
               comment="Short name of vehicle part category \"{}\""
               .format(name))
