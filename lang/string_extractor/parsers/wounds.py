from ..helper import get_singular_name
from ..write_text import write_text


def parse_wound(json, origin):
    name = get_singular_name(json)

    write_text(json["name"], origin,
               comment="Name of a wound",
               plural=True)

    write_text(json["description"], origin,
               comment=f"Description of  wound '{name}'")


def parse_wound_fix(json, origin):
    name = get_singular_name(json)

    write_text(json["name"], origin,
               comment="Name of a wound fix")

    write_text(json["description"], origin,
               comment=f"Description of wound fix'{name}'")

    write_text(json.get("success_msg"), origin,
               comment=f"Message of succesful wound fix '{name}'")
