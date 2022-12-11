from ..write_text import write_text


def parse_palette(json, origin):
    if "signs" in json:
        for x in sorted(json["signs"]):
            write_text(json["signs"][x]["signage"], origin,
                       comment="Signage in mapgen palette")
