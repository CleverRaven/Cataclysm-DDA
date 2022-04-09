from ..write_text import write_text


def parse_species(json, origin):
    id = json["id"]
    if "description" in json:
        write_text(json["description"], origin,
                   comment="Description of species \"{}\"".format(id))
    if "footsteps" in json:
        write_text(json["footsteps"], origin,
                   comment="Foot steps of species \"{}\"".format(id))
