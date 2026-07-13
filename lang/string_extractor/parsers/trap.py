from ..helper import get_singular_name
from ..write_text import write_text


def parse_trap(json, origin):
    name = get_singular_name(json)

    write_text(json.get("name"), origin, comment="Name of a trap")

    if "vehicle_data" in json:
        write_text(json["vehicle_data"].get("sound"), origin,
                   comment=f"Trap-vehicle collision message for trap '{name}'")

    for key in ["memorial_male", "memorial_female"]:
        write_text(json.get(key), origin,
                   comment=f"Memorial message of trap '{name}'")

    write_text(json.get("trigger_message_u"), origin,
               comment=f"Message when player triggers trap '{name}'")

    write_text(json.get("trigger_message_npc"), origin,
               comment=f"Message when NPC triggers trap '{name}'")
