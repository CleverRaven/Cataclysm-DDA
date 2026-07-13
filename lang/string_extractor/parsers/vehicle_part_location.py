from ..helper import get_singular_name
from ..write_text import write_text


def parse_vehicle_part_location(json, origin):
    name = get_singular_name(json)

    write_text(json.get("name"), origin,
               comment="Name of vehicle part location")
    write_text(json.get("desc"), origin,
               comment=f"Description of vehicle part location '{name}'")
