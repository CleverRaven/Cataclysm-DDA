from ..write_text import write_text


def parse_oter_vision(json, origin):
    for level in json.get("levels", []):
        write_text(level.get("name"), origin,
                   comment="Overmap terrain vision level name")
