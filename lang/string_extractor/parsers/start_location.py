from ..write_text import write_text


def parse_start_location(json, origin):
    if "name" in json:
        id = json["id"]
        write_text(json["name"], origin,
                   comment="Name of starting location \"{}\"".format(id))
