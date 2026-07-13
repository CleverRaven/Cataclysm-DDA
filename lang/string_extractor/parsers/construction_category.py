from ..write_text import write_text


def parse_construction_category(json, origin):
    write_text(json.get("name"), origin,
               comment="Construction category name")
