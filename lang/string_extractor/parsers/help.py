from ..helper import get_singular_name
from ..write_text import write_text


def parse_help(json, origin):
    name = get_singular_name(json)

    write_text(json.get("name"), origin, comment="Help menu")

    for msg in json.get("messages", []):
        write_text(msg, origin,
                   comment=f"Help message in menu '{name}'")
