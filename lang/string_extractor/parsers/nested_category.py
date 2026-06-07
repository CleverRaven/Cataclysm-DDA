from ..helper import get_singular_name
from ..write_text import write_text


def parse_nested_category(json, origin):
    name = get_singular_name(json)

    write_text(json.get("name"), origin, comment="Name of the nested group")

    write_text(json.get("description"), origin,
               comment=f"Description of the category '{name}'")
