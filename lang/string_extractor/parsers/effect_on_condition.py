from .effect import parse_effect


def parse_effect_on_condition(json, origin):
    parse_effect(json["effect"], origin)
    if "false_effect" in json:
        parse_effect(json["false_effect"], origin)
