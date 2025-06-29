from ..write_text import write_text


def parse_mod_migration(json, origin):
    id = json["id"]

    if "removal_message" in json:
        write_text(json["removal_message"], origin,
                   comment="Message to explain removal of mod \"{}\"".format(id))
