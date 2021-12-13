from ..write_text import write_text


def parse_overmap_terrain(json, origin):
    if "name" in json:
        write_text(json["name"], origin, comment="Overmap terrain name")
