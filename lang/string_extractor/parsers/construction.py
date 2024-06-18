from ..write_text import write_text


def parse_construction(json, origin):
    if "pre_note" in json:
        write_text(json["pre_note"], origin,
                   comment="Prerequisite of terrain constrution")
