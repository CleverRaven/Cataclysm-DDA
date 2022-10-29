from ..helper import get_singular_name
from ..write_text import write_text


def parse_movement_mode(json, origin):
    name = get_singular_name(json["name"])
    write_text(json["name"], origin, comment="Movement mode name")
    write_text(json["character"], origin,
               comment="Character displayed in the move menu for "
               "movement mode \"{}\"".format(name))
    write_text(json["panel_char"], origin,
               comment="Character displayed in the panel for "
               "movement mode \"{}\"".format(name))
    write_text(json["change_good_none"], origin,
               comment="Successfully switched to movement mode "
               "\"{}\" with no steed".format(name))
    write_text(json["change_good_animal"], origin,
               comment="Successfully switched to movement mode "
               "\"{}\" with animal steed".format(name))
    write_text(json["change_good_mech"], origin,
               comment="Successfully switched to movement mode "
               "\"{}\" with mechanical steed".format(name))
    if "change_bad_none" in json:
        write_text(json["change_bad_none"], origin,
                   comment="Failed to switched to movement mode "
                   "\"{}\" with no steed".format(name))
    if "change_bad_animal" in json:
        write_text(json["change_bad_animal"], origin,
                   comment="Failed to switched to movement mode "
                   "\"{}\" with animal steed".format(name))
    if "change_bad_mech" in json:
        write_text(json["change_bad_mech"], origin,
                   comment="Failed to switched to movement mode "
                   "\"{}\" with mechanical steed".format(name))
