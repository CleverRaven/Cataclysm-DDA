from .condition import parse_condition
from ..write_text import write_text
from ..write_text import write_translation_or_var


def parse_effect(effects, origin, comment=""):
    if not type(effects) is list:
        effects = [effects]
    for eff in effects:
        if type(eff) is dict:
            if not comment:
                if "id" in eff:
                    comment = "effect \"{}\"".format(eff["id"])
                elif "effect_id" in eff:
                    comment = "effect \"{}\"".format(eff["effect_id"])
                else:
                    comment = "an effect"
            if "u_buy_monster" in eff and "name" in eff:
                write_translation_or_var(eff["name"], origin,
                                         comment="Nickname for creature '{}' "
                                         "in {}".format(eff["u_buy_monster"],
                                                        comment))
            if "snippet" not in eff or eff["snippet"] is False:
                if "message" in eff:
                    write_translation_or_var(eff["message"], origin,
                                             comment="Message in {}"
                                             .format(comment))
                if "u_message" in eff:
                    write_translation_or_var(eff["u_message"], origin,
                                             comment="Player message in {}"
                                             .format(comment))
                if "npc_message" in eff:
                    write_translation_or_var(eff["npc_message"], origin,
                                             comment="NPC message in {}"
                                             .format(comment))
                if "u_make_sound" in eff:
                    write_translation_or_var(eff["u_make_sound"], origin,
                                             comment="Player makes sound in {}"
                                             .format(comment))
                if "npc_make_sound" in eff:
                    write_translation_or_var(eff["npc_make_sound"], origin,
                                             comment="NPC makes sound in {}"
                                             .format(comment))
            if "u_roll_remainder" in eff or "npc_roll_remainder" in eff:
                if "u_roll_remainder" in eff:
                    roll_type = "player"
                else:
                    roll_type = "NPC"
                if "message" in eff:
                    write_translation_or_var(eff["message"], origin,
                                             comment="{} roll message in {}"
                                             .format(roll_type, comment))
            for cast_spell_key in ["u_cast_spell", "npc_cast_spell"]:
                if cast_spell_key not in eff:
                    continue
                spell_obj = eff[cast_spell_key]
                if "message" in spell_obj:
                    write_translation_or_var(spell_obj["message"], origin,
                                             comment="Player spell casting "
                                             "message in {}".format(comment))
                if "npc_message" in spell_obj:
                    write_translation_or_var(spell_obj["npc_message"], origin,
                                             comment="NPC spell casting "
                                             "message in {}".format(comment))
            if ("set_string_var" in eff and
                    "i18n" in eff and eff["i18n"] is True):
                str_vals = eff["set_string_var"]
                if type(str_vals) is not list:
                    str_vals = [str_vals]
                for val in str_vals:
                    write_translation_or_var(val, origin,
                                             comment="Text variable value in "
                                             "{}".format(comment))
            if ("u_spawn_monster" in eff or "npc_spawn_monster" in eff or
                    "u_spawn_npc" in eff or "npc_spawn_npc" in eff):
                if "u_spawn_monster" in eff or "npc_spawn_monster" in eff:
                    spawn_type = "monster"
                else:
                    spawn_type = "NPC"
                if "spawn_message" in eff:
                    write_text(eff["spawn_message"], origin,
                               comment="Player sees {} spawns in {}"
                               .format(spawn_type, comment))
                if "spawn_message_plural" in eff:
                    write_text(eff["spawn_message_plural"], origin,
                               comment="Player sees {}s spawn in {}"
                               .format(spawn_type, comment))
            if "u_teleport" in eff or "npc_teleport" in eff:
                if "success_message" in eff:
                    write_translation_or_var(eff["success_message"], origin,
                                             comment="Success message in "
                                             "teleportation in {}"
                                             .format(comment))
                if "fail_message" in eff:
                    write_translation_or_var(eff["fail_message"], origin,
                                             comment="Fail message in "
                                             "teleportation in {}"
                                             .format(comment))
            if "variables" in eff:
                parse_effect_variables(eff["variables"], origin, comment)
            if "run_eoc_selector" in eff:
                eoc_list = eff["run_eoc_selector"]
                if type(eoc_list) is not list:
                    eoc_list = [eoc_list]
                for eoc in eoc_list:
                    if type(eoc) is dict and "id" in eoc:
                        parse_effect_on_condition(eoc, origin,
                                                  comment="nested EOC option "
                                                  "in {}".format(comment))
                for name in eff.get("names", []):
                    write_translation_or_var(name, origin,
                                             comment="EOC option name in {}"
                                             .format(comment))
                for desc in eff.get("descriptions", []):
                    write_translation_or_var(desc, origin,
                                             comment="EOC option description "
                                             "in {}".format(comment))
                if "title" in eff:
                    write_text(eff["title"], origin,
                               comment="EOC selection title in {}"
                               .format(comment))
            if "title" in eff:
                if "u_run_inv_eocs" in eff or "u_map_run_item_eocs" in eff:
                    write_translation_or_var(eff["title"], origin,
                                             comment="Player inventory menu "
                                             "title in {}".format(comment))
                if "npc_run_inv_eocs" in eff or "npc_map_run_item_eocs" in eff:
                    write_translation_or_var(eff["title"], origin,
                                             comment="NPC inventory menu "
                                             "title in {}".format(comment))
            if "place_override" in eff:
                write_translation_or_var(eff["place_override"], origin,
                                         comment="place name in {}"
                                         .format(comment))
            # Nested effects
            if "weighted_list_eocs" in eff:
                for e in eff["weighted_list_eocs"]:
                    if type(e[0]) is dict:
                        if "effect" in e[0]:
                            parse_effect(e[0]["effect"], origin,
                                         comment="nested effect in {}"
                                         .format(comment))
            if "run_eocs" in eff:
                eoc_list = eff["run_eocs"]
                if type(eoc_list) is not list:
                    eoc_list = [eoc_list]
                for eoc in eoc_list:
                    if type(eoc) is dict and "id" in eoc:
                        parse_effect_on_condition(eoc, origin,
                                                  comment="nested EOCs in {}"
                                                  .format(comment))
            for key, cond_type in [("true_eocs", "true"),
                                   ("false_eocs", "false")]:
                if key not in eff:
                    continue
                eoc_list = eff[key]
                if type(eoc_list) is not list:
                    eoc_list = [eoc_list]
                for eoc in eoc_list:
                    if type(eoc) is not dict:
                        continue
                    parse_effect_on_condition(
                        eoc, origin,
                        comment="nested EOCs on {} condition in {}"
                        .format(cond_type, comment))
            if "if" in eff:
                parse_effect(eff["then"], origin,
                             comment="nested effect in then statement in {}"
                             .format(comment))
                if "else" in eff:
                    parse_effect(eff["else"], origin,
                                 comment="nested effect in else statement in "
                                 "{}".format(comment))
            if "switch" in eff:
                for e in eff["cases"]:
                    parse_effect(e["effect"], origin,
                                 comment="nested effect in switch statement "
                                 "in {}".format(comment))
            if "foreach" in eff:
                parse_effect(eff["effect"], origin,
                             comment="nested effect in foreach statement in {}"
                             .format(comment))


def parse_effect_on_condition(json, origin, comment=""):
    id = json["id"]
    if "effect" in json:
        if comment:
            parse_effect(json["effect"], origin,
                         comment="effect in EoC \"{}\" in {}"
                         .format(id, comment))
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


def parse_effect_variables(json, origin, comment):
    if type(json) is dict:
        json = [json]
    for jsobj in json:
        for key, val in jsobj.items():
            if type(val) is dict and val.get("i18n", False):
                write_translation_or_var(val, origin,
                                         comment="value of variable '{}' in {}"
                                         .format(key, comment))
