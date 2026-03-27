from ..helper import get_singular_name
from ..write_text import write_text


def parse_map_extra(json, origin):
    name = get_singular_name(json)

    write_text(json.get("name"), origin,
               comment="Name of map extra")
    write_text(json.get("description"), origin,
               comment=f"Description of map extra '{name}'")
