from ..write_text import write_text


def parse_field_type(json, origin):
    for fd in json.get("intensity_levels", []):
        if "name" in fd:
            write_text(fd["name"], origin, comment="Field intensity level")
            id_or_name = fd["name"]
        else:
            id_or_name = json["id"]
        for eff in fd.get("effects", []):
            if "message" in eff:
                write_text(eff["message"], origin,
                           comment="Player message on effect of field {}"
                           .format(id_or_name))
            if "message_npc" in eff:
                write_text(eff["message_npc"], origin,
                           comment="NPC message on effect of field {}"
                           .format(id_or_name))
    if "npc_complain" in json:
        write_text(json["npc_complain"]["speech"], origin,
                   comment="Field NPC complaint")
