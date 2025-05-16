from ..write_text import write_text


def parse_morale_type(json, origin):
    if "text" in json:
        write_text(json["text"], origin, comment="Morale text")
