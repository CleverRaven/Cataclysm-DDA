from .condition import parse_condition
from ..write_text import write_text


def parse_effect(effects, origin, comment=""):
    if not type(effects) is list:
        effects = [effects]
    for eff in effects:
        if type(eff) is dict:
            if not comment:
                if "id" in eff:
                    comment = "effect \"{}\"".format(eff["id"])
                else:
                    comment = "an effect"
            if "message" in eff:
                write_text(eff["message"], origin,
                           comment="Message in {}".format(comment))
            if "u_buy_monster" in eff and "name" in eff:
                write_text(eff["name"], origin,
                           comment="Nickname for creature '{}' in {}".
                           format(eff["u_buy_monster"], comment))
            if "u_message" in eff:
                if "snippet" in eff and eff["snippet"] is True:
                    continue
                write_text(eff["u_message"], origin,
                           comment="Player message in {}".format(comment))
            if "run_eocs" in eff:
                if type(eff["run_eocs"]) is list:
                    for eoc in eff["run_eocs"]:
                        if type(eoc) is dict:
                            parse_effect_on_condition(eoc, origin)


def parse_effect_on_condition(json, origin, comment=""):
    id = json["id"]
    if comment:
        parse_effect(json["effect"], origin,
                     comment="effect in EoC \"{}\" in {}".format(id, comment))
    else:
        parse_effect(json["effect"], origin,
                     comment="effect in EoC \"{}\"".format(id))
    if "false_effect" in json:
        if comment:
            parse_effect(json["false_effect"], origin,
                         comment="false effect in EoC \"{}\" in {}"
                         .format(id, comment))
        else:
            parse_effect(json["false_effect"], origin,
                         comment="false effect in EoC \"{}\"".format(id))
    if "condition" in json:
        parse_condition(json["condition"], origin)
