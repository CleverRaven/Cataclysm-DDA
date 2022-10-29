from ..helper import get_singular_name
from ..write_text import write_text


def parse_gunmod(json, origin):
    write_text(json["name"], origin, comment="Gun mod name", plural=True)
    name = get_singular_name(json["name"])

    if "description" in json:
        write_text(json["description"], origin,
                   comment="Description of gun mod \"{}\"".format(name))

    if "mode_modifier" in json:
        for mode in json["mode_modifier"]:
            write_text(mode[1], origin,
                       comment="Firing mode of gun mod \"{}\"".format(name))

    if "variants" in json:
        for variant in json["variants"]:
            variant_name = get_singular_name(variant["name"])
            write_text(variant["name"], origin,
                       comment="Variant name of gun mod \"{}\"".format(name),
                       plural=True)
            write_text(variant["description"], origin,
                       comment="Description of variant \"{1}\""
                       "of gun mod \"{0}\""
                       .format(name, variant_name))

    if "location" in json:
        write_text(json["location"], origin,
                   comment="Location of gun mod \"{}\"".format(name))

    if "mod_targets" in json:
        for target in json["mod_targets"]:
            write_text(target, origin, context="gun_type_type",
                       comment="Target of gun mod \"{}\"".format(name))
