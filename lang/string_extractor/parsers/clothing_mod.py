from ..write_text import write_text


def parse_clothing_mod(json, origin):
    write_text(json["implement_prompt"], origin,
               comment="Implement clothing modification prompt text")
    write_text(json["destroy_prompt"], origin,
               comment="Destroy clothing modification prompt text")
