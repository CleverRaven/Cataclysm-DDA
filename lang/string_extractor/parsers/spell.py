from ..helper import get_singular_name
from ..write_text import write_text


def parse_spell(json, origin):
    name = get_singular_name(json)

    write_text(json.get("name"), origin,
               comment="Name of a spell")
    write_text(json.get("description"), origin,
               comment=f"Description of spell '{name}'")
    write_text(json.get("sound_description"), origin,
               comment=f"Sound description of spell '{name}'")
    write_text(json.get("message"), origin,
               comment=f"Message of spell '{name}'")
