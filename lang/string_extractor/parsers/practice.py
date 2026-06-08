from ..helper import get_singular_name
from ..write_text import write_text
from .effect import parse_effect_on_condition


def parse_practice(json, origin):
    name = get_singular_name(json)

    write_text(json.get("name"), origin,
               comment="Practice name")
    write_text(json.get("description"), origin,
               comment=f"Description of practice '{name}'")
    for eoc in json.get("result_eocs", []):
        parse_effect_on_condition(eoc, origin,
                                  f"Result of practice '{name}'")
