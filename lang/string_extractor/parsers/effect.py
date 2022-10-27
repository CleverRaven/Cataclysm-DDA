from ..write_text import write_text


def parse_effect(effects, origin):
    if not type(effects) is list:
        effects = [effects]
    for eff in effects:
        if type(eff) is dict:
            if "message" in eff:
                write_text(eff["message"], origin,
                           comment="Message in an effect")
            if "u_buy_monster" in eff and "name" in eff:
                write_text(eff["name"], origin,
                           comment="Nickname for creature '{}'".
                           format(eff["u_buy_monster"]))
            if "u_message" in eff:
                if "snippet" in eff and eff["snippet"] is True:
                    continue
                write_text(eff["u_message"], origin,
                           comment="Message about the player in an effect")
            if "run_eocs" in eff:
                if type(eff["run_eocs"]) is list:
                    for eoc in eff["run_eocs"]:
                        if "false_effect" in eoc:
                            parse_effect(eoc["false_effect"], origin)
                        elif "effect" in eoc:
                            parse_effect(eoc["effect"], origin)
