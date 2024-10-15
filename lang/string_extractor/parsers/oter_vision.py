from ..write_text import write_text


def parse_oter_vision(json, origin):
    if "levels" in json:
        for level in json["levels"]:
            if "name" in level:
                write_text(level["name"], origin,
                           comment="Overmap terrain vision level name")
