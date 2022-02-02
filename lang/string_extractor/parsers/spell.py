from ..helper import get_singular_name
from ..write_text import write_text


def parse_spell(json, origin):
    name = get_singular_name(json["name"])
    write_text(json["name"], origin, comment="Spell name")

    if "description" in json:
        write_text(json["description"], origin,
                   comment="Description of spell \"{}\"".format(name))

    if "sound_description" in json:
        write_text(json["sound_description"], origin,
                   comment="Sound description of spell \"{}\"".format(name))

    if "message" in json:
        write_text(json["message"], origin,
                   comment="Message of spell \"{}\"".format(name))
