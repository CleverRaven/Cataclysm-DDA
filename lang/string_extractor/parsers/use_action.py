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


def parse_use_action(json, origin, item_name):
    if type(json) is dict:
        for msg_key in use_action_msg_keys:
            if msg_key in json:
                write_text(json[msg_key], origin,
                           comment="\"{0}\" action message of item \"{1}\""
                           .format(msg_key, item_name))
        if json["type"] == "place_trap" and "bury" in json:
            write_text(json["bury"]["done_message"], origin,
                       comment="bury trap message of item \"{}\""
                       .format(item_name))
        if (json["type"] == "effect_on_conditions" and
                "effect_on_conditions" in json):
            for e in json["effect_on_conditions"]:
                if type(e) is dict:
                    parse_effect_on_condition(e, origin,
                                              "use action of item \"{}\""
                                              .format(item_name))
    elif type(json) is list:
        for use_action in json:
            parse_use_action(use_action, origin, item_name)
