from ..helper import get_singular_name
from ..write_text import write_text


def parse_magazine(json, origin):
    write_text(json["name"], origin, comment="Magazine name", plural=True)
    name = get_singular_name(json["name"])

    if "description" in json:
        write_text(json["description"], origin,
                   comment="Description of magazine \"{}\"".format(name))

    if "variants" in json:
        for variant in json["variants"]:
            variant_name = get_singular_name(variant["name"])
            write_text(variant["name"], origin,
                       comment="Variant name of magazine \"{}\"".format(name),
                       plural=True)
            write_text(variant["description"], origin,
                       comment="Description of variant \"{0}\" "
                       "of magazine \"{1}\"".format(variant_name, name))
