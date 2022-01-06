from ..write_text import write_text


def parse_vitamin(json, origin):
    if "name" in json:
        write_text(json["name"], origin, comment="Vitamin name")
