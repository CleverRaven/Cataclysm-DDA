from ..helper import get_singular_name
from ..write_text import write_text


def parse_fault(json, origin):
    name = get_singular_name(json["name"])
    write_text(json["name"], origin, comment="Name of a fault")
    write_text(json["description"], origin,
               comment="Description of fault \"{}\"".format(name))
    for method in json["mending_methods"]:
        method_name = get_singular_name(method["name"])
        write_text(method["name"], origin,
                   comment="Name of mending method to fault \"{}\""
                           .format(name))
        if "description" in method:
            write_text(method["description"], origin,
                       comment="Description of mending method "
                               "\"{0}\" to fault \"{1}\""
                               .format(name, method_name))
        if "success_msg" in method:
            write_text(method["success_msg"], origin,
                       comment="Success message of mending method "
                               "\"{0}\" to fault \"{1}\""
                               .format(name, method_name))
