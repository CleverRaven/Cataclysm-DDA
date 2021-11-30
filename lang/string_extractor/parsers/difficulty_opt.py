from ..write_text import write_text


def parse_difficulty_opt(json, origin):
    write_text(json["name"], origin, comment="Difficulty rating")
