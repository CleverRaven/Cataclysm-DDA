from ..helper import is_tag
from ..write_text import write_text


def parse_harvest(json, origin):
    if "message" in json:
        if not is_tag(json["message"]):
            write_text(json["message"], origin, comment="Harvest message")
