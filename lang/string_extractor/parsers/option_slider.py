from ..helper import get_singular_name
from ..write_text import write_text


def parse_option_slider(json, origin):
    name = get_singular_name(json["name"])
    write_text(json["name"], origin,
               comment="Name of an option slider", c_format=False)
    if "levels" in json:
        for level in json["levels"]:
            if "name" in level:
                write_text(level["name"], origin,
                           comment="Name of a slider position in the "
                           "\"{}\" option slider".format(name))
            if "description" in level:
                write_text(level["description"], origin,
                           comment="Description of a slider position in the "
                           "\"{}\" option slider".format(name))
