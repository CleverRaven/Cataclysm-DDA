from ..write_text import write_text


def parse_overmap_terrain(json, origin):
    write_text(json.get("name"), origin, comment="Overmap terrain name")
