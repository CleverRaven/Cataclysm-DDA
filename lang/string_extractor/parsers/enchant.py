from ..helper import get_singular_name
from .effect import parse_effect
from ..write_text import write_text


def parse_enchant(json, origin):
    if not (json and type(json) is dict):
        return

    name = get_singular_name(json)

    write_text(json.get("name"), origin,
               comment="Name of a enchantment")
    write_text(json.get("description"), origin,
               comment=f"Description of enchantment '{name}'")

    if "hit_me_effect" in json:
        parse_effect(json["hit_me_effect"], origin)

    for vision in json.get("special_vision", []):
        for description in vision.get("descriptions", []):
            parse_description(description, origin)


def parse_description(description, origin):
    if "text" in description:
        special_vision_id = description["id"]
        write_text(description["text"], origin,
                   comment="Description of creature revealed by special "
                   f"vision '{special_vision_id}'")
