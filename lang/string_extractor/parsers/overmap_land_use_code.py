from ..helper import get_singular_name
from ..write_text import write_text


def parse_overmap_land_use_code(json, origin):
    name = get_singular_name(json)

    write_text(json.get("name"), origin,
               comment="Name of an overmap land use code")
    write_text(json.get("detailed_definition"), origin,
               comment=f"Description of overmap land use code '{name}'")
