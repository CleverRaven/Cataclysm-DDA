from ..write_text import write_text


def parse_construction_group(json, origin):
    write_text(json.get("name"), origin,
               comment="Construction group name")
