from ..helper import get_singular_name
from ..write_text import write_text
from .effect import parse_effect
from .mapgen import parse_mapgen_object


def parse_mission_definition(json, origin):
    name = get_singular_name(json)

    write_text(json["name"], origin, comment="Mission name")
    write_text(json.get("description"), origin,
               comment=f"Description of mission '{name}'")

    if "dialogue" in json:
        for key in ["describe", "offer", "accepted", "rejected", "advice",
                    "inquire", "success", "success_lie", "failure"]:
            write_text(json["dialogue"].get(key), origin,
                       comment=f"Dialogue line '{key}' in mission '{name}'")

    for key in ["start", "end", "fail"]:
        if key in json:
            if "effect" in json[key]:
                parse_effect(json[key]["effect"], origin)
            if "update_mapgen" in json[key]:
                parse_mapgen_object(json[key]["update_mapgen"], origin,
                                    om=f"update on {key} of mission '{name}'")
