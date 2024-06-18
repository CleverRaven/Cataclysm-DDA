from ..write_text import write_text


def parse_sub_body_part(json, origin):
    write_text(json["name"], origin, comment="Name of sub body part")

    if "name_multiple" in json:
        write_text(json["name_multiple"], origin,
                   comment="Name of sub body part")
