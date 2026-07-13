from ..helper import get_singular_name
from ..write_text import write_text


def parse_gun(json, origin):
    name = get_singular_name(json)

    for mode in json.get("modes", []):
        write_text(mode[1], origin, comment="Firing mode of gun")

    if "skill" in json:
        if json["skill"] != "archery":
            write_text(json["skill"], origin,
                       context="gun_type_type",
                       comment="Skill associated with gun")

    write_text(json.get("reload_noise"), origin,
               comment=f"Reload noise of gun '{name}'")

    for loc in json.get("valid_mod_locations", []):
        write_text(loc[0], origin, comment="Location of gun mod")
