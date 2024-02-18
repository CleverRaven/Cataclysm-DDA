from ..helper import get_singular_name
from ..write_text import write_text


def parse_mapgen(json, origin):
    if "object" not in json:
        return

    om = ""
    if "om_terrain" in json:
        if type(json["om_terrain"]) is str:
            om = json["om_terrain"]
        elif type(json["om_terrain"]) is list:
            if len(json["om_terrain"]) == 0:
                om = ""
            elif type(json["om_terrain"][0]) is str:
                om = ", ".join(json["om_terrain"])
            elif type(json["om_terrain"][0]) is list:
                om = ", ".join(", ".join(i) for i in json["om_terrain"])

    for key in ["place_specials", "place_signs"]:
        if key in json["object"]:
            for sign in json["object"][key]:
                if "signage" in sign:
                    write_text(sign["signage"], origin,
                               comment="Signage placed on map {}".format(om))

    if "signs" in json["object"]:
        for sign in json["object"]["signs"]:
            if "signage" in json["object"]["signs"][sign]:
                write_text(json["object"]["signs"][sign]["signage"], origin,
                           comment="Signage placed on map {}".format(om))

    if "computers" in json["object"]:
        for key in json["object"]["computers"]:
            com = json["object"]["computers"][key]
            com_name = ""
            if "name" in com:
                com_name = get_singular_name(com["name"])
                write_text(
                    com["name"], origin,
                    comment="Computer name placed on map {}".format(om))
            for opt in com.get("options", []):
                if "name" in opt:
                    write_text(opt["name"], origin,
                               comment="Interactive menu name in computer "
                               "\"{}\" placed on map {}".format(com_name, om))
            if "access_denied" in com:
                write_text(com["access_denied"], origin,
                           comment="Access denied message on computer \"{}\""
                           " placed on map {}".format(com_name, om))

    if "place_computers" in json["object"]:
        for computer in json["object"]["place_computers"]:
            if "name" in computer:
                write_text(computer["name"], origin,
                           comment="Computer name placed on map {}".format(om))
