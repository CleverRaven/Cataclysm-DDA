from ..write_text import write_text


def parse_requirement(json, origin):
    write_text(json.get("name"), origin,
               comment="Display name of a crafting requirement")
