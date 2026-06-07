from ..write_text import write_text


def parse_damage_info_order(json, origin):
    write_text(json.get("verb"), origin,
               comment="A verb describing how this damage type is applied")
