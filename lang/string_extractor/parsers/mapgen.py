from ..helper import get_singular_name
from ..write_text import write_text


def parse_mapgen(json, origin):
    if "object" not in json:
        return

    om = json.get("nested_mapgen_id", "???")

    if "om_terrain" in json:
        if type(json["om_terrain"]) is str:
            om = json["om_terrain"]
        elif type(json["om_terrain"]) is list and json["om_terrain"]:
            if type(json["om_terrain"][0]) is str:
                om = ", ".join(json["om_terrain"])
            elif type(json["om_terrain"][0]) is list:
                om = ", ".join(", ".join(i) for i in json["om_terrain"])

    parse_mapgen_object(json["object"], origin, om)


def parse_mapgen_object(json, origin, om):
    if type(json) is list:
        for j in json:
            parse_mapgen_object(j, origin, om)
        return

    for key in ["place_specials", "place_signs"]:
        for sign in json.get(key, []):
            write_text(sign.get("signage"), origin,
                       comment=f"Signage placed on map '{om}'")

    for sign in json.get("signs", []):
        if "signage" in json["signs"][sign]:
            write_text(json["signs"][sign]["signage"], origin,
                       comment=f"Signage placed on map '{om}'")

    for computer in json.get("computers", {}).values():
        com_name = get_singular_name(computer)

        write_text(com_name, origin,
                   comment=f"Computer name placed on map '{om}'")

        write_text(computer.get("access_denied"), origin,
                   comment="Access denied message on computer"
                   f" '{com_name}'")

        for option in computer.get("options", []):
            write_text(option.get("name"), origin,
                       comment="Interactive menu name in computer"
                       f" '{com_name}'")

    for computer in json.get("place_computers", []):
        write_text(computer.get("name"), origin,
                   comment=f"Computer name placed on map '{om}'")

    monsters = json.get("place_monster", [])
    if "monster" in json:
        monsters += [*json["monster"].values()]

    for m in monsters:
        if "name" in m:
            desc = m.get("monster") or "group " + m.get("group")
            write_text(m["name"], origin,
                       comment="Name of the monster"
                       f" '{desc}' placed on map {om}")

    for graffiti in json.get("place_graffiti", []):
        write_text(graffiti.get("text"), origin,
                   comment=f"Graffiti placed on map '{om}'")
