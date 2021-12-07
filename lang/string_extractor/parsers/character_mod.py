from ..write_text import write_text


def parse_character_mod(json, origin):
    write_text(json["description"], origin,
               comment="Description of the modifier as displayed in the UI")
