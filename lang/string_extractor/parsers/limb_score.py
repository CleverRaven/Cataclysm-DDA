from ..write_text import write_text


def parse_limb_score(json, origin):
    write_text(json["name"], origin,
               comment="Name of the limb score as displayed in the UI")
