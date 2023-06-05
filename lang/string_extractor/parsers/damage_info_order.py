from ..write_text import write_text


def parse_damage_info_order(json, origin):
    if "verb" in json:
        write_text(json["verb"], origin,
                   comment="A verb describing how this damage type"
                           "is applied (ex: \"bashing\")")
