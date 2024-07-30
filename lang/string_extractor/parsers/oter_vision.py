from ..write_text import write_text


def parse_oter_vision(json, origin):
    if "levels" in json:
        for level in json["levels"]:
            if "name" in json:
                write_text(json["name"], origin,
                           comment="oter vision level name")
