from ..write_text import write_text


def parse_palette(json, origin):
    for x in sorted(json.get("signs", [])):
        write_text(json["signs"][x]["signage"], origin,
                   comment="Signage in mapgen palette")
