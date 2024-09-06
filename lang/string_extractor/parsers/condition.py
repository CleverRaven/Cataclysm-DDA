from ..write_text import write_translation_or_var


def parse_condition(conditions, origin):
    if not type(conditions) is list:
        conditions = [conditions]
    for cond in conditions:
        if type(cond) is dict:
            # if *_query is an object, the message is taken from elsewhere
            if "u_query" in cond and type(cond["u_query"]) is str:
                write_translation_or_var(cond["u_query"], origin,
                                         comment="Query message shown in a "
                                         "popup")
            if "npc_query" in cond and type(cond["npc_query"]) is str:
                write_translation_or_var(cond["npc_query"], origin,
                                         comment="Query message shown in a "
                                         "popup")
            if "and" in cond:
                parse_condition(cond["and"], origin)
            if "or" in cond:
                parse_condition(cond["or"], origin)
            if "not" in cond:
                parse_condition(cond["not"], origin)
