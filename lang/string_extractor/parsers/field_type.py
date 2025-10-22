from ..helper import get_singular_name
from ..write_text import write_text


def parse_field_type(json, origin):
    field_names = []

    for fd in json.get("intensity_levels", []):
        name = get_singular_name(fd)
        field_names.append(name)

        write_text(fd.get("name"), origin, comment="Field intensity level")

        for eff in fd.get("effects", []):
            write_text(eff.get("message"), origin,
                       comment=f"Player message on effect of field '{name}'")
            write_text(eff.get("message_npc"), origin,
                       comment=f"NPC message on effect of field '{name}'")

    if "npc_complain" in json:
        write_text(json["npc_complain"]["speech"], origin,
                   comment="Field NPC complaint")

    if "msg_success" in json.get("bash", []):
        write_text(json["bash"]["msg_success"], origin,
                   comment=f"Bashing message of fields: {field_names}")
