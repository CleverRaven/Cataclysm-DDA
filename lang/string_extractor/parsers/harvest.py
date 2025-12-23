from ..write_text import write_text


def parse_harvest(json, origin):
    write_text(json.get("message"), origin, comment="Harvest message")
