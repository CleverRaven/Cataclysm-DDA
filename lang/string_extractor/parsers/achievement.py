from ..helper import get_singular_name
from ..write_text import write_text


def parse_achievement(json, origin):
    name = get_singular_name(json)

    write_text(json.get("name"), origin,
               comment="Name of achievement")
    write_text(json.get("description"), origin,
               comment=f"Description of achievement '{name}'")

    for req in json.get("requirements", []):
        write_text(req.get("description"), origin,
                   comment=f"Requirement of achievement '{name}'")
