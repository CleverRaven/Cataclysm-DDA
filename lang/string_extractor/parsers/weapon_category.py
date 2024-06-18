from ..write_text import write_text


def parse_weapon_category(json, origin):
    write_text(json["name"], origin, comment="Weapon category name")
