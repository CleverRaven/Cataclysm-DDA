from ..write_text import write_text


def parse_sub_body_part(json, origin):
    write_text(json["name"], origin,
               comment="Name of sub body part")
    write_text(json.get("name_multiple"), origin,
               comment="Name of sub body part")
