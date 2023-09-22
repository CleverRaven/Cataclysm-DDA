from ..helper import get_singular_name
from ..write_text import write_text


def parse_climbing_aid(json, origin):
    name = ""
    if "name" in json:
        name = get_singular_name(json["name"])
        write_text(json["name"], origin, comment="Name of a gun", plural=True)
    elif "id" in json:
        name = json["id"]

    if "down" in json:
        down = json["down"]

        if "menu_text" in down:
            write_text(down["menu_text"], origin,
                       comment="Menu option text for climbing down with aid \"{}\"".format(name))
        if "menu_cant" in down:
            write_text(down["menu_cant"], origin,
                       comment="Menu option text when we can't climb down with climbing aid \"{}\"".format(name))
        if "confirm_text" in down:
            write_text(down["confirm_text"], origin,
                       comment="Comfirmation prompt when climbing down with aid \"{}\"".format(name))
        if "msg_before" in down:
            write_text(down["msg_before"], origin,
                       comment="Message shown before climbing down with aid \"{}\"".format(name))
        if "msg_after" in down:
            write_text(down["msg_after"], origin,
                       comment="Message shown after climbing down with aid \"{}\"".format(name))
