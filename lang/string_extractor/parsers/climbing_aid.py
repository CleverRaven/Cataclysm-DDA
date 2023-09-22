from ..helper import get_singular_name
from ..write_text import write_text


def parse_climbing_aid(json, origin):
    name = ""
    if "name" in json:
        name = get_singular_name(json["name"])
        write_text(json["name"], origin, comment="Name of a gun", plural=True)
    elif "id" in json:
        name = json["id"]

    down = None
    if "down" in json:
        down = json["down"]

    if down and "menu_text" in down:
        write_text(down["menu_text"], origin,
                   comment="Menu text for descent with \"{}\"".format(name))
    if down and "menu_cant" in down:
        write_text(down["menu_cant"], origin,
                   comment="Menu disables descent with \"{}\"".format(name))
    if down and "confirm_text" in down:
        write_text(down["confirm_text"], origin,
                   comment="Query: really descend with \"{}\"?".format(name))
    if down and "msg_before" in down:
        write_text(down["msg_before"], origin,
                   comment="Message before descent with \"{}\"".format(name))
    if down and "msg_after" in down:
        write_text(down["msg_after"], origin,
                   comment="Message after descent with \"{}\"".format(name))
