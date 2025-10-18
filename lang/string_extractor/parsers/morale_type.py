from ..write_text import write_text


def parse_morale_type(json, origin):
    write_text(json.get("text"), origin, comment="Morale text")
