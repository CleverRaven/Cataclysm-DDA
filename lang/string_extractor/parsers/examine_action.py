from ..write_text import write_text
from .effect import parse_effect_on_condition


def parse_examine_action(json, origin, name):
    if not (json and type(json) is dict):
        return

    ename = f"examine action of {name}"

    write_text(json.get("query_msg"), origin,
               comment=f"Query message for {ename}")
    write_text(json.get("success_msg"), origin,
               comment=f"Message when {ename} succeeds")
    write_text(json.get("redundant_msg"), origin,
               comment=f"Message when {ename} would be redundant")
    write_text(json.get("condition_fail_msg"), origin,
               comment=f"Message when {ename} fails its condition")

    for eoc in json.get("effect_on_conditions", []):
        parse_effect_on_condition(eoc, origin, ename)
