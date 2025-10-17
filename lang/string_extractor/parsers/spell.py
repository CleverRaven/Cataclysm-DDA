from ..helper import get_singular_name
from ..write_text import write_text


def parse_spell(json, origin):
    id = ""
    name = ""

    if "id" in json:
        id = json["id"]
    else:
        id = json["abstract"]

    if "name" in json:
        name = get_singular_name(json["name"])
        write_text(json["name"], origin,
                   comment="Name of spell \"{}\"".format(id))

    if not name == "":
        id = id + " ( " + name + " )"

    if "description" in json:
        write_text(json["description"], origin,
                   comment="Description of spell \"{}\"".format(id))

    if "sound_description" in json:
        write_text(json["sound_description"], origin,
                   comment="Sound description of spell \"{}\"".format(id))

    if "message" in json:
        write_text(json["message"], origin,
                   comment="Message of spell \"{}\"".format(id))
