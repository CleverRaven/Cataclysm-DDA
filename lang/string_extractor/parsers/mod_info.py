from ..helper import get_singular_name
from ..write_text import write_text


def parse_mod_info(json, origin):
    name = get_singular_name(json["name"])
    write_text(json["name"], origin, comment="MOD name")
    write_text(json["description"], origin,
               comment="Description of MOD \"{}\"".format(name))
