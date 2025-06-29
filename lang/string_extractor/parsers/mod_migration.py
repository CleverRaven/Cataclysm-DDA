from ..write_text import write_text


def parse_mod_migration(json, origin):
    id = json["id"]

    if "removal_reason" in json:
        write_text(json["removal_reason"], origin,
                   comment="Explaination for removal of mod \"{}\"".format(id))
