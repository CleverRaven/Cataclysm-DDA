from ..write_text import write_text, write_variable


def parse_effect_on_condition(json, origin, comment=""):
    if (not json) or (not type(json) is dict) or ("id" not in json):
        return

    if not comment:
        comment = f"EoC '{json['id']}'"

    parse_effect(json.get("effect"), origin, comment=comment)
    parse_effect(json.get("false_effect"), origin,
                 comment="false effect in " + comment)
    _parse_condition(json.get("condition"), origin)


def parse_effect(effects, origin, comment=""):
    if not effects:
        return

    if not type(effects) is list:
        effects = [effects]

    for eff in effects:
        if not type(eff) is dict:
            continue
        if not comment:
            comment = "an effect"
            if eff_name := eff.get("id") or eff.get("effect_id"):
                comment = f"effect '{eff_name}'"
        _parse_effect_entry(eff, origin, comment)


def _parse_effect_entry(eff, origin, name):

    _parse_effect_variables(eff, origin, name)
    _parse_effect_nested(eff, origin, name)

    write_variable(eff.get("place_override"), origin,
                   comment=f"Place name in {name}")

    if not eff.get("snippet"):
        write_variable(eff.get("message"), origin,
                       comment=f"Message in {name}")
        write_variable(eff.get("u_message"), origin,
                       comment=f"Player message in {name}")
        write_variable(eff.get("npc_message"), origin,
                       comment=f"NPC message in {name}")
        write_variable(eff.get("u_make_sound"), origin,
                       comment=f"Player makes sound in {name}")
        write_variable(eff.get("npc_make_sound"), origin,
                       comment=f"NPC makes sound in {name}")

    if "u_roll_remainder" in eff or "npc_roll_remainder" in eff:
        roll_type = "NPC"
        if "u_roll_remainder" in eff:
            roll_type = "Player"
        write_variable(eff.get("message"), origin,
                       comment=f"{roll_type} roll message in {name}")

    if "u_cast_spell" in eff:
        write_variable(eff["u_cast_spell"].get("message"), origin,
                       comment=f"Player spell casting message in {name}")
    if "npc_cast_spell" in eff:
        write_variable(eff["npc_cast_spell"].get("npc_message"), origin,
                       comment=f"NPC spell casting message in {name}")

    if "u_buy_monster" in eff:
        write_variable(eff.get("name"), origin,
                       comment="Nickname for creature '{}' in {}"
                       .format(eff["u_buy_monster"], name))

    if (
        "u_spawn_monster" in eff or
        "npc_spawn_monster" in eff or
        "u_spawn_npc" in eff or
        "npc_spawn_npc" in eff
    ):
        spawn_type = "NPC"
        if "u_spawn_monster" in eff or "npc_spawn_monster" in eff:
            spawn_type = "monster"

        write_text(eff.get("spawn_message"), origin,
                   comment=f"Player sees {spawn_type} spawns in {name}")
        write_text(eff.get("spawn_message_plural"), origin,
                   comment=f"Player sees {spawn_type}s spawn in {name}")

    if "u_teleport" in eff or "npc_teleport" in eff:
        write_variable(eff.get("success_message"), origin,
                       comment=f"Success message in teleportation in {name}")
        write_variable(eff.get("fail_message"), origin,
                       comment=f"Fail message in teleportation in {name}")

    if "u_run_inv_eocs" in eff or "u_map_run_item_eocs" in eff:
        write_variable(eff.get("title"), origin,
                       comment=f"Player inventory menu title in {name}")
    if "npc_run_inv_eocs" in eff or "npc_map_run_item_eocs" in eff:
        write_variable(eff.get("title"), origin,
                       comment=f"NPC inventory menu title in {name}")

    if ("set_string_var" in eff) and (eff.get("i18n") is True):
        str_vals = eff["set_string_var"]
        if type(str_vals) is not list:
            str_vals = [str_vals]
        for val in str_vals:
            write_variable(val, origin,
                           comment=f"Text variable value in {name}")

    if "set_string_var" in eff and "string_input" in eff:
        string_input = eff["string_input"]
        write_variable(string_input.get("title"), origin,
                       comment=f"String input window's title in {name}")
        write_variable(string_input.get("description"), origin,
                       comment=f"String input window's description in {name}")
        write_variable(string_input.get("default_text"), origin,
                       comment=f"String input window's default text in {name}")


def _parse_effect_nested(eff, origin, name):
    parse_effect(eff.get("then"), origin,
                 comment=f"'then' statement in {name}")
    parse_effect(eff.get("else"), origin,
                 comment=f"'else' statement in {name}")

    for e in eff.get("cases", []):
        parse_effect(e["effect"], origin,
                     comment=f"'switch' statement in {name}")

    if "foreach" in eff:
        parse_effect(eff["effect"], origin,
                     comment=f"'foreach' statement in {name}")

    write_text(eff.get("title"), origin,
               comment=f"EOC selection title in {name}")

    for name in eff.get("names", []):
        write_variable(name, origin,
                       comment="EOC option name")
    for desc in eff.get("descriptions", []):
        write_variable(desc, origin,
                       comment=f"EOC option description for '{name}'")

    for key in ["run_eocs", "true_eocs", "false_eocs", "run_eoc_selector"]:
        eoc_list = eff.get(key, [])
        if type(eoc_list) is not list:
            eoc_list = [eoc_list]
        for eoc in eoc_list:
            parse_effect_on_condition(eoc, origin)

    for e in eff.get("weighted_list_eocs", []):
        if type(e[0]) is dict:
            parse_effect(e[0].get("effect"), origin,
                         comment=name)


def _parse_effect_variables(json, origin, comment):
    json = json.get("variables")
    if not json:
        return

    if type(json) is dict:
        json = [json]

    for jo in json:
        for key, val in jo.items():
            if type(val) is dict and val.get("i18n"):
                write_variable(
                    val, origin,
                    comment=f"Value of variable '{key}' in {comment}")


def _parse_condition(conditions, origin):
    if not conditions:
        return
    if not type(conditions) is list:
        conditions = [conditions]
    for cond in conditions:
        if type(cond) is dict:
            # if *_query is an object, the message is taken from elsewhere
            if type(cond.get("u_query")) is str:
                write_variable(cond["u_query"], origin,
                               comment="Query message shown in a popup")
            if type(cond.get("npc_query")) is str:
                write_variable(cond["npc_query"], origin,
                               comment="Query message shown in a popup")
            _parse_condition(cond.get("and"), origin)
            _parse_condition(cond.get("or"), origin)
            _parse_condition(cond.get("not"), origin)
