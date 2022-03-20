from ..helper import get_singular_name
from ..write_text import write_text


def parse_overmap_land_use_code(json, origin):
    name = get_singular_name(json.get("name", ""))
    if "name" in json:
        write_text(json["name"], origin,
                   comment="Name of an overmap land use code")
    write_text(json["detailed_definition"], origin, c_format=False,
               comment="Detailed definition of overmap land use code \"{}\""
               .format(name))
