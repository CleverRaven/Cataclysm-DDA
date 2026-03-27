from ..helper import get_singular_name
from ..write_text import write_text


def parse_technique(json, origin):
    name = get_singular_name(json)

    write_text(json.get("name"), origin,
               comment="Martial technique name")
    write_text(json.get("description"), origin,
               comment=f"Description of martial technique '{name}'")

    for msg in json.get("messages", []):
        write_text(msg, origin,
                   comment=f"Message of martial technique '{name}'")

    write_text(json.get("condition_desc"), origin,
               comment="Condition description of "
               f"martial technique '{name}'")
