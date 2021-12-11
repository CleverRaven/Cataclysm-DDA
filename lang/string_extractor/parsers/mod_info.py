from ..helper import get_singular_name
from ..write_text import write_text


def parse_mod_info(json, origin):
    name = get_singular_name(json["name"])
    write_text(json["name"], origin, comment="MOD name", c_format=False)
    write_text(json["description"], origin, c_format=False,
               comment="Description of MOD \"{}\"".format(name))
