from ..write_text import write_text


def parse_skill_display_type(json, origin):
    ident = json["id"]
    write_text(json["display_string"], origin,
               comment=f"Display string for skill display type '{ident}'")
