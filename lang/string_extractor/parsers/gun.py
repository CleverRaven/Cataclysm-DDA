from ..helper import get_singular_name
from .use_action import parse_use_action
from ..write_text import write_text


def parse_gun(json, origin):
    name = ""
    if "name" in json:
        name = get_singular_name(json["name"])
        write_text(json["name"], origin, comment="Name of a gun", plural=True)
    elif "id" in json:
        name = json["id"]

    if "description" in json:
        write_text(json["description"], origin,
                   comment="Description of gun \"{}\"".format(name))

    if "use_action" in json:
        parse_use_action(json["use_action"], origin, name)

    if "variants" in json:
        for variant in json["variants"]:
            variant_name = get_singular_name(variant["name"])
            write_text(variant["name"], origin,
                       comment="Variant name of gun \"{}\"".format(name),
                       plural=True)
            write_text(variant["description"], origin,
                       comment="Description of variant \"{0}\" of gun \"{1}\""
                       .format(name, variant_name))

    if "modes" in json:
        for mode in json["modes"]:
            write_text(mode[1], origin,
                       comment="Firing mode of gun \"{}\"".format(name))

    if "skill" in json:
        if json["skill"] != "archery":
            write_text(json["skill"], origin, context="gun_type_type",
                       comment="Skill associated with gun \"{}\"".format(name))

    if "reload_noise" in json:
        write_text(json["reload_noise"], origin,
                   comment="Reload noise of gun \"{}\"".format(name))

    if "valid_mod_locations" in json:
        for loc in json["valid_mod_locations"]:
            write_text(loc[0], origin,
                       comment="Valid mod location of gun \"{}\""
                       .format(name))
