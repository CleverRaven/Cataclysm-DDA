from ..helper import get_singular_name
from ..write_text import write_text


def parse_skill(json, origin):
    name = get_singular_name(json)

    write_text(json.get("name"), origin,
               comment="Skill name")
    write_text(json.get("description"), origin,
               comment=f"Description of skill '{name}'")

    for level in json.get("level_descriptions_theory", []):
        write_text(level["description"], origin,
                   comment=f"Level description of skill '{name}'")

    for level in json.get("level_descriptions_practice", []):
        write_text(level["description"], origin,
                   comment=f"Level description of skill '{name}'")
