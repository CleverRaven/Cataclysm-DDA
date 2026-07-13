from ..helper import get_singular_name
from ..write_text import write_text


def parse_loot_zone(json, origin):
    name = get_singular_name(json)

    write_text(json.get("name"), origin,
               comment="Name of loot zone")
    write_text(json.get("description"), origin,
               comment=f"Description of loot zone '{name}'")
