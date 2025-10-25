from ..helper import get_singular_name
from ..write_text import write_text


def parse_movement_mode(json, origin):
    name = get_singular_name(json)

    write_text(json["name"], origin, comment="Movement mode name")

    write_text(json["character"], origin,
               comment="Character displayed in the move menu for "
               f"movement mode '{name}'")
    write_text(json["panel_char"], origin,
               comment="Character displayed in the panel for "
               f"movement mode '{name}'")
    write_text(json["change_good_none"], origin,
               comment="Successfully switched to movement mode "
               f"'{name}' with no steed")
    write_text(json["change_good_animal"], origin,
               comment="Successfully switched to movement mode "
               f"'{name}' with animal steed")
    write_text(json["change_good_mech"], origin,
               comment="Successfully switched to movement mode "
               f"'{name}' with mechanical steed")

    write_text(json.get("change_bad_none"), origin,
               comment="Failed to switched to movement mode "
               f"'{name}' with no steed")
    write_text(json.get("change_bad_animal"), origin,
               comment="Failed to switched to movement mode "
               f"'{name}' with animal steed")
    write_text(json.get("change_bad_mech"), origin,
               comment="Failed to switched to movement mode "
               f"'{name}' with mechanical steed")
