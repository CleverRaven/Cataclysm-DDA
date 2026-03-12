from ..write_text import write_text


def parse_ammunition_type(json, origin):
    write_text(json.get("name"), origin, comment="Name of an ammunition type")
