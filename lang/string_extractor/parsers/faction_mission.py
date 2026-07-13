from ..helper import get_singular_name
from ..write_text import write_text


def parse_faction_mission(json, origin):
    name = get_singular_name(json)

    write_text(json.get("name"), origin, comment="Faction mission name")

    write_text(json.get("desc"), origin,
               comment=f"Description of faction mission '{name}'")

    write_text(json.get("time"), origin,
               comment=f"Duration of faction mission '{name}'")

    write_text(json.get("footer"), origin,
               comment=f"Footer of faction mission '{name}'")

    write_text(json.get("items_label"), origin,
               comment=f"Gather items header of faction mission '{name}'")

    for item in json.get("items_possibilities", []):
        write_text(item, origin,
                   comment=f"Possible item from faction mission '{name}'")

    for item in json.get("effects", []):
        write_text(item, origin,
                   comment=f"Effect of faction mission '{name}'")
