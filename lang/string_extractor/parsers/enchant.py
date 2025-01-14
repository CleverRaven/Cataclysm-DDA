from ..helper import get_singular_name
from .effect import parse_effect
from ..write_text import write_text


def parse_enchant(json, origin):
    if "name" in json:
        name = get_singular_name(json["name"])
        write_text(json["name"], origin,
                   comment="Name of a enchantment")

    if "description" in json:
        write_text(json["description"], origin,
                   comment="Description of enchantment \"{}\"".format(name))

    if "hit_me_effect" in json:
        parse_effect(json["hit_me_effect"], origin)

    if "special_vision" in json:
        for vision in json["special_vision"]:
            if "descriptions" in vision:
                for description in vision["descriptions"]:
                    parse_description(description, origin)


def parse_description(description, origin):
    if "text" in description:
        special_vision_id = description["id"]
        write_text(description["text"], origin,
                   comment="Description of creature revealed by special "
                   "vision \"{}\"".format(special_vision_id))
