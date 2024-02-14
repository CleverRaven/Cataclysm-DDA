from ..write_text import write_text
from .effect import parse_effect


def parse_field_type(json, origin):
    for fd in json.get("intensity_levels", []):
        if "name" in fd:
            write_text(fd["name"], origin, comment="Field intensity level")
        if "effects" in fd:
            parse_effect(fd["effects"], origin,
                         comment="effect in field {}".format(fd["name"])
                         if "name" in fd else "")
    if "npc_complain" in json:
        write_text(json["npc_complain"]["speech"], origin,
                   comment="Field NPC complaint")
