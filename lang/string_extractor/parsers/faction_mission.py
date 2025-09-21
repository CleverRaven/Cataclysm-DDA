from ..helper import get_singular_name
from ..write_text import write_text


def parse_faction_mission(json, origin):
    name = get_singular_name(json.get("name", json["id"]))
    if "name" in json:
        write_text(json["name"], origin, comment="Faction mission name")
    if "desc" in json:
        write_text(json["desc"], origin,
                   comment="Description of faction mission "
                   "\"{}\"".format(name))
    if "time" in json:
        write_text(json["time"], origin,
                   comment="Duration of faction mission \"{}\"".format(name))
    if "footer" in json:
        write_text(json["footer"], origin,
                   comment="Footer of faction mission \"{}\"".format(name))
    if "items_label" in json:
        write_text(json["items_label"], origin,
                   comment="Gather items header label of faction mission "
                   "\"{}\"".format(name))
    if "items_possibilities" in json:
        for item in json["items_possibilities"]:
            write_text(item, origin,
                       comment="Possible item from faction mission "
                       "\"{}\"".format(name))
    if "effects" in json:
        for item in json["effects"]:
            write_text(item, origin,
                       comment="Effect of faction mission \"{}\"".format(name))
