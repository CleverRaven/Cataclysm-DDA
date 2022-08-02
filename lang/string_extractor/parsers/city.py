from ..write_text import write_text


def parse_city(json, origin):
    if "name" in json:
        write_text(json["name"], origin, comment="City name")
