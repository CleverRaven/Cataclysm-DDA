from ..helper import get_singular_name
from ..write_text import write_text
from .examine_action import parse_examine_action


def parse_terrain(json, origin):
    name = ""
    if "name" in json:
        name = get_singular_name(json["name"])
        write_text(json["name"], origin, comment="Terrain name")
    if "description" in json:
        write_text(json["description"], origin,
                   comment="Description of terrain \"{}\"".format(name))
    if "bash" in json:
        if "sound" in json["bash"]:
            write_text(json["bash"]["sound"], origin,
                       comment="Bashing sound")
        if "sound_fail" in json["bash"]:
            write_text(json["bash"]["sound_fail"], origin,
                       comment="Bashing failed sound")
    if "boltcut" in json:
        if "message" in json["boltcut"]:
            write_text(json["boltcut"]["message"], origin,
                       comment="Boltcut message of terrain \"{}\"".
                       format(name))
        if "sound" in json["boltcut"]:
            write_text(json["boltcut"]["sound"], origin,
                       comment="Boltcut sound of terrain \"{}\"".format(name))
    if "hacksaw" in json:
        if "message" in json["hacksaw"]:
            write_text(json["hacksaw"]["message"], origin,
                       comment="Hacksaw message of terrain \"{}\""
                       .format(name))
        if "sound" in json["hacksaw"]:
            write_text(json["hacksaw"]["sound"], origin,
                       comment="Hacksaw sound of terrain \"{}\"".format(name))
    if "prying" in json:
        if "message" in json["prying"]:
            write_text(json["prying"]["message"], origin,
                       comment="Prying message of terrain \"{}\"".format(name))
        if "prying_data" in json["prying"]:
            if "failure" in json["prying"]["prying_data"]:
                write_text(json["prying"]["prying_data"]["failure"], origin,
                           comment="Prying failure of terrain \"{}\""
                           .format(name))
    if "lockpick_message" in json:
        write_text(json["lockpick_message"], origin,
                   comment="Lockpick message of terrain \"{}\"".format(name))
    if "examine_action" in json:
        parse_examine_action(json["examine_action"], origin,
                             "terrain \"{}\"".format(name))
