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

    parse_mapgen_object(json["object"], origin, om)


def parse_mapgen_object(json, origin, om):
    if type(json) is list:
        for j in json:
            parse_mapgen_object(j, origin, om)

    for key in ["place_specials", "place_signs"]:
        if key in json:
            for sign in json[key]:
                if "signage" in sign:
                    write_text(sign["signage"], origin,
                               comment="Signage placed on map {}".format(om))

    if "signs" in json:
        for sign in json["signs"]:
            if "signage" in json["signs"][sign]:
                write_text(json["signs"][sign]["signage"], origin,
                           comment="Signage placed on map {}".format(om))

    if "computers" in json:
        for key in json["computers"]:
            com = json["computers"][key]
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

    if "place_computers" in json:
        for computer in json["place_computers"]:
            if "name" in computer:
                write_text(computer["name"], origin,
                           comment="Computer name placed on map {}".format(om))

    monsters = []
    if "place_monster" in json:
        monsters += json["place_monster"]
    if "monster" in json:
        monsters += [*json["monster"].values()]

    for m in monsters:
        if "name" in m:
            desc = ""
            if "monster" in m:
                desc = "\"{}\"".format(m["monster"])
            elif "group" in m:
                desc = "group \"{}\"".format(m["group"])
            write_text(m["name"], origin,
                       comment="Name of the monster {} placed on map {}"
                       .format(desc, om))

    if "place_graffiti" in json:
        for graffiti in json["place_graffiti"]:
            if "text" in graffiti:
                write_text(graffiti["text"], origin,
                           comment="Graffiti placed on map {}".format(om))
