from ..write_text import write_text


def parse_activity_type(json, origin):
    if "verb" in json:
        write_text(json["verb"], origin,
                   comment="Verb in activity \"{}\"".format(json["id"]))
