from .effect import parse_effect_on_condition
from ..write_text import write_text


use_action_msg_keys = [
    "activate_msg",
    "activation_message",
    "auto_extinguish_message",
    "bury_question",
    "charges_extinguish_message",
    "deactive_msg",
    "descriptions",
    "description",
    "done_message",
    "failure_message",
    "friendly_msg",
    "gerund",
    "holster_msg",
    "holster_prompt",
    "hostile_msg",
    "lacks_fuel_message",
    "menu_text",
    "message",
    "msg",
    "need_charges_msg",
    "need_fire_msg",
    "no_deactivate_msg",
    "noise_message",
    "non_interactive_msg",
    "not_ready_msg",
    "out_of_power_msg",
    "sound_message",
    "success_message",
    "unfold_msg",
    "use_message",
    "verb",
    "voluntary_extinguish_message",
    "water_extinguish_message",
]


def parse_use_action(json, origin, name):
    if not json:
        return

    if type(json) is list:
        for use_action in json:
            parse_use_action(use_action, origin, name)

    if not type(json) is dict:
        return

    for key in use_action_msg_keys:
        write_text(json.get(key), origin,
                   comment=f"'{key}' action message "
                   f"of item '{name}'")

    if json["type"] == "place_trap" and "bury" in json:
        write_text(json["bury"]["done_message"], origin,
                   comment=f"Bury trap message of item '{name}'")

    if (json["type"] == "effect_on_conditions" and
            "effect_on_conditions" in json):
        for e in json["effect_on_conditions"]:
            parse_effect_on_condition(e, origin,
                                      f"use action of item '{name}'")
