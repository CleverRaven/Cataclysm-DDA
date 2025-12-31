from ..helper import get_singular_name
from ..write_text import write_text


def parse_proficiency(json, origin):
    name = get_singular_name(json)

    write_text(json.get("name"), origin,
               comment="Proficiency name")
    write_text(json.get("description"), origin,
               comment=f"Description of proficiency '{name}'")
