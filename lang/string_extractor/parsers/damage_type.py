from ..write_text import write_text


def parse_damage_type(json, origin):
    write_text(json["name"], origin,
               comment="Name of the damage type as displayed in item info")
