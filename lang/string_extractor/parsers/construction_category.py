from ..write_text import write_text


def parse_construction_category(json, origin):
    if "name" in json:
        write_text(json["name"], origin, comment="Construction category name")
