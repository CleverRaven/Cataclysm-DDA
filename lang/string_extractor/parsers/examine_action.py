from ..write_text import write_text
from .effect import parse_effect_on_condition


def parse_examine_action(json, origin, name):
    ename = "{}'s examine action".format(name)
    if "query_msg" in json:
        write_text(json["query_msg"], origin,
                   comment="Query message for {}".format(ename))
    if "success_msg" in json:
        write_text(json["success_msg"], origin,
                   comment="Message when {} succeeds".format(ename))
    if "redundant_msg" in json:
        write_text(json["redundant_msg"], origin,
                   comment="Message when {} would be redundant".format(ename))
    if "condition_fail_msg" in json:
        write_text(json["condition_fail_msg"], origin,
                   comment="Message when {} fails its condition".format(ename))
    if "effect_on_conditions" in json:
        for eoc in json["effect_on_conditions"]:
            if type(eoc) is dict:
                parse_effect_on_condition(eoc, origin, ename)
