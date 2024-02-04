import itertools

from .effect import parse_effect
from ..helper import is_tag
from ..write_text import write_text


all_genders = ["f", "m", "n"]


def gender_options(subject):
    return [subject + ":" + g for g in all_genders]


dynamic_line_string_keys = [
    # from `parsers_simple` in `condition.cpp`
    "u_male", "u_female", "npc_male", "npc_female",
    "has_no_assigned_mission", "has_assigned_mission",
    "has_many_assigned_missions", "has_no_available_mission",
    "has_available_mission", "has_many_available_missions",
    "mission_complete", "mission_incomplete", "mission_has_generic_rewards",
    "npc_available", "npc_following", "npc_friend", "npc_hostile",
    "npc_train_skills", "npc_train_styles", "npc_train_spells",
    "at_safe_space", "is_day", "npc_has_activity",
    "is_outside", "u_is_outside", "npc_is_outside", "u_has_camp",
    "u_can_stow_weapon", "npc_can_stow_weapon", "u_can_drop_weapon",
    "npc_can_drop_weapon", "u_has_weapon", "npc_has_weapon",
    "u_driving", "npc_driving", "has_pickup_list", "is_by_radio", "has_reason",
    "u_is_avatar", "npc_is_avatar", "u_is_npc", "npc_is_npc",
    "u_is_character", "npc_is_character", "u_is_monster", "npc_is_monster",
    "u_is_item", "npc_is_item", "u_is_furniture", "npc_is_furniture",
    "player_see_u", "player_see_npc",
    # yes/no strings for complex conditions, 'and' list
    "yes", "no", "concatenate"
]


def parse_dynamic_line(json, origin, comment=[]):
    if type(json) is list:
        for line in json:
            parse_dynamic_line(line, origin, comment)

    elif type(json) is dict:
        if "str" in json:
            write_text(json, origin, comment=comment, c_format=False)

        if "gendered_line" in json:
            text = json["gendered_line"]
            subjects = json["relevant_genders"]
            options = [gender_options(subject) for subject in subjects]
            for context_list in itertools.product(*options):
                context = " ".join(context_list)
                write_text(text, origin, context=context,
                           comment=comment, c_format=False)

        for key in dynamic_line_string_keys:
            if key in json:
                parse_dynamic_line(json[key], origin, comment=comment)

    elif type(json) == str:
        if not is_tag(json):
            write_text(json, origin, comment=comment, c_format=False)


def parse_response(json, origin):
    if "text" in json:
        write_text(json["text"], origin, c_format=False,
                   comment="Response to NPC dialogue")

    if "truefalsetext" in json:
        write_text(json["truefalsetext"]["true"], origin, c_format=False,
                   comment="Affirmative response to NPC dialogue")
        write_text(json["truefalsetext"]["false"], origin, c_format=False,
                   comment="Negative response to NPC dialogue")

    if "failure_explanation" in json:
        write_text(json["failure_explanation"], origin, c_format=False,
                   comment="Failure explanation for NPC dialogue response")

    if "success" in json:
        parse_response(json["success"], origin)

    if "failure" in json:
        parse_response(json["failure"], origin)

    if "speaker_effect" in json:
        speaker_effects = json["speaker_effect"]
        if not type(speaker_effects) is list:
            speaker_effects = [speaker_effects]
        for eff in speaker_effects:
            if "effect" in eff:
                parse_effect(eff["effect"], origin)

    if "effect" in json:
        parse_effect(json["effect"], origin)


def parse_talk_topic(json, origin):
    if "dynamic_line" in json:
        comment = ["NPC dialogue line"]
        parse_dynamic_line(json["dynamic_line"], origin, comment=comment)

    if "responses" in json:
        for response in json["responses"]:
            parse_response(response, origin)

    if "repeat_responses" in json:
        if type(json["repeat_responses"]) is dict:
            if "response" in json["repeat_responses"]:
                parse_response(json["repeat_responses"]["response"], origin)
        elif type(json["repeat_responses"]) is list:
            for response in json["repeat_responses"]:
                if "response" in response:
                    parse_response(response["response"], origin)

    if "effect" in json:
        parse_effect(json["effect"], origin)
