from ..helper import get_singular_name
from ..write_text import write_text


def parse_body_part(json, origin):
    # See comments in `body_part_struct::load` of bodypart.cpp about why xxx
    # and xxx_multiple are not inside a single translation object.

    name = get_singular_name(json)

    write_text(json.get("name"), origin,
               comment="Name of body part")

    write_text(json.get("name_multiple"), origin,
               comment="Name of body part")

    write_text(json.get("accusative"), origin,
               comment="Accusative name of body part")

    write_text(json.get("accusative_multiple"), origin,
               comment="Accusative name of body part")

    write_text(json.get("encumbrance_text"), origin,
               comment=f"Encumbrance text of body part '{name}'")

    write_text(json.get("heading"), origin,
               comment=f"Heading of body part '{name}'")

    write_text(json.get("heading_multiple"), origin,
               comment=f"Heading of body part '{name}'")

    write_text(json.get("smash_message"), origin,
               comment=f"Smash message of body part '{name}'")

    write_text(json.get("hp_bar_ui_text"), origin,
               comment=f"HP bar UI text of body part '{name}'")
