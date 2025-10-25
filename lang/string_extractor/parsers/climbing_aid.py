from ..helper import get_singular_name
from ..write_text import write_text


def parse_climbing_aid(json, origin):

    down = json.get("down")
    if not down:
        return

    name = get_singular_name(json)

    write_text(down.get("menu_text"), origin,
               comment=f"Menu text for descent with '{name}'")
    write_text(down.get("menu_cant"), origin,
               comment=f"Menu disables descent with '{name}'")
    write_text(down.get("confirm_text"), origin,
               comment=f"Query: really descend with '{name}'?")
    write_text(down.get("msg_before"), origin,
               comment=f"Message before descent with '{name}'")
    write_text(down.get("msg_after"), origin,
               comment=f"Message after descent with '{name}'")
