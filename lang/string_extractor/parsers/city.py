from ..write_text import write_text


def parse_city(json, origin):
    write_text(json.get("name"), origin, comment="City name")
