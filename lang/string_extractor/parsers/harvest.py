from ..write_text import write_text


def parse_harvest(json, origin):
    if "message" in json:
        write_text(json["message"], origin, comment="Harvest message")
