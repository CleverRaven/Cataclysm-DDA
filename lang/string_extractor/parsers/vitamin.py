from ..write_text import write_text


def parse_vitamin(json, origin):
    write_text(json.get("name"), origin, comment="Vitamin name")
