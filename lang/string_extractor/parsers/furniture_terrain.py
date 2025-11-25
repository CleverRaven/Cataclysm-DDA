from ..helper import get_singular_name
from ..write_text import write_text
from .examine_action import parse_examine_action


def parse_furniture_terrain(json, origin):
    name = get_singular_name(json)
    jtype = json["type"].lower()

    write_text(json.get("name"), origin,
               comment=f"{jtype.capitalize()} name")
    write_text(json.get("description"), origin,
               comment=f"Description of {jtype} '{name}'")

    parse_examine_action(json.get("examine_action"), origin,
                         f"{jtype} '{name}'")

    write_text(json.get("lockpick_message"), origin,
               comment=f"Lockpick message of {jtype} '{name}'")

    prying = {}
    if prying := json.get("prying"):
        write_text(prying.get("message"), origin,
                   comment=f"Prying action message of {jtype} '{name}'")
        if "prying_data" in prying:
            write_text(prying["prying_data"].get("failure"), origin,
                       comment=f"Prying failure message of {jtype} '{name}'")

    if "bash" in json:
        write_text(json["bash"].get("sound"), origin,
                   comment="Bashing sound")
        write_text(json["bash"].get("sound_fail"), origin,
                   comment="Bashing failed sound")

    if jtype == "terrain":
        parse_terrain(json, origin, name)


def parse_terrain(json, origin, name):
    if "boltcut" in json:
        write_text(json["boltcut"].get("message"), origin,
                   comment=f"Boltcut message of terrain '{name}'")
        write_text(json["boltcut"].get("sound"), origin,
                   comment=f"Boltcut sound of terrain '{name}'")

    if "hacksaw" in json:
        write_text(json["hacksaw"].get("message"), origin,
                   comment=f"Hacksaw message of terrain '{name}'")
        write_text(json["hacksaw"].get("sound"), origin,
                   comment=f"Hacksaw sound of terrain '{name}'")
