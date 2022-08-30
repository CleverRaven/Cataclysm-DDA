from ..write_text import write_text


def parse_addiction_type(json, origin):
    id = json["id"]
    if "name" in json:
        write_text(json["name"], origin,
                   comment="Name of the \"{}\" addiction's "
                   "effect as it appears in the player's status".format(id))

    if "type_name" in json:
        write_text(json["type_name"], origin,
                   comment="The name of the \"{}\" addiction's source"
                   .format(id))

    if "description" in json:
        write_text(json["description"], origin, c_format=False,
                   comment="Description of the \"{}\" addiction's effects as "
                   "it appears in the player's status".format(id))
