from ..write_text import write_text


def parse_construction_group(json, origin):
    if "name" in json:
        write_text(json["name"], origin, comment="Construction group name")
