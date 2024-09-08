from ..write_text import write_text
from .effect import parse_effect_on_condition


def parse_examine_action(json, origin, name):
    if "query_msg" in json:
        write_text(json["query_msg"], origin,
                   comment="Query message of {}".format(name))
    if "success_msg" in json:
        write_text(json["success_msg"], origin,
                   comment="Success message of {}".format(name))
    if "redundant_msg" in json:
        write_text(json["redundant_msg"], origin,
                   comment="Redundant message of {}".format(name))
    if "effect_on_conditions" in json:
        for eoc in json["effect_on_conditions"]:
            if type(eoc) is dict:
                parse_effect_on_condition(eoc, origin,
                                          "examine action of {}".format(name))
