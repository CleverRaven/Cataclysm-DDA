from ..helper import get_singular_name
from ..write_text import write_text


def parse_body_part(json, origin):
    # See comments in `body_part_struct::load` of bodypart.cpp about why xxx
    # and xxx_multiple are not inside a single translation object.

    name = ""
    if "name" in json:
        name = get_singular_name(json["name"])
        write_text(json["name"], origin, comment="Name of body part")
    elif "id" in json:
        name = json["id"]

    if "name_multiple" in json:
        write_text(json["name_multiple"], origin, comment="Name of body part")

    if "accusative" in json:
        write_text(json["accusative"], origin,
                   comment="Accusative name of body part")

    if "accusative_multiple" in json:
        write_text(json["accusative_multiple"], origin,
                   comment="Accusative name of body part")

    if "encumbrance_text" in json:
        write_text(json["encumbrance_text"], origin,
                   comment="Encumbrance text of body part \"{}\"".format(name))

    if "heading" in json:
        write_text(json["heading"], origin,
                   comment="Heading of body part \"{}\"".format(name))

    if "heading_multiple" in json:
        write_text(json["heading_multiple"], origin,
                   comment="Heading of body part \"{}\"".format(name))

    if "smash_message" in json:
        write_text(json["smash_message"], origin,
                   comment="Smash message of body part \"{}\"".format(name))

    if "hp_bar_ui_text" in json:
        write_text(json["hp_bar_ui_text"], origin,
                   comment="HP bar UI text of body part \"{}\"".format(name))
