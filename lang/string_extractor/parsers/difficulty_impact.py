from ..write_text import write_text


def parse_difficulty_impact(json, origin):
    write_text(json["name"], origin,
               comment="Aspect of gameplay affected by difficulty")
