from ..write_text import write_text


def parse_vehicle(json, origin):
    write_text(json.get("name"), origin, comment="Vehicle name")
