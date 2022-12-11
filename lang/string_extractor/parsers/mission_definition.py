from ..helper import get_singular_name
from ..write_text import write_text
from .effect import parse_effect


def parse_mission_definition(json, origin):
    name = get_singular_name(json["name"])
    write_text(json["name"], origin, comment="Mission name")

    if "description" in json:
        write_text(json["description"], origin,
                   comment="Description of mission \"{}\"".format(name))

    if "dialogue" in json:
        for key in ["describe", "offer", "accepted", "rejected", "advice",
                    "inquire", "success", "success_lie", "failure"]:
            if key in json["dialogue"]:
                write_text(json["dialogue"][key], origin,
                           comment="Dialogue line in mission \"{}\"".
                           format(name))

    for key in ["start", "end", "fail"]:
        if key in json and "effect" in json[key]:
            parse_effect(json[key]["effect"], origin)
