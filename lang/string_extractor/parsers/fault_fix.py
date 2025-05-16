from ..helper import get_singular_name
from ..write_text import write_text


def parse_fault_fix(json, origin):
    name = get_singular_name(json["name"])
    write_text(json["name"], origin,
               comment="Name of fault_fix \"{}\"".format(name))
    if "success_msg" not in json:
        return
    write_text(json["success_msg"], origin,
               comment="Success message of fault_fix \"{}\"".format(name))
