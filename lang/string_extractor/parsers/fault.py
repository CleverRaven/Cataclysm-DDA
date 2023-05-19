from ..helper import get_singular_name
from ..write_text import write_text


def parse_fault(json, origin):
    name = get_singular_name(json["name"])
    write_text(json["name"], origin, comment="Name of a fault")
    write_text(json["description"], origin,
               comment="Description of fault \"{}\"".format(name))
    if "item_prefix" in json:
        write_text(json["item_prefix"], origin,
                   comment="Prefix for item's name \"{}\"".format(name))
