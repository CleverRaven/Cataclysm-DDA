from ..write_text import write_text


def parse_construction(json, origin):
    write_text(json.get("pre_note"), origin,
               comment="Prerequisite of terrain constrution")
