from ..helper import get_singular_name
from ..write_text import write_text


def parse_addiction_type(json, origin):
    name = get_singular_name(json)

    write_text(json.get("name"), origin,
               comment="Name of addiction effect as"
               " it appears in the player's status")

    write_text(json.get("type_name"), origin,
               comment=f"Name of '{name}' addiction's source")

    write_text(json.get("description"), origin,
               comment=f"Description of addiction effect '{name}'"
               " as it appears in the player's status")
